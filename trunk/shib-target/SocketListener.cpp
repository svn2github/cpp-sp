/*
 *  Copyright 2001-2005 Internet2
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * SocketListener.cpp
 * 
 * Berkeley Socket-based Listener implementation
 */

#include "SocketListener.h"

#include <errno.h>
#include <sstream>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

using namespace std;
using namespace log4cpp;
using namespace saml;
using namespace shibboleth;
using namespace shibtarget;

namespace shibtarget {
  
    // Manages the pool of connections
    class SocketPool
    {
    public:
        SocketPool(Category& log, const SocketListener* listener)
            : m_log(log), m_listener(listener), m_lock(shibboleth::Mutex::create()) {}
        ~SocketPool();
        SocketListener::ShibSocket get();
        void put(SocketListener::ShibSocket s);
  
    private:
        SocketListener::ShibSocket connect();
        
        const SocketListener* m_listener;
        Category& m_log;
        auto_ptr<Mutex> m_lock;
        stack<SocketListener::ShibSocket> m_pool;
    };
  
    // Worker threads in server
    class ServerThread {
    public:
        ServerThread(SocketListener::ShibSocket& s, SocketListener* listener, unsigned long id);
        ~ServerThread();
        void run();
        bool job();

    private:
        SocketListener::ShibSocket m_sock;
        Thread* m_child;
        SocketListener* m_listener;
        string m_id;
        char m_buf[16384];
    };
}

SocketListener::ShibSocket SocketPool::connect()
{
#ifdef _DEBUG
    saml::NDC ndc("connect");
#endif

    m_log.debug("trying to connect to listener");

    SocketListener::ShibSocket sock;
    if (!m_listener->create(sock)) {
        m_log.error("cannot create socket");
        throw ListenerException("Cannot create socket");
    }

    bool connected = false;
    int num_tries = 3;

    for (int i = num_tries-1; i >= 0; i--) {
        if (m_listener->connect(sock)) {
            connected = true;
            break;
        }
    
        m_log.warn("cannot connect socket (%u)...%s", sock, (i > 0 ? "retrying" : ""));

        if (i) {
#ifdef WIN32
            Sleep(2000*(num_tries-i));
#else
            sleep(2*(num_tries-i));
#endif
        }
    }

    if (!connected) {
        m_log.crit("socket server unavailable, failing");
        m_listener->close(sock);
        throw ListenerException("Cannot connect to listener process, a site adminstrator should be notified.");
    }

    m_log.debug("socket (%u) connected successfully", sock);
    return sock;
}

SocketPool::~SocketPool()
{
    while (!m_pool.empty()) {
#ifdef WIN32
        closesocket(m_pool.top());
#else
        ::close(m_pool.top());
#endif
        m_pool.pop();
    }
}

SocketListener::ShibSocket SocketPool::get()
{
    m_lock->lock();
    if (m_pool.empty()) {
        m_lock->unlock();
        return connect();
    }
    SocketListener::ShibSocket ret=m_pool.top();
    m_pool.pop();
    m_lock->unlock();
    return ret;
}

void SocketPool::put(SocketListener::ShibSocket s)
{
    m_lock->lock();
    m_pool.push(s);
    m_lock->unlock();
}

SocketListener::SocketListener(const DOMElement* e) : log(&Category::getInstance(SHIBT_LOGCAT".Listener")),
    m_shutdown(NULL), m_child_lock(NULL), m_child_wait(NULL), m_socketpool(NULL), m_socket((ShibSocket)0)
{
    // Are we a client?
    if (ShibTargetConfig::getConfig().isEnabled(ShibTargetConfig::InProcess)) {
        m_socketpool=new SocketPool(*log,this);
    }
    // Are we a server?
    if (ShibTargetConfig::getConfig().isEnabled(ShibTargetConfig::OutOfProcess)) {
        m_child_lock = Mutex::create();
        m_child_wait = CondWait::create();
    }
}

SocketListener::~SocketListener()
{
    delete m_socketpool;
    delete m_child_wait;
    delete m_child_lock;
}

