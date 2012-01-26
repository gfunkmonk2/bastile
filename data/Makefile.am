SUBDIRS = icons

@INTLTOOL_SCHEMAS_RULE@

schemasdir = $(MATECONF_SCHEMA_FILE_DIR)
schemas_in_files = bastile.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

if MATECONF_SCHEMAS_INSTALL
install-data-local:
	@MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) mateconftool-2 \
		--makefile-install-rule $(schemas_DATA) || \
		(echo ;\
		echo "*****************************************************"; \
		echo "Installation of schemas failed, install them manually"; \
		echo "*****************************************************";)
	@true
else
install-data-local:
endif

EXTRA_DIST = $(schemas_in_files)

CLEANFILES = $(schemas_DATA)
