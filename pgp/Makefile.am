
uidir = $(datadir)/bastile/ui/
localedir = $(datadir)/locale
bastilelibexecbindir = $(libdir)/bastile/

INCLUDES = -I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libegg \
	-I$(top_srcdir)/libmatecryptui \
	-I$(top_srcdir)/libbastile \
	$(BASTILE_CFLAGS) \
	-DDATA_DIR=\""$(datadir)"\" \
	-DBASTILE_UIDIR=\""$(uidir)"\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-DEXECDIR=\""$(bastilelibexecbindir)"\" \
	-DLDAP_DEPRECATED \
	-DLIBMATECRYPTUI_API_SUBJECT_TO_CHANGE \
	-DGETTEXT_PACKAGE=\""bastile\""

noinst_LTLIBRARIES = libbastile-pgp.la

if WITH_LDAP
LDAP_SRCS = bastile-ldap-source.c bastile-ldap-source.h
else
LDAP_SRCS = 
endif

if WITH_HKP
HKP_SRCS = bastile-hkp-source.c bastile-hkp-source.h
else
HKP_SRCS =
endif

if WITH_KEYSERVER
KEYSERVER_SRCS = bastile-server-source.c bastile-server-source.h 
else
KEYSERVER_SRCS = 
endif

libbastile_pgp_la_SOURCES = \
	bastile-gpgme.c bastile-gpgme.h \
	bastile-gpgme-add-subkey.c \
	bastile-gpgme-add-uid.c \
	bastile-gpgme-dialogs.h \
	bastile-gpgme-data.c bastile-gpgme-data.h \
	bastile-gpgme-expires.c \
	bastile-gpgme-generate.c \
	bastile-gpgme-key.c bastile-gpgme-key.h \
	bastile-gpgme-key-op.c bastile-gpgme-key-op.h \
	bastile-gpgme-operation.c bastile-gpgme-operation.h \
	bastile-gpgme-photo.c bastile-gpgme-photo.h \
	bastile-gpgme-photos.c \
	bastile-gpgme-revoke.c \
	bastile-gpgme-sign.c \
	bastile-gpgme-source.c bastile-gpgme-source.h \
	bastile-gpgme-subkey.c bastile-gpgme-subkey.h \
	bastile-gpgme-uid.c bastile-gpgme-uid.h \
	bastile-gpg-op.h bastile-gpg-op.c \
	bastile-gpg-options.c bastile-gpg-options.h \
	bastile-pgp.c bastile-pgp.h \
	bastile-pgp-commands.c bastile-pgp-commands.h \
	bastile-pgp-key.c bastile-pgp-key.h \
	bastile-pgp-key-properties.c \
	bastile-pgp-keysets.c bastile-pgp-keysets.h \
	bastile-pgp-photo.c bastile-pgp-photo.h \
	bastile-pgp-module.c bastile-pgp-module.h \
	bastile-pgp-signature.c bastile-pgp-signature.h \
	bastile-pgp-subkey.c bastile-pgp-subkey.h \
	bastile-pgp-uid.c bastile-pgp-uid.h \
	bastile-signer.c bastile-pgp-dialogs.h \
	$(VALA_CFILES) $(VALA_HFILES) \
	$(KEYSERVER_SRCS) \
	$(LDAP_SRCS) \
	$(HKP_SRCS)

libbastile_pgp_la_LIBADD = \
	$(top_builddir)/libbastile/libbastile.la \
	$(top_builddir)/common/libbastile-common.la \
	$(top_builddir)/libmatecryptui/libmatecryptui.la

bastilelibexecbin_PROGRAMS = xloadimage

xloadimage_SOURCES = bastile-xloadimage.c

ui_DATA = \
	bastile-add-subkey.xml \
	bastile-add-uid.xml \
	bastile-expires.xml \
	bastile-pgp-public-key-properties.xml \
	bastile-pgp-private-key-properties.xml \
	bastile-pgp-generate.xml \
	bastile-revoke.xml \
	bastile-sign.xml \
	bastile-signer.xml

EXTRA_DIST = \
	$(ui_DATA)
