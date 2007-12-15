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
 * XMLServiceProvider.cpp
 *
 * XML-based SP configuration and mgmt
 */

#include "internal.h"
#include "exceptions.h"
#include "AccessControl.h"
#include "Application.h"
#include "RequestMapper.h"
#include "ServiceProvider.h"
#include "SessionCache.h"
#include "SPConfig.h"
#include "handler/SessionInitiator.h"
#include "remoting/ListenerService.h"
#include "util/DOMPropertySet.h"
#include "util/SPConstants.h"

#if defined(XMLTOOLING_LOG4SHIB)
# include <log4shib/PropertyConfigurator.hh>
#elif defined(XMLTOOLING_LOG4CPP)
# include <log4cpp/PropertyConfigurator.hh>
#else
# error "Supported logging library not available."
#endif
#include <xercesc/util/XMLUniDefs.hpp>
#include <xmltooling/XMLToolingConfig.h>
#include <xmltooling/util/NDC.h>
#include <xmltooling/util/ReloadableXMLFile.h>
#include <xmltooling/util/XMLHelper.h>

#ifndef SHIBSP_LITE
# include "TransactionLog.h"
# include "attribute/filtering/AttributeFilter.h"
# include "attribute/resolver/AttributeExtractor.h"
# include "attribute/resolver/AttributeResolver.h"
# include "security/PKIXTrustEngine.h"
# include <saml/SAMLConfig.h>
# include <saml/binding/ArtifactMap.h>
# include <saml/binding/SAMLArtifact.h>
# include <saml/saml1/core/Assertions.h>
# include <saml/saml2/binding/SAML2ArtifactType0004.h>
# include <saml/saml2/metadata/ChainingMetadataProvider.h>
# include <xmltooling/security/ChainingTrustEngine.h>
# include <xmltooling/util/ReplayCache.h>
using namespace opensaml::saml2;
using namespace opensaml::saml2p;
using namespace opensaml::saml2md;
using namespace opensaml;
#endif

using namespace shibsp;
using namespace xmltooling;
using namespace std;

namespace {

#if defined (_MSC_VER)
    #pragma warning( push )
    #pragma warning( disable : 4250 )
#endif

    static vector<const Handler*> g_noHandlers;

    // Application configuration wrapper
    class SHIBSP_DLLLOCAL XMLApplication : public Application, public Remoted, public DOMPropertySet, public DOMNodeFilter
    {
    public:
        XMLApplication(const ServiceProvider*, const DOMElement* e, const XMLApplication* base=NULL);
        ~XMLApplication() { cleanup(); }
    
        const char* getHash() const {return m_hash.c_str();}

#ifndef SHIBSP_LITE
        SAMLArtifact* generateSAML1Artifact(const EntityDescriptor* relyingParty) const {
            throw ConfigurationException("No support for SAML 1.x artifact generation.");
        }
        SAML2Artifact* generateSAML2Artifact(const EntityDescriptor* relyingParty) const {
            pair<bool,int> index = make_pair(false,0);
            const PropertySet* props = getRelyingParty(relyingParty);
            if (props)
                index = getInt("artifactEndpointIndex");
            if (!index.first)
                index = getArtifactEndpointIndex();
            return new SAML2ArtifactType0004(SAMLConfig::getConfig().hashSHA1(getString("entityID").second),index.first ? index.second : 1);
        }

        MetadataProvider* getMetadataProvider(bool required=true) const {
            if (required && !m_base && !m_metadata)
                throw ConfigurationException("No MetadataProvider available.");
            return (!m_metadata && m_base) ? m_base->getMetadataProvider() : m_metadata;
        }
        TrustEngine* getTrustEngine(bool required=true) const {
            if (required && !m_base && !m_trust)
                throw ConfigurationException("No TrustEngine available.");
            return (!m_trust && m_base) ? m_base->getTrustEngine() : m_trust;
        }
        AttributeExtractor* getAttributeExtractor() const {
            return (!m_attrExtractor && m_base) ? m_base->getAttributeExtractor() : m_attrExtractor;
        }
        AttributeFilter* getAttributeFilter() const {
            return (!m_attrFilter && m_base) ? m_base->getAttributeFilter() : m_attrFilter;
        }
        AttributeResolver* getAttributeResolver() const {
            return (!m_attrResolver && m_base) ? m_base->getAttributeResolver() : m_attrResolver;
        }
        CredentialResolver* getCredentialResolver() const {
            return (!m_credResolver && m_base) ? m_base->getCredentialResolver() : m_credResolver;
        }
        const PropertySet* getRelyingParty(const EntityDescriptor* provider) const;
        const vector<const XMLCh*>& getAudiences() const {
            return (m_audiences.empty() && m_base) ? m_base->getAudiences() : m_audiences;
        }
#endif
        string getNotificationURL(const char* resource, bool front, unsigned int index) const;

        const set<string>& getRemoteUserAttributeIds() const {
            return (m_remoteUsers.empty() && m_base) ? m_base->getRemoteUserAttributeIds() : m_remoteUsers;
        }

        const SessionInitiator* getDefaultSessionInitiator() const;
        const SessionInitiator* getSessionInitiatorById(const char* id) const;
        const Handler* getDefaultAssertionConsumerService() const;
        const Handler* getAssertionConsumerServiceByIndex(unsigned short index) const;
        const vector<const Handler*>& getAssertionConsumerServicesByBinding(const XMLCh* binding) const;
        const Handler* getHandler(const char* path) const;
        void getHandlers(vector<const Handler*>& handlers) const;

        void receive(DDF& in, ostream& out) {
            // Only current function is to return the headers to clear.
            DDF header;
            DDF ret=DDF(NULL).list();
            DDFJanitor jret(ret);
            for (vector< pair<string,string> >::const_iterator i = m_unsetHeaders.begin(); i!=m_unsetHeaders.end(); ++i) {
                header = DDF(i->first.c_str()).string(i->second.c_str());
                ret.add(header);
            }
            out << ret;
        }

        // Provides filter to exclude special config elements.
        short acceptNode(const DOMNode* node) const;
    
    private:
        void cleanup();
        const XMLApplication* m_base;
        string m_hash;
#ifndef SHIBSP_LITE
        MetadataProvider* m_metadata;
        TrustEngine* m_trust;
        AttributeExtractor* m_attrExtractor;
        AttributeFilter* m_attrFilter;
        AttributeResolver* m_attrResolver;
        CredentialResolver* m_credResolver;
        vector<const XMLCh*> m_audiences;

        // RelyingParty properties
        DOMPropertySet* m_partyDefault;
#ifdef HAVE_GOOD_STL
        map<xstring,PropertySet*> m_partyMap;
#else
        map<const XMLCh*,PropertySet*> m_partyMap;
#endif
#endif
        set<string> m_remoteUsers;
        vector<string> m_frontLogout,m_backLogout;

        // manage handler objects
        vector<Handler*> m_handlers;

        // maps location (path info) to applicable handlers
        map<string,const Handler*> m_handlerMap;

        // maps unique indexes to consumer services
        map<unsigned int,const Handler*> m_acsIndexMap;
        
        // pointer to default consumer service
        const Handler* m_acsDefault;

        // maps binding strings to supporting consumer service(s)
#ifdef HAVE_GOOD_STL
        typedef map<xstring,vector<const Handler*> > ACSBindingMap;
#else
        typedef map<string,vector<const Handler*> > ACSBindingMap;
#endif
        ACSBindingMap m_acsBindingMap;

        // pointer to default session initiator
        const SessionInitiator* m_sessionInitDefault;

        // maps unique ID strings to session initiators
        map<string,const SessionInitiator*> m_sessionInitMap;

        // pointer to default artifact resolution service
        const Handler* m_artifactResolutionDefault;

        pair<bool,int> getArtifactEndpointIndex() const {
            if (m_artifactResolutionDefault) return m_artifactResolutionDefault->getInt("index");
            return m_base ? m_base->getArtifactEndpointIndex() : make_pair(false,0);
        }
    };