bool SocketListener::run(bool* shutdown)
{
#ifdef _DEBUG
    saml::NDC ndc("run");
#endif

    // Save flag to monitor for shutdown request.
    m_shutdown=shutdown;
    unsigned long count = 0;

    if (!create(m_socket)) {
        log->crit("failed to create socket");
        return false;
    }
    if (!bind(m_socket,true)) {
        this->close(m_socket);
        log->crit("failed to bind to socket.");
        return false;
    }

    while (!*m_shutdown) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(m_socket, &readfds);
        struct timeval tv = { 0, 0 };
        tv.tv_sec = 5;
    
        switch (select(m_socket + 1, &readfds, 0, 0, &tv)) {
#ifdef WIN32
            case SOCKET_ERROR:
#else
            case -1:
#endif
                if (errno == EINTR) continue;
                log_error();
                log->error("select() on main listener socket failed");
                return false;
        
            case 0:
                continue;
        
            default:
            {
                // Accept the connection.
                SocketListener::ShibSocket newsock;
                if (!accept(m_socket, newsock))
                    log->crit("failed to accept incoming socket connection");

                // We throw away the result because the children manage themselves...
                try {
                    new ServerThread(newsock,this,++count);
                }
                catch (...) {
                    log->crit("error starting new server thread to service incoming request");
                }
            }
        }
    }
    log->info("listener service shutting down");

    // Wait for all children to exit.
    m_child_lock->lock();
    while (!m_children.empty())
        m_child_wait->wait(m_child_lock);
    m_child_lock->unlock();

    this->close(m_socket);
    m_socket=(ShibSocket)0;
    return true;
}

DDF SocketListener::send(const DDF& in)
{
#ifdef _DEBUG
    saml::NDC ndc("send");
#endif

    log->debug("sending message: %s", in.name());

    // Serialize data for transmission.
    ostringstream os;
    os << in;
    string ostr(os.str());

    // Loop on the RPC in case we lost contact the first time through
#ifdef WIN32
    u_long len;
#else
    uint32_t len;
#endif
    int retry = 1;
    SocketListener::ShibSocket sock;
    while (retry >= 0) {
        sock = m_socketpool->get();
        
        int outlen = ostr.length();
        len = htonl(outlen);
        if (send(sock,(char*)&len,sizeof(len)) != sizeof(len) || send(sock,ostr.c_str(),outlen) != outlen) {
            log_error();
            this->close(sock);
            if (retry)
                retry--;
            else
                throw ListenerException("Failure sending remoted message ($1).", params(1,in.name()));
        }
        else {
            // SUCCESS.
            retry = -1;
        }
    }

    log->debug("send completed, reading response message");

    // Read the message.
    if (recv(sock,(char*)&len,sizeof(len)) != sizeof(len)) {
        log->error("error reading size of output message");
        this->close(sock);
        throw ListenerException("Failure receiving response to remoted message ($1).", params(1,in.name()));
    }
    len = ntohl(len);
    
    char buf[16384];
    int size_read;
    stringstream is;
    while (len && (size_read = recv(sock, buf, sizeof(buf))) > 0) {
        is.write(buf, size_read);
        len -= size_read;
    }
    
    if (len) {
        log->error("error reading output message from socket");
        this->close(sock);
        throw ListenerException("Failure receiving response to remoted message ($1).", params(1,in.name()));
    }
    
    m_socketpool->put(sock);

    // Unmarshall data.
    DDF out;
    is >> out;
    
    // Check for exception to unmarshall and throw, otherwise return.
    if (out.isstring() && out.name() && !strcmp(out.name(),"exception")) {
        // Reconstitute exception object.
        DDFJanitor jout(out);
        SAMLException* except=NULL;
        try { 
            istringstream es(out.string());
            except=SAMLException::getInstance(es);
        }
        catch (SAMLException& e) {
            log->error("caught SAML Exception while building the SAMLException: %s", e.what());
            log->error("XML was: %s", out.string());
            throw ListenerException("Remote call failed with an unparsable exception.");
        }

        auto_ptr<SAMLException> wrapper(except);
        wrapper->raise();
    }

    return out;
}

bool SocketListener::log_error() const
{
#ifdef WIN32
    int rc=WSAGetLastError();
#else
    int rc=errno;
#endif
#ifdef HAVE_STRERROR_R
    char buf[256];
    memset(buf,0,sizeof(buf));
    strerror_r(rc,buf,sizeof(buf));
    log->error("socket call resulted in error (%d): %s",rc,isprint(*buf) ? buf : "no message");
#else
    const char* buf=strerror(rc);
    log->error("socket call resulted in error (%d): %s",rc,isprint(*buf) ? buf : "no message");
#endif
    return false;
}

