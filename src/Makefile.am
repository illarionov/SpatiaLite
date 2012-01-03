
SUBDIRS = headers \
	gaiaaux \
	gaiaexif \
	gaiageo \
	spatialite \
	shapefiles \
	srsinit \
	virtualtext

INCLUDES = @CFLAGS@
INCLUDES += -I$(top_srcdir)/src/headers

lib_LTLIBRARIES = libspatialite.la

libspatialite_la_SOURCES = versioninfo/version.c

libspatialite_la_LIBADD = ./gaiaaux/libgaiaaux.la \
	./gaiaexif/libgaiaexif.la \
	./gaiageo/libgaiageo.la \
	./spatialite/libsplite.la \
	./shapefiles/libshapefiles.la \
	./srsinit/libsrsinit.la \
	./virtualtext/libvirtualtext.la


if MINGW
libspatialite_la_LDFLAGS = -version-info 4:0:2 -no-undefined
else
libspatialite_la_LDFLAGS = -version-info 4:0:2
libspatialite_la_LIBADD += -lpthread -ldl
endif

MOSTLYCLEANFILES = *.gcna *.gcno *.gcda