    // Top-level configuration implementation
    class SHIBSP_DLLLOCAL XMLConfig;
    class SHIBSP_DLLLOCAL XMLConfigImpl : public DOMPropertySet, public DOMNodeFilter
    {
    public:
        XMLConfigImpl(const DOMElement* e, bool first, const XMLConfig* outer, Category& log);
        ~XMLConfigImpl();
        
        RequestMapper* m_requestMapper;
        map<string,Application*> m_appmap;
#ifndef SHIBSP_LITE
        map< string,pair< PropertySet*,vector<const SecurityPolicyRule*> > > m_policyMap;
        map< string, vector< pair< string, pair<string,string> > > > m_transportOptionMap;
#endif
        
        // Provides filter to exclude special config elements.
        short acceptNode(const DOMNode* node) const;

        void setDocument(DOMDocument* doc) {
            m_document = doc;
        }

    private:
        void doExtensions(const DOMElement* e, const char* label, Category& log);
        void cleanup();

        const XMLConfig* m_outer;
        DOMDocument* m_document;
    };

    class SHIBSP_DLLLOCAL XMLConfig : public ServiceProvider, public ReloadableXMLFile
#ifndef SHIBSP_LITE
        ,public Remoted
#endif
    {
    public:
        XMLConfig(const DOMElement* e) : ReloadableXMLFile(e, Category::getInstance(SHIBSP_LOGCAT".Config")),
            m_impl(NULL), m_listener(NULL), m_sessionCache(NULL)
#ifndef SHIBSP_LITE
            , m_tranLog(NULL)
#endif
        {
        }
        
        void init() {
            load();
        }

        ~XMLConfig() {
            delete m_impl;
            delete m_sessionCache;
            delete m_listener;
#ifndef SHIBSP_LITE
            delete m_tranLog;
            SAMLConfig::getConfig().setArtifactMap(NULL);
            XMLToolingConfig::getConfig().setReplayCache(NULL);
            for_each(m_storage.begin(), m_storage.end(), cleanup_pair<string,StorageService>());
#endif
        }

        // PropertySet
        const PropertySet* getParent() const { return m_impl->getParent(); }
        void setParent(const PropertySet* parent) {return m_impl->setParent(parent);}
        pair<bool,bool> getBool(const char* name, const char* ns=NULL) const {return m_impl->getBool(name,ns);}
        pair<bool,const char*> getString(const char* name, const char* ns=NULL) const {return m_impl->getString(name,ns);}
        pair<bool,const XMLCh*> getXMLString(const char* name, const char* ns=NULL) const {return m_impl->getXMLString(name,ns);}
        pair<bool,unsigned int> getUnsignedInt(const char* name, const char* ns=NULL) const {return m_impl->getUnsignedInt(name,ns);}
        pair<bool,int> getInt(const char* name, const char* ns=NULL) const {return m_impl->getInt(name,ns);}
        void getAll(map<string,const char*>& properties) const {return m_impl->getAll(properties);}
        const PropertySet* getPropertySet(const char* name, const char* ns="urn:mace:shibboleth:2.0:native:sp:config") const {return m_impl->getPropertySet(name,ns);}
        const DOMElement* getElement() const {return m_impl->getElement();}

        // ServiceProvider
#ifndef SHIBSP_LITE
        // Remoted
        void receive(DDF& in, ostream& out);

        TransactionLog* getTransactionLog() const {
            if (m_tranLog)
                return m_tranLog;
            throw ConfigurationException("No TransactionLog available.");
        }

        StorageService* getStorageService(const char* id) const {
            if (id) {
                map<string,StorageService*>::const_iterator i=m_storage.find(id);
                if (i!=m_storage.end())
                    return i->second;
            }
            return NULL;
        }
#endif

        ListenerService* getListenerService(bool required=true) const {
            if (required && !m_listener)
                throw ConfigurationException("No ListenerService available.");
            return m_listener;
        }

        SessionCache* getSessionCache(bool required=true) const {
            if (required && !m_sessionCache)
                throw ConfigurationException("No SessionCache available.");
            return m_sessionCache;
        }

        RequestMapper* getRequestMapper(bool required=true) const {
            if (required && !m_impl->m_requestMapper)
                throw ConfigurationException("No RequestMapper available.");
            return m_impl->m_requestMapper;
        }

        const Application* getApplication(const char* applicationId) const {
            map<string,Application*>::const_iterator i=m_impl->m_appmap.find(applicationId);
            return (i!=m_impl->m_appmap.end()) ? i->second : NULL;
        }

#ifndef SHIBSP_LITE
        const PropertySet* getPolicySettings(const char* id) const {
            map<string,pair<PropertySet*,vector<const SecurityPolicyRule*> > >::const_iterator i = m_impl->m_policyMap.find(id);
            if (i!=m_impl->m_policyMap.end())
                return i->second.first;
            throw ConfigurationException("Security Policy ($1) not found, check <SecurityPolicies> element.", params(1,id));
        }

        const vector<const SecurityPolicyRule*>& getPolicyRules(const char* id) const {
            map<string,pair<PropertySet*,vector<const SecurityPolicyRule*> > >::const_iterator i = m_impl->m_policyMap.find(id);
            if (i!=m_impl->m_policyMap.end())
                return i->second.second;
            throw ConfigurationException("Security Policy ($1) not found, check <SecurityPolicies> element.", params(1,id));
        }

        bool setTransportOptions(const char* id, SOAPTransport& transport) const {
            bool ret = true;
            map< string, vector< pair< string, pair<string,string> > > >::const_iterator p =
                m_impl->m_transportOptionMap.find(id);
            if (p == m_impl->m_transportOptionMap.end())
                return ret;
            vector< pair< string, pair<string,string> > >::const_iterator opt;
            for (opt = p->second.begin(); opt != p->second.end(); ++opt) {
                if (!transport.setProviderOption(opt->first.c_str(), opt->second.first.c_str(), opt->second.second.c_str())) {
                    m_log.error("failed to set SOAPTransport option (%s)", opt->second.first.c_str());
                    ret = false;
                }
            }
            return ret;
        }
#endif

    protected:
        pair<bool,DOMElement*> load();

    private:
        friend class XMLConfigImpl;
        XMLConfigImpl* m_impl;
        mutable ListenerService* m_listener;
        mutable SessionCache* m_sessionCache;
#ifndef SHIBSP_LITE
        mutable TransactionLog* m_tranLog;
        mutable map<string,StorageService*> m_storage;
#endif
    };

#if defined (_MSC_VER)
    #pragma warning( pop )
#endif

