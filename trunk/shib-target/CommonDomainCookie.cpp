/*
 *  Copyright 2001-2005 Internet2
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

/*
 * CommonDomainCookie.cpp -- SAML 2.0 discovery cookie implementation
 *
 * Scott Cantor
 * 5/17/2005
 */

#include "internal.h"
#include <xercesc/util/Base64.hpp>

using namespace std;
using namespace shibtarget;
using namespace log4cpp;

const char CommonDomainCookie::CDCName[] = "_saml_idp";

CommonDomainCookie::CommonDomainCookie(const char* cookie)
{
    if (!cookie)
        return;

    Category& log=Category::getInstance(SHIBT_LOGCAT".CommonDomainCookie");

    // Copy it so we can URL-decode it.
    char* b64=strdup(cookie);
    ShibTarget::url_decode(b64);

    // Chop it up and save off elements.
    vector<string> templist;
    char* ptr=b64;
    while (*ptr) {
        while (*ptr && isspace(*ptr)) ptr++;
        char* end=ptr;
        while (*end && !isspace(*end)) end++;
        templist.push_back(string(ptr,end-ptr));
        ptr=end;
    }
    free(b64);

    // Now Base64 decode the list.
    for (vector<string>::iterator i=templist.begin(); i!=templist.end(); i++) {
        unsigned int len;
        XMLByte* decoded=Base64::decode(reinterpret_cast<const XMLByte*>(i->c_str()),&len);
        if (decoded && *decoded) {
            m_list.push_back(reinterpret_cast<char*>(decoded));
            XMLString::release(&decoded);
        }
        else
            log.warn("cookie element does not appear to be base64-encoded");
    }
}

const char* CommonDomainCookie::set(const char* providerId)
{
    // First scan the list for this IdP.
    for (vector<string>::iterator i=m_list.begin(); i!=m_list.end(); i++) {
        if (*i == providerId) {
            m_list.erase(i);
            break;
        }
    }
    
    // Append it to the end.
    m_list.push_back(providerId);
    
    // Now rebuild the delimited list.
    string delimited;
    for (vector<string>::const_iterator j=m_list.begin(); j!=m_list.end(); j++) {
        if (!delimited.empty()) delimited += ' ';
        
        unsigned int len;
        XMLByte* b64=Base64::encode(reinterpret_cast<const XMLByte*>(j->c_str()),j->length(),&len);
        XMLByte *pos, *pos2;
        for (pos=b64, pos2=b64; *pos2; pos2++)
            if (isgraph(*pos2))
                *pos++=*pos2;
        *pos=0;
        
        delimited += reinterpret_cast<char*>(b64);
        XMLString::release(&b64);
    }
    
    m_encoded=ShibTarget::url_encode(delimited.c_str());
    return m_encoded.c_str();
}