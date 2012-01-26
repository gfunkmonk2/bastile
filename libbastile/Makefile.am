
uidir = $(datadir)/bastile/ui/
localedir = $(datadir)/locale
pixmapsdir = $(datadir)/pixmaps/bastile/
bastilelibexecbindir = $(libdir)/bastile/

INCLUDES = -I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libmatecryptui \
	$(BASTILE_CFLAGS) \
	-DDATA_DIR=\""$(datadir)"\" \
	-DBASTILE_UIDIR=\""$(uidir)"\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-DPIXMAPSDIR=\""$(pixmapsdir)"\" \
	-DEXECDIR=\""$(bastilelibexecbindir)"\" \
	-DLIBMATECRYPTUI_API_SUBJECT_TO_CHANGE

MARSHAL_SRCS = \
	bastile-marshal.c bastile-marshal.h

BUILT_SOURCES = \
	$(MARSHAL_SRCS)

noinst_LTLIBRARIES = libbastile.la

if WITH_KEYSERVER
KEYSERVER_SRCS = bastile-keyserver-control.c bastile-keyserver-control.h
else
KEYSERVER_SRCS = 
endif

libbastile_la_SOURCES = \
	bastile-check-button-control.c bastile-check-button-control.h \
	bastile-combo-keys.c bastile-combo-keys.h \
	bastile-commands.c bastile-commands.h \
	bastile-context.c bastile-context.h \
	bastile-dns-sd.c bastile-dns-sd.h \
	bastile-mateconf.c bastile-mateconf.h \
	bastile-gtkstock.c bastile-gtkstock.h \
	bastile-libdialogs.h \
	bastile-notification.c \
	bastile-object.c bastile-object.h \
	bastile-object-model.c bastile-object-model.h \
	bastile-object-widget.c bastile-object-widget.h \
	bastile-operation.c bastile-operation.h \
	bastile-passphrase.c bastile-passphrase.h \
	bastile-prefs.c bastile-prefs.h \
	bastile-progress.c bastile-progress.h \
	bastile-secure-memory.c bastile-secure-memory.h \
	bastile-servers.c bastile-servers.h \
	bastile-set.c bastile-set.h \
	bastile-set-model.c bastile-set-model.h \
	bastile-source.c bastile-source.h \
	bastile-transfer-operation.c bastile-transfer-operation.h \
	bastile-types.c bastile-types.h \
	bastile-unix-signal.c bastile-unix-signal.h \
	bastile-unknown.c bastile-unknown.h \
	bastile-unknown-source.c bastile-unknown-source.h \
	bastile-util.c bastile-util.h \
	bastile-validity.c bastile-validity.h \
	bastile-view.c bastile-view.h \
	bastile-widget.c bastile-widget.h \
	$(MARSHAL_SRCS) \
	$(KEYSERVER_SRCS)
	
libbastile_la_LIBADD = \
	$(top_builddir)/common/libbastile-common.la \
	$(top_builddir)/libmatecryptui/libmatecryptui.la

bastile-marshal.h: bastile-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=bastile_marshal > $@

bastile-marshal.c: bastile-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"bastile-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=bastile_marshal >> $@

ui_DATA = \
	bastile-prefs.xml \
	bastile-add-keyserver.xml \
	bastile-progress.xml \
	bastile-notify.xml

EXTRA_DIST = $(ui_DATA) \
	bastile-marshal.list
