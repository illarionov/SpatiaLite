
INCLUDES = @CFLAGS@
INCLUDES += -I$(top_srcdir)/src/headers

noinst_LTLIBRARIES = libsplite.la

libsplite_la_SOURCES = mbrcache.c \
	spatialite.c \
	virtualdbf.c \
	virtualXL.c \
	virtualfdo.c \
	virtualspatialindex.c \
	virtualnetwork.c \
	virtualshape.c

MOSTLYCLEANFILES = *.gcna *.gcno *.gcda