// actual function run in listener on server threads
void* server_thread_fn(void* arg)
{
    ServerThread* child = (ServerThread*)arg;

    // First, let's block all signals
    Thread::mask_all_signals();

    // Run the child until it exits.
    child->run();

    // Now we can clean up and exit the thread.
    delete child;
    return NULL;
}

ServerThread::ServerThread(SocketListener::ShibSocket& s, SocketListener* listener, unsigned long id)
    : m_sock(s), m_child(NULL), m_listener(listener)
{

    ostringstream buf;
    buf << "[" << id << "]";
    m_id = buf.str();

    // Create the child thread
    m_child = Thread::create(server_thread_fn, (void*)this);
    m_child->detach();
}

ServerThread::~ServerThread()
{
    // Then lock the children map, remove this socket/thread, signal waiters, and return
    m_listener->m_child_lock->lock();
    m_listener->m_children.erase(m_sock);
    m_listener->m_child_lock->unlock();
    m_listener->m_child_wait->signal();
  
    delete m_child;
}

void ServerThread::run()
{
    saml::NDC ndc(m_id);

    // Before starting up, make sure we fully "own" this socket.
    m_listener->m_child_lock->lock();
    while (m_listener->m_children.find(m_sock)!=m_listener->m_children.end())
        m_listener->m_child_wait->wait(m_listener->m_child_lock);
    m_listener->m_children[m_sock] = m_child;
    m_listener->m_child_lock->unlock();
    
    fd_set readfds;
    struct timeval tv = { 0, 0 };

    while(!*(m_listener->m_shutdown)) {
        FD_ZERO(&readfds);
        FD_SET(m_sock, &readfds);
        tv.tv_sec = 1;

        switch (select(m_sock+1, &readfds, 0, 0, &tv)) {
#ifdef WIN32
        case SOCKET_ERROR:
#else
        case -1:
#endif
            if (errno == EINTR) continue;
            m_listener->log_error();
            m_listener->log->error("select() on incoming request socket (%u) returned error", m_sock);
            return;

        case 0:
            break;

        default:
            if (!job()) {
                m_listener->log_error();
                m_listener->log->error("I/O failure processing request on socket (%u)", m_sock);
                m_listener->close(m_sock);
                return;
            }
        }
    }
}

bool ServerThread::job()
{
    Category& log = Category::getInstance("shibd.Listener");

    DDF out;
    DDFJanitor jout(out);
#ifdef WIN32
    u_long len;
#else
    uint32_t len;
#endif

    try {
        // Lock the configuration.
        IConfig* conf=ShibTargetConfig::getConfig().getINI();
        Locker locker(conf);

        // Read the message.
        if (m_listener->recv(m_sock,(char*)&len,sizeof(len)) != sizeof(len)) {
            log.error("error reading size of input message");
            return false;
        }
        len = ntohl(len);
        
        int size_read;
        stringstream is;
        while (len && (size_read = m_listener->recv(m_sock, m_buf, sizeof(m_buf))) > 0) {
            is.write(m_buf, size_read);
            len -= size_read;
        }
        
        if (len) {
            log.error("error reading input message from socket");
            return false;
        }
        
        // Unmarshall the message.
        DDF in;
        DDFJanitor jin(in);
        is >> in;

        // Dispatch the message.
        out=m_listener->receive(in);
    }
    catch (SAMLException &e) {
        log.error("error processing incoming message: %s", e.what());
        ostringstream os;
        os << e;
        out=DDF("exception").string(os.str().c_str());
    }
#ifndef _DEBUG
    catch (...) {
        log.error("unexpected error processing incoming message");
        ListenerException ex("An unexpected error occurred while processing an incoming message.");
        ostringstream os;
        os << ex;
        out=DDF("exception").string(os.str().c_str());
    }
#endif
    
    // Return whatever's available.
    ostringstream xmlout;
    xmlout << out;
    string response(xmlout.str());
    int outlen = response.length();
    len = htonl(outlen);
    if (m_listener->send(m_sock,(char*)&len,sizeof(len)) != sizeof(len)) {
        log.error("error sending output message size");
        return false;
    }
    if (m_listener->send(m_sock,response.c_str(),outlen) != outlen) {
        log.error("error sending output message");
        return false;
    }
    
    return true;
}
