## $Id$ 

AUTOMAKE_OPTIONS = foreign

pkglibdir = ${libdir}/@PACKAGE@
pkglogdir = ${localstatedir}/log/@PACKAGE@
pkgdocdir = ${datadir}/doc/@PACKAGE@
shirelogdir = ${localstatedir}/log/httpd
pkgxmldir = $(datadir)/xml/@PACKAGE@
pkgrundir = $(localstatedir)/run/@PACKAGE@
pkgsysconfdir = $(sysconfdir)/@PACKAGE@
pkgsysconf_DATA = \
	shibd-redhat \
	shibd-debian \
	shibd-osx.plist \
	apache.config \
	apache2.config \
	apache22.config

# The config files are installed "special".  Unlike the entries in
# pkgsysconf_DATA, these files are installed as "foo.dist" and only
# installed as "foo" if "foo" does not already exist.  In other words,
# the config files will not overwrite existing versions.
BUILTCONFIGFILES = \
	shibboleth2.xml \
	native.logger \
	shibd.logger \
	attribute-map.xml \
	attribute-policy.xml

# While BUILTCONFIGFILES are processed, these are not; so we should pull
# them from SRCDIR.
CONFIGFILES = \
    example-metadata.xml \
	console.logger \
	syslog.logger \
	accessError.html \
	sessionError.html \
	metadataError.html \
	bindingTemplate.html \
	localLogout.html \
	globalLogout.html \
	sslError.html \
	sp-example.key \
	sp-example.crt

#
# Some of these need building
#
do-build-file:
	rm -f ${FILE}.tmp
	sed < ${srcdir}/${FILE}.in > ${FILE}.tmp \
        -e 's:@-PREFIX-@:${prefix}:g' \
        -e 's:@-PKGLIBDIR-@:${pkglibdir}:g' \
        -e 's:@-PKGSYSCONFDIR-@:${pkgsysconfdir}:g' \
        -e 's:@-PKGDOCDIR-@:${pkgdocdir}:g' \
        -e 's:@-PKGXMLDIR-@:${pkgxmldir}:g' \
        -e 's:@-XMLTOOLINGXMLDIR-@:${XMLTOOLINGXMLDIR}:g' \
        -e 's:@-OPENSAMLXMLDIR-@:${OPENSAMLXMLDIR}:g' \
        -e 's:@-PKGLOGDIR-@:${pkglogdir}:g' \
        -e 's:@-SHIRELOGDIR-@:${shirelogdir}:g' \
        -e 's:@-PKGRUNDIR-@:${pkgrundir}:g'
	cmp -s ${FILE} ${FILE}.tmp || mv ${FILE}.tmp ${FILE}
	rm -f ${FILE}.tmp

apache.config: ${srcdir}/apache.config.in Makefile ${top_builddir}/config.status
	$(MAKE) do-build-file FILE=$@

apache2.config: ${srcdir}/apache2.config.in Makefile ${top_builddir}/config.status
	$(MAKE) do-build-file FILE=$@

apache22.config: ${srcdir}/apache22.config.in Makefile ${top_builddir}/config.status
	$(MAKE) do-build-file FILE=$@

shibd-redhat: ${srcdir}/shibd-redhat.in Makefile ${top_builddir}/config.status
	$(MAKE) do-build-file FILE=$@

shibd-debian: ${srcdir}/shibd-debian.in Makefile ${top_builddir}/config.status
	$(MAKE) do-build-file FILE=$@

shibd-osx.plist: ${srcdir}/shibd-osx.plist.in Makefile ${top_builddir}/config.status
	$(MAKE) do-build-file FILE=$@

shibd.logger: ${srcdir}/shibd.logger.in Makefile ${top_builddir}/config.status
	$(MAKE) do-build-file FILE=$@

native.logger: ${srcdir}/native.logger.in Makefile ${top_builddir}/config.status
	$(MAKE) do-build-file FILE=$@

shibboleth2.xml: ${srcdir}/shibboleth2.xml.in Makefile ${top_builddir}/config.status
	$(MAKE) do-build-file FILE=$@

attribute-map.xml: ${srcdir}/attribute-map.xml.in Makefile ${top_builddir}/config.status
	$(MAKE) do-build-file FILE=$@

attribute-policy.xml: ${srcdir}/attribute-policy.xml.in Makefile ${top_builddir}/config.status
	$(MAKE) do-build-file FILE=$@

all-data-local: $(BUILTCONFIGFILES)

install-data-local:	all-data-local
	$(mkinstalldirs) $(DESTDIR)$(pkglogdir)
	$(mkinstalldirs) $(DESTDIR)$(pkgdocdir)
	$(mkinstalldirs) $(DESTDIR)$(shirelogdir)
	$(mkinstalldirs) $(DESTDIR)$(pkgrundir)
	$(mkinstalldirs) $(DESTDIR)$(pkgxmldir)
	$(mkinstalldirs) $(DESTDIR)$(pkgsysconfdir)
	for f in $(BUILTCONFIGFILES); do \
		if test ! -f $(DESTDIR)${pkgsysconfdir}/$$f; then \
			$(INSTALL_DATA) $$f $(DESTDIR)${pkgsysconfdir}; \
		fi; \
		$(INSTALL_DATA) $$f $(DESTDIR)${pkgsysconfdir}/$$f.dist; \
        done
	for f in $(CONFIGFILES); do \
		if test ! -f $(DESTDIR)${pkgsysconfdir}/$$f; then \
			$(INSTALL_DATA) $(srcdir)/$$f $(DESTDIR)${pkgsysconfdir}; \
		fi; \
		$(INSTALL_DATA) $(srcdir)/$$f $(DESTDIR)${pkgsysconfdir}/$$f.dist; \
        done

CLEANFILES = \
	apache.config \
	apache2.config \
	apache22.config \
	shibd-redhat \
	shibd-debian \
	shibd-osx.plist \
	shibd.logger \
	native.logger \
	shibboleth2.xml \
	attribute-map.xml \
	attribute-policy.xml

EXTRA_DIST = \
	shibboleth2.xml.in \
	attribute-map.xml.in \
	attribute-policy.xml.in \
	native.logger.in \
	shibd.logger.in \
	apache.config.in \
	apache2.config.in \
	apache22.config.in \
	shibd-redhat.in \
	shibd-debian.in \
	shibd-osx.plist.in \
	$(CONFIGFILES)
