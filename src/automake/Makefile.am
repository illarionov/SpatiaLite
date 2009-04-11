
AM_CFLAGS = -D_LARGE_FILE=1 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE=1 \
        -DSQLITE_ENABLE_RTREE=1

INCLUDES = @CFLAGS@
INCLUDES += -I./headers/spatialite

lib_LTLIBRARIES = libspatialite.la

libspatialite_la_SOURCES = spatialite.c \
	sqlite3.c

if MINGW
libspatialite_la_LDFLAGS = -version-info 2:3:0 -no-undefined
libspatialite_la_LIBADD = \
	@GEOS_LIB_CLA@ \
	@GEOS_LIB_LA@ \
	@PROJ_LIB_LA@ \
	-liconv -lm -lstdc++
else
if MACOSX
libspatialite_la_LDFLAGS = -version-info 2:3:0
libspatialite_la_LIBADD = \
	@GEOS_LIB_CLA@ \
	@GEOS_LIB_LA@ \
	@PROJ_LIB_LA@ \
	-liconv -lm -lstdc++ -lpthread -ldl
else
libspatialite_la_LDFLAGS = -version-info 2:3:0
libspatialite_la_LIBADD = \
	@GEOS_LIB_CLA@ \
	@GEOS_LIB_LA@ \
	@PROJ_LIB_LA@ \
	-lm -lstdc++ -lpthread -ldl
endif
endif

AUTOMAKE_OPTIONS = dist-zip

EXTRA_DIST = makefile.vc nmake.opt libspatialite.def libspatialite-geos.def

SUBDIRS = headers

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = spatialite.pc
