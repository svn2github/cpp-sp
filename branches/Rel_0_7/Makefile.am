## $Id$

AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I .

EXTRA_DIST = \
        schemas/cs-sstc-schema-assertion-01.xsd \
        schemas/cs-sstc-schema-protocol-01.xsd \
        schemas/soap-envelope.xsd \
        schemas/xmldsig-core-schema.xsd \
        schemas/xmldsig-filter2.xsd \
        schemas/xml.xsd \
	schemas/shibboleth.xsd \
	schemas/eduPerson.xsd \
	configs/accessError.html \
	configs/shar.logger \
	configs/shireError.html \
	configs/apache.config \
	configs/shibboleth.ini \
	configs/ca-bundle.crt \
	configs/shibboleth.logger \
	configs/rmError.html \
	configs/shire.logger \
	libtool.m4 acx_pthread.m4 acx_rpctest.m4

SUBDIRS = @WANT_SUBDIRS@

dist-hook:
	cp ../*.txt ../*.html $(distdir)

