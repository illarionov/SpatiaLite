
SUBDIRS = epsg_update

INCLUDES = @CFLAGS@
INCLUDES += -I$(top_srcdir)/src/headers

noinst_LTLIBRARIES = libsrsinit.la

libsrsinit_la_SOURCES = srs_init.c

MOSTLYCLEANFILES = *.gcna *.gcno *.gcda

EXTRA_DIST = epsg_inlined.c

