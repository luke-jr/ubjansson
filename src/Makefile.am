EXTRA_DIST = jansson.def

include_HEADERS = ubjansson.h

lib_LTLIBRARIES = libubjansson.la
libubjansson_la_SOURCES = \
	dump.c \
	error.c \
	jansson_private.h \
	load.c
libubjansson_la_CFLAGS = \
	$(jansson_CFLAGS)
libubjansson_la_LDFLAGS = \
	$(jansson_LIBS) \
	-no-undefined \
	-export-symbols-regex '^ubjson_' \
	-version-info 0:0:0

if GCC
# These flags are gcc specific
AM_CFLAGS = -Wall -Wextra -Wdeclaration-after-statement -Werror
endif
