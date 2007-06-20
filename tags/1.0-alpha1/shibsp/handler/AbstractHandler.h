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
 * @file shibsp/handler/AbstractHandler.h
 * 
 * Base class for handlers based on a DOMPropertySet. 
 */

#ifndef __shibsp_abshandler_h__
#define __shibsp_abshandler_h__

#include <shibsp/handler/Handler.h>
#include <shibsp/util/DOMPropertySet.h>

#include <log4cpp/Category.hh>
#ifndef SHIBSP_LITE
# include <saml/saml2/core/Protocols.h>
#endif
#include <xmltooling/XMLObject.h>
#include <xmltooling/io/HTTPRequest.h>
#include <xmltooling/io/HTTPResponse.h>

namespace shibsp {

    class SHIBSP_API Application;
    class SHIBSP_API SPRequest;

#if defined (_MSC_VER)
    #pragma warning( push )
    #pragma warning( disable : 4250 )
#endif

    /**
     * Base class for handlers based on a DOMPropertySet.
     */
    class SHIBSP_API AbstractHandler : public virtual Handler, public DOMPropertySet
    {
    protected:
        /**
         * Constructor
         * 
         * @param e         DOM element to load as property set. 
         * @param filter    optional filter controls what child elements to include as nested PropertySets
         * @param remapper  optional map of property rename rules for legacy property support
         */
        AbstractHandler(
            const xercesc::DOMElement* e,
            log4cpp::Category& log,
            xercesc::DOMNodeFilter* filter=NULL,
            const std::map<std::string,std::string>* remapper=NULL
            );

#ifndef SHIBSP_LITE
        /**
         * Examines a protocol response message for errors and raises an annotated exception
         * if an error is found.
         * 
         * <p>The base class version understands SAML 1.x and SAML 2.0 responses.
         * 
         * @param response      a response message of some known protocol
         */
        virtual void checkError(const xmltooling::XMLObject* response) const;

        /**
         * Prepares Status information in a SAML 2.0 response.
         * 
         * @param response  a SAML 2.0 response message
         * @param code      SAML status code
         * @param ex        optional message to pass back
         */
        void prepareResponse(
            opensaml::saml2p::StatusResponseType& response, const XMLCh* code, const XMLCh* subcode=NULL, const char* msg=NULL
            ) const;
#endif

        /**
         * Implements various mechanisms to preserve RelayState,
         * such as cookies or StorageService-backed keys.
         * 
         * <p>If a supported mechanism can be identified, the input parameter will be
         * replaced with a suitable state key.
         * 
         * @param application   the associated Application
         * @param response      outgoing HTTP response
         * @param relayState    RelayState token to supply with message
         */
        virtual void preserveRelayState(const Application& application, xmltooling::HTTPResponse& response, std::string& relayState) const;

        /**
         * Implements various mechanisms to recover RelayState,
         * such as cookies or StorageService-backed keys.
         * 
         * <p>If a supported mechanism can be identified, the input parameter will be
         * replaced with the recovered state information.
         * 
         * @param application   the associated Application
         * @param request       incoming HTTP request
         * @param relayState    RelayState token supplied with message
         * @param clear         true iff the token state should be cleared
         */
        virtual void recoverRelayState(
            const Application& application, xmltooling::HTTPRequest& request, std::string& relayState, bool clear=true
            ) const;
        
        /** Logging object. */
        log4cpp::Category& m_log;
        
        /** Configuration namespace for custom properties. */
        xmltooling::auto_ptr_char m_configNS;

    public:
        virtual ~AbstractHandler() {}
    };

#if defined (_MSC_VER)
    #pragma warning( pop )
#endif

};

#endif /* __shibsp_abshandler_h__ */
