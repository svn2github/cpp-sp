## $Id$

AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = shar test-client

if USE_OUR_ONCRPC
AM_CFLAGS = -I${top_srcdir}/oncrpc
AM_CXXFLAGS = -I${top_srcdir}/oncrpc
ONCRPC_LIBS = ${top_builddir}/oncrpc/liboncrpc.la
endif

test_client_SOURCES = test-client.cpp

shar_SOURCES = shar.cpp shar-utils.cpp
noinst_HEADERS = shar-utils.h

test_client_LDADD = \
  $(top_builddir)/shib-target/libshib-target.la \
  $(top_builddir)/shib/libshib.la

shar_LDADD = \
  $(top_builddir)/shib-target/libshib-target.la \
  $(top_builddir)/shib/libshib.la \
  $(ONCRPC_LIBS)

EXTRA_DIST = shar.dsp shar_win32.cpp testclient.dsp resource.h shar.rc
