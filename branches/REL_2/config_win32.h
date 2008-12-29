/* config.h.  Generated by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define to 1 if you have the declaration of `strerror_r', and to 0 if you
   don't. */
#define HAVE_DECL_STRERROR_R 0

/* Define to 1 if you have the declaration of `svcfd_create', and to 0 if you
   don't. */
#define HAVE_DECL_SVCFD_CREATE 1

/* Define to 1 if you have the declaration of `sys_errlist', and to 0 if you
   don't. */
/* #undef HAVE_DECL_SYS_ERRLIST */

/* Define to 1 if you have the <dlfcn.h> header file. */
/* #undef HAVE_DLFCN_H */

/* Define to 1 if you have the `gmtime_r' function. */
/* #undef HAVE_GMTIME_R */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `crypto' library (-lcrypto). */
#define HAVE_LIBCRYPTO 1

/* Define to 1 if you have the `dmallocxx' library (-ldmallocxx). */
/* #undef HAVE_LIBDMALLOCXX */

/* Define if log4shib library is used. */
#define SHIBSP_LOG4SHIB 1

/* Define if log4cpp library is used. */
/* #undef SHIBSP_LOG4CPP */

/* Define to 1 if you have the `ssl' library (-lssl). */
#define HAVE_LIBSSL 1

/* Define if Xerces-C library was found */
#define HAVE_LIBXERCESC 1

#include <xercesc/util/XercesVersion.hpp>

#if (XERCES_VERSION_MAJOR < 3)
# define SHIBSP_XERCESC_HAS_XMLBYTE_RELEASE 1
# define SHIBSP_XERCESC_SHORT_ACCEPTNODE 1
#endif

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* define if the compiler implements namespaces */
#define HAVE_NAMESPACES 1

/* Define if you have POSIX threads libraries and header files. */
/* #undef HAVE_PTHREAD */

/* Define if saml library was found */
#define HAVE_SAML 1

/* Define to 1 if you have the <stdint.h> header file. */
/* #undef HAVE_STDINT_H */

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
/* #undef HAVE_STRCASECMP */

/* Define to 1 if you have the `strchr' function. */
#define HAVE_STRCHR 1

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the `strftime' function. */
#define HAVE_STRFTIME 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strstr' function. */
#define HAVE_STRSTR 1

/* Define to 1 if you have the `strtok_r' function. */
/* #undef HAVE_STRTOK_R */

/* Define to 1 if the system has the type `struct rpcent'. */
/* #undef HAVE_STRUCT_RPCENT */

/* Define to 1 if you have the `strerror_r' function. */
/* #undef HAVE_STRERROR_R */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the `timegm' function. */
/* #undef HAVE_TIMEGM */

/* Define to 1 if you have the <unistd.h> header file. */
/* #undef HAVE_UNISTD_H */

/* Name of package */
#define PACKAGE "shibboleth"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "shibboleth-users@internet2.edu"

/* Define to the full name of this package. */
#define PACKAGE_NAME "shibboleth"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "shibboleth 2.2"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "shibboleth"

/* Define to the version of this package. */
#define PACKAGE_VERSION "2.2"

/* Define to the necessary symbol if this constant uses a non-standard name on
   your system. */
/* #undef PTHREAD_CREATE_JOINABLE */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if your <sys/time.h> declares `struct tm'. */
/* #undef TM_IN_SYS_TIME */

/* Version number of package */
#define VERSION "2.2"

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `unsigned' if <sys/types.h> does not define. */
/* #undef size_t */
