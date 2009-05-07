/*
 *  Copyright 2001-2009 Internet2
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
 * shibsp/attribute/Attribute.cpp
 *
 * A resolved attribute.
 */

#include "internal.h"
#include "SPConfig.h"
#ifndef SHIBSP_LITE
# include "attribute/AttributeDecoder.h"
#endif
#include "attribute/SimpleAttribute.h"
#include "attribute/ScopedAttribute.h"
#include "attribute/NameIDAttribute.h"
#include "attribute/ExtensibleAttribute.h"
#include "util/SPConstants.h"

#include <xercesc/util/XMLUniDefs.hpp>

using namespace shibsp;
using namespace xmltooling;
using namespace std;

namespace shibsp {

    SHIBSP_DLLLOCAL Attribute* SimpleAttributeFactory(DDF& in) {
        return new SimpleAttribute(in);
    }

    SHIBSP_DLLLOCAL Attribute* ScopedAttributeFactory(DDF& in) {
        return new ScopedAttribute(in);
    }

    SHIBSP_DLLLOCAL Attribute* NameIDAttributeFactory(DDF& in) {
        return new NameIDAttribute(in);
    }

    SHIBSP_DLLLOCAL Attribute* ExtensibleAttributeFactory(DDF& in) {
        return new ExtensibleAttribute(in);
    }

#ifndef SHIBSP_LITE
    SHIBSP_DLLLOCAL PluginManager<AttributeDecoder,xmltooling::QName,const DOMElement*>::Factory StringAttributeDecoderFactory;
    SHIBSP_DLLLOCAL PluginManager<AttributeDecoder,xmltooling::QName,const DOMElement*>::Factory ScopedAttributeDecoderFactory;
    SHIBSP_DLLLOCAL PluginManager<AttributeDecoder,xmltooling::QName,const DOMElement*>::Factory NameIDAttributeDecoderFactory;
    SHIBSP_DLLLOCAL PluginManager<AttributeDecoder,xmltooling::QName,const DOMElement*>::Factory NameIDFromScopedAttributeDecoderFactory;
    SHIBSP_DLLLOCAL PluginManager<AttributeDecoder,xmltooling::QName,const DOMElement*>::Factory KeyInfoAttributeDecoderFactory;
    SHIBSP_DLLLOCAL PluginManager<AttributeDecoder,xmltooling::QName,const DOMElement*>::Factory DOMAttributeDecoderFactory;

    static const XMLCh _StringAttributeDecoder[] = UNICODE_LITERAL_22(S,t,r,i,n,g,A,t,t,r,i,b,u,t,e,D,e,c,o,d,e,r);
    static const XMLCh _ScopedAttributeDecoder[] = UNICODE_LITERAL_22(S,c,o,p,e,d,A,t,t,r,i,b,u,t,e,D,e,c,o,d,e,r);
    static const XMLCh _NameIDAttributeDecoder[] = UNICODE_LITERAL_22(N,a,m,e,I,D,A,t,t,r,i,b,u,t,e,D,e,c,o,d,e,r);
    static const XMLCh _NameIDFromScopedAttributeDecoder[] = UNICODE_LITERAL_32(N,a,m,e,I,D,F,r,o,m,S,c,o,p,e,d,A,t,t,r,i,b,u,t,e,D,e,c,o,d,e,r);
    static const XMLCh _KeyInfoAttributeDecoder[] =UNICODE_LITERAL_23(K,e,y,I,n,f,o,A,t,t,r,i,b,u,t,e,D,e,c,o,d,e,r);
    static const XMLCh _DOMAttributeDecoder[] =    UNICODE_LITERAL_19(D,O,M,A,t,t,r,i,b,u,t,e,D,e,c,o,d,e,r);

    static const XMLCh caseSensitive[] =           UNICODE_LITERAL_13(c,a,s,e,S,e,n,s,i,t,i,v,e);
#endif
};

#ifndef SHIBSP_LITE
xmltooling::QName shibsp::StringAttributeDecoderType(shibspconstants::SHIB2ATTRIBUTEMAP_NS, _StringAttributeDecoder);
xmltooling::QName shibsp::ScopedAttributeDecoderType(shibspconstants::SHIB2ATTRIBUTEMAP_NS, _ScopedAttributeDecoder);
xmltooling::QName shibsp::NameIDAttributeDecoderType(shibspconstants::SHIB2ATTRIBUTEMAP_NS, _NameIDAttributeDecoder);
xmltooling::QName shibsp::NameIDFromScopedAttributeDecoderType(shibspconstants::SHIB2ATTRIBUTEMAP_NS, _NameIDFromScopedAttributeDecoder);
xmltooling::QName shibsp::KeyInfoAttributeDecoderType(shibspconstants::SHIB2ATTRIBUTEMAP_NS, _KeyInfoAttributeDecoder);
xmltooling::QName shibsp::DOMAttributeDecoderType(shibspconstants::SHIB2ATTRIBUTEMAP_NS, _DOMAttributeDecoder);

void shibsp::registerAttributeDecoders()
{
    SPConfig& conf = SPConfig::getConfig();
    conf.AttributeDecoderManager.registerFactory(StringAttributeDecoderType, StringAttributeDecoderFactory);
    conf.AttributeDecoderManager.registerFactory(ScopedAttributeDecoderType, ScopedAttributeDecoderFactory);
    conf.AttributeDecoderManager.registerFactory(NameIDAttributeDecoderType, NameIDAttributeDecoderFactory);
    conf.AttributeDecoderManager.registerFactory(NameIDFromScopedAttributeDecoderType, NameIDFromScopedAttributeDecoderFactory);
    conf.AttributeDecoderManager.registerFactory(KeyInfoAttributeDecoderType, KeyInfoAttributeDecoderFactory);
    conf.AttributeDecoderManager.registerFactory(DOMAttributeDecoderType, DOMAttributeDecoderFactory);
}

AttributeDecoder::AttributeDecoder(const DOMElement *e) : m_caseSensitive(true)
{
    if (e) {
        const XMLCh* flag = e->getAttributeNS(NULL,caseSensitive);
        if (flag && (*flag == chLatin_f || *flag == chDigit_0))
            m_caseSensitive = false;
    }
}
#endif

void shibsp::registerAttributeFactories()
{
    Attribute::registerFactory("", SimpleAttributeFactory);
    Attribute::registerFactory("Simple", SimpleAttributeFactory);
    Attribute::registerFactory("Scoped", ScopedAttributeFactory);
    Attribute::registerFactory("NameID", NameIDAttributeFactory);
    Attribute::registerFactory("Extensible", ExtensibleAttributeFactory);
}

map<string,Attribute::AttributeFactory*> Attribute::m_factoryMap;

Attribute* Attribute::unmarshall(DDF& in)
{
    map<string,AttributeFactory*>::const_iterator i = m_factoryMap.find(in.name() ? in.name() : "");
    if (i == m_factoryMap.end())
        throw AttributeException("No registered factory for Attribute of type ($1).", params(1,in.name()));
    return (i->second)(in);
}
