## $Id$

AUTOMAKE_OPTIONS = foreign

pkgsysconfdir = $(sysconfdir)/@PACKAGE@

lib_LTLIBRARIES = libshib-target.la

if USE_OUR_ONCRPC
libshib_target_la_LIBADD = ${top_builddir}/oncrpc/liboncrpc.la
AM_CPPFLAGS = -I${top_srcdir}/oncrpc
endif

libshib_targetdir = $(includedir)/shib-target
libshib_target_HEADERS = shib-target.h shibrpc.h shib-paths.h
noinst_HEADERS = internal.h

libshib_target_la_SOURCES = \
	shib-ccache.cpp \
	shib-config.cpp \
	shib-ini.cpp \
	shib-mlp.cpp \
	shib-rm.cpp \
	shib-rpcerror.cpp \
	shib-rpchandle.cpp \
	shib-shire.cpp \
	shib-sock.cpp \
	shibrpc-clnt.c \
	shibrpc-server.cpp \
	shibrpc-svc.c \
	shibrpc-xdr.c \
	XML.cpp \
	XMLRequestMapper.cpp

# this is different from the project version
# http://sources.redhat.com/autobook/autobook/autobook_91.html
libshib_target_la_LDFLAGS = -version-info 4:1:0



RPCGEN=rpcgen -M
RPCGEN_SRC=shibrpc.x

very-clean: 
	$(RM) shibrpc.h shibrpc-xdr.c shibrpc-clnt.c shibrpc-svc.c

rpcgen:
	$(MAKE) do_rpcgen RPCGEN_TARGET=shibrpc.h RPCGEN_ARGS=-h
	$(MAKE) do_rpcgen RPCGEN_TARGET=shibrpc-xdr.c RPCGEN_ARGS=-c
	$(MAKE) do_rpcgen RPCGEN_TARGET=shibrpc-clnt.c RPCGEN_ARGS=-l
	$(MAKE) do_rpcgen RPCGEN_TARGET=shibrpc-svc.c RPCGEN_ARGS=-m
	$(MAKE) do_rpcgen RPCGEN_TARGET=shibrpc-server-stubs.c RPCGEN_ARGS=-Ss

do_rpcgen:
	$(RM) $(RPCGEN_TARGET)
	$(RPCGEN) $(RPCGEN_ARGS) -o $(RPCGEN_TARGET) $(RPCGEN_SRC)

shib-paths.h: ${srcdir}/shib-paths.h.in Makefile ${top_builddir}/config.status
	rm -f $@.tmp
	sed < ${srcdir}/$@.in > $@.tmp \
	    -e 's:@-PKGSYSCONFDIR-@:${pkgsysconfdir}:'
	cmp -s $@ $@.tmp || mv $@.tmp $@
	rm -f $@.tmp

install-exec-hook:
	for la in $(lib_LTLIBRARIES) ; do rm -f $(DESTDIR)$(libdir)/$$la ; done

EXTRA_DIST = shibtarget.dsp shib-paths.h.in resource.h shib-target.rc
BUILT_SOURCES = shib-paths.h
