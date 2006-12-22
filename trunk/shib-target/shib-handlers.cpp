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

/*
 * shib-handlers.cpp -- profile handlers that plug into SP
 *
 * Scott Cantor
 * 5/17/2005
 */

#include "internal.h"

#include <ctime>
#include <saml/util/CommonDomainCookie.h>
#include <shibsp/SPConfig.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

using namespace shibsp;
using namespace shibtarget;
using namespace shibboleth;
using namespace saml;
using namespace log4cpp;
using namespace std;

using opensaml::CommonDomainCookie;

namespace {
  class SessionInitiator : virtual public IHandler
  {
  public:
    SessionInitiator(const DOMElement* e) {}
    ~SessionInitiator() {}
    pair<bool,void*> run(ShibTarget* st, bool isHandler=true) const;
    pair<bool,void*> ShibAuthnRequest(
        ShibTarget* st,
        const IHandler* shire,
        const char* dest,
        const char* target,
        const char* providerId
        ) const;
  };

  class SAML1Consumer : virtual public IHandler, public virtual IRemoted
  {
  public:
    SAML1Consumer(const DOMElement* e);
    ~SAML1Consumer();
    pair<bool,void*> run(ShibTarget* st, bool isHandler=true) const;
    DDF receive(const DDF& in);
  private:
    string m_address;
    static int counter;
  };

  int SAML1Consumer::counter = 0;

  class ShibLogout : virtual public IHandler
  {
  public:
    ShibLogout(const DOMElement* e) {}
    ~ShibLogout() {}
    pair<bool,void*> run(ShibTarget* st, bool isHandler=true) const;
  };
}


IPlugIn* ShibSessionInitiatorFactory(const DOMElement* e)
{
    return new SessionInitiator(e);
}

IPlugIn* SAML1POSTFactory(const DOMElement* e)
{
    return new SAML1Consumer(e);
}

IPlugIn* SAML1ArtifactFactory(const DOMElement* e)
{
    return new SAML1Consumer(e);
}

IPlugIn* ShibLogoutFactory(const DOMElement* e)
{
    return new ShibLogout(e);
}

pair<bool,void*> SessionInitiator::run(ShibTarget* st, bool isHandler) const
{
    string dupresource;
    const char* resource=NULL;
    const IHandler* ACS=NULL;
    const IApplication* app=st->getApplication();
    
    if (isHandler) {
        /* 
         * Binding is CGI query string with:
         *  target      the resource to direct back to later
         *  acsIndex    optional index of an ACS to use on the way back in
         *  providerId  optional direct invocation of a specific IdP
         */
        const char* option=st->getRequestParameter("acsIndex");
        if (option)
            ACS=app->getAssertionConsumerServiceByIndex(atoi(option));
        option=st->getRequestParameter("providerId");
        
        resource=st->getRequestParameter("target");
        if (!resource || !*resource) {
            pair<bool,const char*> home=app->getString("homeURL");
            if (home.first)
                resource=home.second;
            else
                throw FatalProfileException("Session initiator requires a target parameter or a homeURL application property.");
        }
        else if (!option) {
            dupresource=resource;
            resource=dupresource.c_str();
        }
        
        if (option) {
            // Here we actually use metadata to invoke the SSO service directly.
            // The only currently understood binding is the Shibboleth profile.
            Metadata m(app->getMetadataProviders());
            const IEntityDescriptor* entity=m.lookup(option);
            if (!entity)
                throw MetadataException("Session initiator unable to locate metadata for provider ($1).", params(1,option));
            const IIDPSSODescriptor* role=entity->getIDPSSODescriptor(Constants::SHIB_NS);
            if (!role)
                throw MetadataException(
                    "Session initiator unable to locate a Shibboleth-aware identity provider role for provider ($1).", params(1,option)
                    );
            const IEndpointManager* SSO=role->getSingleSignOnServiceManager();
            const IEndpoint* ep=SSO->getEndpointByBinding(Constants::SHIB_AUTHNREQUEST_PROFILE_URI);
            if (!ep)
                throw MetadataException(
                    "Session initiator unable to locate compatible SSO service for provider ($1).", params(1,option)
                    );
            auto_ptr_char dest(ep->getLocation());
            return ShibAuthnRequest(
                st,ACS ? ACS : app->getDefaultAssertionConsumerService(),dest.get(),resource,app->getString("providerId").second
                );
        }
    }
    else {
        // We're running as a "virtual handler" from within the filter.
        // The target resource is the current one and everything else is defaulted.
        resource=st->getRequestURL();
    }
    
    if (!ACS) ACS=app->getDefaultAssertionConsumerService();
    
    // For now, we only support external session initiation via a wayfURL
    pair<bool,const char*> wayfURL=getProperties()->getString("wayfURL");
    if (!wayfURL.first)
        throw ConfigurationException("Session initiator is missing wayfURL property.");

    pair<bool,const XMLCh*> wayfBinding=getProperties()->getXMLString("wayfBinding");
    if (!wayfBinding.first || !XMLString::compareString(wayfBinding.second,Constants::SHIB_AUTHNREQUEST_PROFILE_URI))
        // Standard Shib 1.x
        return ShibAuthnRequest(st,ACS,wayfURL.second,resource,app->getString("providerId").second);
    else if (!XMLString::compareString(wayfBinding.second,Constants::SHIB_LEGACY_AUTHNREQUEST_PROFILE_URI))
        // Shib pre-1.2
        return ShibAuthnRequest(st,ACS,wayfURL.second,resource,NULL);
    else if (!strcmp(getProperties()->getString("wayfBinding").second,"urn:mace:shibboleth:1.0:profiles:EAuth")) {
        // TODO: Finalize E-Auth profile URI
        pair<bool,bool> localRelayState=st->getConfig()->getPropertySet("InProcess")->getBool("localRelayState");
        if (!localRelayState.first || !localRelayState.second)
            throw ConfigurationException("E-Authn requests cannot include relay state, so localRelayState must be enabled.");

        // Here we store the state in a cookie.
        pair<string,const char*> shib_cookie=st->getCookieNameProps("_shibstate_");
        st->setCookie(shib_cookie.first,ShibTarget::url_encode(resource) + shib_cookie.second);
        return make_pair(true, st->sendRedirect(wayfURL.second));
    }
   
    throw UnsupportedProfileException("Unsupported WAYF binding ($1).", params(1,getProperties()->getString("wayfBinding").second));
}

