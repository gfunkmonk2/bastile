
uidir = $(datadir)/bastile/ui/
localedir = $(datadir)/locale
pixmapsdir = $(datadir)/pixmaps/

INCLUDES = -I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libegg \
	-I$(top_srcdir)/libmatecryptui \
	-I$(top_srcdir)/libbastile \
	$(BASTILE_CFLAGS) \
	-DDATA_DIR=\""$(datadir)"\" \
	-DBASTILE_UIDIR=\""$(uidir)"\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-DPIXMAPSDIR=\""$(pixmapsdir)"\" \
	-DLIBMATECRYPTUI_API_SUBJECT_TO_CHANGE \
	-DGETTEXT_PACKAGE=\""bastile\""

if WITH_KEYSERVER
KEYSERVER_SRCS = bastile-keyserver-search.c bastile-keyserver-sync.c bastile-keyserver-sync.h
KEYSERVER_SRCS += bastile-keyserver-results.c bastile-keyserver-results.h
else
KEYSERVER_SRCS = 
endif

if WITH_PGP
PGP_LDADD = $(top_builddir)/pgp/libbastile-pgp.la
else
PGP_LDADD = 
endif

if WITH_SSH
SSH_LDADD = $(top_builddir)/ssh/libbastile-ssh.la
else
SSH_LDADD = 
endif

if WITH_PKCS11
PKCS11_LDADD = $(top_builddir)/pkcs11/libbastile-pkcs11.la
else
PKCS11_LDADD = 
endif

bin_PROGRAMS = bastile

bastile_SOURCES = \
	bastile-preferences.c bastile-preferences.h \
	eggtreemultidnd.c eggtreemultidnd.h \
	bastile-generate-select.c bastile-generate-select.h \
	bastile-key-manager.c bastile-key-manager.h \
	bastile-key-manager-store.c bastile-key-manager-store.h \
	bastile-main.c \
	bastile-viewer.c bastile-viewer.h \
	bastile-windows.h \
	$(KEYSERVER_SRCS)

bastile_LDADD = \
	$(top_builddir)/libbastile/libbastile.la \
	$(PGP_LDADD) \
	$(SSH_LDADD) \
	$(PKCS11_LDADD) \
	$(top_builddir)/gkr/libbastile-gkr.la \
	$(top_builddir)/common/libbastile-common.la \
	$(top_builddir)/libmatecryptui/libmatecryptui.la \
	$(top_builddir)/libegg/libeggdatetime.la \
	$(BASTILE_LIBS)

ui_DATA = \
	bastile-key-manager.ui \
	bastile-key-manager.xml \
	bastile-change-passphrase.xml \
	bastile-generate-select.xml \
	bastile-keyserver-search.xml \
	bastile-keyserver-results.ui \
	bastile-keyserver-results.xml \
	bastile-keyserver-sync.xml

desktopdir = $(datadir)/applications
desktop_in_files = bastile.desktop.in 
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

man_MANS = bastile.1

EXTRA_DIST = \
	$(man_MANS) \
	$(ui_DATA) \
	bastile.desktop.in.in

CLEANFILES = *.xmlp* *.bak \
    $(desktop_DATA) \
    bastile.desktop.in 
