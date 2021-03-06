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


/* shib.h - Shibboleth header file

   Scott Cantor
   6/4/02

   $History:$
*/

#ifndef __shib_h__
#define __shib_h__

#include <saml.h>

#ifdef WIN32
# ifndef SHIB_EXPORTS
#  define SHIB_EXPORTS __declspec(dllimport)
# endif
#else
# define SHIB_EXPORTS
#endif

namespace shibboleth
{
    class SHIB_EXPORTS UnsupportedProtocolException : public saml::SAMLException
    {
    public:
        explicit UnsupportedProtocolException(const char* msg) : saml::SAMLException(msg) {}
        explicit UnsupportedProtocolException(const std::string& msg) : saml::SAMLException(msg) {}
        explicit UnsupportedProtocolException(saml::QName codes[], const char* msg) : saml::SAMLException(codes,msg) {}
        explicit UnsupportedProtocolException(saml::QName codes[], const std::string& msg) : saml::SAMLException(codes, msg) {}
    };

    struct SHIB_EXPORTS Constants
    {
        static const XMLCh POLICY_CLUBSHIB[];
        static const XMLCh SHIB_ATTRIBUTE_NAMESPACE_URI[];
    };

    
    struct SHIB_EXPORTS IOriginSiteMapper
    {
        virtual saml::Iterator<saml::xstring> getHandleServiceNames(const XMLCh* originSite)=0;
        virtual void* getHandleServiceKey(const XMLCh* handleService)=0;
        virtual saml::Iterator<saml::xstring> getSecurityDomains(const XMLCh* originSite)=0;
        virtual saml::Iterator<void*> getTrustedRoots()=0;
    };

    class SHIB_EXPORTS ShibConfig
    {
    public:
        // global per-process setup and shutdown of Shibboleth runtime
        static bool init(ShibConfig* pconfig);
        static void term();

        // enables runtime and clients to access configuration
        static const ShibConfig* getConfig();

    /* start of external configuration */
        IOriginSiteMapper* origin_mapper;
    /* end of external configuration */

    private:
        static const ShibConfig* g_config;
    };


    class SHIB_EXPORTS XML
    {
    public:
        // URI constants
        static const XMLCh SHIB_NS[];
        static const XMLCh SHIB_SCHEMA_ID[];

        struct SHIB_EXPORTS Literals
        {
            // Shibboleth vocabulary

            // XML vocabulary
	    static const XMLCh InvalidHandle[];
            static const XMLCh xmlns_shib[];
        };
    };


    class SHIB_EXPORTS SAMLBindingFactory
    {
    public:
        static saml::SAMLBinding* getInstance(const XMLCh* protocol=saml::SAMLBinding::SAML_SOAP_HTTPS);
    };
}

#endif
