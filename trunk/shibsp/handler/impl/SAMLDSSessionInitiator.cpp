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
 * SAMLDSSessionInitiator.cpp
 * 
 * SAML Discovery Service support.
 */

#include "internal.h"
#include "Application.h"
#include "exceptions.h"
#include "SPRequest.h"
#include "handler/AbstractHandler.h"
#include "handler/SessionInitiator.h"

#include <xmltooling/XMLToolingConfig.h>
#include <xmltooling/util/URLEncoder.h>

using namespace shibsp;
using namespace opensaml;
using namespace xmltooling;
using namespace std;

namespace shibsp {

#if defined (_MSC_VER)
    #pragma warning( push )
    #pragma warning( disable : 4250 )
#endif

    class SHIBSP_DLLLOCAL SAMLDSSessionInitiator : public SessionInitiator, public AbstractHandler
    {
    public:
        SAMLDSSessionInitiator(const DOMElement* e, const char* appId)
                : AbstractHandler(e, Category::getInstance(SHIBSP_LOGCAT".SessionInitiator")), m_url(NULL), m_returnParam(NULL) {
            pair<bool,const char*> url = getString("URL");
            if (!url.first)
                throw ConfigurationException("SAMLDS SessionInitiator requires a URL property.");
            m_url = url.second;
            url = getString("entityIDParam");
            if (url.first)
                m_returnParam = url.second;
        }
        virtual ~SAMLDSSessionInitiator() {}
        
        pair<bool,long> run(SPRequest& request, const char* entityID=NULL, bool isHandler=true) const;

    private:
        const char* m_url;
        const char* m_returnParam;
    };

#if defined (_MSC_VER)
    #pragma warning( pop )
#endif

    SessionInitiator* SHIBSP_DLLLOCAL SAMLDSSessionInitiatorFactory(const pair<const DOMElement*,const char*>& p)
    {
        return new SAMLDSSessionInitiator(p.first, p.second);
    }

};

pair<bool,long> SAMLDSSessionInitiator::run(SPRequest& request, const char* entityID, bool isHandler) const
{
    // The IdP CANNOT be specified for us to run. Otherwise, we'd be redirecting to a DS
    // anytime the IdP's metadata was wrong.
    if (entityID && *entityID)
        return make_pair(false,0);

    string target;
    const char* option;
    bool isPassive=false;
    const Application& app=request.getApplication();

    if (isHandler) {
        option = request.getParameter("SAMLDS");
        if (option && !strcmp(option,"1"))
            throw saml2md::MetadataException("No identity provider was selected by user.");
        
        option = request.getParameter("target");
        if (option)
            target = option;
        recoverRelayState(request.getApplication(), request, target, false);

        option = request.getParameter("isPassive");
        if (option)
            isPassive = !strcmp(option,"true");
    }
    else {
        // We're running as a "virtual handler" from within the filter.
        // The target resource is the current one and everything else is
        // defaulted or set by content policy.
        target=request.getRequestURL();
        pair<bool,bool> passopt = getBool("isPassive");
        isPassive = passopt.first && passopt.second;
    }

    m_log.debug("sending request to SAMLDS (%s)", m_url);

    // Compute the return URL. We start with a self-referential link.
    string returnURL=request.getHandlerURL(target.c_str());
    pair<bool,const char*> thisloc = getString("Location");
    if (thisloc.first) returnURL += thisloc.second;
    returnURL += "?SAMLDS=1"; // signals us not to loop if we get no answer back

    if (isHandler) {
        // We may already have RelayState set if we looped back here,
        // but just in case target is a resource, we reset it back.
        option = request.getParameter("target");
        if (option)
            target = option;
    }
    preserveRelayState(request.getApplication(), request, target);

    const URLEncoder* urlenc = XMLToolingConfig::getConfig().getURLEncoder();
    if (isHandler) {
        // Now the hard part. The base assumption is to append the entire query string, if any,
        // to the self-link. But we want to replace target with the RelayState-preserved value
        // to hide it from the DS.
        const char* query = request.getQueryString();
        if (query) {
            // See if it starts with target.
            if (!strncmp(query, "target=", 7)) {
                // We skip this altogether and advance the query past it to the first separator.
                query = strchr(query, '&');
                // If we still have more, just append it.
                if (query && *(++query))
                    returnURL = returnURL + '&' + query;
            }
            else {
                // There's something in the query before target appears, so we have to find it.
                thisloc.second = strstr(query,"&target=");
                if (thisloc.second) {
                    // We found it, so first append everything up to it.
                    returnURL += '&';
                    returnURL.append(query, thisloc.second - query);
                    query = thisloc.second + 8; // move up just past the equals sign.
                    thisloc.second = strchr(query, '&');
                    if (thisloc.second)
                        returnURL += thisloc.second;
                }
                else {
                    // No target in the existing query, so just append it as is.
                    returnURL = returnURL + '&' + query;
                }
            }
        }

        // Now append the sanitized target as needed.
        if (!target.empty())
            returnURL = returnURL + "&target=" + urlenc->encode(target.c_str());
    }
    else if (!target.empty()) {
        // For a virtual handler, we just append target to the return link.
        returnURL = returnURL + "&target=" + urlenc->encode(target.c_str());;
    }

    string req=string(m_url) + (strchr(m_url,'?') ? '&' : '?') + "entityID=" + urlenc->encode(app.getString("entityID").second) +
        "&return=" + urlenc->encode(returnURL.c_str());
    if (m_returnParam)
        req = req + "&returnIDParam=" + m_returnParam;
    if (isPassive)
        req += "&isPassive=true";

    return make_pair(true, request.sendRedirect(req.c_str()));
}