// Handles Shib 1.x AuthnRequest profile.
pair<bool,void*> SessionInitiator::ShibAuthnRequest(
    ShibTarget* st,
    const IHandler* shire,
    const char* dest,
    const char* target,
    const char* providerId
    ) const
{
    // Compute the ACS URL. We add the ACS location to the base handlerURL.
    // Legacy configs will not have the Location property specified, so no suffix will be added.
    string ACSloc=st->getHandlerURL(target);
    pair<bool,const char*> loc=shire ? shire->getProperties()->getString("Location") : pair<bool,const char*>(false,NULL);
    if (loc.first) ACSloc+=loc.second;
    
    char timebuf[16];
    sprintf(timebuf,"%u",time(NULL));
    string req=string(dest) + "?shire=" + ShibTarget::url_encode(ACSloc.c_str()) + "&time=" + timebuf;

    // How should the resource value be preserved?
    pair<bool,bool> localRelayState=st->getConfig()->getPropertySet("InProcess")->getBool("localRelayState");
    if (!localRelayState.first || !localRelayState.second) {
        // The old way, just send it along.
        req+="&target=" + ShibTarget::url_encode(target);
    }
    else {
        // Here we store the state in a cookie and send a fixed
        // value to the IdP so we can recognize it on the way back.
        pair<string,const char*> shib_cookie=st->getCookieNameProps("_shibstate_");
        st->setCookie(shib_cookie.first,ShibTarget::url_encode(target) + shib_cookie.second);
        req+="&target=cookie";
    }
    
    // Only omitted for 1.1 style requests.
    if (providerId)
        req+="&providerId=" + ShibTarget::url_encode(providerId);

    return make_pair(true, st->sendRedirect(req));
}

SAML1Consumer::SAML1Consumer(const DOMElement* e)
{
    m_address += ('A' + (counter++));
    m_address += "::SAML1Consumer::run";

    // Register for remoted messages.
    if (SPConfig::getConfig().isEnabled(SPConfig::OutOfProcess)) {
        IListener* listener=ShibTargetConfig::getConfig().getINI()->getListener();
        if (listener)
            listener->regListener(m_address.c_str(),this);
        else
            throw ListenerException("Plugin requires a Listener service");
    }
}

SAML1Consumer::~SAML1Consumer()
{
    IListener* listener=ShibTargetConfig::getConfig().getINI()->getListener();
    if (listener && SPConfig::getConfig().isEnabled(SPConfig::OutOfProcess))
        listener->unregListener(m_address.c_str(),this);
    counter--;
}

/*
 * IPC message definitions:
 * 
 *  [A-Z]::SAML1Consumer::run
 * 
 *      IN
 *      application_id
 *      client_address
 *      recipient
 *      SAMLResponse or SAMLart list
 * 
 *      OUT
 *      key
 *      provider_id
 */
