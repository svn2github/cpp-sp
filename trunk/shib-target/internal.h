/*
 * The Shibboleth License, Version 1.
 * Copyright (c) 2002
 * University Corporation for Advanced Internet Development, Inc.
 * All rights reserved
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution, if any, must include
 * the following acknowledgment: "This product includes software developed by
 * the University Corporation for Advanced Internet Development
 * <http://www.ucaid.edu>Internet2 Project. Alternately, this acknowledegement
 * may appear in the software itself, if and wherever such third-party
 * acknowledgments normally appear.
 *
 * Neither the name of Shibboleth nor the names of its contributors, nor
 * Internet2, nor the University Corporation for Advanced Internet Development,
 * Inc., nor UCAID may be used to endorse or promote products derived from this
 * software without specific prior written permission. For written permission,
 * please contact shibboleth@shibboleth.org
 *
 * Products derived from this software may not be called Shibboleth, Internet2,
 * UCAID, or the University Corporation for Advanced Internet Development, nor
 * may Shibboleth appear in their name, without prior written permission of the
 * University Corporation for Advanced Internet Development.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND WITH ALL FAULTS. ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, AND NON-INFRINGEMENT ARE DISCLAIMED AND THE ENTIRE RISK
 * OF SATISFACTORY QUALITY, PERFORMANCE, ACCURACY, AND EFFORT IS WITH LICENSEE.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER, CONTRIBUTORS OR THE UNIVERSITY
 * CORPORATION FOR ADVANCED INTERNET DEVELOPMENT, INC. BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/* internal.h - internally visible declarations

   Scott Cantor
   6/29/03

   $History:$
*/

#ifndef __shibtarget_internal_h__
#define __shibtarget_internal_h__

#ifdef WIN32
# define SHIBTARGET_EXPORTS __declspec(dllexport)
#endif

// eventually we might be able to support autoconf via cygwin...
#if defined (_MSC_VER) || defined(__BORLANDC__)
# include "config_win32.h"
#else
# include "config.h"
#endif

#include "shib-target.h"

#ifdef __cplusplus

#include "ccache-utils.h"

#include <log4cpp/Category.hh>

namespace shibtarget {

    // Wraps the actual RPC connection
    class RPCHandle
    {
    public:
        RPCHandle(const char* shar, u_long program, u_long version);
        ~RPCHandle();

        CLIENT* connect(void);  // connects and returns the CLIENT handle
        void disconnect();      // disconnects, should not return disconnected handles to pool!

    private:
        log4cpp::Category* log;
        const char* m_shar;
        u_long m_program;
        u_long m_version;
        CLIENT*  m_clnt;
        ShibSocket m_sock;
    };
  
    // Manages the pool of connections
    class RPCHandlePool
    {
    public:
        RPCHandlePool() :  m_lock(shibboleth::Mutex::create()) {}
        ~RPCHandlePool();
        RPCHandle* get();
        void put(RPCHandle*);
  
    private:
        std::auto_ptr<shibboleth::Mutex> m_lock;
        std::stack<RPCHandle*> m_pool;
    };
  
    // Cleans up after use
    class RPC
    {
    public:
        RPC();
        ~RPC() {delete m_handle;}
        RPCHandle* operator->() {return m_handle;}
        void pool() {m_pool.put(m_handle); m_handle=NULL;}
    
    private:
        RPCHandlePool& m_pool;
        RPCHandle* m_handle;
    };

    // An implementation of the URL->application mapping API using an XML file
    class XMLApplicationMapper : public IApplicationMapper, public shibboleth::ReloadableXMLFile
    {
    public:
        XMLApplicationMapper(const DOMElement* e) : shibboleth::ReloadableXMLFile(e) {}
        ~XMLApplicationMapper() {}

        const char* getApplicationFromURL(const char* url) const;
        const XMLCh* getXMLChApplicationFromURL(const char* url) const;
        const char* getApplicationFromParsedURL(
            const char* scheme, const char* hostname, unsigned int port, const char* path=NULL
            ) const;
        const XMLCh* getXMLChApplicationFromParsedURL(
            const char* scheme, const char* hostname, unsigned int port, const char* path=NULL
            ) const;

    protected:
        virtual shibboleth::ReloadableXMLFileImpl* newImplementation(const char* pathname) const;
        virtual shibboleth::ReloadableXMLFileImpl* newImplementation(const DOMElement* e) const;
    };

    class STConfig : public ShibTargetConfig
    {
    public:
        STConfig(const char* app_name, const char* inifile);
        ~STConfig();
        void ref();
        void init();
        void shutdown();
        ShibINI& getINI() const { return *ini; }
        IApplicationMapper* getApplicationMapper() const { return m_applicationMapper; }
        saml::Iterator<shibboleth::IMetadata*> getMetadataProviders() const { return metadatas; }
        saml::Iterator<shibboleth::IRevocation*> getRevocationProviders() const { return revocations; }
        saml::Iterator<shibboleth::ITrust*> getTrustProviders() const { return trusts; }
        saml::Iterator<shibboleth::ICredentials*> getCredentialProviders() const { return creds; }
        saml::Iterator<shibboleth::IAAP*> getAAPProviders() const { return aaps; }
        saml::Iterator<const XMLCh*> getPolicies() const { return saml::Iterator<const XMLCh*>(policies); }
        RPCHandlePool& getRPCHandlePool() { return m_rpcpool; }
     
    private:
        saml::SAMLConfig& samlConf;
        shibboleth::ShibConfig& shibConf;
        ShibINI* ini;
        std::string m_app_name;
        int refcount;
        std::vector<const XMLCh*> policies;
        std::string m_SocketName;
#ifdef WANT_TCP_SHAR
        std::vector<std::string> m_SocketACL;
#endif
        IApplicationMapper* m_applicationMapper;
        std::vector<shibboleth::IMetadata*> metadatas;
        std::vector<shibboleth::IRevocation*> revocations;
        std::vector<shibboleth::ITrust*> trusts;
        std::vector<shibboleth::ICredentials*> creds;
        std::vector<shibboleth::IAAP*> aaps;
        
        RPCHandlePool m_rpcpool;
      
        friend const char* ::shib_target_sockname();
        friend const char* ::shib_target_sockacl(unsigned int);
    };

    class XML
    {
    public:
        // URI constants
        static const XMLCh APPMAP_NS[];
        static const XMLCh APPMAP_SCHEMA_ID[];

        struct Literals
        {
            static const XMLCh ApplicationID[];
            static const XMLCh ApplicationMap[];
            static const XMLCh Host[];
            static const XMLCh Name[];
            static const XMLCh Path[];
            static const XMLCh Port[];
            static const XMLCh Scheme[];
        };
    };
}

#endif

#endif
