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

/* XMLMetadata.cpp - a metadata implementation that uses an XML-based registry

   Scott Cantor
   9/27/02

   $History:$
*/

#include "internal.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <log4cpp/Category.hh>
#include <xercesc/framework/URLInputSource.hpp>
#include <xsec/enc/OpenSSL/OpenSSLCryptoX509.hpp>

using namespace shibboleth;
using namespace saml;
using namespace log4cpp;
using namespace std;

namespace shibboleth {

    class XMLMetadataImpl : public ReloadableXMLFileImpl
    {
    public:
        XMLMetadataImpl(const char* pathname) : ReloadableXMLFileImpl(pathname) { init(); }
        XMLMetadataImpl(const DOMElement* e) : ReloadableXMLFileImpl(e) { init(); }
        void init();
        ~XMLMetadataImpl();
    
        class ContactInfo : public IContactInfo
        {
        public:
            ContactInfo(ContactType type, const XMLCh* name, const XMLCh* email)
                : m_type(type), m_name(name), m_email(email) {}
        
            ContactType getType() const { return m_type; }
            const char* getName() const { return m_name.get(); }
            const char* getEmail() const { return m_email.get(); }
        
        private:
            ContactType m_type;
            auto_ptr_char m_name, m_email;
        };
        
        class Authority : public IAuthority
        {
        public:
            Authority(const XMLCh* name, const XMLCh* url) : m_name(name), m_url(url) {}
        
            const XMLCh* getName() const { return m_name; }
            const char* getURL() const { return m_url.get(); }
        
        private:
            const XMLCh* m_name;
            auto_ptr_char m_url;
        };
    
        class OriginSite : public IOriginSite
        {
        public:
            OriginSite(const XMLCh* name, const XMLCh* errorURL) : m_name(name), m_errorURL(errorURL) {}
            ~OriginSite();
        
            const XMLCh* getName() const {return m_name;}
            Iterator<const XMLCh*> getGroups() const {return m_groups;}
            Iterator<const IContactInfo*> getContacts() const {return m_contacts;}
            const char* getErrorURL() const {return m_errorURL.get();}
            bool validate(const saml::Iterator<ITrust*>& trusts, const Iterator<XSECCryptoX509*>& certs) const
                {Trust t(trusts); return t.validate(this,certs);}
            bool validate(const saml::Iterator<ITrust*>& trusts, const Iterator<const XMLCh*>& certs) const
                {Trust t(trusts); return t.validate(this,certs);}
            Iterator<const IAuthority*> getHandleServices() const {return m_handleServices;}
            Iterator<const IAuthority*> getAttributeAuthorities() const {return m_attributes;}
            Iterator<std::pair<const XMLCh*,bool> > getSecurityDomains() const {return m_domains;}

        private:
            friend class XMLMetadataImpl;
            const XMLCh* m_name;
            auto_ptr_char m_errorURL;
            vector<const IContactInfo*> m_contacts;
            vector<const IAuthority*> m_handleServices;
            vector<const IAuthority*> m_attributes;
            vector<pair<const XMLCh*,bool> > m_domains;
            vector<const XMLCh*> m_groups;
        };

    #ifdef HAVE_GOOD_STL
        typedef map<xstring,OriginSite*> sitemap_t;
    #else
        typedef map<string,OriginSite*> sitemap_t;
    #endif
        sitemap_t m_sites;
    };

    class XMLMetadata : public IMetadata, public ReloadableXMLFile
    {
    public:
        XMLMetadata(const DOMElement* e) : ReloadableXMLFile(e) {}
        ~XMLMetadata() {}

        const ISite* lookup(const XMLCh* site) const;
        
    protected:
        virtual ReloadableXMLFileImpl* newImplementation(const char* pathname) const;
        virtual ReloadableXMLFileImpl* newImplementation(const DOMElement* e) const;
    };
}

extern "C" IMetadata* XMLMetadataFactory(const DOMElement* e)
{
    XMLMetadata* m=new XMLMetadata(e);
    try
    {
        m->getImplementation();
    }
    catch (...)
    {
        delete m;
        throw;
    }
    return m;    
}