DDF SAML1Consumer::receive(const DDF& in)
{
#ifdef _DEBUG
    saml::NDC ndc("receive");
#endif
    Category& log=Category::getInstance(SHIBT_LOGCAT".SAML1Consumer");

    // Find application.
    const char* aid=in["application_id"].string();
    const IApplication* app=aid ? ShibTargetConfig::getConfig().getINI()->getApplication(aid) : NULL;
    if (!app) {
        // Something's horribly wrong.
        log.error("couldn't find application (%s) for new session", aid ? aid : "(missing)");
        throw SAMLException("Unable to locate application for new session, deleted?");
    }

    // Check required parameters.
    const char* client_address=in["client_address"].string();
    const char* recipient=in["recipient"].string();
    if (!client_address || !recipient)
        throw SAMLException("Required parameters missing in call to SAML1Consumer::run");
    
    log.debug("processing new assertion for %s", client_address);
    log.debug("recipient: %s", recipient);
    log.debug("application: %s", app->getId());

    // Access the application config. It's already locked behind us.
    STConfig& stc=static_cast<STConfig&>(ShibTargetConfig::getConfig());
    IConfig* conf=stc.getINI();

    auto_ptr_XMLCh wrecipient(recipient);

    pair<bool,bool> checkAddress=pair<bool,bool>(false,true);
    pair<bool,bool> checkReplay=pair<bool,bool>(false,true);
    const IPropertySet* props=app->getPropertySet("Sessions");
    if (props) {
        checkAddress=props->getBool("checkAddress");
        if (!checkAddress.first)
            checkAddress.second=true;
        checkReplay=props->getBool("checkReplay");
        if (!checkReplay.first)
            checkReplay.second=true;
    }

    // Supports either version...
    pair<bool,unsigned int> version=getProperties()->getUnsignedInt("MinorVersion","urn:oasis:names:tc:SAML:1.0:protocol");
    if (!version.first)
        version.second=1;

    const IRoleDescriptor* role=NULL;
    Metadata m(app->getMetadataProviders());
    SAMLBrowserProfile::BrowserProfileResponse bpr;

    try {
        const char* samlResponse=in["SAMLResponse"].string();
        if (samlResponse) {
            // POST profile
            log.debug("executing Browser/POST profile...");
            bpr=app->getBrowserProfile()->receive(
                samlResponse,
                wrecipient.get(),
                checkReplay.second ? conf->getReplayCache() : NULL,
                version.second
                );
        }
        else {
            // Artifact profile
            vector<const char*> SAMLart;
            DDF arts=in["SAMLart"];
            DDF art=arts.first();
            while (art.isstring()) {
                SAMLart.push_back(art.string());
                art=arts.next();
            }
            auto_ptr<SAMLBrowserProfile::ArtifactMapper> artifactMapper(app->getArtifactMapper());
            log.debug("executing Browser/Artifact profile...");
            bpr=app->getBrowserProfile()->receive(
                SAMLart,
                wrecipient.get(),
                artifactMapper.get(),
                checkReplay.second ? conf->getReplayCache() : NULL,
                version.second
                );

            // Blow it away to clear any locks that might be held.
            delete artifactMapper.release();
        }

        // Try and map to metadata (again).
        // Once the metadata layer is in the SAML core, the repetition should be fixed.
        const IEntityDescriptor* provider=m.lookup(bpr.assertion->getIssuer());
        if (!provider && bpr.authnStatement->getSubject()->getNameIdentifier() &&
                bpr.authnStatement->getSubject()->getNameIdentifier()->getNameQualifier())
            provider=m.lookup(bpr.authnStatement->getSubject()->getNameIdentifier()->getNameQualifier());
        if (provider) {
            const IIDPSSODescriptor* IDP=provider->getIDPSSODescriptor(
                version.second==1 ? saml::XML::SAML11_PROTOCOL_ENUM : saml::XML::SAML10_PROTOCOL_ENUM
                );
            role=IDP;
        }
        
        // This isn't likely, since the profile must have found a role.
        if (!role) {
            MetadataException ex("Unable to locate role-specific metadata for identity provider.");
            annotateException(&ex,provider); // throws it
        }
    
        // Maybe verify the client address....
        if (checkAddress.second) {
            log.debug("verifying client address");
            // Verify the client address exists
            const XMLCh* wip = bpr.authnStatement->getSubjectIP();
            if (wip && *wip) {
                // Verify the client address matches authentication
                auto_ptr_char this_ip(wip);
                if (strcmp(client_address, this_ip.get())) {
                    FatalProfileException ex(
                        SESSION_E_ADDRESSMISMATCH,
                       "Your client's current address ($1) differs from the one used when you authenticated "
                        "to your identity provider. To correct this problem, you may need to bypass a proxy server. "
                        "Please contact your local support staff or help desk for assistance.",
                        params(1,client_address)
                        );
                    annotateException(&ex,role); // throws it
                }
            }
        }
    }
    catch (SAMLException&) {
        bpr.clear();
        throw;
    }
    catch (...) {
        log.error("caught unknown exception");
        bpr.clear();
#ifdef _DEBUG
        throw;
#else
        SAMLException e("An unexpected error occurred while creating your session.");
        annotateException(&e,role);
#endif
    }

    // It passes all our tests -- create a new session.
    log.info("creating new session");

    DDF out;
    try {
        // Insert into cache.
        auto_ptr_char authContext(bpr.authnStatement->getAuthMethod());
        string key=conf->getSessionCache()->insert(
            app,
            role->getEntityDescriptor(),
            client_address,
            bpr.authnStatement->getSubject(),
            authContext.get(),
            bpr.response
            );
        // objects owned by cache now
        log.debug("new session id: %s", key.c_str());
        auto_ptr_char oname(role->getEntityDescriptor()->getId());
        out=DDF(NULL).structure();
        out.addmember("key").string(key.c_str());
        out.addmember("provider_id").string(oname.get());
    }
    catch (...) {
#ifdef _DEBUG
        throw;
#else
        SAMLException e("An unexpected error occurred while creating your session.");
        annotateException(&e,role);
#endif
    }

    return out;
}