    static const XMLCh _Application[] =         UNICODE_LITERAL_11(A,p,p,l,i,c,a,t,i,o,n);
    static const XMLCh Applications[] =         UNICODE_LITERAL_12(A,p,p,l,i,c,a,t,i,o,n,s);
    static const XMLCh _ArtifactMap[] =         UNICODE_LITERAL_11(A,r,t,i,f,a,c,t,M,a,p);
    static const XMLCh _AttributeExtractor[] =  UNICODE_LITERAL_18(A,t,t,r,i,b,u,t,e,E,x,t,r,a,c,t,o,r);
    static const XMLCh _AttributeFilter[] =     UNICODE_LITERAL_15(A,t,t,r,i,b,u,t,e,F,i,l,t,e,r);
    static const XMLCh _AttributeResolver[] =   UNICODE_LITERAL_17(A,t,t,r,i,b,u,t,e,R,e,s,o,l,v,e,r);
    static const XMLCh _AssertionConsumerService[] = UNICODE_LITERAL_24(A,s,s,e,r,t,i,o,n,C,o,n,s,u,m,e,r,S,e,r,v,i,c,e);
    static const XMLCh _ArtifactResolutionService[] =UNICODE_LITERAL_25(A,r,t,i,f,a,c,t,R,e,s,o,l,u,t,i,o,n,S,e,r,v,i,c,e);
    static const XMLCh _Audience[] =            UNICODE_LITERAL_8(A,u,d,i,e,n,c,e);
    static const XMLCh Binding[] =              UNICODE_LITERAL_7(B,i,n,d,i,n,g);
    static const XMLCh Channel[]=               UNICODE_LITERAL_7(C,h,a,n,n,e,l);
    static const XMLCh _CredentialResolver[] =  UNICODE_LITERAL_18(C,r,e,d,e,n,t,i,a,l,R,e,s,o,l,v,e,r);
    static const XMLCh DefaultRelyingParty[] =  UNICODE_LITERAL_19(D,e,f,a,u,l,t,R,e,l,y,i,n,g,P,a,r,t,y);
    static const XMLCh _Extensions[] =          UNICODE_LITERAL_10(E,x,t,e,n,s,i,o,n,s);
    static const XMLCh _fatal[] =               UNICODE_LITERAL_5(f,a,t,a,l);
    static const XMLCh _Handler[] =             UNICODE_LITERAL_7(H,a,n,d,l,e,r);
    static const XMLCh _id[] =                  UNICODE_LITERAL_2(i,d);
    static const XMLCh InProcess[] =            UNICODE_LITERAL_9(I,n,P,r,o,c,e,s,s);
    static const XMLCh Library[] =              UNICODE_LITERAL_7(L,i,b,r,a,r,y);
    static const XMLCh Listener[] =             UNICODE_LITERAL_8(L,i,s,t,e,n,e,r);
    static const XMLCh Location[] =             UNICODE_LITERAL_8(L,o,c,a,t,i,o,n);
    static const XMLCh logger[] =               UNICODE_LITERAL_6(l,o,g,g,e,r);
    static const XMLCh _LogoutInitiator[] =     UNICODE_LITERAL_15(L,o,g,o,u,t,I,n,i,t,i,a,t,o,r);
    static const XMLCh _ManageNameIDService[] = UNICODE_LITERAL_19(M,a,n,a,g,e,N,a,m,e,I,D,S,e,r,v,i,c,e);
    static const XMLCh _MetadataProvider[] =    UNICODE_LITERAL_16(M,e,t,a,d,a,t,a,P,r,o,v,i,d,e,r);
    static const XMLCh Notify[] =               UNICODE_LITERAL_6(N,o,t,i,f,y);
    static const XMLCh _option[] =              UNICODE_LITERAL_6(o,p,t,i,o,n);
    static const XMLCh OutOfProcess[] =         UNICODE_LITERAL_12(O,u,t,O,f,P,r,o,c,e,s,s);
    static const XMLCh _path[] =                UNICODE_LITERAL_4(p,a,t,h);
    static const XMLCh Policy[] =               UNICODE_LITERAL_6(P,o,l,i,c,y);
    static const XMLCh _provider[] =            UNICODE_LITERAL_8(p,r,o,v,i,d,e,r);
    static const XMLCh RelyingParty[] =         UNICODE_LITERAL_12(R,e,l,y,i,n,g,P,a,r,t,y);
    static const XMLCh _ReplayCache[] =         UNICODE_LITERAL_11(R,e,p,l,a,y,C,a,c,h,e);
    static const XMLCh _RequestMapper[] =       UNICODE_LITERAL_13(R,e,q,u,e,s,t,M,a,p,p,e,r);
    static const XMLCh Rule[] =                 UNICODE_LITERAL_4(R,u,l,e);
    static const XMLCh SecurityPolicies[] =     UNICODE_LITERAL_16(S,e,c,u,r,i,t,y,P,o,l,i,c,i,e,s);
    static const XMLCh _SessionCache[] =        UNICODE_LITERAL_12(S,e,s,s,i,o,n,C,a,c,h,e);
    static const XMLCh _SessionInitiator[] =    UNICODE_LITERAL_16(S,e,s,s,i,o,n,I,n,i,t,i,a,t,o,r);
    static const XMLCh _SingleLogoutService[] = UNICODE_LITERAL_19(S,i,n,g,l,e,L,o,g,o,u,t,S,e,r,v,i,c,e);
    static const XMLCh Site[] =                 UNICODE_LITERAL_4(S,i,t,e);
    static const XMLCh _StorageService[] =      UNICODE_LITERAL_14(S,t,o,r,a,g,e,S,e,r,v,i,c,e);
    static const XMLCh TCPListener[] =          UNICODE_LITERAL_11(T,C,P,L,i,s,t,e,n,e,r);
    static const XMLCh TransportOption[] =      UNICODE_LITERAL_15(T,r,a,n,s,p,o,r,t,O,p,t,i,o,n);
    static const XMLCh _TrustEngine[] =         UNICODE_LITERAL_11(T,r,u,s,t,E,n,g,i,n,e);
    static const XMLCh _type[] =                UNICODE_LITERAL_4(t,y,p,e);
    static const XMLCh UnixListener[] =         UNICODE_LITERAL_12(U,n,i,x,L,i,s,t,e,n,e,r);

#ifndef SHIBSP_LITE
    class SHIBSP_DLLLOCAL PolicyNodeFilter : public DOMNodeFilter
    {
    public:
        short acceptNode(const DOMNode* node) const {
            return FILTER_REJECT;
        }
    };
#endif
};

namespace shibsp {
    ServiceProvider* XMLServiceProviderFactory(const DOMElement* const & e)
    {
        return new XMLConfig(e);
    }
};

XMLApplication::XMLApplication(
    const ServiceProvider* sp,
    const DOMElement* e,
    const XMLApplication* base
    ) : Application(sp), m_base(base),
#ifndef SHIBSP_LITE
        m_metadata(NULL), m_trust(NULL),
        m_attrExtractor(NULL), m_attrFilter(NULL), m_attrResolver(NULL),
        m_credResolver(NULL), m_partyDefault(NULL),
