/*
 *  Copyright 2001-2006 Internet2
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
 * ServiceProvider.cpp
 * 
 * Interface to a Shibboleth ServiceProvider instance.
 */

#include "internal.h"
#include "ServiceProvider.h"

#include <xercesc/dom/DOM.hpp>

using namespace shibsp;
using namespace xmltooling;
using namespace xercesc;
using namespace std;

namespace shibsp {
    //SHIBSP_DLLLOCAL PluginManager<ServiceProvider,const DOMElement*>::Factory XMLServiceProviderFactory;
};

void SHIBSP_API shibsp::registerServiceProviders()
{
    //SPConfig::getConfig().ServiceProviderManager.registerFactory(XML_SERVICE_PROVIDER, XMLServiceProviderFactory);
}
