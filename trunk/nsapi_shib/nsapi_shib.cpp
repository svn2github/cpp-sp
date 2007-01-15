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

/**
 * nsapi_shib.cpp
 * 
 * Shibboleth NSAPI filter
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

#include <shibsp/RequestMapper.h>
#include <shibsp/SPConfig.h>
#include <xmltooling/util/NDC.h>
#include <xmltooling/util/Threads.h>

// SAML Runtime
#include <saml/saml.h>
#include <shib/shib.h>
#include <shib-target/shib-target.h>

#include <fstream>
#include <sstream>

#ifdef WIN32
# include <process.h>
# define XP_WIN32
#else
# define XP_UNIX
#endif

#define MCC_HTTPD
#define NET_SSL

extern "C"
{
#include <nsapi.h>
}

using namespace shibsp;
using namespace shibtarget;
using namespace xmltooling;
using namespace std;

// macros to output text to client
#define NET_WRITE(str) \
    if (IO_ERROR==net_write(sn->csd,str,strlen(str))) return REQ_EXIT

namespace {
    ShibTargetConfig* g_Config=NULL;
    string g_ServerName;
    string g_ServerScheme;
    string g_unsetHeaderValue;
}

PluginManager<RequestMapper,const DOMElement*>::Factory SunRequestMapFactory;

extern "C" NSAPI_PUBLIC void nsapi_shib_exit(void*)
{
    if (g_Config)
        g_Config->shutdown();
    g_Config = NULL;
}

extern "C" NSAPI_PUBLIC int nsapi_shib_init(pblock* pb, ::Session* sn, Request* rq)
{
    // Save off a default hostname for this virtual server.
    char* name=pblock_findval("server-name",pb);
    if (name)
        g_ServerName=name;
    else {
        name=server_hostname;
        if (name)
            g_ServerName=name;
        else {
            name=util_hostname();
            if (name) {
                g_ServerName=name;
                FREE(name);
            }
            else {
                pblock_nvinsert("error","unable to determine web server hostname",pb);
                return REQ_ABORTED;
            }
        }
    }
    name=pblock_findval("server-scheme",pb);
    if (name)
        g_ServerScheme=name;

    log_error(LOG_INFORM,"nsapi_shib_init",sn,rq,"nsapi_shib loaded for host (%s)",g_ServerName.c_str());

#ifndef _DEBUG
    try {
#endif
        const char* schemadir=pblock_findval("shib-schemas",pb);
        if (!schemadir)
            schemadir=getenv("SHIBSCHEMAS");
        if (!schemadir)
            schemadir=SHIB_SCHEMAS;
        const char* config=pblock_findval("shib-config",pb);
        if (!config)
            config=getenv("SHIBCONFIG");
        if (!config)
            config=SHIB_CONFIG;
        g_Config=&ShibTargetConfig::getConfig();
        SPConfig::getConfig().setFeatures(
            SPConfig::Listener |
            SPConfig::Caching |
            SPConfig::Metadata |
            SPConfig::AAP |
            SPConfig::RequestMapping |
            SPConfig::InProcess |
            SPConfig::Logging
            );
        if (!g_Config->init(schemadir)) {
            g_Config=NULL;
            pblock_nvinsert("error","unable to initialize Shibboleth libraries",pb);
            return REQ_ABORTED;
        }

        SPConfig::getConfig().RequestMapperManager.registerFactory(XML_REQUEST_MAPPER,&SunRequestMapFactory);

        if (!g_Config->load(config)) {
            g_Config=NULL;
            pblock_nvinsert("error","unable to initialize load Shibboleth configuration",pb);
            return REQ_ABORTED;
        }

        daemon_atrestart(nsapi_shib_exit,NULL);

        ServiceProvider* conf=SPConfig::getConfig().getServiceProvider();
        Locker locker(conf);
        const PropertySet* props=conf->getPropertySet("Local");
        if (props) {
            pair<bool,const char*> unsetValue=props->getString("unsetHeaderValue");
            if (unsetValue.first)
                g_unsetHeaderValue = unsetValue.second;
        }
#ifndef _DEBUG
    }
    catch (...) {
        g_Config=NULL;
        pblock_nvinsert("error","caught exception, unable to initialize Shibboleth libraries",pb);
        return REQ_ABORTED;
    }
#endif
    return REQ_PROCEED;
}

/********************************************************************************/
// NSAPI Shib Target Subclass

