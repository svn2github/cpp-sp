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
 * @file shibsp/base.h
 * 
 * Base header file definitions
 * Must be included prior to including any other header
 */

#ifndef __shibsp_base_h__
#define __shibsp_base_h__

#include <saml/base.h>

// Windows and GCC4 Symbol Visibility Macros
#ifdef WIN32
  #define SHIBSP_IMPORT __declspec(dllimport)
  #define SHIBSP_EXPORT __declspec(dllexport)
  #define SHIBSP_DLLLOCAL
  #define SHIBSP_DLLPUBLIC
#else
  #define SHIBSP_IMPORT
  #ifdef GCC_HASCLASSVISIBILITY
    #define SHIBSP_EXPORT __attribute__ ((visibility("default")))
    #define SHIBSP_DLLLOCAL __attribute__ ((visibility("hidden")))
    #define SHIBSP_DLLPUBLIC __attribute__ ((visibility("default")))
  #else
    #define SHIBSP_EXPORT
    #define SHIBSP_DLLLOCAL
    #define SHIBSP_DLLPUBLIC
  #endif
#endif

// Define SHIBSP_API for DLL builds
#ifdef SHIBSP_EXPORTS
  #define SHIBSP_API SHIBSP_EXPORT
#else
  #define SHIBSP_API SHIBSP_IMPORT
#endif

// Throwable classes must always be visible on GCC in all binaries
#ifdef WIN32
  #define SHIBSP_EXCEPTIONAPI(api) api
#elif defined(GCC_HASCLASSVISIBILITY)
  #define SHIBSP_EXCEPTIONAPI(api) SHIBSP_EXPORT
#else
  #define SHIBSP_EXCEPTIONAPI(api)
#endif

#ifdef WIN32

/**
 * Default catalog path on Windows.
 */
# define SHIBSP_SCHEMAS "/opt/shibboleth-sp/share/xml/xmltooling/catalog.xml;/opt/shibboleth-sp/share/xml/opensaml/catalog.xml;/opt/shibboleth-sp/share/xml/shibboleth/catalog.xml"

/**
 * Default path to configuration file on Windows.
 */
# define SHIBSP_CONFIG "/opt/shibboleth-sp/etc/shibboleth/shibboleth.xml"

#else
# include <shibsp/paths.h>
#endif

/**
 * Logging category for Service Provider functions.
 */
#define SHIBSP_LOGCAT "Shibboleth.SP"

/**
 * Logging category for Service Provider auditing.
 */
#define SHIBSP_TX_LOGCAT "Shibboleth-TRANSACTION"

/**
 * Controls default logging level of console tools and other situations
 * where fully-configured logging isn't used.
 */
#define SHIBSP_LOGGING "WARN"

#endif /* __shibsp_base_h__ */