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


#include "../shib/shib.h"
#include <sstream>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

using namespace std;
using namespace saml;
using namespace shibboleth;

SAMLResponse* HS(const char* key)
{
    XMLDateTime now();

    auto_ptr<XMLCh> hsname(XMLString::transcode("wayf.internet2.edu"));
    auto_ptr<XMLCh> recip(XMLString::transcode("https://shire.target.com"));
    auto_ptr<XMLCh> handle(XMLString::transcode("foo"));
    auto_ptr<XMLCh> domain(XMLString::transcode("urn:mace:incommon:pilot:example.edu"));
    auto_ptr<XMLCh> method(XMLString::transcode("urn:mace:shibboleth:authmethod"));

    ShibPOSTProfile* p=ShibPOSTProfileFactory::getInstance(Iterator<const XMLCh*>(),hsname.get());
    return p->prepare(
            recip.get(),
            handle.get(),
            domain.get(),
            (XMLCh*)NULL,
            method.get(),
            time(NULL),
            Iterator<SAMLAuthorityBinding*>(),
            NULL);
}

int main(int argc,char* argv[])
{
    SAMLConfig& conf1=SAMLConfig::getConfig();
    ShibConfig& conf2=ShibConfig::getConfig();
    char* path="";
    char* key="";

    for (int i=1; i<argc; i++)
    {
        if (!strcmp(argv[i],"-d") && i+1<argc)
            path=argv[++i];
        else if (!strcmp(argv[i],"-k") && i+1<argc)
            key=argv[++i];
    }

    conf1.schema_dir=path;
    conf1.binding_defaults.ssl_calist="/opt/shibboleth/etc/shibboleth/ca-bundle.crt";
    if (!conf1.init())
        cerr << "unable to initialize SAML runtime" << endl;

    if (!conf2.init())
        cerr << "unable to initialize Shibboleth runtime" << endl;

    conf2.addMetadata("edu.internet2.middleware.shibboleth.metadata.XML","http://wayf.internet2.edu/incommon/sites.xml");

    try
    {
//        SAMLResponse* r=HS();
//        cout << "Generated Response: " << endl << *r << endl;
        auto_ptr<XMLCh> recip(XMLString::transcode("https://shib2.internet2.edu/shib/SHIRE"));
        ShibPOSTProfile* p=ShibPOSTProfileFactory::getInstance(EMPTY(const XMLCh*),recip.get(),300);

//        auto_ptr<XMLByte> buf(r->toBase64(NULL));
//        delete r;

        char ch;
        string buf;
        cin >> ch;
        while (!cin.fail())
        {
            buf+=ch;
            cin >> ch;
        }

        SAMLResponse* r2=p->accept((const XMLByte*)buf.c_str());
        cout << "Consumed Response: " << endl << *r2 << endl;

        const SAMLAssertion* a=p->getSSOAssertion(*r2);
        const SAMLAuthenticationStatement* s=p->getSSOStatement(*a);
        if (!p->checkReplayCache(*a))
            throw ReplayedAssertionException("detected replay attack");

        delete r2;
    }
    catch(SAMLException& e)
    {
        cerr << "caught a SAML exception: " << e << endl;
    }
    catch(XMLException& e)
    {
        cerr << "caught an XML exception: "; xmlout(cerr,e.getMessage()); cerr << endl;
    }
/*    catch(...)
    {
        cerr << "caught an unknown exception" << endl;
    }*/

    conf2.term();
    conf1.term();
    return 0;
}
