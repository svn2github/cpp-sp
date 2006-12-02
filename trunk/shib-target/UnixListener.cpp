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
 * UnixListener.cpp
 * 
 * Unix Domain-based SocketListener implementation
 */

#include "RPCListener.h"

#ifdef HAVE_UNISTD_H
# include <sys/socket.h>
# include <sys/un.h>
# include <unistd.h>
# include <arpa/inet.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>		/* for chmod() */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

using namespace std;
using namespace saml;
using namespace shibboleth;
using namespace shibtarget;
using namespace log4cpp;

static const XMLCh address[] = { chLatin_a, chLatin_d, chLatin_d, chLatin_r, chLatin_e, chLatin_s, chLatin_s, chNull };

class UnixListener : virtual public SocketListener
{
public:
    UnixListener(const DOMElement* e);
    ~UnixListener() {if (m_bound) unlink(m_address.c_str());}

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
    string m_address;
    mutable bool m_bound;
};

IPlugIn* UnixListenerFactory(const DOMElement* e)
{
    return new UnixListener(e);
}

UnixListener::UnixListener(const DOMElement* e) : RPCListener(e), m_address("/var/run/shar-socket"), m_bound(false)
{
    // We're stateless, but we need to load the configuration.
    const XMLCh* tag=e->getAttributeNS(NULL,address);
    if (tag && *tag) {
        auto_ptr_char a(tag);
        m_address=a.get();
    }
}

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 100
#endif

bool UnixListener::create(ShibSocket& sock) const
{
    sock = socket(PF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)
        return log_error();
    return true;
}

bool UnixListener::bind(ShibSocket& s, bool force) const
{
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof (addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, m_address.c_str(), UNIX_PATH_MAX);

    if (force)
        unlink(m_address.c_str());

    if (::bind(s, (struct sockaddr *)&addr, sizeof (addr)) < 0) {
        log_error();
        close(s);
        return false;
    }

    // Make sure that only the creator can read -- we don't want just
    // anyone connecting, do we?
    if (chmod(m_address.c_str(),0777) < 0) {
        log_error();
        close(s);
        unlink(m_address.c_str());
        return false;
    }

    listen(s, 3);
    return m_bound=true;
}

bool UnixListener::connect(ShibSocket& s) const
{
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof (addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, m_address.c_str(), UNIX_PATH_MAX);

    if (::connect(s, (struct sockaddr *)&addr, sizeof (addr)) < 0)
        return log_error();
    return true;
}

bool UnixListener::close(ShibSocket& s) const
{
    ::close(s);
    return true;
}

bool UnixListener::accept(ShibSocket& listener, ShibSocket& s) const
{
    s=::accept(listener,NULL,NULL);
    if (s < 0)
        return log_error();
    return true;
}

CLIENT* UnixListener::getClientHandle(ShibSocket& s, u_long program, u_long version) const
{
    struct sockaddr_in sin;
    memset (&sin, 0, sizeof (sin));
    sin.sin_port = 1;
    return clnttcp_create(&sin, program, version, &s, 0, 0);
}