class ShibTargetNSAPI : public ShibTarget
{
  string m_uri;
  mutable string m_body;
  mutable bool m_gotBody;
  vector<XSECCryptoX509*> m_certs;

public:
  ShibTargetNSAPI(pblock* pb, ::Session* sn, Request* rq) : m_gotBody(false) {
    m_pb = pb;
    m_sn = sn;
    m_rq = rq;

    // Get everything but hostname...
    const char* uri=pblock_findval("uri", rq->reqpb);
    const char* qstr=pblock_findval("query", rq->reqpb);

    string url;
    if (uri) {
        url = uri;
        m_uri = uri;
    }
    if (qstr)
        url=url + '?' + qstr;
    
    const char* host=NULL;
#ifdef vs_is_default_vs
    // This is 6.0 or later, so we can distinguish requests to name-based vhosts.
    if (!vs_is_default_vs)
        // The beauty here is, a non-default vhost can *only* be accessed if the client
        // specified the exact name in the Host header. So we can trust the Host header.
        host=pblock_findval("host", rq->headers);
    else
#endif
    // In other cases, we're going to rely on the initialization process...
    host=g_ServerName.c_str();
  }
  ~ShibTargetNSAPI() {}

  const char* getScheme() const {
    return security_active ? "https" : "http";
  }
  const char* getHostname() const {
#ifdef vs_is_default_vs
    // This is 6.0 or later, so we can distinguish requests to name-based vhosts.
    if (!vs_is_default_vs)
        // The beauty here is, a non-default vhost can *only* be accessed if the client
        // specified the exact name in the Host header. So we can trust the Host header.
        return pblock_findval("host", m_rq->headers);
    else
#endif
    // In other cases, we're going to rely on the initialization process...
    return g_ServerName.c_str();
  }
  int getPort() const {
    return server_portnum;
  }
  const char* getRequestURI() const {
    return m_uri.c_str();
  }
  const char* getMethod() const {
    return pblock_findval("method", m_rq->reqpb);
  }
  string getContentType() const {
    char* content_type = "";
    request_header("content-type", &content_type, m_sn, m_rq);
    return content_type;
  }
  long getContentLength() const {
    if (m_gotBody)
        return m_body.length();
    char* content_length="";
    request_header("content-length", &content_length, m_sn, m_rq);
    return atoi(content_length);
  }
  string getRemoteAddr() const {
    return pblock_findval("ip", m_sn->client);
  }
  void log(SPLogLevel level, const string& msg) {
    AbstractSPRequest::log(level,msg);
    if (level>=SPError)
        log_error(LOG_FAILURE, "nsapi_shib", m_sn, m_rq, const_cast<char*>(msg.c_str()));
  }
  const char* getQueryString() const { 
    return pblock_findval("query", m_rq->reqpb);
  }
  const char* getRequestBody() const {
    if (m_gotBody)
        return m_body.c_str();
    char* content_length=NULL;
    if (request_header("content-length", &content_length, m_sn, m_rq)!=REQ_PROCEED ||
         atoi(content_length) > 1024*1024) // 1MB?
      throw opensaml::BindingException("Blocked POST request body exceeding size limit.");
    else {
      char ch=IO_EOF+1;
      int cl=atoi(content_length);
      m_gotBody=true;
      while (cl && ch != IO_EOF) {
        ch=netbuf_getc(m_sn->inbuf);
        // Check for error.
        if(ch==IO_ERROR)
          break;
        m_body += ch;
        cl--;
      }
      if (cl)
        throw opensaml::BindingException("Error reading POST request body from browser.");
      return m_body.c_str();
    }
  }
  void clearHeader(const char* name) {
    if (!strcmp(name,"REMOTE_USER")) {
        param_free(pblock_remove("auth-user",m_rq->vars));
        param_free(pblock_remove("remote-user",m_rq->headers));
    }
    else {
        param_free(pblock_remove(name, m_rq->headers));
        pblock_nvinsert(name, g_unsetHeaderValue.c_str() ,m_rq->headers);
    }
  }
  void setHeader(const char* name, const char* value) {
    pblock_nvinsert(name, value, m_rq->headers);
  }
  string getHeader(const char* name) const {
    char* hdr = NULL;
    if (request_header(const_cast<char*>(name), &hdr, m_sn, m_rq) != REQ_PROCEED)
      hdr = NULL;
    return string(hdr ? hdr : "");
  }
  void setRemoteUser(const char* user) {
    pblock_nvinsert("remote-user", user, m_rq->headers);
    pblock_nvinsert("auth-user", user, m_rq->vars);
  }
  string getRemoteUser() const {
    return getHeader("remote-user");
  }
  void setResponseHeader(const char* name, const char* value) {
    pblock_nvinsert(name, value, m_rq->srvhdrs);
  }

