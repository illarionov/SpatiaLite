ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = dist-zip

EXTRA_DIST = makefile.vc nmake.opt mainpage.doxy README.coverage

SUBDIRS = src test examples

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = spatialite.pc

coverage-init:
	lcov --directory src --capture --initial --output-file libspatialite_cov.info

coverage::
	lcov --directory src --output-file libspatialite_cov.info --capture
	genhtml -o covresults libspatialite_cov.info

MOSTLYCLEANFILES = libspatialite_cov.info
