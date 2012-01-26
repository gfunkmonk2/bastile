
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

AM_LDFLAGS = @NETLIBS@
	
noinst_LTLIBRARIES = libbastile-ssh.la

libbastile_ssh_la_SOURCES = \
	bastile-ssh.h bastile-ssh.c \
	bastile-ssh-module.c bastile-ssh-module.h \
	bastile-algo.c bastile-algo.h \
	bastile-ssh-commands.c bastile-ssh-commands.h \
	bastile-ssh-dialogs.h \
	bastile-ssh-generate.c \
	bastile-ssh-key-data.c bastile-ssh-key-data.h \
	bastile-ssh-key.c bastile-ssh-key.h  \
	bastile-ssh-key-properties.c \
	bastile-ssh-source.c bastile-ssh-source.h \
	bastile-ssh-operation.c bastile-ssh-operation.h \
	bastile-ssh-upload.c

libbastile_ssh_la_LIBADD = \
	$(top_builddir)/libbastile/libbastile.la \
	$(top_builddir)/libmatecryptui/libmatecryptui.la

bastilelibexecbin_PROGRAMS = bastile-ssh-askpass

bastile_ssh_askpass_SOURCES = bastile-ssh-askpass.c
bastile_ssh_askpass_LDADD = \
	$(top_builddir)/libbastile/libbastile.la \
	$(top_builddir)/libmatecryptui/libmatecryptui.la \
	$(BASTILE_LIBS)

ui_DATA = \
	bastile-ssh-key-properties.xml \
	bastile-ssh-generate.xml \
	bastile-ssh-upload.xml

EXTRA_DIST = \
	$(ui_DATA)