  long sendResponse(istream& in, long status) {
    string msg;
    char buf[1024];
    while (in) {
        in.read(buf,1024);
        msg.append(buf,in.gcount());
    }
    pblock_nvinsert("connection","close",m_rq->srvhdrs);
    pblock_nninsert("content-length", msg.length(), m_rq->srvhdrs);
    protocol_status(m_sn, m_rq, status, NULL);
    protocol_start_response(m_sn, m_rq);
    net_write(m_sn->csd,const_cast<char*>(msg.c_str()),msg.length());
    return REQ_EXIT;
  }
  long sendRedirect(const char* url) {
    param_free(pblock_remove("content-type", m_rq->srvhdrs));
    pblock_nninsert("content-length", 0, m_rq->srvhdrs);
    pblock_nvinsert("expires", "01-Jan-1997 12:00:00 GMT", m_rq->srvhdrs);
    pblock_nvinsert("cache-control", "private,no-store,no-cache", m_rq->srvhdrs);
    pblock_nvinsert("location", url, m_rq->srvhdrs);
    pblock_nvinsert("connection","close",m_rq->srvhdrs);
    protocol_status(m_sn, m_rq, PROTOCOL_REDIRECT, NULL);
    protocol_start_response(m_sn, m_rq);
    return REQ_ABORTED;
  }
  long returnDecline() { return REQ_NOACTION; }
  long returnOK() { return REQ_PROCEED; }
  const vector<XSECCryptoX509*>& getClientCertificates() const {
      return m_certs;
  }

  pblock* m_pb;
  ::Session* m_sn;
  Request* m_rq;
};

/********************************************************************************/

int WriteClientError(::Session* sn, Request* rq, char* func, char* msg)
{
    log_error(LOG_FAILURE,func,sn,rq,msg);
    protocol_status(sn,rq,PROTOCOL_SERVER_ERROR,msg);
    return REQ_ABORTED;
}

#undef FUNC
#define FUNC "shibboleth"
extern "C" NSAPI_PUBLIC int nsapi_shib(pblock* pb, ::Session* sn, Request* rq)
{
  ostringstream threadid;
  threadid << "[" << getpid() << "] nsapi_shib" << '\0';
  xmltooling::NDC ndc(threadid.str().c_str());

  try {
    ShibTargetNSAPI stn(pb, sn, rq);

    // Check user authentication
    pair<bool,long> res = stn.doCheckAuthN();
    if (res.first) return (int)res.second;

    // user authN was okay -- export the assertions now
    param_free(pblock_remove("auth-user",rq->vars));
    // This seems to be required in order to eventually set
    // the auth-user var.
    pblock_nvinsert("auth-type","shibboleth",rq->vars);
    res = stn.doExportAssertions();
    if (res.first) return (int)res.second;

    // Check the Authorization
    res = stn.doCheckAuthZ();
    if (res.first) return (int)res.second;

    // this user is ok.
    return REQ_PROCEED;
  }
  catch (exception& e) {
    log_error(LOG_FAILURE,FUNC,sn,rq,const_cast<char*>(e.what()));
    return WriteClientError(sn, rq, FUNC, "Shibboleth module threw an exception, see web server log for error.");
  }
#ifndef _DEBUG
  catch (...) {
    return WriteClientError(sn, rq, FUNC, "Shibboleth module threw an uncaught exception.");
  }
#endif
}


#undef FUNC
#define FUNC "shib_handler"
extern "C" NSAPI_PUBLIC int shib_handler(pblock* pb, ::Session* sn, Request* rq)
{
  ostringstream threadid;
  threadid << "[" << getpid() << "] shib_handler" << '\0';
  xmltooling::NDC ndc(threadid.str().c_str());

  try {
    ShibTargetNSAPI stn(pb, sn, rq);

    pair<bool,long> res = stn.doHandler();
    if (res.first) return (int)res.second;

    return WriteClientError(sn, rq, FUNC, "Shibboleth handler did not do anything.");
  }
  catch (exception& e) {
    log_error(LOG_FAILURE,FUNC,sn,rq,const_cast<char*>(e.what()));
    return WriteClientError(sn, rq, FUNC, "Shibboleth handler threw an exception, see web server log for error.");
  }
#ifndef _DEBUG
  catch (...) {
    return WriteClientError(sn, rq, FUNC, "Shibboleth handler threw an unknown exception.");
  }
#endif
}


class SunRequestMapper : public virtual RequestMapper, public virtual PropertySet
{
public:
    SunRequestMapper(const DOMElement* e);
    ~SunRequestMapper() { delete m_mapper; delete m_stKey; delete m_propsKey; }
    Lockable* lock() { return m_mapper->lock(); }
    void unlock() { m_stKey->setData(NULL); m_propsKey->setData(NULL); m_mapper->unlock(); }
    Settings getSettings(const SPRequest& request) const;
    
