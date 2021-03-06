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
 * LogoutHandler.cpp
 * 
 * Base class for logout-related handlers.
 */

#include "internal.h"
#include "exceptions.h"
#include "Application.h"
#include "ServiceProvider.h"
#include "SessionCache.h"
#include "handler/LogoutHandler.h"
#include "util/TemplateParameters.h"

#include <fstream>
#include <log4cpp/Category.hh>
#include <xmltooling/XMLToolingConfig.h>
#include <xmltooling/util/URLEncoder.h>

#ifndef SHIBSP_LITE
#endif

using namespace shibsp;
using namespace xmltooling;
using namespace std;

pair<bool,long> LogoutHandler::run(SPRequest& request, bool isHandler) const
{
    // If we're inside a chain, so do nothing.
    if (getParent())
        return make_pair(false,0);
    
    // If this isn't a LogoutInitiator, we only "continue" a notification loop, rather than starting one.
    if (!m_initiator && !request.getParameter("notifying"))
        return make_pair(false,0);

    // Try another front-channel notification. No extra parameters and the session is implicit.
    pair<bool,long> ret = notifyFrontChannel(request.getApplication(), request, request);
    if (ret.first)
        return ret;

    return make_pair(false,0);
}

void LogoutHandler::receive(DDF& in, ostream& out)
{
    DDF ret(NULL);
    DDFJanitor jout(ret);
    if (in["notify"].integer() != 1)
        throw ListenerException("Unsupported operation.");

    // Find application.
    const char* aid=in["application_id"].string();
    const Application* app=aid ? SPConfig::getConfig().getServiceProvider()->getApplication(aid) : NULL;
    if (!app) {
        // Something's horribly wrong.
        log4cpp::Category::getInstance(SHIBSP_LOGCAT".Logout").error("couldn't find application (%s) for logout", aid ? aid : "(missing)");
        throw ConfigurationException("Unable to locate application for logout, deleted?");
    }

    vector<string> sessions;
    DDF s = in["sessions"];
    DDF temp = s.first();
    while (temp.isstring()) {
        sessions.push_back(temp.string());
        temp = s.next();
        if (notifyBackChannel(*app, sessions))
            ret.integer(1);
    }

    out << ret;
}

pair<bool,long> LogoutHandler::notifyFrontChannel(
    const Application& application,
    const HTTPRequest& request,
    HTTPResponse& response,
    const map<string,string>* params
    ) const
{
    // Index of notification point starts at 0.
    unsigned int index = 0;
    const char* param = request.getParameter("index");
    if (param)
        index = atoi(param);

    // Fetch the next front notification URL and bump the index for the next round trip.
    string loc = application.getNotificationURL(request, true, index++);
    if (loc.empty())
        return make_pair(false,0);

    const URLEncoder* encoder = XMLToolingConfig::getConfig().getURLEncoder();

    // Start with an "action" telling the application what this is about.
    loc = loc + (strchr(loc.c_str(),'?') ? '&' : '?') + "action=logout";

    // Now we create a second URL representing the return location back to us.
    const char* start = request.getRequestURL();
    const char* end = strchr(start,'?');
    string tempstr(start, end ? end-start : strlen(start));
    ostringstream locstr(tempstr);

    // Add a signal that we're coming back from notification and the next index.
    locstr << "?notifying=1&index=" << index;

    // We preserve anything we're instructed to directly.
    if (params) {
        for (map<string,string>::const_iterator p = params->begin(); p!=params->end(); ++p)
            locstr << '&' << p->first << '=' << encoder->encode(p->second.c_str());
    }
    else {
        for (vector<string>::const_iterator q = m_preserve.begin(); q!=m_preserve.end(); ++q) {
            param = request.getParameter(q->c_str());
            if (param)
                locstr << '&' << *q << '=' << encoder->encode(param);
        }
    }

    // Add the return parameter to the destination location and redirect.
    loc = loc + "&return=" + encoder->encode(locstr.str().c_str());
    return make_pair(true,response.sendRedirect(loc.c_str()));
}

bool LogoutHandler::notifyBackChannel(const Application& application, const vector<string>& sessions) const
{
    if (SPConfig::getConfig().isEnabled(SPConfig::OutOfProcess)) {
#ifndef SHIBSP_LITE
        return true;
#else
        return false;
#endif
    }

    // When not out of process, we remote the back channel work.
    DDF out,in(m_address.c_str());
    DDFJanitor jin(in), jout(out);
    in.addmember("notify").integer(1);
    in.addmember("application_id").string(application.getId());
    DDF s = in.addmember("sessions").list();
    for (vector<string>::const_iterator i = sessions.begin(); i!=sessions.end(); ++i) {
        DDF temp = DDF(NULL).string(i->c_str());
        s.add(temp);
    }
    out=application.getServiceProvider().getListenerService()->send(in);
    return (out.integer() == 1);
}

pair<bool,long> LogoutHandler::sendLogoutPage(const Application& application, HTTPResponse& response, bool local, const char* status) const
{
    pair<bool,const char*> prop = application.getString(local ? "localLogout" : "globalLogout");
    if (prop.first) {
        response.setContentType("text/html");
        response.setResponseHeader("Expires","01-Jan-1997 12:00:00 GMT");
        response.setResponseHeader("Cache-Control","private,no-store,no-cache");
        ifstream infile(prop.second);
        if (!infile)
            throw ConfigurationException("Unable to access $1 HTML template.", params(1,local ? "localLogout" : "globalLogout"));
        TemplateParameters tp;
        tp.setPropertySet(application.getPropertySet("Errors"));
        if (status)
            tp.m_map["logoutStatus"] = status;
        stringstream str;
        XMLToolingConfig::getConfig().getTemplateEngine()->run(infile, str, tp);
        return make_pair(true,response.sendResponse(str));
    }
    prop = application.getString("homeURL");
    if (!prop.first)
        prop.second = "/";
    return make_pair(true,response.sendRedirect(prop.second));
}
