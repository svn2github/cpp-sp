/**
 * Licensed to the University Corporation for Advanced Internet
 * Development, Inc. (UCAID) under one or more contributor license
 * agreements. See the NOTICE file distributed with this work for
 * additional information regarding copyright ownership.
 *
 * UCAID licenses this file to you under the Apache License,
 * Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the
 * License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 */

/**
 * TCPListener.cpp
 *
 * TCP-based SocketListener implementation.
 */

#include "internal.h"
#include "exceptions.h"
#include "remoting/impl/SocketListener.h"
#include "util/IPRange.h"

#include <xercesc/util/XMLUniDefs.hpp>
#include <xmltooling/unicode.h>
#include <xmltooling/util/XMLHelper.h>

#ifdef WIN32
# include <winsock2.h>
# include <ws2tcpip.h>
#endif

#ifdef HAVE_UNISTD_H
# include <sys/socket.h>
# include <sys/un.h>
# include <netdb.h>
# include <unistd.h>
# include <arpa/inet.h>
# include <netinet/in.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>		/* for chmod() */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

using namespace shibsp;
using namespace xmltooling;
using namespace xercesc;
using namespace std;

namespace shibsp {
    class TCPListener : virtual public SocketListener
    {
    public:
        TCPListener(const DOMElement* e);
        ~TCPListener() {}

        bool create(ShibSocket& s) const;
        bool bind(ShibSocket& s, bool force=false) const;
        bool connect(ShibSocket& s) const;
        bool close(ShibSocket& s) const;
        bool accept(ShibSocket& listener, ShibSocket& s) const;

        int send(ShibSocket& s, const char* buf, int len) const {
            return ::send(s, buf, len, 0);
        }

        int recv(ShibSocket& s, char* buf, int buflen) const {
            return ::recv(s, buf, buflen, 0);
        }

    private:
        bool setup_tcp_sockaddr();

        string m_address;
        unsigned short m_port;
        vector<IPRange> m_acl;
#ifdef HAVE_STRUCT_SOCKADDR_STORAGE
        struct sockaddr_storage m_sockaddr;
#else
        struct sockaddr_in m_sockaddr;
#endif
    };

    ListenerService* SHIBSP_DLLLOCAL TCPListenerServiceFactory(const DOMElement* const & e)
    {
        return new TCPListener(e);
    }

    static const XMLCh address[] = UNICODE_LITERAL_7(a,d,d,r,e,s,s);
    static const XMLCh port[] = UNICODE_LITERAL_4(p,o,r,t);
    static const XMLCh acl[] = UNICODE_LITERAL_3(a,c,l);
};

TCPListener::TCPListener(const DOMElement* e)
    : SocketListener(e),
      m_address(XMLHelper::getAttrString(e, getenv("SHIBSP_LISTENER_ADDRESS"), address)),
      m_port(XMLHelper::getAttrInt(e, 0, port))
{
    if (m_address.empty())
        m_address = "127.0.0.1";

    if (m_port == 0) {
        const char* p = getenv("SHIBSP_LISTENER_PORT");
        if (p && *p)
            m_port = atoi(p);
        if (m_port == 0)
            m_port = 1600;
    }

    int j = 0;
    string aclbuf = XMLHelper::getAttrString(e, "127.0.0.1", acl);
    for (unsigned int i = 0;  i < aclbuf.length();  ++i) {
        if (aclbuf.at(i) == ' ') {
            try {
                m_acl.push_back(IPRange::parseCIDRBlock(aclbuf.substr(j, i-j).c_str()));
            }
            catch (exception& ex) {
                log->error("invalid CIDR block (%s): %s", aclbuf.substr(j, i-j).c_str(), ex.what());
            }
            j = i + 1;
        }
    }
    try {
        m_acl.push_back(IPRange::parseCIDRBlock(aclbuf.substr(j, aclbuf.length()-j).c_str()));
    }
    catch (exception& ex) {
        log->error("invalid CIDR block (%s): %s", aclbuf.substr(j, aclbuf.length()-j).c_str(), ex.what());
    }

    if (m_acl.empty()) {
        log->warn("invalid CIDR range(s) in acl property, allowing 127.0.0.1 as a fall back");
        m_acl.push_back(IPRange::parseCIDRBlock("127.0.0.1"));
    }

    if (!setup_tcp_sockaddr()) {
        throw ConfigurationException("Unable to use configured socket address property.");
    }
}

