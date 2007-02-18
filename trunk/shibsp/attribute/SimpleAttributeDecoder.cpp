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
 * SimpleAttributeDecoder.cpp
 * 
 * Decodes SAML into SimpleAttributes
 */

#include "internal.h"
#include "attribute/AttributeDecoder.h"
#include "attribute/SimpleAttribute.h"

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
    class SimpleAttributeDecoder : virtual public AttributeDecoder
    {
    public:
        SimpleAttributeDecoder(const DOMElement* e) {}
        ~SimpleAttributeDecoder() {}

        shibsp::Attribute* decode(const char* id, const XMLObject* xmlObject) const;
    };

    AttributeDecoder* SHIBSP_DLLLOCAL SimpleAttributeDecoderFactory(const DOMElement* const & e)
    {
        return new SimpleAttributeDecoder(e);
    }
};

shibsp::Attribute* SimpleAttributeDecoder::decode(const char* id, const XMLObject* xmlObject) const
{
    char* val;
    auto_ptr<SimpleAttribute> simple(new SimpleAttribute(id));
    vector<string>& dest = simple->getValues();
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
                log.debug("decoding SimpleAttribute (%s) from SAML 2 Attribute (%s) with %lu value(s)", id, n.get() ? n.get() : "unnamed", values.size());
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
                    log.debug("decoding SimpleAttribute (%s) from SAML 1 Attribute (%s) with %lu value(s)", id, n.get() ? n.get() : "unnamed", values.size());
                }
            }
            else {
                log.warn("XMLObject type not recognized by SimpleAttributeDecoder, no values returned");
                return NULL;
            }
        }

        for (; v!=stop; ++v) {
            if (!(*v)->hasChildren()) {
                val = toUTF8((*v)->getTextContent());
                if (val && *val)
                    dest.push_back(val);
                else
                    log.warn("skipping empty AttributeValue");
                delete[] val;
            }
            else {
                log.warn("skipping complex AttributeValue");
            }
        }

        return dest.empty() ? NULL : simple.release();
    }

    const NameID* saml2name = dynamic_cast<const NameID*>(xmlObject);
    if (saml2name) {
        if (log.isDebugEnabled()) {
            auto_ptr_char f(saml2name->getFormat());
            log.debug("decoding SimpleAttribute (%s) from SAML 2 NameID with Format (%s)", id, f.get() ? f.get() : "unspecified");
        }
        val = toUTF8(saml2name->getName());
    }
    else {
        const NameIdentifier* saml1name = dynamic_cast<const NameIdentifier*>(xmlObject);
        if (saml1name) {
            if (log.isDebugEnabled()) {
                auto_ptr_char f(saml1name->getFormat());
                log.debug("decoding SimpleAttribute (%s) from SAML 1 NameIdentifier with Format (%s)", id, f.get() ? f.get() : "unspecified");
            }
            val = toUTF8(saml1name->getName());
        }
        else {
            log.warn("XMLObject type not recognized by SimpleAttributeDecoder, no values returned");
            return NULL;
        }
    }

    if (val && *val)
        dest.push_back(val);
    else
        log.warn("ignoring empty NameID");
    delete[] val;
    return dest.empty() ? NULL : simple.release();
}
