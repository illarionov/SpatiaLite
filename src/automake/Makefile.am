ACLOCAL_AMFLAGS = -I m4

INCLUDES = @CFLAGS@
INCLUDES += -I$(top_srcdir)/src/headers

lib_LTLIBRARIES = libspatialite.la

libspatialite_la_SOURCES = spatialite.c \
	sqlite3.c

if MINGW
libspatialite_la_LDFLAGS = -version-info 3:0:0 -no-undefined
else 
if ANDROID
libspatialite_la_LDFLAGS = -version-info 5:0:2
libspatialite_la_LIBADD = -ldl
else
libspatialite_la_LDFLAGS = -version-info 5:0:2
libspatialite_la_LIBADD = -lpthread -ldl
endif
endif

AUTOMAKE_OPTIONS = dist-zip

EXTRA_DIST = makefile.vc nmake.opt spatialite-sql-latest.html

SUBDIRS = headers

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = spatialite.pc