bool TCPListener::setup_tcp_sockaddr()
{
    struct addrinfo* ret = nullptr;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;

    if (getaddrinfo(m_address.c_str(), nullptr, &hints, &ret) != 0) {
        log->error("unable to parse server address (%s)", m_address.c_str());
        return false;
    }

    if (ret->ai_family == AF_INET) {
        memcpy(&m_sockaddr, ret->ai_addr, ret->ai_addrlen);
        freeaddrinfo(ret);
        ((struct sockaddr_in*)&m_sockaddr)->sin_port=htons(m_port);
        return true;
    }
#if defined(AF_INET6) && defined(HAVE_STRUCT_SOCKADDR_STORAGE)
    else if (ret->ai_family == AF_INET6) {
        memcpy(&m_sockaddr, ret->ai_addr, ret->ai_addrlen);
        freeaddrinfo(ret);
        ((struct sockaddr_in6*)&m_sockaddr)->sin6_port=htons(m_port);
        return true;
    }
#endif

    log->error("unknown address type (%d)", ret->ai_family);
    freeaddrinfo(ret);
    return false;
}

bool TCPListener::create(ShibSocket& s) const
{
#ifdef HAVE_STRUCT_SOCKADDR_STORAGE
    s = socket(m_sockaddr.ss_family, SOCK_STREAM, 0);
#else
    s = socket(m_sockaddr.sin_family, SOCK_STREAM, 0);
#endif
#ifdef WIN32
    if(s == INVALID_SOCKET)
#else
    if (s < 0)
#endif
        return log_error("socket");
    return true;
}

bool TCPListener::bind(ShibSocket& s, bool force) const
{
    // XXX: Do we care about the return value from setsockopt?
    int opt = 1;
    ::setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

#ifdef WIN32
    if (SOCKET_ERROR==::bind(s, (const struct sockaddr*)&m_sockaddr, sizeof(m_sockaddr)) || SOCKET_ERROR==::listen(s, 3)) {
        log_error("bind");
        close(s);
        return false;
    }
#else
# ifdef HAVE_STRUCT_SOCKADDR_STORAGE
    if (::bind(s, (const struct sockaddr*)&m_sockaddr, m_sockaddr.ss_len) < 0)
# else
    if (::bind(s, (const struct sockaddr*)&m_sockaddr, m_sockaddr.sin_len) < 0)
# endif
    {
        log_error("bind");
        close(s);
        return false;
    }
    ::listen(s, 3);
#endif
    return true;
}

bool TCPListener::connect(ShibSocket& s) const
{
#ifdef WIN32
    if(SOCKET_ERROR==::connect(s, (const struct sockaddr*)&m_sockaddr, sizeof(m_sockaddr)))
        return log_error("connect");
#else
# ifdef HAVE_STRUCT_SOCKADDR_STORAGE
    if (::connect(s, (const struct sockaddr*)&m_sockaddr, m_sockaddr.ss_len) < 0)
# else
    if (::connect(s, (const struct sockaddr*)&m_sockaddr, m_sockaddr.sin_len) < 0)
# endif
        return log_error("connect");
#endif
    return true;
}

bool TCPListener::close(ShibSocket& s) const
{
#ifdef WIN32
    closesocket(s);
#else
    ::close(s);
#endif
    return true;
}

bool TCPListener::accept(ShibSocket& listener, ShibSocket& s) const
{
#ifdef HAVE_STRUCT_SOCKADDR_STORAGE
    struct sockaddr_storage addr;
#else
    struct sockaddr_in addr;
#endif
    memset(&addr, 0, sizeof(addr));

#ifdef WIN32
    int size=sizeof(addr);
    s=::accept(listener, (struct sockaddr*)&addr, &size);
    if(s==INVALID_SOCKET)
#else
    socklen_t size=sizeof(addr);
    s=::accept(listener, (struct sockaddr*)&addr, &size);
    if (s < 0)
#endif
        return log_error("accept");
    bool found = false;
    for (vector<IPRange>::const_iterator acl = m_acl.begin(); !found && acl != m_acl.end(); ++acl) {
        found = acl->contains((const struct sockaddr*)&addr);
    }
    if (!found) {
        close(s);
        s = -1;
        log->error("accept() rejected client with invalid address");
        return false;
    }
    return true;
}
