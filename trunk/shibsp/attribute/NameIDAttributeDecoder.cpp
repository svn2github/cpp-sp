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
 * NameIDAttributeDecoder.cpp
 * 
 * Decodes SAML into NameIDAttributes
 */

#include "internal.h"
#include "attribute/AttributeDecoder.h"
#include "attribute/NameIDAttribute.h"

#include <log4cpp/Category.hh>
#include <saml/saml1/core/Assertions.h>
#include <saml/saml2/core/Assertions.h>

using namespace shibsp;
using namespace opensaml::saml1;
using namespace opensaml::saml2;
using namespace xmltooling;
using namespace log4cpp;
using namespace std;

namespace shibsp {
    static XMLCh formatter[] = UNICODE_LITERAL_9(f,o,r,m,a,t,t,e,r);

    class SHIBSP_DLLLOCAL NameIDAttributeDecoder : virtual public AttributeDecoder
    {
    public:
        NameIDAttributeDecoder(const DOMElement* e) : m_formatter(e ? e->getAttributeNS(NULL,formatter) : NULL) {}
        ~NameIDAttributeDecoder() {}

        shibsp::Attribute* decode(const char* id, const XMLObject* xmlObject) const;

    private:
        void extract(const NameID* n, vector<NameIDAttribute::Value>& dest) const;
        void extract(const NameIdentifier* n, vector<NameIDAttribute::Value>& dest) const;
        auto_ptr_char m_formatter;
    };

    AttributeDecoder* SHIBSP_DLLLOCAL NameIDAttributeDecoderFactory(const DOMElement* const & e)
    {
        return new NameIDAttributeDecoder(e);
    }
};

shibsp::Attribute* NameIDAttributeDecoder::decode(const char* id, const XMLObject* xmlObject) const
{
    auto_ptr<NameIDAttribute> nameid(
        new NameIDAttribute(id, (m_formatter.get() && *m_formatter.get()) ? m_formatter.get() : DEFAULT_NAMEID_FORMATTER)
        );
    vector<NameIDAttribute::Value>& dest = nameid->getValues();
    vector<XMLObject*>::const_iterator v,stop;

    Category& log = Category::getInstance(SHIBSP_LOGCAT".AttributeDecoder");
    
    if (xmlObject && XMLString::equals(opensaml::saml1::Attribute::LOCAL_NAME,xmlObject->getElementQName().getLocalPart())) {
        const opensaml::saml2::Attribute* saml2attr = dynamic_cast<const opensaml::saml2::Attribute*>(xmlObject);
        if (saml2attr) {
            const vector<XMLObject*>& values = saml2attr->getAttributeValues();
            v = values.begin();
            stop = values.end();
            if (log.isDebugEnabled()) {
                auto_ptr_char n(saml2attr->getName());
                log.debug("decoding NameIDAttribute (%s) from SAML 2 Attribute (%s) with %lu value(s)", id, n.get() ? n.get() : "unnamed", values.size());
            }
        }
        else {
            const opensaml::saml1::Attribute* saml1attr = dynamic_cast<const opensaml::saml1::Attribute*>(xmlObject);
            if (saml1attr) {
                const vector<XMLObject*>& values = saml2attr->getAttributeValues();
                v = values.begin();
                stop = values.end();
                if (log.isDebugEnabled()) {
                    auto_ptr_char n(saml1attr->getAttributeName());
                    log.debug("decoding NameIDAttribute (%s) from SAML 1 Attribute (%s) with %lu value(s)", id, n.get() ? n.get() : "unnamed", values.size());
                }
            }
            else {
                log.warn("XMLObject type not recognized by NameIDAttributeDecoder, no values returned");
                return NULL;
            }
        }

        for (; v!=stop; ++v) {
            const NameID* n2 = dynamic_cast<const NameID*>(*v);
            if (n2)
                extract(n2, dest);
            else {
                const NameIdentifier* n1=dynamic_cast<const NameIdentifier*>(*v);
                if (n1)
                    extract(n1, dest);
                else if ((*v)->hasChildren()) {
                    const list<XMLObject*>& values = (*v)->getOrderedChildren();
                    for (list<XMLObject*>::const_iterator vv = values.begin(); vv!=values.end(); ++vv) {
                        if (n2=dynamic_cast<const NameID*>(*vv))
                            extract(n2, dest);
                        else if (n1=dynamic_cast<const NameIdentifier*>(*vv))
                            extract(n1, dest);
                        else
                            log.warn("skipping AttributeValue without a recognizable NameID/NameIdentifier");
                    }
                }
            }
        }

        return dest.empty() ? NULL : nameid.release();
    }

    const NameID* saml2name = dynamic_cast<const NameID*>(xmlObject);
    if (saml2name) {
        if (log.isDebugEnabled()) {
            auto_ptr_char f(saml2name->getFormat());
            log.debug("decoding NameIDAttribute (%s) from SAML 2 NameID with Format (%s)", id, f.get() ? f.get() : "unspecified");
        }
        extract(saml2name, dest);
    }
    else {
        const NameIdentifier* saml1name = dynamic_cast<const NameIdentifier*>(xmlObject);
        if (saml1name) {
            if (log.isDebugEnabled()) {
                auto_ptr_char f(saml1name->getFormat());
                log.debug("decoding NameIDAttribute (%s) from SAML 1 NameIdentifier with Format (%s)", id, f.get() ? f.get() : "unspecified");
            }
            extract(saml1name, dest);
        }
        else {
            log.warn("XMLObject type not recognized by NameIDAttributeDecoder, no values returned");
            return NULL;
        }
    }

    return dest.empty() ? NULL : nameid.release();
}

void NameIDAttributeDecoder::extract(const NameID* n, vector<NameIDAttribute::Value>& dest) const
{
    char* name = toUTF8(n->getName());
    if (name && *name) {
        dest.push_back(NameIDAttribute::Value());
        NameIDAttribute::Value& val = dest.back();
        val.m_Name = name;
        char* str = toUTF8(n->getFormat());
        if (str) {
            val.m_Format = str;
            delete[] str;
        }
        str = toUTF8(n->getNameQualifier());
        if (str) {
            val.m_NameQualifier = str;
            delete[] str;
        }
        str = toUTF8(n->getSPNameQualifier());
        if (str) {
            val.m_SPNameQualifier = str;
            delete[] str;
        }
        str = toUTF8(n->getSPProvidedID());
        if (str) {
            val.m_SPProvidedID = str;
            delete[] str;
        }
    }
    delete[] name;
}

void NameIDAttributeDecoder::extract(const NameIdentifier* n, vector<NameIDAttribute::Value>& dest) const
{
    char* name = toUTF8(n->getName());
    if (name && *name) {
        dest.push_back(NameIDAttribute::Value());
        NameIDAttribute::Value& val = dest.back();
        val.m_Name = name;
        char* str = toUTF8(n->getFormat());
        if (str) {
            val.m_Format = str;
            delete[] str;
        }
        str = toUTF8(n->getNameQualifier());
        if (str) {
            val.m_NameQualifier = str;
            delete[] str;
        }
    }
    delete[] name;
}
