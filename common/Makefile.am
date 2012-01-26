
localedir = $(datadir)/locale

INCLUDES = -I$(top_builddir) \
	-I$(top_srcdir) \
	$(BASTILE_CFLAGS)

noinst_LTLIBRARIES = libbastile-common.la

libbastile_common_la_SOURCES = \
	bastile-bind.c bastile-bind.h \
	bastile-cleanup.c bastile-cleanup.h \
	bastile-object-list.c bastile-object-list.h \
	bastile-registry.c bastile-registry.h \
	bastile-secure-buffer.c bastile-secure-buffer.h
