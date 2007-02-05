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

#ifdef WIN32
# define _CRT_NONSTDC_NO_DEPRECATE 1
# define _CRT_SECURE_NO_DEPRECATE 1
#endif

#include <shib-target/shib-target.h>
#include <shibsp/SPConfig.h>
#include <shibsp/util/SPConstants.h>

using namespace shibsp;
using namespace shibtarget;
using namespace opensaml::saml2md;
using namespace saml;
using namespace std;

int main(int argc,char* argv[])
{
    char* h_param=NULL;
    char* q_param=NULL;
    char* f_param=NULL;
    char* a_param=NULL;
    char* path=NULL;
    char* config=NULL;

    for (int i=1; i<argc; i++) {
        if (!strcmp(argv[i],"-c") && i+1<argc)
            config=argv[++i];
        else if (!strcmp(argv[i],"-d") && i+1<argc)
            path=argv[++i];
        else if (!strcmp(argv[i],"-h") && i+1<argc)
            h_param=argv[++i];
        else if (!strcmp(argv[i],"-q") && i+1<argc)
            q_param=argv[++i];
        else if (!strcmp(argv[i],"-f") && i+1<argc)
            f_param=argv[++i];
        else if (!strcmp(argv[i],"-a") && i+1<argc)
            a_param=argv[++i];
    }

    if (!h_param || !q_param) {
        cerr << "usage: shibtest -h <handle> -q <origin_site> [-f <format URI> -a <application_id> -d <schema path> -c <config>]" << endl;
        exit(0);
    }
    
    if (!path)
        path=getenv("SHIBSCHEMAS");
    if (!path)
        path=SHIB_SCHEMAS;
    if (!config)
        config=getenv("SHIBCONFIG");
    if (!config)
        config=SHIB_CONFIG;
    if (!a_param)
        a_param="default";

    ShibTargetConfig& conf=ShibTargetConfig::getConfig();
    SPConfig::getConfig().setFeatures(
        SPConfig::Metadata |
        SPConfig::Trust |
        SPConfig::Credentials |
        SPConfig::AAP |
        SPConfig::OutOfProcess |
        SPConfig::Caching
        );
    if (!conf.init(path) || !conf.load(config))
        return -10;

    ServiceProvider* sp=SPConfig::getConfig().getServiceProvider();
    xmltooling::Locker locker(sp);

    try {
        const IApplication* app=dynamic_cast<const IApplication*>(sp->getApplication(a_param));
        if (!app)
            throw SAMLException("specified <Application> section not found in configuration");

        auto_ptr_XMLCh domain(q_param);
        auto_ptr_XMLCh handle(h_param);
        auto_ptr_XMLCh format(f_param);
        auto_ptr_XMLCh resource(app->getString("providerId").second);

        auto_ptr<SAMLRequest> req(
            new SAMLRequest(
                new SAMLAttributeQuery(
                    new SAMLSubject(
                        new SAMLNameIdentifier(
                            handle.get(),
                            domain.get(),
                            format.get() ? format.get() : shibspconstants::SHIB1_NAMEID_FORMAT_URI
                            )
                        ),
                    resource.get()
                    )
                )
            );

        MetadataProvider* m=app->getMetadataProvider();
        xmltooling::Locker locker(m);
        const EntityDescriptor* site=m->getEntityDescriptor(domain.get());
        if (!site)
            throw MetadataException("Unable to locate specified origin site's metadata.");

        // Try to locate an AA role.
        const AttributeAuthorityDescriptor* AA=site->getAttributeAuthorityDescriptor(saml::XML::SAML11_PROTOCOL_ENUM);
        if (!AA)
            throw MetadataException("Unable to locate metadata for origin site's Attribute Authority.");

        ShibHTTPHook::ShibHTTPHookCallContext ctx(app->getCredentialUse(site),AA);

        SAMLResponse* response=NULL;
        const vector<AttributeService*>& endpoints=AA->getAttributeServices();
        for (vector<AttributeService*>::const_iterator ep=endpoints.begin(); !response && ep!=endpoints.end(); ++ep) {
            try {
                // Get a binding object for this protocol.
                const SAMLBinding* binding = app->getBinding((*ep)->getBinding());
                if (!binding) {
                    continue;
                }
                response=binding->send((*ep)->getLocation(), *(req.get()), &ctx);
            }
            catch (exception&) {
            }
        }

        if (!response)
            throw opensaml::BindingException("unable to successfully query for attributes");

        Iterator<SAMLAssertion*> i=response->getAssertions();
        if (i.hasNext())
        {
            SAMLAssertion* a=i.next();
            cout << "Issuer: "; xmlout(cout,a->getIssuer()); cout << endl;
            const SAMLDateTime* exp=a->getNotOnOrAfter();
            cout << "Expires: ";
            if (exp)
              xmlout(cout,exp->getRawData());
            else
                cout << "None";
            cout << endl;

            Iterator<SAMLStatement*> j=a->getStatements();
            if (j.hasNext())
            {
                SAMLAttributeStatement* s=dynamic_cast<SAMLAttributeStatement*>(j.next());
                if (s)
                {
                    const SAMLNameIdentifier* sub=s->getSubject()->getNameIdentifier();
                    cout << "Format: "; xmlout(cout,sub->getFormat()); cout << endl;
                    cout << "Domain: "; xmlout(cout,sub->getNameQualifier()); cout << endl;
                    cout << "Handle: "; xmlout(cout,sub->getName()); cout << endl;

                    Iterator<SAMLAttribute*> attrs=s->getAttributes();
                    while (attrs.hasNext())
                    {
                        SAMLAttribute* attr=attrs.next();
                        cout << "Attribute Name: "; xmlout(cout,attr->getName()); cout << endl;
                        Iterator<const XMLCh*> vals=attr->getValues();
                        while (vals.hasNext())
                        {
                            cout << "Attribute Value: ";
                            xmlout(cout,vals.next());
                            cout << endl;
                        }
                    }
                }
            }
        }
    }
    catch(exception& e)
    {
        cerr << "caught an exception: " << e.what() << endl;
    }

    conf.shutdown();
    return 0;
}
