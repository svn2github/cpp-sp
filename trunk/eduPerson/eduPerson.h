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


/* eduPerson.h - Shibboleth eduPerson attribute extensions

   Scott Cantor
   6/4/02

   $History:$
*/

#ifndef __eduPerson_h__
#define __eduPerson_h__

#include <saml/saml.h>

#ifdef WIN32
# ifndef EDUPERSON_EXPORTS
#  define EDUPERSON_EXPORTS __declspec(dllimport)
# endif
#else
# define EDUPERSON_EXPORTS
#endif

namespace eduPerson
{
    class EDUPERSON_EXPORTS ScopedAttribute : public saml::SAMLAttribute
    {
    public:
        ScopedAttribute(const XMLCh* name, const XMLCh* ns, const XMLCh* defaultScope, const saml::QName* type=NULL,
                        long lifetime=0, const saml::Iterator<const XMLCh*>& scopes=saml::Iterator<const XMLCh*>(),
                        const saml::Iterator<const XMLCh*>& values=saml::Iterator<const XMLCh*>());
        ScopedAttribute(DOMElement* e);
        virtual ~ScopedAttribute();

        virtual DOMNode* toDOM(DOMDocument* doc=NULL, bool xmlns=true) const;
        virtual saml::SAMLObject* clone() const;

        virtual saml::Iterator<saml::xstring> getValues() const;
        virtual saml::Iterator<std::string> getSingleByteValues() const;

        static const XMLCh Scope[];

    protected:
        virtual bool accept(DOMElement* e) const;
        virtual bool addValue(DOMElement* e);

        saml::xstring m_defaultScope;
        std::vector<saml::xstring> m_scopes;
        mutable std::vector<saml::xstring> m_scopedValues;
    };

    class EDUPERSON_EXPORTS EPPNAttribute : public ScopedAttribute
    {
    public:
        EPPNAttribute(const XMLCh* defaultScope, long lifetime=0, const XMLCh* scope=NULL, const XMLCh* value=NULL);
        EPPNAttribute(DOMElement* e);
        virtual ~EPPNAttribute();

        virtual void addValues(DOMElement* e);
        virtual saml::SAMLObject* clone() const;
    };

    class EDUPERSON_EXPORTS AffiliationAttribute : public ScopedAttribute
    {
    public:
        AffiliationAttribute(const XMLCh* defaultScope, long lifetime=0,
                             const saml::Iterator<const XMLCh*>& scopes=saml::Iterator<const XMLCh*>(),
                             const saml::Iterator<const XMLCh*>& values=saml::Iterator<const XMLCh*>());
        AffiliationAttribute(DOMElement* e);
        virtual ~AffiliationAttribute();

        virtual void addValues(DOMElement* e);
        virtual saml::SAMLObject* clone() const;
    };

    class EDUPERSON_EXPORTS PrimaryAffiliationAttribute : public ScopedAttribute
    {
    public:
        PrimaryAffiliationAttribute(const XMLCh* defaultScope, long lifetime=0, const XMLCh* scope=NULL, const XMLCh* value=NULL);
        PrimaryAffiliationAttribute(DOMElement* e);
        virtual ~PrimaryAffiliationAttribute();

        virtual void addValues(DOMElement* e);
        virtual saml::SAMLObject* clone() const;
    };

    class EDUPERSON_EXPORTS EntitlementAttribute : public saml::SAMLAttribute
    {
    public:
        EntitlementAttribute(long lifetime=0, const saml::Iterator<const XMLCh*>& values=saml::Iterator<const XMLCh*>());
        EntitlementAttribute(DOMElement* e);
        virtual ~EntitlementAttribute();

        virtual void addValues(DOMElement* e);
        virtual saml::SAMLObject* clone() const;
    };

    struct EDUPERSON_EXPORTS XML
    {
        static const XMLCh EDUPERSON_NS[];
        static const XMLCh EDUPERSON_SCHEMA_ID[];
    };

    struct EDUPERSON_EXPORTS Constants
    {
        static const XMLCh EDUPERSON_PRINCIPAL_NAME[];
        static const XMLCh EDUPERSON_AFFILIATION[];
        static const XMLCh EDUPERSON_PRIMARY_AFFILIATION[];
        static const XMLCh EDUPERSON_ENTITLEMENT[];

        static const XMLCh EDUPERSON_PRINCIPAL_NAME_TYPE[];
        static const XMLCh EDUPERSON_AFFILIATION_TYPE[];
    };
}

#endif