ReloadableXMLFileImpl* XMLMetadata::newImplementation(const DOMElement* e) const
{
    return new XMLMetadataImpl(e);
}

ReloadableXMLFileImpl* XMLMetadata::newImplementation(const char* pathname) const
{
    return new XMLMetadataImpl(pathname);
}

XMLMetadataImpl::OriginSite::~OriginSite()
{
    for (vector<const IContactInfo*>::iterator i=m_contacts.begin(); i!=m_contacts.end(); i++)
        delete const_cast<IContactInfo*>(*i);
    for (vector<const IAuthority*>::iterator j=m_handleServices.begin(); j!=m_handleServices.end(); j++)
        delete const_cast<IAuthority*>(*j);
    for (vector<const IAuthority*>::iterator k=m_attributes.begin(); k!=m_attributes.end(); k++)
        delete const_cast<IAuthority*>(*k);
}

void XMLMetadataImpl::init()
{
    NDC ndc("XMLMetadataImpl");
    Category& log=Category::getInstance(SHIB_LOGCAT".XMLMetadataImpl");

    try
    {
        if (XMLString::compareString(XML::SHIB_NS,m_root->getNamespaceURI()) ||
            XMLString::compareString(XML::Literals::SiteGroup,m_root->getLocalName()))
        {
            log.error("Construction requires a valid site file: (shib:SiteGroup as root element)");
            throw MetadataException("Construction requires a valid site file: (shib:SiteGroup as root element)");
        }

        // Loop over the OriginSite elements.
        DOMNodeList* nlist = m_root->getElementsByTagNameNS(XML::SHIB_NS,XML::Literals::OriginSite);
        for (int i=0; nlist && i<nlist->getLength(); i++)
        {
            const XMLCh* os_name=static_cast<DOMElement*>(nlist->item(i))->getAttributeNS(NULL,XML::Literals::Name);
            if (!os_name || !*os_name)
                continue;

            OriginSite* os_obj =
                new OriginSite(os_name,static_cast<DOMElement*>(nlist->item(i))->getAttributeNS(NULL,XML::Literals::ErrorURL));
#ifdef HAVE_GOOD_STL
            m_sites[os_name]=os_obj;
#else
            auto_ptr<char> os_name2(XMLString::transcode(os_name));
            m_sites[os_name2.get()]=os_obj;
#endif

            // Record all the SiteGroups containing this site.
            DOMNode* group=nlist->item(i)->getParentNode();
            while (group && group->getNodeType()==DOMNode::ELEMENT_NODE)
            {
                os_obj->m_groups.push_back(static_cast<DOMElement*>(group)->getAttributeNS(NULL,XML::Literals::Name));
                group=group->getParentNode();
            }

            DOMNode* os_child=nlist->item(i)->getFirstChild();
            while (os_child)
            {
                if (os_child->getNodeType()!=DOMNode::ELEMENT_NODE)
                {
                    os_child=os_child->getNextSibling();
                    continue;
                }

                // Process the various kinds of OriginSite children that we care about...
                if (!XMLString::compareString(XML::SHIB_NS,os_child->getNamespaceURI()) &&
                    !XMLString::compareString(XML::Literals::Contact,os_child->getLocalName()))
                {
                    ContactInfo::ContactType type;
                    DOMElement* con=static_cast<DOMElement*>(os_child);
                    if (!XMLString::compareString(con->getAttributeNS(NULL,XML::Literals::Type),XML::Literals::technical))
                        type=IContactInfo::technical;
                    else if (!XMLString::compareString(con->getAttributeNS(NULL,XML::Literals::Type),XML::Literals::administrative))
                        type=IContactInfo::administrative;
                    else if (!XMLString::compareString(con->getAttributeNS(NULL,XML::Literals::Type),XML::Literals::billing))
                        type=IContactInfo::billing;
                    else if (!XMLString::compareString(con->getAttributeNS(NULL,XML::Literals::Type),XML::Literals::other))
                        type=IContactInfo::other;
                    ContactInfo* cinfo=new ContactInfo(
                        type,
                        con->getAttributeNS(NULL,XML::Literals::Name),
                        con->getAttributeNS(NULL,XML::Literals::Email)
                        );
                    os_obj->m_contacts.push_back(cinfo);
                }
                else if (!XMLString::compareString(XML::SHIB_NS,os_child->getNamespaceURI()) &&
                       !XMLString::compareString(XML::Literals::HandleService,os_child->getLocalName()))
                {
                    const XMLCh* hs_name=static_cast<DOMElement*>(os_child)->getAttributeNS(NULL,XML::Literals::Name);
                    const XMLCh* hs_loc=static_cast<DOMElement*>(os_child)->getAttributeNS(NULL,XML::Literals::Location);
                    if (hs_name && *hs_name && hs_loc && *hs_loc)
                        os_obj->m_handleServices.push_back(new Authority(hs_name,hs_loc));
                }
                else if (!XMLString::compareString(XML::SHIB_NS,os_child->getNamespaceURI()) &&
                       !XMLString::compareString(XML::Literals::AttributeAuthority,os_child->getLocalName()))
                {
                    const XMLCh* aa_name=static_cast<DOMElement*>(os_child)->getAttributeNS(NULL,XML::Literals::Name);
                    const XMLCh* aa_loc=static_cast<DOMElement*>(os_child)->getAttributeNS(NULL,XML::Literals::Location);
                    if (aa_name && *aa_name && aa_loc && *aa_loc)
                        os_obj->m_attributes.push_back(new Authority(aa_name,aa_loc));
                }
                else if (!XMLString::compareString(XML::SHIB_NS,os_child->getNamespaceURI()) &&
                            !XMLString::compareString(XML::Literals::Domain,os_child->getLocalName()))
                {
                    const XMLCh* dom=os_child->getFirstChild()->getNodeValue();
                    if (dom && *dom)
                    {
                        static const XMLCh one[]={ chDigit_1, chNull };
                        static const XMLCh tru[]={ chLatin_t, chLatin_r, chLatin_u, chLatin_e, chNull };
                        const XMLCh* regexp=static_cast<DOMElement*>(os_child)->getAttributeNS(NULL,XML::Literals::regexp);
                        bool flag=(!XMLString::compareString(regexp,one) || !XMLString::compareString(regexp,tru));
                        os_obj->m_domains.push_back(pair<const XMLCh*,bool>(dom,flag));
                    }
                }
                os_child = os_child->getNextSibling();
            }
        }
    }
    catch (SAMLException& e)
    {
        log.errorStream() << "Error while parsing site configuration: " << e.what() << CategoryStream::ENDLINE;
        for (sitemap_t::iterator i=m_sites.begin(); i!=m_sites.end(); i++)
            delete i->second;
        if (m_doc)
            m_doc->release();
        throw;
    }
    catch (...)
    {
        log.error("Unexpected error while parsing site configuration");
        for (sitemap_t::iterator i=m_sites.begin(); i!=m_sites.end(); i++)
            delete i->second;
        if (m_doc)
            m_doc->release();
        throw;
    }
}

XMLMetadataImpl::~XMLMetadataImpl()
{
    for (sitemap_t::iterator i=m_sites.begin(); i!=m_sites.end(); i++)
        delete i->second;
}

const ISite* XMLMetadata::lookup(const XMLCh* site) const
{
    XMLMetadataImpl* impl=dynamic_cast<XMLMetadataImpl*>(getImplementation());
#ifdef HAVE_GOOD_STL
    XMLMetadataImpl::sitemap_t::const_iterator i=impl->m_sites.find(site);
#else
    auto_ptr<char> temp(XMLString::transcode(site));
    XMLMetadataImpl::sitemap_t::const_iterator i=impl->m_sites.find(temp.get());
#endif
    return (i==impl->m_sites.end()) ? NULL : i->second;
}
