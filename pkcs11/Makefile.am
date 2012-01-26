
uidir = $(datadir)/bastile/ui/
localedir = $(datadir)/locale
bastilelibexecbindir = $(libdir)/bastile/

INCLUDES = -I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libmatecryptui \
	-I$(top_srcdir)/libbastile \
	$(BASTILE_CFLAGS) \
	-DDATA_DIR=\""$(datadir)"\" \
	-DBASTILE_UIDIR=\""$(uidir)"\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-DEXECDIR=\""$(bastilelibexecbindir)"\" \
	-DLIBMATECRYPTUI_API_SUBJECT_TO_CHANGE \
	-DGCR_API_SUBJECT_TO_CHANGE \
	-DGCK_API_SUBJECT_TO_CHANGE \
	-DGETTEXT_PACKAGE=\""bastile\""

noinst_LTLIBRARIES = libbastile-pkcs11.la

libbastile_pkcs11_la_SOURCES = \
	bastile-pkcs11-certificate.c bastile-pkcs11-certificate.h \
	bastile-pkcs11-certificate-props.c bastile-pkcs11-certificate-props.h \
	bastile-pkcs11-commands.c bastile-pkcs11-commands.h \
	bastile-pkcs11-module.c bastile-pkcs11-module.h \
	bastile-pkcs11-helpers.c bastile-pkcs11-helpers.h \
	bastile-pkcs11-object.c bastile-pkcs11-object.h \
	bastile-pkcs11-operations.c bastile-pkcs11-operations.h \
	bastile-pkcs11-source.c bastile-pkcs11-source.h\
	bastile-pkcs11.c bastile-pkcs11.h

libbastile_pkcs11_la_LIBADD = \
	$(top_builddir)/libbastile/libbastile.la \
	$(top_builddir)/libmatecryptui/libmatecryptui.la \
	$(GCR_LIBS)

ui_DATA =

EXTRA_DIST = \
	$(ui_DATA)
