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
 * mdquery.cpp
 * 
 * SAML Metadata Query tool layered on SP configuration
 */

#if defined (_MSC_VER) || defined(__BORLANDC__)
# include "config_win32.h"
#else
# include "config.h"
#endif

#ifdef WIN32
# define _CRT_NONSTDC_NO_DEPRECATE 1
# define _CRT_SECURE_NO_DEPRECATE 1
#endif

#include <shibsp/Application.h>
#include <shibsp/exceptions.h>
#include <shibsp/SPConfig.h>
#include <shibsp/ServiceProvider.h>
#include <shibsp/util/SPConstants.h>
#include <saml/saml2/metadata/Metadata.h>
#include <xmltooling/logging.h>

using namespace shibsp;
using namespace opensaml::saml2md;
using namespace opensaml;
using namespace xmltooling::logging;
using namespace xmltooling;
using namespace std;

int main(int argc,char* argv[])
{
    char* entityID = NULL;
    char* appID = "default";
    bool strict = true;

    for (int i=1; i<argc; i++) {
        if (!strcmp(argv[i],"-e") && i+1<argc)
            entityID=argv[++i];
        else if (!strcmp(argv[i],"-a") && i+1<argc)
            appID=argv[++i];
        else if (!strcmp(argv[i],"--nostrict"))
            strict = false;
    }

    if (!entityID) {
        cerr << "usage: mdquery -e <entityID> [-a <application id> --nostrict]" << endl;
        exit(0);
    }

    char* path=getenv("SHIBSP_SCHEMAS");
    if (!path)
        path=SHIBSP_SCHEMAS;
    char* config=getenv("SHIBSP_CONFIG");
    if (!config)
        config=SHIBSP_CONFIG;

    XMLToolingConfig::getConfig().log_config(getenv("SHIBSP_LOGGING") ? getenv("SHIBSP_LOGGING") : SHIBSP_LOGGING);

    SPConfig& conf=SPConfig::getConfig();
    conf.setFeatures(SPConfig::Metadata | SPConfig::OutOfProcess);
    if (!conf.init(path))
        return -1;

    try {
        static const XMLCh _path[] = UNICODE_LITERAL_4(p,a,t,h);
        static const XMLCh validate[] = UNICODE_LITERAL_8(v,a,l,i,d,a,t,e);
        xercesc::DOMDocument* dummydoc=XMLToolingConfig::getConfig().getParser().newDocument();
        XercesJanitor<xercesc::DOMDocument> docjanitor(dummydoc);
        xercesc::DOMElement* dummy = dummydoc->createElementNS(NULL,_path);
        auto_ptr_XMLCh src(config);
        dummy->setAttributeNS(NULL,_path,src.get());
        dummy->setAttributeNS(NULL,validate,xmlconstants::XML_ONE);
        conf.setServiceProvider(conf.ServiceProviderManager.newPlugin(XML_SERVICE_PROVIDER,dummy));
        conf.getServiceProvider()->init();
    }
    catch (exception&) {
        conf.term();
        return -2;
    }

    ServiceProvider* sp=conf.getServiceProvider();
    sp->lock();

    Category& log = Category::getInstance(SHIBSP_LOGCAT".Utility.MDQuery");

    const Application* app = sp->getApplication(appID);
    if (!app) {
        log.error("unknown application ID (%s)", appID);
        sp->unlock();
        conf.term();
        return -3;
    }

    app->getMetadataProvider()->lock();
    const EntityDescriptor* entity = app->getMetadataProvider()->getEntityDescriptor(entityID, strict);
    if (entity) {
        XMLHelper::serialize(entity->marshall(), cout, true);
    }
    else {
        log.error("no metadata found for (%s)", entityID);
    }

    app->getMetadataProvider()->unlock();

    sp->unlock();
    conf.term();
    return 0;
}
