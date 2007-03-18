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
 * @file shibsp/binding/SOAPClient.h
 * 
 * Specialized SOAPClient for SP environment.
 */

#ifndef __shibsp_soap11client_h__
#define __shibsp_soap11client_h__

#include <shibsp/Application.h>
#include <saml/binding/SOAPClient.h>
#include <xmltooling/security/CredentialResolver.h>

namespace shibsp {

    /**
     * Specialized SOAPClient for SP environment.
     */
    class SHIBSP_API SOAPClient : public opensaml::SOAPClient
    {
    public:
        /**
         * Creates a SOAP client instance for an Application to use.
         * 
         * @param application   reference to Application
         * @param policy        reference to (empty) SecurityPolicy to apply
         */
        SOAPClient(const Application& application, opensaml::SecurityPolicy& policy);
        
        virtual ~SOAPClient() {
            if (m_credResolver)
                m_credResolver->unlock();
        }

        /**
         * Override handles message signing for SAML payloads.
         * 
         * @param env       SOAP envelope to send
         * @param peer      peer to send message to, expressed in TrustEngine terms
         * @param endpoint  URL of endpoint to recieve message
         */
        void send(const soap11::Envelope& env, const xmltooling::KeyInfoSource& peer, const char* endpoint);

        void reset();

    protected:
        /**
         * Override prepares transport by applying policy settings from Application.
         * 
         * @param transport reference to transport layer
         */
        void prepareTransport(xmltooling::SOAPTransport& transport);

        /** Application supplied to client. */
        const Application& m_app;

        /** Properties associated with the Application's security policy. */
        const PropertySet* m_settings;

        /** CredentialUse properties, set after transport prep. */
        const PropertySet* m_credUse;

        /** Locked CredentialResolver for transport, set after transport prep. */
        xmltooling::CredentialResolver* m_credResolver;
    };

};

#endif /* __shibsp_soap11client_h__ */