    pair<bool,bool> getBool(const char* name, const char* ns=NULL) const;
    pair<bool,const char*> getString(const char* name, const char* ns=NULL) const;
    pair<bool,const XMLCh*> getXMLString(const char* name, const char* ns=NULL) const;
    pair<bool,unsigned int> getUnsignedInt(const char* name, const char* ns=NULL) const;
    pair<bool,int> getInt(const char* name, const char* ns=NULL) const;
    const PropertySet* getPropertySet(const char* name, const char* ns="urn:mace:shibboleth:target:config:1.0") const;
    const DOMElement* getElement() const;

private:
    RequestMapper* m_mapper;
    ThreadKey* m_stKey;
    ThreadKey* m_propsKey;
};

RequestMapper* SunRequestMapFactory(const DOMElement* const & e)
{
    return new SunRequestMapper(e);
}

SunRequestMapper::SunRequestMapper(const DOMElement* e) : m_mapper(NULL), m_stKey(NULL), m_propsKey(NULL)
{
    m_mapper = SPConfig::getConfig().RequestMapperManager.newPlugin(XML_REQUEST_MAPPER,e);
    m_stKey=ThreadKey::create(NULL);
    m_propsKey=ThreadKey::create(NULL);
}

RequestMapper::Settings SunRequestMapper::getSettings(const SPRequest& request) const
{
    Settings s=m_mapper->getSettings(request);
    m_stKey->setData((void*)dynamic_cast<const ShibTargetNSAPI*>(&request));
    m_propsKey->setData((void*)s.first);
    return pair<const PropertySet*,AccessControl*>(this,s.second);
}

pair<bool,bool> SunRequestMapper::getBool(const char* name, const char* ns) const
{
    const ShibTargetNSAPI* stn=reinterpret_cast<const ShibTargetNSAPI*>(m_stKey->getData());
    const PropertySet* s=reinterpret_cast<const PropertySet*>(m_propsKey->getData());
    if (stn && !ns && name) {
        // Override boolean properties.
        const char* param=pblock_findval(name,stn->m_pb);
        if (param && (!strcmp(param,"1") || !strcasecmp(param,"true")))
            return make_pair(true,true);
    }
    return s ? s->getBool(name,ns) : make_pair(false,false);
}

pair<bool,const char*> SunRequestMapper::getString(const char* name, const char* ns) const
{
    const ShibTargetNSAPI* stn=reinterpret_cast<const ShibTargetNSAPI*>(m_stKey->getData());
    const PropertySet* s=reinterpret_cast<const PropertySet*>(m_propsKey->getData());
    if (stn && !ns && name) {
        // Override string properties.
        if (!strcmp(name,"authType"))
            return pair<bool,const char*>(true,"shibboleth");
        else {
            const char* param=pblock_findval(name,stn->m_pb);
            if (param)
                return make_pair(true,param);
        }
    }
    return s ? s->getString(name,ns) : pair<bool,const char*>(false,NULL);
}

pair<bool,const XMLCh*> SunRequestMapper::getXMLString(const char* name, const char* ns) const
{
    const PropertySet* s=reinterpret_cast<const PropertySet*>(m_propsKey->getData());
    return s ? s->getXMLString(name,ns) : pair<bool,const XMLCh*>(false,NULL);
}

pair<bool,unsigned int> SunRequestMapper::getUnsignedInt(const char* name, const char* ns) const
{
    const ShibTargetNSAPI* stn=reinterpret_cast<const ShibTargetNSAPI*>(m_stKey->getData());
    const PropertySet* s=reinterpret_cast<const PropertySet*>(m_propsKey->getData());
    if (stn && !ns && name) {
        // Override int properties.
        const char* param=pblock_findval(name,stn->m_pb);
        if (param)
            return pair<bool,unsigned int>(true,strtol(param,NULL,10));
    }
    return s ? s->getUnsignedInt(name,ns) : pair<bool,unsigned int>(false,0);
}

pair<bool,int> SunRequestMapper::getInt(const char* name, const char* ns) const
{
    const ShibTargetNSAPI* stn=reinterpret_cast<const ShibTargetNSAPI*>(m_stKey->getData());
    const PropertySet* s=reinterpret_cast<const PropertySet*>(m_propsKey->getData());
    if (stn && !ns && name) {
        // Override int properties.
        const char* param=pblock_findval(name,stn->m_pb);
        if (param)
            return pair<bool,int>(true,atoi(param));
    }
    return s ? s->getInt(name,ns) : pair<bool,int>(false,0);
}

const PropertySet* SunRequestMapper::getPropertySet(const char* name, const char* ns) const
{
    const PropertySet* s=reinterpret_cast<const PropertySet*>(m_propsKey->getData());
    return s ? s->getPropertySet(name,ns) : NULL;
}

const DOMElement* SunRequestMapper::getElement() const
{
    const PropertySet* s=reinterpret_cast<const PropertySet*>(m_propsKey->getData());
    return s ? s->getElement() : NULL;
}
