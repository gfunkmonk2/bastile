
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
	-DGETTEXT_PACKAGE=\""bastile\""

noinst_LTLIBRARIES = libbastile-gkr.la

libbastile_gkr_la_SOURCES = \
	bastile-gkr.c bastile-gkr.h \
	bastile-gkr-add-item.c \
	bastile-gkr-add-keyring.c \
	bastile-gkr-dialogs.h bastile-gkr-dialogs.c \
	bastile-gkr-item.c bastile-gkr-item.h \
	bastile-gkr-item-commands.c bastile-gkr-item-commands.h \
	bastile-gkr-item-properties.c \
	bastile-gkr-keyring.c bastile-gkr-keyring.h \
	bastile-gkr-keyring-commands.c bastile-gkr-keyring-commands.h \
	bastile-gkr-keyring-properties.c \
	bastile-gkr-module.c bastile-gkr-module.h \
	bastile-gkr-operation.c bastile-gkr-operation.h \
	bastile-gkr-source.c bastile-gkr-source.h
	
libbastile_gkr_la_LIBADD = \
	$(top_builddir)/libbastile/libbastile.la \
	$(top_builddir)/common/libbastile-common.la \
	$(top_builddir)/libmatecryptui/libmatecryptui.la

ui_DATA = \
	bastile-add-keyring.xml \
	bastile-gkr-add-item.xml \
	bastile-gkr-item-properties.xml \
	bastile-gkr-keyring.xml

EXTRA_DIST = \
	$(ui_DATA)