pair<bool,void*> SAML1Consumer::run(ShibTarget* st, bool isHandler) const
{
    DDF in,out;
    DDFJanitor jin(in),jout(out);

    pair<bool,const XMLCh*> binding=getProperties()->getXMLString("Binding");
    if (!binding.first || !XMLString::compareString(binding.second,SAMLBrowserProfile::BROWSER_POST)) {
#ifdef HAVE_STRCASECMP
        if (strcasecmp(st->getRequestMethod(), "POST")) {
#else
        if (_stricmp(st->getRequestMethod(), "POST")) {
#endif
            st->log(ShibTarget::LogLevelInfo, "SAML 1.x Browser/POST handler ignoring non-POST request");
            return pair<bool,void*>(false,NULL);
        }
#ifdef HAVE_STRCASECMP
        if (!st->getContentType() || strcasecmp(st->getContentType(),"application/x-www-form-urlencoded")) {
#else
        if (!st->getContentType() || _stricmp(st->getContentType(),"application/x-www-form-urlencoded")) {
#endif
            st->log(ShibTarget::LogLevelInfo, "SAML 1.x Browser/POST handler ignoring submission with unknown content-type.");
            return pair<bool,void*>(false,NULL);
        }

        const char* samlResponse = st->getRequestParameter("SAMLResponse");
        if (!samlResponse) {
            st->log(ShibTarget::LogLevelInfo, "SAML 1.x Browser/POST handler ignoring request with no SAMLResponse parameter.");
            return pair<bool,void*>(false,NULL);
        }

        in=DDF(m_address.c_str()).structure();
        in.addmember("SAMLResponse").string(samlResponse);
    }
    else if (!XMLString::compareString(binding.second,SAMLBrowserProfile::BROWSER_ARTIFACT)) {
#ifdef HAVE_STRCASECMP
        if (strcasecmp(st->getRequestMethod(), "GET")) {
#else
        if (_stricmp(st->getRequestMethod(), "GET")) {
#endif
            st->log(ShibTarget::LogLevelInfo, "SAML 1.x Browser/Artifact handler ignoring non-GET request");
            return pair<bool,void*>(false,NULL);
        }

        const char* SAMLart=st->getRequestParameter("SAMLart");
        if (!SAMLart) {
            st->log(ShibTarget::LogLevelInfo, "SAML 1.x Browser/Artifact handler ignoring request with no SAMLart parameter.");
            return pair<bool,void*>(false,NULL);
        }

        in=DDF(m_address.c_str()).structure();
        DDF artlist=in.addmember("SAMLart").list();

        while (SAMLart) {
            artlist.add(DDF(NULL).string(SAMLart));
            SAMLart=st->getRequestParameter("SAMLart",artlist.integer());
        }
    }
    
    // Compute the endpoint location.
    string hURL=st->getHandlerURL(st->getRequestURL());
    pair<bool,const char*> loc=getProperties()->getString("Location");
    string recipient=loc.first ? hURL + loc.second : hURL;
    in.addmember("recipient").string(recipient.c_str());

    // Add remaining parameters.
    in.addmember("application_id").string(st->getApplication()->getId());
    in.addmember("client_address").string(st->getRemoteAddr());

    out=st->getConfig()->getListener()->send(in);
    if (!out["key"].isstring())
        throw FatalProfileException("Remote processing of SAML 1.x Browser profile did not return a usable session key.");
    string key=out["key"].string();

    st->log(ShibTarget::LogLevelDebug, string("profile processing succeeded, new session created (") + key + ")");

    const char* target=st->getRequestParameter("TARGET");
    if (target && !strcmp(target,"default")) {
        pair<bool,const char*> homeURL=st->getApplication()->getString("homeURL");
        target=homeURL.first ? homeURL.second : "/";
    }
    else if (!target || !strcmp(target,"cookie")) {
        // Pull the target value from the "relay state" cookie.
        pair<string,const char*> relay_cookie = st->getCookieNameProps("_shibstate_");
        const char* relay_state = st->getCookie(relay_cookie.first);
        if (!relay_state || !*relay_state) {
            // No apparent relay state value to use, so fall back on the default.
            pair<bool,const char*> homeURL=st->getApplication()->getString("homeURL");
            target=homeURL.first ? homeURL.second : "/";
        }
        else {
            char* rscopy=strdup(relay_state);
            ShibTarget::url_decode(rscopy);
            hURL=rscopy;
            free(rscopy);
            target=hURL.c_str();
        }
        st->setCookie(relay_cookie.first,relay_cookie.second);
    }

    // We've got a good session, set the session cookie.
    pair<string,const char*> shib_cookie=st->getCookieNameProps("_shibsession_");
    st->setCookie(shib_cookie.first, key + shib_cookie.second);

    const char* providerId=out["provider_id"].string();
    if (providerId) {
        const IPropertySet* sessionProps=st->getApplication()->getPropertySet("Sessions");
        pair<bool,bool> idpHistory=sessionProps->getBool("idpHistory");
        if (!idpHistory.first || idpHistory.second) {
            // Set an IdP history cookie locally (essentially just a CDC).
            CommonDomainCookie cdc(st->getCookie(CommonDomainCookie::CDCName));

            // Either leave in memory or set an expiration.
            pair<bool,unsigned int> days=sessionProps->getUnsignedInt("idpHistoryDays");
                if (!days.first || days.second==0)
                    st->setCookie(CommonDomainCookie::CDCName,string(cdc.set(providerId)) + shib_cookie.second);
                else {
                    time_t now=time(NULL) + (days.second * 24 * 60 * 60);
#ifdef HAVE_GMTIME_R
                    struct tm res;
                    struct tm* ptime=gmtime_r(&now,&res);
#else
                    struct tm* ptime=gmtime(&now);
#endif
                    char timebuf[64];
                    strftime(timebuf,64,"%a, %d %b %Y %H:%M:%S GMT",ptime);
                    st->setCookie(
                        CommonDomainCookie::CDCName,
                        string(cdc.set(providerId)) + shib_cookie.second + "; expires=" + timebuf
                        );
            }
        }
    }

    // Now redirect to the target.
    return make_pair(true, st->sendRedirect(target));
}

pair<bool,void*> ShibLogout::run(ShibTarget* st, bool isHandler) const
{
    // Recover the session key.
    pair<string,const char*> shib_cookie = st->getCookieNameProps("_shibsession_");
    const char* session_id = st->getCookie(shib_cookie.first);
    
    // Logout is best effort.
    if (session_id && *session_id) {
        try {
            st->getConfig()->getSessionCache()->remove(session_id,st->getApplication(),st->getRemoteAddr());
        }
        catch (SAMLException& e) {
            st->log(ShibTarget::LogLevelError, string("logout processing failed with exception: ") + e.what());
        }
#ifndef _DEBUG
        catch (...) {
            st->log(ShibTarget::LogLevelError, "logout processing failed with unknown exception");
        }
#endif
        // We send the cookie property alone, which acts as an empty value.
        st->setCookie(shib_cookie.first,shib_cookie.second);
    }
    
    const char* ret=st->getRequestParameter("return");
    if (!ret)
        ret=getProperties()->getString("ResponseLocation").second;
    if (!ret)
        ret=st->getApplication()->getString("homeURL").second;
    if (!ret)
        ret="/";
    return make_pair(true, st->sendRedirect(ret));
}
