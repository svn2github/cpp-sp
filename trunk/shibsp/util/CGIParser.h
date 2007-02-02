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
 * @file shibsp/util/CGIParser.h
 * 
 * CGI GET/POST parameter parsing
 */

#ifndef __shibsp_cgi_h__
#define __shibsp_cgi_h__

#include <shibsp/SPRequest.h>

namespace shibsp {

    /**
     * CGI GET/POST parameter parsing
     */
    class SHIBSP_API CGIParser
    {
        MAKE_NONCOPYABLE(CGIParser);
    public:
        /**
         * Constructor
         * 
         * @param request   SP request
         */
        CGIParser(const SPRequest& request);

        ~CGIParser();

        typedef std::multimap<std::string,char*>::const_iterator walker;
        
        /**
         * Returns a pair of bounded iterators around the values of a parameter.
         * 
         * @param name  name of parameter
         * @return  a pair of multimap iterators surrounding the matching value(s)
         */
        std::pair<walker,walker> getParameters(const char* name) const;
        
    private:
        char* fmakeword(char stop, unsigned int *cl, const char** ppch);
        char* makeword(char *line, char stop);
        void plustospace(char *str);

        std::multimap<std::string,char*> kvp_map;
    };
};

#endif /* __shibsp_cgi_h__ */
