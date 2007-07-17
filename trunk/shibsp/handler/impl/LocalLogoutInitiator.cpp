/*
 *  Copyright 2001-2007 Internet2
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
 * LocalLogoutInitiator.cpp
 * 
 * Logs out a session locally.
 */

#include "internal.h"
#include "Application.h"
#include "ServiceProvider.h"
#include "SessionCache.h"
#include "handler/AbstractHandler.h"
#include "handler/LogoutHandler.h"

using namespace shibsp;
using namespace xmltooling;
using namespace log4cpp;
using namespace std;

namespace shibsp {

#if defined (_MSC_VER)
    #pragma warning( push )
    #pragma warning( disable : 4250 )
#endif

    class SHIBSP_DLLLOCAL LocalLogoutInitiator : public AbstractHandler, public LogoutHandler
    {
    public:
        LocalLogoutInitiator(const DOMElement* e, const char* appId);
        virtual ~LocalLogoutInitiator() {}
        
        void setParent(const PropertySet* parent);
        pair<bool,long> run(SPRequest& request, bool isHandler=true) const;

    private:
        string m_appId;
        vector<Handler*> m_handlers;
    };

#if defined (_MSC_VER)
    #pragma warning( pop )
#endif

    Handler* SHIBSP_DLLLOCAL LocalLogoutInitiatorFactory(const pair<const DOMElement*,const char*>& p)
    {
        return new LocalLogoutInitiator(p.first, p.second);
    }
};

LocalLogoutInitiator::LocalLogoutInitiator(const DOMElement* e, const char* appId)
    : AbstractHandler(e, Category::getInstance(SHIBSP_LOGCAT".LogoutInitiator")), m_appId(appId)
{
    pair<bool,const char*> loc = getString("Location");
    if (loc.first) {
        string address = string(appId) + loc.second + "::run::LocalLI";
        setAddress(address.c_str());
    }
}

void LocalLogoutInitiator::setParent(const PropertySet* parent)
{
    DOMPropertySet::setParent(parent);
    pair<bool,const char*> loc = getString("Location");
    if (loc.first) {
        string address = m_appId + loc.second + "::run::LocalLI";
        setAddress(address.c_str());
    }
    else {
        m_log.warn("no Location property in Local LogoutInitiator (or parent), can't register as remoted handler");
    }
}

pair<bool,long> LocalLogoutInitiator::run(SPRequest& request, bool isHandler) const
{
    // Defer to base class first.
    pair<bool,long> ret = LogoutHandler::run(request, isHandler);
    if (ret.first)
        return ret;

    // Get session ID from cookie.
    pair<string,const char*> shib_cookie = request.getApplication().getCookieNameProps("_shibsession_");
    const char* session_id = request.getCookie(shib_cookie.first.c_str());
    if (session_id) {
        // Do back channel notification.
        vector<string> sessions(1, session_id);
        if (!notifyBackChannel(request.getApplication(), request.getRequestURL(), sessions, true)) {
            request.getApplication().getServiceProvider().getSessionCache()->remove(session_id, request.getApplication());
            return sendLogoutPage(request.getApplication(), request, true, "Partial logout failure.");
        }
        request.getServiceProvider().getSessionCache()->remove(session_id, request.getApplication());
    }

    return sendLogoutPage(request.getApplication(), request, true, "Logout was successful.");
}
