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
 * @file shibsp/attribute/resolver/AttributeResolver.h
 * 
 * The service that resolves the attributes for a particular subject.
 */

#ifndef __shibsp_resolver_h__
#define __shibsp_resolver_h__

#include <shibsp/base.h>

#include <saml/Assertion.h>
#include <saml/saml2/metadata/Metadata.h>
#include <xmltooling/Lockable.h>

namespace shibsp {

    class SHIBSP_API Application;
    class SHIBSP_API Attribute;
    class SHIBSP_API Session;
    class SHIBSP_API ResolutionContext;

#if defined (_MSC_VER)
    #pragma warning( push )
    #pragma warning( disable : 4250 4251 )
#endif

    /**
     * The service that resolves the attributes for a particular subject.
     */
    class SHIBSP_API AttributeResolver : public virtual xmltooling::Lockable
    {
        MAKE_NONCOPYABLE(AttributeResolver);
    protected:
        AttributeResolver() {}
    public:
        virtual ~AttributeResolver() {}

        /**
         * Creates a ResolutionContext based on session bootstrap material.
         * 
         * <p>This enables resolution to occur ahead of session creation so that
         * Attributes can be supplied while creating the session.
         * 
         * @param application       reference to Application that owns the eventual Session
         * @param client_addr       network address of client
         * @param issuer            issuing metadata of assertion issuer, if known
         * @param nameid            principal identifier, normalized to SAML 2, if any
         * @param tokens            assertions initiating the session, if any
         * @return  newly created ResolutionContext, owned by caller
         */
        virtual ResolutionContext* createResolutionContext(
            const Application& application,
            const char* client_addr,
            const opensaml::saml2md::EntityDescriptor* issuer,
            const opensaml::saml2::NameID* nameid,
            const std::vector<const opensaml::Assertion*>* tokens=NULL
            ) const=0;

        /**
         * Creates a ResolutionContext for an existing Session.
         * 
         * @param application   reference to Application that owns the Session
         * @param session       reference to Session
         * @return  newly created ResolutionContext, owned by caller
         */
        virtual ResolutionContext* createResolutionContext(const Application& application, const Session& session) const=0;
        

        /**
         * Gets the attributes for a given subject and returns them in the supplied context.
         * 
         * @param ctx           resolution context to use to resolve attributes
         * @param attributes    list of attributes to resolve or NULL to resolve all attributes
         * 
         * @throws AttributeResolutionException thrown if there is a problem resolving the attributes for the subject
         */
        virtual void resolveAttributes(ResolutionContext& ctx, const std::vector<const char*>* attributes=NULL) const=0;
    };

#if defined (_MSC_VER)
    #pragma warning( pop )
#endif

    /**
     * Registers AttributeResolver classes into the runtime.
     */
    void SHIBSP_API registerAttributeResolvers();

    /** AttributeResolver based on a simple mapping of SAML information. */
    #define SIMPLE_ATTRIBUTE_RESOLVER "Simple"
};

#endif /* __shibsp_resolver_h__ */
