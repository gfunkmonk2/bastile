if WITH_TESTS
TESTS_DIR = tests
else
TESTS_DIR = 
endif

SUBDIRS = . $(TESTS_DIR)

# -----------------------------------------------------------------
# LIBRARY HEADERS
# 

incdir = $(includedir)/libmatecryptui

inc_HEADERS = \
    $(srcdir)/cryptui.h \
    $(srcdir)/cryptui-keyset.h \
    $(srcdir)/cryptui-key-store.h \
    $(srcdir)/cryptui-key-combo.h \
    $(srcdir)/cryptui-key-list.h \
    $(srcdir)/cryptui-key-chooser.h

# -----------------------------------------------------------------
# MARSHALLERS
# 

MARSHALLERS = \
    cryptui-marshal.c cryptui-marshal.h

cryptui-marshal.h: cryptui-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN) $(GLIB_GENMARSHAL) $< --header --prefix=cryptui_marshal > $@

cryptui-marshal.c: cryptui-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN) echo "#include \"cryptui-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=cryptui_marshal >> $@
	

# -----------------------------------------------------------------
# LIBRARY SOURCES
# 

lib_LTLIBRARIES = libmatecryptui.la

libmatecryptui_src = \
    $(srcdir)/cryptui.c \
    $(srcdir)/cryptui-keyset.c \
    $(srcdir)/cryptui-key-store.c \
    $(srcdir)/cryptui-key-combo.c \
    $(srcdir)/cryptui-key-list.c \
    $(srcdir)/cryptui-key-chooser.c

libmatecryptui_la_SOURCES = \
    cryptui-defines.h \
    cryptui-priv.h \
    $(inc_HEADERS) \
    $(libmatecryptui_src) \
    $(MARSHALLERS)
    
libmatecryptui_la_LDFLAGS = \
    -version-info $(LIBMATECRYPTUI_LT_RELEASE)
    
libmatecryptui_la_LIBADD = $(LIBMATECRYPTUI_LIBS) 

# ------------------------------------------------------------------
# PKGCONFIG
# 

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = matecryptui-$(LIBMATECRYPTUI_MAJOR).pc

matecryptui-$(LIBMATECRYPTUI_MAJOR).pc: matecryptui.pc
	$(AM_V_GEN) cp matecryptui.pc matecryptui-$(LIBMATECRYPTUI_MAJOR).pc

# ------------------------------------------------------------------
# COMMON

INCLUDES = -I$(top_builddir) \
	-I$(top_srcdir) \
	$(LIBMATECRYPTUI_CFLAGS) \
	-DLIBMATECRYPTUI_BUILD=1 \
	-DLIBMATECRYPTUI_API_SUBJECT_TO_CHANGE

EXTRA_DIST = \
	cryptui-marshal.list \
	libmatecryptui.pc.in
    
CLEANFILES = \
	cryptui-marshal.h \
    	cryptui-marshal.c \
    	$(pkgconfig_DATA)

BUILT_SOURCES = \
	$(MARSHALLERS)

# ------------------------------------------------------------------
# GObject Introspection
BUILT_GIRSOURCES =

if HAVE_INTROSPECTION
CryptUI-@LIBMATECRYPTUI_MAJOR@.gir: $(INTROSPECTION_SCANNER) libmatecryptui.la
	$(QUIET_GEN)$(INTROSPECTION_SCANNER) -v \
	--namespace CryptUI --nsversion=@LIBMATECRYPTUI_MAJOR@ \
	$(INCLUDES) \
	--include=Gtk-$(GTK_API_VERSION) \
	--library=libmatecryptui.la \
	--pkg gtk+-$(GTK_API_VERSION) \
	--output $@ \
	$(inc_HEADERS) \
	$(libmatecryptui_src)

BUILT_GIRSOURCES += CryptUI-@LIBMATECRYPTUI_MAJOR@.gir

# INTROSPECTION_GIRDIR/INTROSPECTION_TYPELIBDIR aren't the right place to
# install anything - we need to install inside our prefix.
girdir = $(datadir)/gir-1.0
gir_DATA = $(BUILT_GIRSOURCES)

typelibsdir = $(libdir)/girepository-1.0/

typelibs_DATA = $(BUILT_GIRSOURCES:.gir=.typelib)

%.typelib: %.gir $(INTROSPECTION_COMPILER)
	$(QUIET_GEN)$(DEBUG) $(INTROSPECTION_COMPILER) --includedir=$(srcdir) --includedir=. $(INTROSPECTION_COMPILER_OPTS) $< -o $(builddir)/$(@F)

CLEANFILES += $(BUILT_GIRSOURCES) $(typelibs_DATA)
endif # HAVE_INTROSPECTION
