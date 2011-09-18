check_PROGRAMS = check_version \
		check_init \
		check_create \
		check_dbf_load \
		check_shp_load \
		shape_cp1252 \
		shape_utf8_1 \
		shape_utf8_2 \
		shape_3d \
		check_xls_load \
		check_math_funcs \
		check_gaia_util \
		check_gaia_utf8 \
		check_extension \
		check_sql_stmt \
		check_virtualtable1 \
		check_virtualtable2 \
		check_virtualtable3

AM_CFLAGS = -I@srcdir@/../src/headers
AM_LDFLAGS = -L../src -lspatialite -lm $(GCOV_FLAGS)

TESTS = $(check_PROGRAMS)

MOSTLYCLEANFILES = *.gcna *.gcno *.gcda

EXTRA_DIST = shapetest1.dbf \
	shapetest1.prj \
	shapetest1.qpj \
	shapetest1.shp \
	shapetest1.shx \
	shp/new-caledonia/buildings.dbf \
	shp/new-caledonia/buildings.prj \
	shp/new-caledonia/buildings.shp \
	shp/new-caledonia/buildings.shx \
	shp/new-caledonia/points.dbf \
	shp/new-caledonia/points.prj \
	shp/new-caledonia/points.shp \
	shp/new-caledonia/points.shx \
	shp/new-caledonia/railways.dbf \
	shp/new-caledonia/railways.shp \
	shp/new-caledonia/railways.shx \
	shp/gaza/aeroway.dbf \
	shp/gaza/aeroway.prj \
	shp/gaza/aeroway.shp \
	shp/gaza/aeroway.shx \
	shp/gaza/barrier.dbf \
	shp/gaza/barrier.prj \
	shp/gaza/barrier.shp \
	shp/gaza/barrier.shx \
	shp/gaza/route.dbf \
	shp/gaza/route.prj \
	shp/gaza/route.shp \
	shp/gaza/route.shx \
	shp/taiwan/hystoric.dbf
	shp/taiwan/hystoric.prj
	shp/taiwan/hystoric.shp
	shp/taiwan/hystoric.shx
	shp/taiwan/leisure.dbf
	shp/taiwan/leisure.prj
	shp/taiwan/leisure.shp
	shp/taiwan/leisure.shx
	shp/taiwan/route.dbf
	shp/taiwan/route.prj
	shp/taiwan/route.shp
	shp/taiwan/route.shx
	shp/merano-3d/points.dbf \
	shp/merano-3d/points.prj \
	shp/merano-3d/points.shp \
	shp/merano-3d/points.shx \
	shp/merano-3d/polygons.dbf \
	shp/merano-3d/polygons.prj \
	shp/merano-3d/polygons.shp \
	shp/merano-3d/polygons.shx \
	shp/merano-3d/roads.dbf \
	shp/merano-3d/roads.prj \
	shp/merano-3d/roads.shp \
	shp/merano-3d/roads.shx
