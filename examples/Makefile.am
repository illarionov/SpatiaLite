noinst_PROGRAMS = demo1 demo2 demo3 demo4

AM_CFLAGS = -I@srcdir@/../src/headers @OMIT_GEOS_FLAGS@
AM_LDFLAGS = -L../src -lspatialite -lm $(GCOV_FLAGS)

MOSTLYCLEANFILES = *.gcna *.gcno *.gcda

EXTRA_DIST = examples.doxy
