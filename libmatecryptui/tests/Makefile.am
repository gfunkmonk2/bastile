
noinst_PROGRAMS= \
	cryptui-test
	
# ------------------------------------------------------------------------------
# UI TEST PROGRAMS

cryptui_test_SOURCES = cryptui-test-ui.c
cryptui_test_LDADD = $(LIBMATECRYPTUI_LIBS) \
	$(top_builddir)/libmatecryptui/libmatecryptui.la 

# ------------------------------------------------------------------------------
# COMMON

INCLUDES = -I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libmatecryptui \
	-DLIBMATECRYPTUI_API_SUBJECT_TO_CHANGE \
	$(LIBMATECRYPTUI_CFLAGS) 