#endif
        m_acsDefault(NULL), m_sessionInitDefault(NULL), m_artifactResolutionDefault(NULL)
{
#ifdef _DEBUG
    xmltooling::NDC ndc("XMLApplication");
#endif
    Category& log=Category::getInstance(SHIBSP_LOGCAT".Application");

    try {
        // First load any property sets.
        load(e,log,this);
        if (base)
            setParent(base);

        SPConfig& conf=SPConfig::getConfig();
#ifndef SHIBSP_LITE
        SAMLConfig& samlConf=SAMLConfig::getConfig();
        XMLToolingConfig& xmlConf=XMLToolingConfig::getConfig();
#endif

        // This used to be an actual hash, but now it's just a hex-encode to avoid xmlsec.
        static char DIGITS[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
        string tohash=getId();
        tohash+=getString("entityID").second;
        for (const char* ch = tohash.c_str(); *ch; ++ch) {
            m_hash += (DIGITS[((unsigned char)(0xF0 & *ch)) >> 4 ]);
            m_hash += (DIGITS[0x0F & *ch]);
        }

        // Load attribute ID lists for REMOTE_USER and header clearing.
        if (conf.isEnabled(SPConfig::InProcess)) {
            pair<bool,const char*> attributes = getString("REMOTE_USER");
            if (attributes.first) {
                char* dup = strdup(attributes.second);
                char* pos;
                char* start = dup;
                while (start && *start) {
                    while (*start && isspace(*start))
                        start++;
                    if (!*start)
                        break;
                    pos = strchr(start,' ');
                    if (pos)
                        *pos=0;
                    m_remoteUsers.insert(start);
                    start = pos ? pos+1 : NULL;
                }
                free(dup);
            }

            attributes = getString("unsetHeaders");
            if (attributes.first) {
                string transformedprefix("HTTP_");
                const char* pch;
                pair<bool,const char*> prefix = getString("metadataAttributePrefix");
                if (prefix.first) {
                    pch = prefix.second;
                    while (*pch) {
                        transformedprefix += (isalnum(*pch) ? toupper(*pch) : '_');
                        pch++;
                    }
                }
                char* dup = strdup(attributes.second);
                char* pos;
                char* start = dup;
                while (start && *start) {
                    while (*start && isspace(*start))
                        start++;
                    if (!*start)
                        break;
                    pos = strchr(start,' ');
                    if (pos)
                        *pos=0;

                    string transformed;
                    pch = start;
                    while (*pch) {
                        transformed += (isalnum(*pch) ? toupper(*pch) : '_');
                        pch++;
                    }
                    m_unsetHeaders.push_back(pair<string,string>(start,string("HTTP_") + transformed));
                    if (prefix.first)
                        m_unsetHeaders.push_back(pair<string,string>(string(prefix.second) + start, transformedprefix + transformed));
                    start = pos ? pos+1 : NULL;
                }
                free(dup);
                m_unsetHeaders.push_back(pair<string,string>("Shib-Application-ID","HTTP_SHIB_APPLICATION_ID"));
            }
        }

        Handler* handler=NULL;
        const PropertySet* sessions = getPropertySet("Sessions");

        // Process assertion export handler.
        pair<bool,const char*> location = sessions ? sessions->getString("exportLocation") : pair<bool,const char*>(false,NULL);
        if (location.first) {
            try {
                DOMElement* exportElement = e->getOwnerDocument()->createElementNS(shibspconstants::SHIB2SPCONFIG_NS,_Handler);
                exportElement->setAttributeNS(NULL,Location,sessions->getXMLString("exportLocation").second);
                pair<bool,const XMLCh*> exportACL = sessions->getXMLString("exportACL");
                if (exportACL.first) {
                    static const XMLCh _acl[] = UNICODE_LITERAL_9(e,x,p,o,r,t,A,C,L);
                    exportElement->setAttributeNS(NULL,_acl,exportACL.second);
                }
                handler = conf.HandlerManager.newPlugin(
                    samlconstants::SAML20_BINDING_URI, pair<const DOMElement*,const char*>(exportElement, getId())
                    );
                m_handlers.push_back(handler);

                // Insert into location map. If it contains the handlerURL, we skip past that part.
                const char* pch = strstr(location.second, sessions->getString("handlerURL").second);
                if (pch)
                    location.second = pch + strlen(sessions->getString("handlerURL").second);
                if (*location.second == '/')
                    m_handlerMap[location.second]=handler;
                else
                    m_handlerMap[string("/") + location.second]=handler;
            }
            catch (exception& ex) {
                log.error("caught exception installing assertion lookup handler: %s", ex.what());
            }
        }

        // Process other handlers.
        bool hardACS=false, hardSessionInit=false, hardArt=false;
        const DOMElement* child = sessions ? XMLHelper::getFirstChildElement(sessions->getElement()) : NULL;
        while (child) {
            try {
                // A handler is based on the Binding property in conjunction with the element name.
                // If it's an ACS or SI, also handle index/id mappings and defaulting.
                if (XMLString::equals(child->getLocalName(),_AssertionConsumerService)) {
                    auto_ptr_char bindprop(child->getAttributeNS(NULL,Binding));
                    if (!bindprop.get() || !*(bindprop.get())) {
                        log.warn("md:AssertionConsumerService element has no Binding attribute, skipping it...");
                        child = XMLHelper::getNextSiblingElement(child);
                        continue;
                    }
                    handler=conf.AssertionConsumerServiceManager.newPlugin(bindprop.get(),make_pair(child, getId()));
                    // Map by binding (may be > 1 per binding, e.g. SAML 1.0 vs 1.1)
#ifdef HAVE_GOOD_STL
                    m_acsBindingMap[handler->getXMLString("Binding").second].push_back(handler);
#else
                    m_acsBindingMap[handler->getString("Binding").second].push_back(handler);
#endif
                    m_acsIndexMap[handler->getUnsignedInt("index").second]=handler;
                    
                    if (!hardACS) {
                        pair<bool,bool> defprop=handler->getBool("isDefault");
                        if (defprop.first) {
                            if (defprop.second) {
                                hardACS=true;
                                m_acsDefault=handler;
                            }
                        }
                        else if (!m_acsDefault)
                            m_acsDefault=handler;
                    }
                }
                else if (XMLString::equals(child->getLocalName(),_SessionInitiator)) {
                    auto_ptr_char type(child->getAttributeNS(NULL,_type));
                    if (!type.get() || !*(type.get())) {
                        log.warn("SessionInitiator element has no type attribute, skipping it...");
                        child = XMLHelper::getNextSiblingElement(child);
                        continue;
                    }
                    SessionInitiator* sihandler=conf.SessionInitiatorManager.newPlugin(type.get(),make_pair(child, getId()));
                    handler=sihandler;
                    pair<bool,const char*> si_id=handler->getString("id");
                    if (si_id.first && si_id.second)
                        m_sessionInitMap[si_id.second]=sihandler;
                    if (!hardSessionInit) {
                        pair<bool,bool> defprop=handler->getBool("isDefault");
                        if (defprop.first) {
                            if (defprop.second) {
                                hardSessionInit=true;
                                m_sessionInitDefault=sihandler;
                            }
                        }
                        else if (!m_sessionInitDefault)
                            m_sessionInitDefault=sihandler;
                    }
                }
                else if (XMLString::equals(child->getLocalName(),_LogoutInitiator)) {
                    auto_ptr_char type(child->getAttributeNS(NULL,_type));
                    if (!type.get() || !*(type.get())) {
                        log.warn("LogoutInitiator element has no type attribute, skipping it...");
                        child = XMLHelper::getNextSiblingElement(child);
                        continue;
                    }
                    handler=conf.LogoutInitiatorManager.newPlugin(type.get(),make_pair(child, getId()));
                }
                else if (XMLString::equals(child->getLocalName(),_ArtifactResolutionService)) {
                    auto_ptr_char bindprop(child->getAttributeNS(NULL,Binding));
                    if (!bindprop.get() || !*(bindprop.get())) {
                        log.warn("md:ArtifactResolutionService element has no Binding attribute, skipping it...");
                        child = XMLHelper::getNextSiblingElement(child);
                        continue;
                    }
                    handler=conf.ArtifactResolutionServiceManager.newPlugin(bindprop.get(),make_pair(child, getId()));
                    
                    if (!hardArt) {
                        pair<bool,bool> defprop=handler->getBool("isDefault");
                        if (defprop.first) {
                            if (defprop.second) {
                                hardArt=true;
                                m_artifactResolutionDefault=handler;
                            }
                        }
                        else if (!m_artifactResolutionDefault)
                            m_artifactResolutionDefault=handler;
                    }
                }
                else if (XMLString::equals(child->getLocalName(),_SingleLogoutService)) {
                    auto_ptr_char bindprop(child->getAttributeNS(NULL,Binding));
                    if (!bindprop.get() || !*(bindprop.get())) {
                        log.warn("md:SingleLogoutService element has no Binding attribute, skipping it...");
                        child = XMLHelper::getNextSiblingElement(child);
                        continue;
                    }
                    handler=conf.SingleLogoutServiceManager.newPlugin(bindprop.get(),make_pair(child, getId()));
                }
                else if (XMLString::equals(child->getLocalName(),_ManageNameIDService)) {
                    auto_ptr_char bindprop(child->getAttributeNS(NULL,Binding));
                    if (!bindprop.get() || !*(bindprop.get())) {
                        log.warn("md:ManageNameIDService element has no Binding attribute, skipping it...");
                        child = XMLHelper::getNextSiblingElement(child);
                        continue;
                    }
                    handler=conf.ManageNameIDServiceManager.newPlugin(bindprop.get(),make_pair(child, getId()));
                }
                else {
                    auto_ptr_char type(child->getAttributeNS(NULL,_type));
                    if (!type.get() || !*(type.get())) {
                        log.warn("Handler element has no type attribute, skipping it...");
                        child = XMLHelper::getNextSiblingElement(child);
                        continue;
                    }
                    handler=conf.HandlerManager.newPlugin(type.get(),make_pair(child, getId()));
                }

                m_handlers.push_back(handler);

                // Insert into location map.
                location=handler->getString("Location");
                if (location.first && *location.second == '/')
                    m_handlerMap[location.second]=handler;
                else if (location.first)
                    m_handlerMap[string("/") + location.second]=handler;

            }
            catch (exception& ex) {
                log.error("caught exception processing handler element: %s", ex.what());
            }
            
            child = XMLHelper::getNextSiblingElement(child);
        }

        // Notification.
        DOMNodeList* nlist=e->getElementsByTagNameNS(shibspconstants::SHIB2SPCONFIG_NS,Notify);
        for (XMLSize_t i=0; nlist && i<nlist->getLength(); i++) {
            if (nlist->item(i)->getParentNode()->isSameNode(e)) {
                const XMLCh* channel = static_cast<DOMElement*>(nlist->item(i))->getAttributeNS(NULL,Channel);
                auto_ptr_char loc(static_cast<DOMElement*>(nlist->item(i))->getAttributeNS(NULL,Location));
                if (loc.get() && *loc.get()) {
                    if (channel && *channel == chLatin_f)
                        m_frontLogout.push_back(loc.get());
                    else
                        m_backLogout.push_back(loc.get());
                }
            }
        }

#ifndef SHIBSP_LITE
        nlist=e->getElementsByTagNameNS(samlconstants::SAML20_NS,Audience::LOCAL_NAME);
        for (XMLSize_t i=0; nlist && i<nlist->getLength(); i++)
            if (nlist->item(i)->getParentNode()->isSameNode(e) && nlist->item(i)->hasChildNodes())
                m_audiences.push_back(nlist->item(i)->getFirstChild()->getNodeValue());

        // Always include our own entityID as an audience.
        m_audiences.push_back(getXMLString("entityID").second);

        if (conf.isEnabled(SPConfig::Metadata)) {
            child = XMLHelper::getFirstChildElement(e,_MetadataProvider);
            if (child) {
                auto_ptr_char type(child->getAttributeNS(NULL,_type));
                log.info("building MetadataProvider of type %s...",type.get());
                try {
                    auto_ptr<MetadataProvider> mp(samlConf.MetadataProviderManager.newPlugin(type.get(),child));
                    mp->init();
                    m_metadata = mp.release();
                }
                catch (exception& ex) {
                    log.crit("error building/initializing MetadataProvider: %s", ex.what());
                }
            }
        }

        if (conf.isEnabled(SPConfig::Trust)) {
            child = XMLHelper::getFirstChildElement(e,_TrustEngine);
            if (child) {
                auto_ptr_char type(child->getAttributeNS(NULL,_type));
                log.info("building TrustEngine of type %s...",type.get());
                try {
                    m_trust = xmlConf.TrustEngineManager.newPlugin(type.get(),child);
                }
                catch (exception& ex) {
                    log.crit("error building TrustEngine: %s", ex.what());
                }
            }
        }

        if (conf.isEnabled(SPConfig::AttributeResolution)) {
            child = XMLHelper::getFirstChildElement(e,_AttributeExtractor);
            if (child) {
                auto_ptr_char type(child->getAttributeNS(NULL,_type));
                log.info("building AttributeExtractor of type %s...",type.get());
                try {
                    m_attrExtractor = conf.AttributeExtractorManager.newPlugin(type.get(),child);
                }
                catch (exception& ex) {
                    log.crit("error building AttributeExtractor: %s", ex.what());
                }
            }

            child = XMLHelper::getFirstChildElement(e,_AttributeFilter);
            if (child) {
                auto_ptr_char type(child->getAttributeNS(NULL,_type));
                log.info("building AttributeFilter of type %s...",type.get());
                try {
                    m_attrFilter = conf.AttributeFilterManager.newPlugin(type.get(),child);
                }
                catch (exception& ex) {
                    log.crit("error building AttributeFilter: %s", ex.what());
                }
            }

            child = XMLHelper::getFirstChildElement(e,_AttributeResolver);
            if (child) {
                auto_ptr_char type(child->getAttributeNS(NULL,_type));
                log.info("building AttributeResolver of type %s...",type.get());
                try {
                    m_attrResolver = conf.AttributeResolverManager.newPlugin(type.get(),child);
                }
                catch (exception& ex) {
                    log.crit("error building AttributeResolver: %s", ex.what());
                }
            }

            if (m_unsetHeaders.empty()) {
                vector<string> unsetHeaders;
                if (m_attrExtractor) {
                    Locker extlock(m_attrExtractor);
                    m_attrExtractor->getAttributeIds(unsetHeaders);
                }
                if (m_attrResolver) {
                    Locker reslock(m_attrResolver);
                    m_attrResolver->getAttributeIds(unsetHeaders);
                }
                if (unsetHeaders.empty()) {
                    if (m_base)
                        m_unsetHeaders.insert(m_unsetHeaders.end(), m_base->m_unsetHeaders.begin(), m_base->m_unsetHeaders.end());
                    else
                        m_unsetHeaders.push_back(pair<string,string>("Shib-Application-ID","HTTP_SHIB_APPLICATION_ID"));
                }
                else {
                    string transformedprefix("HTTP_");
                    const char* pch;
                    pair<bool,const char*> prefix = getString("metadataAttributePrefix");
                    if (prefix.first) {
                        pch = prefix.second;
                        while (*pch) {
                            transformedprefix += (isalnum(*pch) ? toupper(*pch) : '_');
                            pch++;
                        }
                    }
                    for (vector<string>::const_iterator hdr = unsetHeaders.begin(); hdr!=unsetHeaders.end(); ++hdr) {
                        string transformed;
                        pch = hdr->c_str();
                        while (*pch) {
                            transformed += (isalnum(*pch) ? toupper(*pch) : '_');
                            pch++;
                        }
                        m_unsetHeaders.push_back(pair<string,string>(*hdr, string("HTTP_") + transformed));
                        if (prefix.first)
                            m_unsetHeaders.push_back(pair<string,string>(string(prefix.second) + *hdr, transformedprefix + transformed));
                    }
                    m_unsetHeaders.push_back(pair<string,string>("Shib-Application-ID","HTTP_SHIB_APPLICATION_ID"));
                }
            }
        }

        if (conf.isEnabled(SPConfig::Credentials)) {
            child = XMLHelper::getFirstChildElement(e,_CredentialResolver);
            if (child) {
                auto_ptr_char type(child->getAttributeNS(NULL,_type));
                log.info("building CredentialResolver of type %s...",type.get());
                try {
                    m_credResolver = xmlConf.CredentialResolverManager.newPlugin(type.get(),child);
                }
                catch (exception& ex) {
                    log.crit("error building CredentialResolver: %s", ex.what());
                }
            }
        }

        // Finally, load relying parties.
        child = XMLHelper::getFirstChildElement(e,DefaultRelyingParty);
        if (child) {
            m_partyDefault=new DOMPropertySet();
            m_partyDefault->load(child,log,this);
            child = XMLHelper::getFirstChildElement(child,RelyingParty);
            while (child) {
                auto_ptr<DOMPropertySet> rp(new DOMPropertySet());
                rp->load(child,log,this);
                rp->setParent(m_partyDefault);
                m_partyMap[child->getAttributeNS(NULL,saml2::Attribute::NAME_ATTRIB_NAME)]=rp.release();
                child = XMLHelper::getNextSiblingElement(child,RelyingParty);
            }
        }
#endif

        // Out of process only, we register a listener endpoint.
        if (!conf.isEnabled(SPConfig::InProcess)) {
            ListenerService* listener = sp->getListenerService(false);
            if (listener) {
                string addr=string(getId()) + "::getHeaders::Application";
                listener->regListener(addr.c_str(),this);
            }
            else
                log.info("no ListenerService available, Application remoting disabled");
        }
    }
    catch (exception&) {
        cleanup();
        throw;
    }
#ifndef _DEBUG
    catch (...) {
        cleanup();
        throw;
    }
#endif
}

void XMLApplication::cleanup()
{
    ListenerService* listener=getServiceProvider().getListenerService(false);
    if (listener && SPConfig::getConfig().isEnabled(SPConfig::OutOfProcess) && !SPConfig::getConfig().isEnabled(SPConfig::InProcess)) {
        string addr=string(getId()) + "::getHeaders::Application";
        listener->unregListener(addr.c_str(),this);
    }
    for_each(m_handlers.begin(),m_handlers.end(),xmltooling::cleanup<Handler>());
    m_handlers.clear();
#ifndef SHIBSP_LITE
    delete m_partyDefault;
    m_partyDefault = NULL;
#ifdef HAVE_GOOD_STL
    for_each(m_partyMap.begin(),m_partyMap.end(),cleanup_pair<xstring,PropertySet>());
#else
    for_each(m_partyMap.begin(),m_partyMap.end(),cleanup_pair<const XMLCh*,PropertySet>());
#endif
    m_partyMap.clear();
    delete m_credResolver;
    m_credResolver = NULL;
    delete m_attrResolver;
    m_attrResolver = NULL;
    delete m_attrFilter;
    m_attrFilter = NULL;
    delete m_attrExtractor;
    m_attrExtractor = NULL;
    delete m_trust;
    m_trust = NULL;
    delete m_metadata;
    m_metadata = NULL;
#endif
}

short XMLApplication::acceptNode(const DOMNode* node) const
{
    const XMLCh* name=node->getLocalName();
    if (XMLString::equals(name,_Application) ||
        XMLString::equals(name,_Audience) ||
        XMLString::equals(name,Notify) ||
        XMLString::equals(name,_Handler) ||
        XMLString::equals(name,_AssertionConsumerService) ||
        XMLString::equals(name,_ArtifactResolutionService) ||
        XMLString::equals(name,_LogoutInitiator) ||
        XMLString::equals(name,_ManageNameIDService) ||
        XMLString::equals(name,_SessionInitiator) ||
        XMLString::equals(name,_SingleLogoutService) ||
        XMLString::equals(name,DefaultRelyingParty) ||
        XMLString::equals(name,RelyingParty) ||
        XMLString::equals(name,_MetadataProvider) ||
        XMLString::equals(name,_TrustEngine) ||
        XMLString::equals(name,_CredentialResolver) ||
        XMLString::equals(name,_AttributeFilter) ||
        XMLString::equals(name,_AttributeExtractor) ||
        XMLString::equals(name,_AttributeResolver))
        return FILTER_REJECT;

    return FILTER_ACCEPT;
}

#ifndef SHIBSP_LITE

const PropertySet* XMLApplication::getRelyingParty(const EntityDescriptor* provider) const
{
    if (!m_partyDefault && m_base)
        return m_base->getRelyingParty(provider);
    else if (!provider)
        return m_partyDefault;
        
#ifdef HAVE_GOOD_STL
    map<xstring,PropertySet*>::const_iterator i=m_partyMap.find(provider->getEntityID());
    if (i!=m_partyMap.end())
        return i->second;
    const EntitiesDescriptor* group=dynamic_cast<const EntitiesDescriptor*>(provider->getParent());
    while (group) {
        if (group->getName()) {
            i=m_partyMap.find(group->getName());
            if (i!=m_partyMap.end())
                return i->second;
        }
        group=dynamic_cast<const EntitiesDescriptor*>(group->getParent());
    }
#else
    map<const XMLCh*,PropertySet*>::const_iterator i=m_partyMap.begin();
    for (; i!=m_partyMap.end(); i++) {
        if (XMLString::equals(i->first,provider->getEntityID()))
            return i->second;
        const EntitiesDescriptor* group=dynamic_cast<const EntitiesDescriptor*>(provider->getParent());
        while (group) {
            if (XMLString::equals(i->first,group->getName()))
                return i->second;
            group=dynamic_cast<const EntitiesDescriptor*>(group->getParent());
        }
    }
#endif
    return m_partyDefault;
}

#endif

string XMLApplication::getNotificationURL(const char* resource, bool front, unsigned int index) const
{
    const vector<string>& locs = front ? m_frontLogout : m_backLogout;
    if (locs.empty())
        return m_base ? m_base->getNotificationURL(resource, front, index) : string();
    else if (index >= locs.size())
        return string();

#ifdef HAVE_STRCASECMP
    if (!resource || (strncasecmp(resource,"http://",7) && strncasecmp(resource,"https://",8)))
#else
    if (!resource || (strnicmp(resource,"http://",7) && strnicmp(resource,"https://",8)))
#endif
        throw ConfigurationException("Request URL was not absolute.");

    const char* handler=locs[index].c_str();
    
    // Should never happen...
    if (!handler || (*handler!='/' && strncmp(handler,"http:",5) && strncmp(handler,"https:",6)))
        throw ConfigurationException(
            "Invalid Location property ($1) in Notify element for Application ($2)",
            params(2, handler ? handler : "null", getId())
            );

    // The "Location" property can be in one of three formats:
    //
    // 1) a full URI:       http://host/foo/bar
    // 2) a hostless URI:   http:///foo/bar
    // 3) a relative path:  /foo/bar
    //
    // #  Protocol  Host        Path
    // 1  handler   handler     handler
    // 2  handler   resource    handler
    // 3  resource  resource    handler

    const char* path = NULL;

    // Decide whether to use the handler or the resource for the "protocol"
    const char* prot;
    if (*handler != '/') {
        prot = handler;
    }
    else {
        prot = resource;
        path = handler;
    }

    // break apart the "protocol" string into protocol, host, and "the rest"
    const char* colon=strchr(prot,':');
    colon += 3;
    const char* slash=strchr(colon,'/');
    if (!path)
        path = slash;

    // Compute the actual protocol and store.
    string notifyURL(prot, colon-prot);

    // create the "host" from either the colon/slash or from the target string
    // If prot == handler then we're in either #1 or #2, else #3.
    // If slash == colon then we're in #2.
    if (prot != handler || slash == colon) {
        colon = strchr(resource, ':');
        colon += 3;      // Get past the ://
        slash = strchr(colon, '/');
    }
    string host(colon, (slash ? slash-colon : strlen(colon)));

    // Build the URL
    notifyURL += host + path;
    return notifyURL;
}

const SessionInitiator* XMLApplication::getDefaultSessionInitiator() const
{
    if (m_sessionInitDefault) return m_sessionInitDefault;
    return m_base ? m_base->getDefaultSessionInitiator() : NULL;
}

const SessionInitiator* XMLApplication::getSessionInitiatorById(const char* id) const
{
    map<string,const SessionInitiator*>::const_iterator i=m_sessionInitMap.find(id);
    if (i!=m_sessionInitMap.end()) return i->second;
    return m_base ? m_base->getSessionInitiatorById(id) : NULL;
}

const Handler* XMLApplication::getDefaultAssertionConsumerService() const
{
    if (m_acsDefault) return m_acsDefault;
    return m_base ? m_base->getDefaultAssertionConsumerService() : NULL;
}

const Handler* XMLApplication::getAssertionConsumerServiceByIndex(unsigned short index) const
{
    map<unsigned int,const Handler*>::const_iterator i=m_acsIndexMap.find(index);
    if (i!=m_acsIndexMap.end()) return i->second;
    return m_base ? m_base->getAssertionConsumerServiceByIndex(index) : NULL;
}

const vector<const Handler*>& XMLApplication::getAssertionConsumerServicesByBinding(const XMLCh* binding) const
{
#ifdef HAVE_GOOD_STL
    ACSBindingMap::const_iterator i=m_acsBindingMap.find(binding);
#else
    auto_ptr_char temp(binding);
    ACSBindingMap::const_iterator i=m_acsBindingMap.find(temp.get());
#endif
    if (i!=m_acsBindingMap.end())
        return i->second;
    return m_base ? m_base->getAssertionConsumerServicesByBinding(binding) : g_noHandlers;
}

const Handler* XMLApplication::getHandler(const char* path) const
{
    string wrap(path);
    map<string,const Handler*>::const_iterator i=m_handlerMap.find(wrap.substr(0,wrap.find('?')));
    if (i!=m_handlerMap.end())
        return i->second;
    return m_base ? m_base->getHandler(path) : NULL;
}

void XMLApplication::getHandlers(vector<const Handler*>& handlers) const
{
    handlers.insert(handlers.end(), m_handlers.begin(), m_handlers.end());
    if (m_base) {
        for (map<string,const Handler*>::const_iterator h = m_base->m_handlerMap.begin(); h != m_base->m_handlerMap.end(); ++h) {
            if (m_handlerMap.count(h->first) == 0)
                handlers.push_back(h->second);
        }
    }
}

short XMLConfigImpl::acceptNode(const DOMNode* node) const
{
    if (!XMLString::equals(node->getNamespaceURI(),shibspconstants::SHIB2SPCONFIG_NS))
        return FILTER_ACCEPT;
    const XMLCh* name=node->getLocalName();
    if (XMLString::equals(name,Applications) ||
        XMLString::equals(name,_ArtifactMap) ||
        XMLString::equals(name,_Extensions) ||
        XMLString::equals(name,Listener) ||
        XMLString::equals(name,Policy) ||
        XMLString::equals(name,_RequestMapper) ||
        XMLString::equals(name,_ReplayCache) ||
        XMLString::equals(name,_SessionCache) ||
        XMLString::equals(name,Site) ||
        XMLString::equals(name,_StorageService) ||
        XMLString::equals(name,TCPListener) ||
        XMLString::equals(name,UnixListener))
        return FILTER_REJECT;

    return FILTER_ACCEPT;
}

void XMLConfigImpl::doExtensions(const DOMElement* e, const char* label, Category& log)
{
    const DOMElement* exts=XMLHelper::getFirstChildElement(e,_Extensions);
    if (exts) {
        exts=XMLHelper::getFirstChildElement(exts,Library);
        while (exts) {
            auto_ptr_char path(exts->getAttributeNS(NULL,_path));
            try {
                if (path.get()) {
                    XMLToolingConfig::getConfig().load_library(path.get(),(void*)exts);
                    log.debug("loaded %s extension library (%s)", label, path.get());
                }
            }
            catch (exception& e) {
                const XMLCh* fatal=exts->getAttributeNS(NULL,_fatal);
                if (fatal && (*fatal==chLatin_t || *fatal==chDigit_1)) {
                    log.fatal("unable to load mandatory %s extension library %s: %s", label, path.get(), e.what());
                    throw;
                }
                else {
                    log.crit("unable to load optional %s extension library %s: %s", label, path.get(), e.what());
                }
            }
            exts=XMLHelper::getNextSiblingElement(exts,Library);
        }
    }
}

XMLConfigImpl::XMLConfigImpl(const DOMElement* e, bool first, const XMLConfig* outer, Category& log)
    : m_requestMapper(NULL), m_outer(outer), m_document(NULL)
{
#ifdef _DEBUG
    xmltooling::NDC ndc("XMLConfigImpl");
#endif

    try {
        SPConfig& conf=SPConfig::getConfig();
#ifndef SHIBSP_LITE
        SAMLConfig& samlConf=SAMLConfig::getConfig();
#endif
        XMLToolingConfig& xmlConf=XMLToolingConfig::getConfig();
        const DOMElement* SHAR=XMLHelper::getFirstChildElement(e,OutOfProcess);
        const DOMElement* SHIRE=XMLHelper::getFirstChildElement(e,InProcess);

        // Initialize log4cpp manually in order to redirect log messages as soon as possible.
        if (conf.isEnabled(SPConfig::Logging)) {
            const XMLCh* logconf=NULL;
            if (conf.isEnabled(SPConfig::OutOfProcess))
                logconf=SHAR->getAttributeNS(NULL,logger);
            else if (conf.isEnabled(SPConfig::InProcess))
                logconf=SHIRE->getAttributeNS(NULL,logger);
            if (!logconf || !*logconf)
                logconf=e->getAttributeNS(NULL,logger);
            if (logconf && *logconf) {
                auto_ptr_char logpath(logconf);
                log.debug("loading new logging configuration from (%s), check log destination for status of configuration",logpath.get());
                XMLToolingConfig::getConfig().log_config(logpath.get());
            }
            
#ifndef SHIBSP_LITE
            if (first)
                m_outer->m_tranLog = new TransactionLog();
#endif
        }
        
        // First load any property sets.
        load(e,log,this);

        const DOMElement* child;
        string plugtype;

        // Much of the processing can only occur on the first instantiation.
        if (first) {
            // Set clock skew.
            pair<bool,unsigned int> skew=getUnsignedInt("clockSkew");
            if (skew.first)
                xmlConf.clock_skew_secs=skew.second;

            // Extensions
            doExtensions(e, "global", log);
            if (conf.isEnabled(SPConfig::OutOfProcess))
                doExtensions(SHAR, "out of process", log);

            if (conf.isEnabled(SPConfig::InProcess))
                doExtensions(SHIRE, "in process", log);
            
            // Instantiate the ListenerService and SessionCache objects.
            if (conf.isEnabled(SPConfig::Listener)) {
                child=XMLHelper::getFirstChildElement(e,UnixListener);
                if (child)
                    plugtype=UNIX_LISTENER_SERVICE;
                else {
                    child=XMLHelper::getFirstChildElement(e,TCPListener);
                    if (child)
                        plugtype=TCP_LISTENER_SERVICE;
                    else {
                        child=XMLHelper::getFirstChildElement(e,Listener);
                        if (child) {
                            auto_ptr_char type(child->getAttributeNS(NULL,_type));
                            if (type.get())
                                plugtype=type.get();
                        }
                    }
                }
                if (child) {
                    log.info("building ListenerService of type %s...", plugtype.c_str());
                    m_outer->m_listener = conf.ListenerServiceManager.newPlugin(plugtype.c_str(), child);
                }
                else {
                    log.fatal("can't build ListenerService, missing conf:Listener element?");
                    throw ConfigurationException("Can't build ListenerService, missing conf:Listener element?");
                }
            }

#ifndef SHIBSP_LITE
            if (m_outer->m_listener && conf.isEnabled(SPConfig::OutOfProcess) && !conf.isEnabled(SPConfig::InProcess)) {
                m_outer->m_listener->regListener("set::RelayState", m_outer->m_listener);
                m_outer->m_listener->regListener("get::RelayState", m_outer->m_listener);
            }
#endif

            if (conf.isEnabled(SPConfig::Caching)) {
                if (conf.isEnabled(SPConfig::OutOfProcess)) {
#ifndef SHIBSP_LITE
                    // First build any StorageServices.
                    child=XMLHelper::getFirstChildElement(e,_StorageService);
                    while (child) {
                        auto_ptr_char id(child->getAttributeNS(NULL,_id));
                        auto_ptr_char type(child->getAttributeNS(NULL,_type));
                        try {
                            log.info("building StorageService (%s) of type %s...", id.get(), type.get());
                            m_outer->m_storage[id.get()] = xmlConf.StorageServiceManager.newPlugin(type.get(),child);
                        }
                        catch (exception& ex) {
                            log.crit("failed to instantiate StorageService (%s): %s", id.get(), ex.what());
                        }
                        child=XMLHelper::getNextSiblingElement(child,_StorageService);
                    }

                    // Replay cache.
                    StorageService* replaySS=NULL;
                    child=XMLHelper::getFirstChildElement(e,_ReplayCache);
                    if (child) {
                        auto_ptr_char ssid(child->getAttributeNS(NULL,_StorageService));
                        if (ssid.get() && *ssid.get()) {
                            if (m_outer->m_storage.count(ssid.get()))
                                replaySS = m_outer->m_storage[ssid.get()];
                            if (replaySS)
                                log.info("building ReplayCache on top of StorageService (%s)...", ssid.get());
                            else
                                log.warn("unable to locate StorageService (%s) for ReplayCache, using dedicated in-memory instance", ssid.get());
                        }
                        xmlConf.setReplayCache(new ReplayCache(replaySS));
                    }
                    else {
                        log.warn("no ReplayCache built, missing conf:ReplayCache element?");
                    }
                    
                    // ArtifactMap
                    child=XMLHelper::getFirstChildElement(e,_ArtifactMap);
                    if (child) {
                        auto_ptr_char ssid(child->getAttributeNS(NULL,_StorageService));
                        if (ssid.get() && *ssid.get() && m_outer->m_storage.count(ssid.get())) {
                            log.info("building ArtifactMap on top of StorageService (%s)...", ssid.get());
                            samlConf.setArtifactMap(new ArtifactMap(child, m_outer->m_storage[ssid.get()]));
                        }
                    }
                    if (samlConf.getArtifactMap()==NULL) {
                        log.info("building in-memory ArtifactMap...");
                        samlConf.setArtifactMap(new ArtifactMap(child));
                    }
#endif
                }
                child=XMLHelper::getFirstChildElement(e,_SessionCache);
                if (child) {
                    auto_ptr_char type(child->getAttributeNS(NULL,_type));
                    log.info("building SessionCache of type %s...",type.get());
                    m_outer->m_sessionCache=conf.SessionCacheManager.newPlugin(type.get(), child);
                }
                else {
                    log.fatal("can't build SessionCache, missing conf:SessionCache element?");
                    throw ConfigurationException("Can't build SessionCache, missing conf:SessionCache element?");
                }
            }
        } // end of first-time-only stuff
        
        // Back to the fully dynamic stuff...next up is the RequestMapper.
        if (conf.isEnabled(SPConfig::RequestMapping)) {
            child=XMLHelper::getFirstChildElement(e,_RequestMapper);
            if (child) {
                auto_ptr_char type(child->getAttributeNS(NULL,_type));
                log.info("building RequestMapper of type %s...",type.get());
                m_requestMapper=conf.RequestMapperManager.newPlugin(type.get(),child);
            }
            else {
                log.fatal("can't build RequestMapper, missing conf:RequestMapper element?");
                throw ConfigurationException("Can't build RequestMapper, missing conf:RequestMapper element?");
            }
        }
        
#ifndef SHIBSP_LITE
        // Load security policies.
        child = XMLHelper::getLastChildElement(e,SecurityPolicies);
        if (child) {
            PolicyNodeFilter filter;
            child = XMLHelper::getFirstChildElement(child,Policy);
            while (child) {
                auto_ptr_char id(child->getAttributeNS(NULL,_id));
                pair< PropertySet*,vector<const SecurityPolicyRule*> >& rules = m_policyMap[id.get()];
                rules.first = NULL;
                auto_ptr<DOMPropertySet> settings(new DOMPropertySet());
                settings->load(child, log, &filter);
                rules.first = settings.release();
                
                // Process Rule elements.
                const DOMElement* rule = XMLHelper::getFirstChildElement(child,Rule);
                while (rule) {
                    auto_ptr_char type(rule->getAttributeNS(NULL,_type));
                    try {
                        rules.second.push_back(samlConf.SecurityPolicyRuleManager.newPlugin(type.get(),rule));
                    }
                    catch (exception& ex) {
                        log.crit("error instantiating policy rule (%s) in policy (%s): %s", type.get(), id.get(), ex.what());
                    }
                    rule = XMLHelper::getNextSiblingElement(rule,Rule);
                }
                
                // Process TransportOption elements.
                rule = XMLHelper::getFirstChildElement(child,TransportOption);
                while (rule) {
                    if (rule->hasChildNodes()) {
                        auto_ptr_char provider(rule->getAttributeNS(NULL,_provider));
                        auto_ptr_char option(rule->getAttributeNS(NULL,_option));
                        auto_ptr_char value(rule->getFirstChild()->getNodeValue());
                        if (provider.get() && *provider.get() && option.get() && *option.get() && value.get() && *value.get()) {
                            m_transportOptionMap[id.get()].push_back(
                                make_pair(string(provider.get()), make_pair(string(option.get()), string(value.get())))
                                );
                        }
                    }
                    rule = XMLHelper::getNextSiblingElement(rule,TransportOption);
                }
                
                child = XMLHelper::getNextSiblingElement(child,Policy);
            }
        }
#endif

        // Load the default application. This actually has a fixed ID of "default". ;-)
        child=XMLHelper::getLastChildElement(e,Applications);
        if (!child) {
            log.fatal("can't build default Application object, missing conf:Applications element?");
            throw ConfigurationException("can't build default Application object, missing conf:Applications element?");
        }
        XMLApplication* defapp=new XMLApplication(m_outer,child);
        m_appmap[defapp->getId()]=defapp;
        
        // Load any overrides.
        child = XMLHelper::getFirstChildElement(child,_Application);
        while (child) {
            auto_ptr<XMLApplication> iapp(new XMLApplication(m_outer,child,defapp));
            if (m_appmap.count(iapp->getId()))
                log.crit("found conf:Application element with duplicate id attribute (%s), skipping it", iapp->getId());
            else
                m_appmap[iapp->getId()]=iapp.release();

            child = XMLHelper::getNextSiblingElement(child,_Application);
        }
    }
    catch (exception&) {
        cleanup();
        throw;
    }
}

XMLConfigImpl::~XMLConfigImpl()
{
    cleanup();
}

void XMLConfigImpl::cleanup()
{
    for_each(m_appmap.begin(),m_appmap.end(),cleanup_pair<string,Application>());
    m_appmap.clear();
#ifndef SHIBSP_LITE
    for (map< string,pair<PropertySet*,vector<const SecurityPolicyRule*> > >::iterator i=m_policyMap.begin(); i!=m_policyMap.end(); ++i) {
        delete i->second.first;
        for_each(i->second.second.begin(), i->second.second.end(), xmltooling::cleanup<SecurityPolicyRule>());
    }
    m_policyMap.clear();
#endif
    delete m_requestMapper;
    m_requestMapper = NULL;
    if (m_document)
        m_document->release();
    m_document = NULL;
}

#ifndef SHIBSP_LITE
void XMLConfig::receive(DDF& in, ostream& out)
{
    if (!strcmp(in.name(), "get::RelayState")) {
        const char* id = in["id"].string();
        const char* key = in["key"].string();
        if (!id || !key)
            throw ListenerException("Required parameters missing for RelayState recovery.");

        string relayState;
        StorageService* storage = getStorageService(id);
        if (storage) {
            if (storage->readString("RelayState",key,&relayState)>0) {
                if (in["clear"].integer())
                    storage->deleteString("RelayState",key);
            }
        }
        else {
            Category::getInstance(SHIBSP_LOGCAT".ServiceProvider").error(
                "Storage-backed RelayState with invalid StorageService ID (%s)", id
                );
        }

        // Repack for return to caller.
        DDF ret=DDF(NULL).string(relayState.c_str());
        DDFJanitor jret(ret);
        out << ret;
    }
    else if (!strcmp(in.name(), "set::RelayState")) {
        const char* id = in["id"].string();
        const char* value = in["value"].string();
        if (!id || !value)
            throw ListenerException("Required parameters missing for RelayState creation.");

        string rsKey;
        StorageService* storage = getStorageService(id);
        if (storage) {
            SAMLConfig::getConfig().generateRandomBytes(rsKey,20);
            rsKey = SAMLArtifact::toHex(rsKey);
            storage->createString("RelayState", rsKey.c_str(), value, time(NULL) + 600);
        }
        else {
            Category::getInstance(SHIBSP_LOGCAT".ServiceProvider").error(
                "Storage-backed RelayState with invalid StorageService ID (%s)", id
                );
        }

        // Repack for return to caller.
        DDF ret=DDF(NULL).string(rsKey.c_str());
        DDFJanitor jret(ret);
        out << ret;
    }
}
#endif

pair<bool,DOMElement*> XMLConfig::load()
{
    // Load from source using base class.
    pair<bool,DOMElement*> raw = ReloadableXMLFile::load();
    
    // If we own it, wrap it.
    XercesJanitor<DOMDocument> docjanitor(raw.first ? raw.second->getOwnerDocument() : NULL);

    XMLConfigImpl* impl = new XMLConfigImpl(raw.second,(m_impl==NULL),this,m_log);
    
    // If we held the document, transfer it to the impl. If we didn't, it's a no-op.
    impl->setDocument(docjanitor.release());

    delete m_impl;
    m_impl = impl;

    return make_pair(false,(DOMElement*)NULL);
}