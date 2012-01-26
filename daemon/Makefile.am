
localedir = $(datadir)/locale
pixmapsdir = $(datadir)/pixmaps/

INCLUDES = -I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libmatecryptui \
	-I$(top_srcdir)/libegg \
	-I$(top_srcdir)/libbastile \
	$(BASTILE_CFLAGS) \
	-DDATA_DIR=\""$(datadir)"\" \
	-DLOCALEDIR=\""$(localedir)"\" \
	-DPIXMAPSDIR=\""$(pixmapsdir)"\" \
	-DLIBMATECRYPTUI_API_SUBJECT_TO_CHANGE \
	-DAUTOSTARTDIR=\""$(datadir)/mate/autostart"\"

bin_PROGRAMS = bastile-daemon

BUILT_SOURCES = \
	bastile-service-bindings.h \
	bastile-service-keyset-bindings.h \
	bastile-service-crypto-bindings.h 

if WITH_PGP
PGP_LDADD = $(top_builddir)/pgp/libbastile-pgp.la
PGP_SRCS = bastile-service-crypto.c
else
PGP_LDADD = 
PGP_SRCS = 
endif

if WITH_SSH
SSH_LDADD = $(top_builddir)/ssh/libbastile-ssh.la
else
SSH_LDADD = 
endif

bastile_daemon_SOURCES = \
    bastile-daemon.c bastile-daemon.h \
    bastile-dbus-server.c bastile-service.c bastile-service.h \
    bastile-service-keyset.c \
    $(PGP_SRCS)

bastile_daemon_LDADD = \
	$(top_builddir)/libbastile/libbastile.la \
	$(PGP_LDADD) \
	$(SSH_LDADD) \
	$(top_builddir)/common/libbastile-common.la \
	$(top_builddir)/libmatecryptui/libmatecryptui.la \
	$(top_builddir)/libegg/libeggdatetime.la \
	$(top_builddir)/libegg/libeggsmclient.la \
	$(BASTILE_LIBS)

man_MANS = bastile-daemon.1

EXTRA_DIST = \
    $(man_MANS) \
    bastile-service.xml \
    bastile-service-keyset.xml \
    bastile-service-crypto.xml \
    bastile-service-bindings.h \
    bastile-service-keyset-bindings.h \
    bastile-service-crypto-bindings.h \
    org.mate.bastile.service.in

CLEANFILES = *.bak \
	org.mate.bastile.service

# DBUS binding files
bastile-service-bindings.h: bastile-service.xml
	dbus-binding-tool --prefix=bastile_service --mode=glib-server --output=$@ $<
bastile-service-keyset-bindings.h: bastile-service-keyset.xml
	dbus-binding-tool --prefix=bastile_service_keyset --mode=glib-server --output=$@ $<
bastile-service-crypto-bindings.h: bastile-service-crypto.xml
	dbus-binding-tool --prefix=bastile_service_crypto --mode=glib-server --output=$@ $<

# Dbus service file
servicedir = $(datadir)/dbus-1/services/
service_in_files = org.mate.bastile.service.in
service_DATA = $(service_in_files:.service.in=.service)

# Rule to make the service file with bindir expanded
$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@bindir\@|$(bindir)|" $< > $@
