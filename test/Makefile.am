check_PROGRAMS = check_version \
		check_init \
		check_create \
		check_dbf_load \
		check_shp_load \
		check_shp_load_3d \
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
		check_virtualtable3 \
		check_virtualtable4 \
		check_virtualtable5 \
		check_virtualtable6 \
		check_mbrcache \
		check_spatialindex

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
	shp/taiwan/hystoric.dbf \
	shp/taiwan/hystoric.prj \
	shp/taiwan/hystoric.shp \
	shp/taiwan/hystoric.shx \
	shp/taiwan/leisure.dbf \
	shp/taiwan/leisure.prj \
	shp/taiwan/leisure.shp \
	shp/taiwan/leisure.shx \
	shp/taiwan/route.dbf \
	shp/taiwan/route.prj \
	shp/taiwan/route.shp \
	shp/taiwan/route.shx \
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
	shp/merano-3d/roads.shx \
	shp/foggia/local_councils.dbf \
	shp/foggia/local_councils.prj \
	shp/foggia/local_councils.shp \
	shp/foggia/local_councils.shx \
	testcase1.xls \
	testcase1.csv \
	sql_stmt_tests/trivial.sqlite_RO \
	sql_stmt_tests/fdo-wkb.sqlite_RO \
	sql_stmt_tests/testdb1.sqlite \
	sql_stmt_tests/acos10.testcase \
	sql_stmt_tests/acos20.testcase \
	sql_stmt_tests/acos2.testcase \
	sql_stmt_tests/acos.testcase \
	sql_stmt_tests/acos-text.testcase \
	sql_stmt_tests/asgeojson1.testcase\
	sql_stmt_tests/asgeojson2.testcase\
	sql_stmt_tests/asgeojson3.testcase\
	sql_stmt_tests/asgeojson4.testcase\
	sql_stmt_tests/asgeojson5.testcase\
	sql_stmt_tests/asgeojson6.testcase\
	sql_stmt_tests/asgeojson7.testcase\
	sql_stmt_tests/asgml10.testcase\
	sql_stmt_tests/asgml11.testcase\
	sql_stmt_tests/asgml1.testcase\
	sql_stmt_tests/asgml2.testcase\
	sql_stmt_tests/asgml3.testcase\
	sql_stmt_tests/asgml4.testcase\
	sql_stmt_tests/asgml5.testcase\
	sql_stmt_tests/asgml6.testcase\
	sql_stmt_tests/asgml7.testcase\
	sql_stmt_tests/asgml8.testcase\
	sql_stmt_tests/asgml9.testcase\
	sql_stmt_tests/asin10.testcase \
	sql_stmt_tests/asin1.testcase \
	sql_stmt_tests/asin20.testcase \
	sql_stmt_tests/asin2.testcase \
	sql_stmt_tests/asin-text.testcase \
	sql_stmt_tests/askml1.testcase\
	sql_stmt_tests/askml2.testcase\
	sql_stmt_tests/askml3.testcase\
	sql_stmt_tests/askml4.testcase\
	sql_stmt_tests/askml5.testcase\
	sql_stmt_tests/askml6.testcase\
	sql_stmt_tests/askml7.testcase\
	sql_stmt_tests/askml8.testcase\
	sql_stmt_tests/askml9.testcase\
	sql_stmt_tests/aswkt-text.testcase \
	sql_stmt_tests/atan00.testcase \
	sql_stmt_tests/atan0.testcase \
	sql_stmt_tests/atan-text.testcase \
	sql_stmt_tests/buffer1.testcase \
	sql_stmt_tests/buffer2.testcase \
	sql_stmt_tests/buffer3.testcase \
	sql_stmt_tests/buffer4.testcase \
	sql_stmt_tests/buffer5.testcase \
	sql_stmt_tests/buffer6.testcase \
	sql_stmt_tests/buffer7.testcase \
	sql_stmt_tests/ceilfloat2.testcase \
	sql_stmt_tests/ceilfloat.testcase \
	sql_stmt_tests/ceil-int.testcase \
	sql_stmt_tests/ceilnull.testcase \
	sql_stmt_tests/checkspatialmetadata1.testcase\
	sql_stmt_tests/checkspatialmetadata2.testcase\
	sql_stmt_tests/checkspatialmetadata3.testcase\
	sql_stmt_tests/ch_m.testcase \
	sql_stmt_tests/closestpoint10.testcase \
	sql_stmt_tests/closestpoint11.testcase \
	sql_stmt_tests/closestpoint12.testcase \
	sql_stmt_tests/closestpoint13.testcase \
	sql_stmt_tests/closestpoint14.testcase \
	sql_stmt_tests/closestpoint15.testcase\
	sql_stmt_tests/closestpoint16.testcase\
	sql_stmt_tests/closestpoint1.testcase \
	sql_stmt_tests/closestpoint2.testcase \
	sql_stmt_tests/closestpoint3.testcase \
	sql_stmt_tests/closestpoint4.testcase \
	sql_stmt_tests/closestpoint5.testcase \
	sql_stmt_tests/closestpoint6.testcase \
	sql_stmt_tests/closestpoint7.testcase \
	sql_stmt_tests/closestpoint8.testcase \
	sql_stmt_tests/closestpoint9.testcase \
	sql_stmt_tests/cm_m.testcase \
	sql_stmt_tests/collectextract1.testcase \
	sql_stmt_tests/collectextract2.testcase \
	sql_stmt_tests/collectextract3.testcase \
	sql_stmt_tests/collectextract4.testcase \
	sql_stmt_tests/collectextract5.testcase \
	sql_stmt_tests/collectextract6.testcase \
	sql_stmt_tests/collectextract7.testcase \
	sql_stmt_tests/collectextract8.testcase \
	sql_stmt_tests/collectextract9.testcase \
	sql_stmt_tests/cos00.testcase \
	sql_stmt_tests/cos0.testcase \
	sql_stmt_tests/cos-text.testcase \
	sql_stmt_tests/cot00.testcase \
	sql_stmt_tests/cot0.testcase \
	sql_stmt_tests/cotpi2.testcase \
	sql_stmt_tests/cot-text.testcase \
	sql_stmt_tests/createtopo10.testcase \
	sql_stmt_tests/createtopo11.testcase \
	sql_stmt_tests/createtopo12.testcase \
	sql_stmt_tests/createtopo13.testcase \
	sql_stmt_tests/createtopo14.testcase \
	sql_stmt_tests/createtopo15.testcase \
	sql_stmt_tests/createtopo16.testcase \
	sql_stmt_tests/createtopo17.testcase \
	sql_stmt_tests/createtopo18.testcase \
	sql_stmt_tests/createtopo1.testcase \
	sql_stmt_tests/createtopo2.testcase \
	sql_stmt_tests/createtopo3.testcase \
	sql_stmt_tests/createtopo4.testcase \
	sql_stmt_tests/createtopo5.testcase \
	sql_stmt_tests/createtopo6.testcase \
	sql_stmt_tests/createtopo7.testcase \
	sql_stmt_tests/createtopo8.testcase \
	sql_stmt_tests/createtopo9.testcase \
	sql_stmt_tests/degrees45.testcase \
	sql_stmt_tests/degrees.testcase \
	sql_stmt_tests/degreestext.testcase \
	sql_stmt_tests/dimension1.testcase \
	sql_stmt_tests/dimension2.testcase \
	sql_stmt_tests/dimension3.testcase \
	sql_stmt_tests/dimension4.testcase \
	sql_stmt_tests/dimension5.testcase \
	sql_stmt_tests/dimension6.testcase \
	sql_stmt_tests/dimension7.testcase \
	sql_stmt_tests/dimension8.testcase \
	sql_stmt_tests/dm_m.testcase \
	sql_stmt_tests/endpoint1.testcase \
	sql_stmt_tests/endpoint1.testcase \
	sql_stmt_tests/exp00.testcase \
	sql_stmt_tests/exp0.testcase \
	sql_stmt_tests/exp10.testcase \
	sql_stmt_tests/exp1.testcase \
	sql_stmt_tests/expneg.testcase \
	sql_stmt_tests/exptext.testcase \
	sql_stmt_tests/fath_m.testcase \
	sql_stmt_tests/fdo_ogr1.testcase\
	sql_stmt_tests/fdo_ogr2.testcase\
	sql_stmt_tests/fdo-wkb.sqlite_RO\
	sql_stmt_tests/floor32.testcase \
	sql_stmt_tests/floorint.testcase \
	sql_stmt_tests/floornull.testcase \
	sql_stmt_tests/ft_m.testcase \
	sql_stmt_tests/geomcollfromtext1.testcase \
	sql_stmt_tests/geomcollfromtext2.testcase \
	sql_stmt_tests/geomconstraints10.testcase\
	sql_stmt_tests/geomconstraints11.testcase\
	sql_stmt_tests/geomconstraints12.testcase\
	sql_stmt_tests/geomconstraints13.testcase\
	sql_stmt_tests/geomconstraints14.testcase\
	sql_stmt_tests/geomconstraints1.testcase\
	sql_stmt_tests/geomconstraints2.testcase\
	sql_stmt_tests/geomconstraints3.testcase\
	sql_stmt_tests/geomconstraints4.testcase\
	sql_stmt_tests/geomconstraints5.testcase\
	sql_stmt_tests/geomconstraints6.testcase\
	sql_stmt_tests/geomconstraints7.testcase\
	sql_stmt_tests/geomconstraints8.testcase\
	sql_stmt_tests/geomconstraints9.testcase\
	sql_stmt_tests/geometryn1.testcase \
	sql_stmt_tests/geometryn2.testcase \
	sql_stmt_tests/geometryn3.testcase \
	sql_stmt_tests/geometryn4.testcase \
	sql_stmt_tests/geometryn5.testcase \
	sql_stmt_tests/geometryn6.testcase \
	sql_stmt_tests/geometryn.testcase \
	sql_stmt_tests/geomfromtext1.testcase \
	sql_stmt_tests/geomfromtext2.testcase \
	sql_stmt_tests/geomfromtext3.testcase \
	sql_stmt_tests/geomfromtext4.testcase \
	sql_stmt_tests/geomfromtext5.testcase \
	sql_stmt_tests/geomfromtext6.testcase \
	sql_stmt_tests/geomfromtext7.testcase \
	sql_stmt_tests/geomfromtext8.testcase \
	sql_stmt_tests/geomfromtext9.testcase \
	sql_stmt_tests/geomtype10.testcase \
	sql_stmt_tests/geomtype11.testcase \
	sql_stmt_tests/geomtype12.testcase \
	sql_stmt_tests/geomtype13.testcase \
	sql_stmt_tests/geomtype14.testcase \
	sql_stmt_tests/geomtype15.testcase \
	sql_stmt_tests/geomtype16.testcase \
	sql_stmt_tests/geomtype17.testcase \
	sql_stmt_tests/geomtype18.testcase \
	sql_stmt_tests/geomtype19.testcase \
	sql_stmt_tests/geomtype1.testcase \
	sql_stmt_tests/geomtype20.testcase \
	sql_stmt_tests/geomtype21.testcase \
	sql_stmt_tests/geomtype22.testcase \
	sql_stmt_tests/geomtype23.testcase \
	sql_stmt_tests/geomtype24.testcase \
	sql_stmt_tests/geomtype25.testcase \
	sql_stmt_tests/geomtype26.testcase \
	sql_stmt_tests/geomtype27.testcase \
	sql_stmt_tests/geomtype28.testcase \
	sql_stmt_tests/geomtype29.testcase \
	sql_stmt_tests/geomtype2.testcase \
	sql_stmt_tests/geomtype30.testcase \
	sql_stmt_tests/geomtype31.testcase \
	sql_stmt_tests/geomtype32.testcase \
	sql_stmt_tests/geomtype33.testcase \
	sql_stmt_tests/geomtype34.testcase \
	sql_stmt_tests/geomtype35.testcase \
	sql_stmt_tests/geomtype36.testcase \
	sql_stmt_tests/geomtype37.testcase \
	sql_stmt_tests/geomtype38.testcase \
	sql_stmt_tests/geomtype39.testcase \
	sql_stmt_tests/geomtype3.testcase \
	sql_stmt_tests/geomtype40.testcase \
	sql_stmt_tests/geomtype41.testcase \
	sql_stmt_tests/geomtype42.testcase \
	sql_stmt_tests/geomtype43.testcase \
	sql_stmt_tests/geomtype44.testcase \
	sql_stmt_tests/geomtype45.testcase \
	sql_stmt_tests/geomtype46.testcase \
	sql_stmt_tests/geomtype47.testcase \
	sql_stmt_tests/geomtype48.testcase \
	sql_stmt_tests/geomtype49.testcase \
	sql_stmt_tests/geomtype4.testcase \
	sql_stmt_tests/geomtype50.testcase \
	sql_stmt_tests/geomtype51.testcase \
	sql_stmt_tests/geomtype52.testcase \
	sql_stmt_tests/geomtype53.testcase \
	sql_stmt_tests/geomtype54.testcase \
	sql_stmt_tests/geomtype55.testcase \
	sql_stmt_tests/geomtype56.testcase \
	sql_stmt_tests/geomtype57.testcase \
	sql_stmt_tests/geomtype58.testcase \
	sql_stmt_tests/geomtype59.testcase \
	sql_stmt_tests/geomtype5.testcase \
	sql_stmt_tests/geomtype60.testcase \
	sql_stmt_tests/geomtype61.testcase \
	sql_stmt_tests/geomtype62.testcase \
	sql_stmt_tests/geomtype63.testcase \
	sql_stmt_tests/geomtype64.testcase \
	sql_stmt_tests/geomtype65.testcase \
	sql_stmt_tests/geomtype66.testcase \
	sql_stmt_tests/geomtype67.testcase \
	sql_stmt_tests/geomtype68.testcase \
	sql_stmt_tests/geomtype69.testcase \
	sql_stmt_tests/geomtype6.testcase \
	sql_stmt_tests/geomtype7.testcase \
	sql_stmt_tests/geomtype8.testcase \
	sql_stmt_tests/geomtype9.testcase \
	sql_stmt_tests/greatcircle-badblob.testcase \
	sql_stmt_tests/greatcircle-poly2.testcase \
	sql_stmt_tests/greatcircle-poly.testcase \
	sql_stmt_tests/greatcircle.testcase \
	sql_stmt_tests/greatcircle-text.testcase \
	sql_stmt_tests/ind_ch_m.testcase \
	sql_stmt_tests/ind_ft_m.testcase \
	sql_stmt_tests/ind_yd_m.testcase \
	sql_stmt_tests/in_m.testcase \
	sql_stmt_tests/isclosed1.testcase \
	sql_stmt_tests/isclosed2.testcase \
	sql_stmt_tests/isclosed3.testcase \
	sql_stmt_tests/isclosed4.testcase \
	sql_stmt_tests/isclosed5.testcase \
	sql_stmt_tests/isring1.testcase \
	sql_stmt_tests/isring2.testcase \
	sql_stmt_tests/isring3.testcase \
	sql_stmt_tests/isring4.testcase \
	sql_stmt_tests/issimple1.testcase \
	sql_stmt_tests/issimple2.testcase \
	sql_stmt_tests/issimple3.testcase \
	sql_stmt_tests/issimple4.testcase \
	sql_stmt_tests/issimple5.testcase \
	sql_stmt_tests/isvalid1.testcase \
	sql_stmt_tests/isvalid2.testcase \
	sql_stmt_tests/isvalid3.testcase \
	sql_stmt_tests/isvalid4.testcase \
	sql_stmt_tests/isvalid5.testcase \
	sql_stmt_tests/isvalid6.testcase \
	sql_stmt_tests/km_m.testcase \
	sql_stmt_tests/linefromtext1.testcase \
	sql_stmt_tests/linefromtext2.testcase \
	sql_stmt_tests/linemerge10.testcase\
	sql_stmt_tests/linemerge1.testcase\
	sql_stmt_tests/linemerge2.testcase\
	sql_stmt_tests/linemerge3.testcase\
	sql_stmt_tests/linemerge4.testcase\
	sql_stmt_tests/linemerge5.testcase\
	sql_stmt_tests/linemerge6.testcase\
	sql_stmt_tests/linemerge7.testcase\
	sql_stmt_tests/linemerge8.testcase\
	sql_stmt_tests/linemerge9.testcase\
	sql_stmt_tests/linesfromrings1.testcase \
	sql_stmt_tests/linesfromrings2.testcase \
	sql_stmt_tests/linesfromrings3.testcase \
	sql_stmt_tests/linesfromrings4.testcase \
	sql_stmt_tests/linesfromrings5.testcase \
	sql_stmt_tests/linesfromrings6.testcase \
	sql_stmt_tests/linesfromrings7.testcase \
	sql_stmt_tests/linesubstring10.testcase\
	sql_stmt_tests/linesubstring11.testcase\
	sql_stmt_tests/linesubstring12.testcase\
	sql_stmt_tests/linesubstring13.testcase\
	sql_stmt_tests/linesubstring14.testcase\
	sql_stmt_tests/linesubstring15.testcase\
	sql_stmt_tests/linesubstring16.testcase\
	sql_stmt_tests/linesubstring17.testcase\
	sql_stmt_tests/linesubstring18.testcase \
	sql_stmt_tests/linesubstring19.testcase\
	sql_stmt_tests/linesubstring1.testcase\
	sql_stmt_tests/linesubstring20.testcase\
	sql_stmt_tests/linesubstring21.testcase\
	sql_stmt_tests/linesubstring22.testcase\
	sql_stmt_tests/linesubstring23.testcase\
	sql_stmt_tests/linesubstring24.testcase\
	sql_stmt_tests/linesubstring25.testcase\
	sql_stmt_tests/linesubstring2.testcase\
	sql_stmt_tests/linesubstring3.testcase\
	sql_stmt_tests/linesubstring4.testcase\
	sql_stmt_tests/linesubstring5.testcase\
	sql_stmt_tests/linesubstring6.testcase\
	sql_stmt_tests/linesubstring7.testcase\
	sql_stmt_tests/linesubstring8.testcase\
	sql_stmt_tests/linesubstring9.testcase\
	sql_stmt_tests/link_m.testcase \
	sql_stmt_tests/log10-0.0.testcase \
	sql_stmt_tests/log10-100.0.testcase \
	sql_stmt_tests/log10-100.testcase \
	sql_stmt_tests/log10-text.testcase \
	sql_stmt_tests/log2-0.0.testcase \
	sql_stmt_tests/log2-0.testcase \
	sql_stmt_tests/log2-4.0.testcase \
	sql_stmt_tests/log2-4.testcase \
	sql_stmt_tests/log2-text.testcase \
	sql_stmt_tests/logn00.testcase \
	sql_stmt_tests/logn0.testcase \
	sql_stmt_tests/logn10.testcase \
	sql_stmt_tests/logn1.testcase \
	sql_stmt_tests/logn2-01.testcase \
	sql_stmt_tests/logn2-02.testcase \
	sql_stmt_tests/logn2-2.2.testcase \
	sql_stmt_tests/logn2-4.2int.testcase \
	sql_stmt_tests/logn2-4.2.testcase \
	sql_stmt_tests/logn2-text2.testcase \
	sql_stmt_tests/logn2-text3.testcase \
	sql_stmt_tests/logn2-text4.testcase \
	sql_stmt_tests/logn2-text.testcase \
	sql_stmt_tests/logn-text.testcase \
	sql_stmt_tests/makeline10.testcase \
	sql_stmt_tests/makeline11.testcase \
	sql_stmt_tests/makeline12.testcase \
	sql_stmt_tests/makeline13.testcase \
	sql_stmt_tests/makeline14.testcase \
	sql_stmt_tests/makeline15.testcase \
	sql_stmt_tests/makeline16.testcase \
	sql_stmt_tests/makeline17.testcase \
	sql_stmt_tests/makeline18.testcase \
	sql_stmt_tests/makeline2.testcase \
	sql_stmt_tests/makeline3.testcase \
	sql_stmt_tests/makeline4.testcase \
	sql_stmt_tests/makeline5.testcase \
	sql_stmt_tests/makeline6.testcase \
	sql_stmt_tests/makeline7.testcase \
	sql_stmt_tests/makeline8.testcase \
	sql_stmt_tests/makeline9.testcase \
	sql_stmt_tests/makeline.testcase \
	sql_stmt_tests/makepoint1.testcase \
	sql_stmt_tests/makepoint2.testcase \
	sql_stmt_tests/makepoint3.testcase \
	sql_stmt_tests/makepoint4.testcase \
	sql_stmt_tests/makepoint5.testcase \
	sql_stmt_tests/makepoint6.testcase \
	sql_stmt_tests/makepoint7.testcase \
	sql_stmt_tests/makepoint-null1.testcase \
	sql_stmt_tests/makepoint-null2.testcase \
	sql_stmt_tests/makepoint-null3.testcase \
	sql_stmt_tests/makepoint-null4.testcase \
	sql_stmt_tests/makepoint-null5.testcase \
	sql_stmt_tests/makepoint-null6.testcase \
	sql_stmt_tests/makepoint-null7.testcase \
	sql_stmt_tests/mbr10.testcase \
	sql_stmt_tests/mbr11.testcase \
	sql_stmt_tests/mbr12.testcase \
	sql_stmt_tests/mbr13.testcase \
	sql_stmt_tests/mbr14.testcase \
	sql_stmt_tests/mbr15.testcase \
	sql_stmt_tests/mbr16.testcase \
	sql_stmt_tests/mbr17.testcase \
	sql_stmt_tests/mbr18.testcase \
	sql_stmt_tests/mbr19.testcase \
	sql_stmt_tests/mbr1.testcase \
	sql_stmt_tests/mbr20.testcase \
	sql_stmt_tests/mbr21.testcase \
	sql_stmt_tests/mbr22.testcase \
	sql_stmt_tests/mbr23.testcase \
	sql_stmt_tests/mbr24.testcase \
	sql_stmt_tests/mbr25.testcase \
	sql_stmt_tests/mbr26.testcase \
	sql_stmt_tests/mbr27.testcase \
	sql_stmt_tests/mbr28.testcase \
	sql_stmt_tests/mbr29.testcase \
	sql_stmt_tests/mbr2.testcase \
	sql_stmt_tests/mbr30.testcase \
	sql_stmt_tests/mbr31.testcase \
	sql_stmt_tests/mbr32.testcase \
	sql_stmt_tests/mbr3.testcase \
	sql_stmt_tests/mbr4.testcase \
	sql_stmt_tests/mbr5.testcase \
	sql_stmt_tests/mbr6.testcase \
	sql_stmt_tests/mbr7.testcase \
	sql_stmt_tests/mbr8.testcase \
	sql_stmt_tests/mbr9.testcase \
	sql_stmt_tests/m_ch.testcase \
	sql_stmt_tests/m_cm.testcase \
	sql_stmt_tests/m_dm.testcase \
	sql_stmt_tests/m_fath.testcase \
	sql_stmt_tests/m_ft.testcase \
	sql_stmt_tests/m_ft-text.testcase \
	sql_stmt_tests/mi_m.testcase \
	sql_stmt_tests/m_ind_ch.testcase \
	sql_stmt_tests/m_ind_ft.testcase \
	sql_stmt_tests/m_ind_yd.testcase \
	sql_stmt_tests/m_in.testcase \
	sql_stmt_tests/m_in_us.testcase \
	sql_stmt_tests/m_km.testcase \
	sql_stmt_tests/mlinefromtext1.testcase \
	sql_stmt_tests/mlinefromtext2.testcase \
	sql_stmt_tests/m_ln.testcase \
	sql_stmt_tests/m_mi.testcase \
	sql_stmt_tests/m_mm.testcase \
	sql_stmt_tests/mm_m.testcase \
	sql_stmt_tests/m_nm.testcase \
	sql_stmt_tests/mpointfromtext1.testcase \
	sql_stmt_tests/mpointfromtext2.testcase \
	sql_stmt_tests/mpolygonfromtext1.testcase \
	sql_stmt_tests/mpolygonfromtext2.testcase \
	sql_stmt_tests/m_us_ch.testcase \
	sql_stmt_tests/m_us_ft.testcase \
	sql_stmt_tests/m_usmi.testcase \
	sql_stmt_tests/m_yd.testcase \
	sql_stmt_tests/m_yd_us.testcase \
	sql_stmt_tests/nm_m.testcase \
	sql_stmt_tests/NumPoints2.testcase \
	sql_stmt_tests/NumPoints3.testcase \
	sql_stmt_tests/NumPoints4.testcase \
	sql_stmt_tests/NumPoints5.testcase \
	sql_stmt_tests/NumPoints6.testcase \
	sql_stmt_tests/NumPoints7.testcase \
	sql_stmt_tests/NumPoints.testcase \
	sql_stmt_tests/output1.testcase \
	sql_stmt_tests/output2.testcase \
	sql_stmt_tests/output3.testcase \
	sql_stmt_tests/output4.testcase \
	sql_stmt_tests/output5.testcase \
	sql_stmt_tests/output6.testcase \
	sql_stmt_tests/output7.testcase \
	sql_stmt_tests/pi.testcase \
	sql_stmt_tests/pointfromtext1.testcase \
	sql_stmt_tests/pointfromtext2.testcase \
	sql_stmt_tests/pointn10.testcase \
	sql_stmt_tests/pointn10.testcase \
	sql_stmt_tests/pointn11.testcase \
	sql_stmt_tests/pointn11.testcase \
	sql_stmt_tests/pointn12.testcase \
	sql_stmt_tests/pointn12.testcase \
	sql_stmt_tests/pointn13.testcase \
	sql_stmt_tests/pointn13.testcase \
	sql_stmt_tests/pointn14.testcase \
	sql_stmt_tests/pointn14.testcase \
	sql_stmt_tests/pointn1.testcase \
	sql_stmt_tests/pointn1.testcase \
	sql_stmt_tests/pointn2.testcase \
	sql_stmt_tests/pointn2.testcase \
	sql_stmt_tests/pointn3.testcase \
	sql_stmt_tests/pointn3.testcase \
	sql_stmt_tests/pointn4.testcase \
	sql_stmt_tests/pointn4.testcase \
	sql_stmt_tests/pointn5.testcase \
	sql_stmt_tests/pointn5.testcase \
	sql_stmt_tests/pointn6.testcase \
	sql_stmt_tests/pointn6.testcase \
	sql_stmt_tests/pointn7.testcase \
	sql_stmt_tests/pointn7.testcase \
	sql_stmt_tests/pointn8.testcase \
	sql_stmt_tests/pointn8.testcase \
	sql_stmt_tests/pointn9.testcase \
	sql_stmt_tests/pointn9.testcase \
	sql_stmt_tests/polygonfromtext1.testcase \
	sql_stmt_tests/polygonfromtext2.testcase \
	sql_stmt_tests/polygonize1.testcase \
	sql_stmt_tests/polygonize2.testcase \
	sql_stmt_tests/polygonize3.testcase \
	sql_stmt_tests/polygonize4.testcase \
	sql_stmt_tests/pow00-1.testcase \
	sql_stmt_tests/pow00-2.testcase \
	sql_stmt_tests/pow00double.testcase \
	sql_stmt_tests/pow00.testcase \
	sql_stmt_tests/pow-badarg1.testcase \
	sql_stmt_tests/pow-badarg2.testcase \
	sql_stmt_tests/pow-edom.testcase \
	sql_stmt_tests/pow-fp.testcase \
	sql_stmt_tests/rad0.testcase \
	sql_stmt_tests/radpi.testcase \
	sql_stmt_tests/rad-text.testcase \
	sql_stmt_tests/reflectcoords1.testcase \
	sql_stmt_tests/reflectcoords2.testcase \
	sql_stmt_tests/reflectcoords3.testcase \
	sql_stmt_tests/reflectcoords4.testcase \
	sql_stmt_tests/reflectcoords5.testcase \
	sql_stmt_tests/reflectcoords6.testcase \
	sql_stmt_tests/reflectcoords7.testcase \
	sql_stmt_tests/reflectcoords8.testcase \
	sql_stmt_tests/relations1.testcase \
	sql_stmt_tests/relations2.testcase \
	sql_stmt_tests/relations3.testcase \
	sql_stmt_tests/relations4.testcase \
	sql_stmt_tests/relations5.testcase \
	sql_stmt_tests/relations6.testcase \
	sql_stmt_tests/ring10.testcase \
	sql_stmt_tests/ring11.testcase \
	sql_stmt_tests/ring12.testcase \
	sql_stmt_tests/ring13.testcase \
	sql_stmt_tests/ring14.testcase \
	sql_stmt_tests/ring15.testcase \
	sql_stmt_tests/ring16.testcase \
	sql_stmt_tests/ring17.testcase \
	sql_stmt_tests/ring18.testcase \
	sql_stmt_tests/ring19.testcase \
	sql_stmt_tests/ring1.testcase \
	sql_stmt_tests/ring20.testcase \
	sql_stmt_tests/ring21.testcase \
	sql_stmt_tests/ring22.testcase \
	sql_stmt_tests/ring23.testcase \
	sql_stmt_tests/ring24.testcase \
	sql_stmt_tests/ring25.testcase \
	sql_stmt_tests/ring26.testcase \
	sql_stmt_tests/ring27.testcase \
	sql_stmt_tests/ring2.testcase \
	sql_stmt_tests/ring3.testcase \
	sql_stmt_tests/ring4.testcase \
	sql_stmt_tests/ring5.testcase \
	sql_stmt_tests/ring6.testcase \
	sql_stmt_tests/ring7.testcase \
	sql_stmt_tests/ring8.testcase \
	sql_stmt_tests/ring9.testcase \
	sql_stmt_tests/rotatecoords10.testcase \
	sql_stmt_tests/rotatecoords11.testcase \
	sql_stmt_tests/rotatecoords12.testcase \
	sql_stmt_tests/rotatecoords13.testcase \
	sql_stmt_tests/rotatecoords14.testcase \
	sql_stmt_tests/rotatecoords1.testcase \
	sql_stmt_tests/rotatecoords2.testcase \
	sql_stmt_tests/rotatecoords3.testcase \
	sql_stmt_tests/rotatecoords4.testcase \
	sql_stmt_tests/rotatecoords5.testcase \
	sql_stmt_tests/rotatecoords6.testcase \
	sql_stmt_tests/rotatecoords7.testcase \
	sql_stmt_tests/rotatecoords8.testcase \
	sql_stmt_tests/rotatecoords9.testcase \
	sql_stmt_tests/round0.testcase \
	sql_stmt_tests/round-29.testcase \
	sql_stmt_tests/round29.testcase \
	sql_stmt_tests/round30.testcase \
	sql_stmt_tests/round-31.testcase \
	sql_stmt_tests/round31.testcase \
	sql_stmt_tests/round3.testcase \
	sql_stmt_tests/round-text.testcase \
	sql_stmt_tests/scalecoords10.testcase \
	sql_stmt_tests/scalecoords11.testcase \
	sql_stmt_tests/scalecoords12.testcase \
	sql_stmt_tests/scalecoords13.testcase \
	sql_stmt_tests/scalecoords1.testcase \
	sql_stmt_tests/scalecoords2.testcase \
	sql_stmt_tests/scalecoords3.testcase \
	sql_stmt_tests/scalecoords4.testcase \
	sql_stmt_tests/scalecoords5.testcase \
	sql_stmt_tests/scalecoords6.testcase \
	sql_stmt_tests/scalecoords7.testcase \
	sql_stmt_tests/scalecoords8.testcase \
	sql_stmt_tests/scalecoords9.testcase \
	sql_stmt_tests/shiftcoords1.testcase \
	sql_stmt_tests/shiftcoords2.testcase \
	sql_stmt_tests/shiftcoords3.testcase \
	sql_stmt_tests/shiftcoords4.testcase \
	sql_stmt_tests/shiftcoords5.testcase \
	sql_stmt_tests/shiftcoords6.testcase \
	sql_stmt_tests/shiftcoords7.testcase \
	sql_stmt_tests/sign00.testcase \
	sql_stmt_tests/sign01.testcase \
	sql_stmt_tests/sign0.testcase \
	sql_stmt_tests/sign10.testcase \
	sql_stmt_tests/signeg2int.testcase \
	sql_stmt_tests/signeg2.testcase \
	sql_stmt_tests/signeg.testcase \
	sql_stmt_tests/signtext.testcase \
	sql_stmt_tests/sin00.testcase \
	sql_stmt_tests/sin0.testcase \
	sql_stmt_tests/singlesidedbuffer11.testcase\
	sql_stmt_tests/singlesidedbuffer12.testcase\
	sql_stmt_tests/singlesidedbuffer13.testcase\
	sql_stmt_tests/singlesidedbuffer14.testcase\
	sql_stmt_tests/singlesidedbuffer1.testcase\
	sql_stmt_tests/singlesidedbuffer2.testcase\
	sql_stmt_tests/singlesidedbuffer3.testcase\
	sql_stmt_tests/singlesidedbuffer4.testcase\
	sql_stmt_tests/singlesidedbuffer5.testcase\
	sql_stmt_tests/singlesidedbuffer6.testcase\
	sql_stmt_tests/singlesidedbuffer7.testcase\
	sql_stmt_tests/singlesidedbuffer8.testcase\
	sql_stmt_tests/singlesidedbuffer9.testcase\
	sql_stmt_tests/sin-pi2.testcase \
	sql_stmt_tests/sinpi2.testcase \
	sql_stmt_tests/sin-pi.testcase \
	sql_stmt_tests/sinpi.testcase \
	sql_stmt_tests/sin-text.testcase \
	sql_stmt_tests/sqrt00.testcase \
	sql_stmt_tests/sqrt0.testcase \
	sql_stmt_tests/sqrt-10.testcase \
	sql_stmt_tests/sqrt-1.testcase \
	sql_stmt_tests/sqrt-640.testcase \
	sql_stmt_tests/sqrt-64.testcase \
	sql_stmt_tests/sqrt-text.testcase \
	sql_stmt_tests/st_area1.testcase \
	sql_stmt_tests/st_area2.testcase \
	sql_stmt_tests/st_area3.testcase \
	sql_stmt_tests/st_area4.testcase \
	sql_stmt_tests/st_area5.testcase \
	sql_stmt_tests/st_area6.testcase \
	sql_stmt_tests/st_area7.testcase \
	sql_stmt_tests/st_area8.testcase \
	sql_stmt_tests/startpoint1.testcase \
	sql_stmt_tests/startpoint1.testcase \
	sql_stmt_tests/startpoint2.testcase \
	sql_stmt_tests/startpoint2.testcase \
	sql_stmt_tests/startpoint3.testcase \
	sql_stmt_tests/startpoint3.testcase \
	sql_stmt_tests/stddev.testcase\
	sql_stmt_tests/st_length1.testcase \
	sql_stmt_tests/st_length2.testcase \
	sql_stmt_tests/st_length3.testcase \
	sql_stmt_tests/st_length4.testcase \
	sql_stmt_tests/st_length5.testcase \
	sql_stmt_tests/st_length6.testcase \
	sql_stmt_tests/st_length7.testcase \
	sql_stmt_tests/st_m1.testcase \
	sql_stmt_tests/st_m2.testcase \
	sql_stmt_tests/st_m3.testcase \
	sql_stmt_tests/st_m4.testcase \
	sql_stmt_tests/st_m5.testcase \
	sql_stmt_tests/st_m6.testcase \
	sql_stmt_tests/st_x1.testcase \
	sql_stmt_tests/st_x2.testcase \
	sql_stmt_tests/st_x3.testcase \
	sql_stmt_tests/st_x4.testcase \
	sql_stmt_tests/st_x5.testcase \
	sql_stmt_tests/st_x6.testcase \
	sql_stmt_tests/st_y1.testcase \
	sql_stmt_tests/st_y2.testcase \
	sql_stmt_tests/st_y3.testcase \
	sql_stmt_tests/st_y4.testcase \
	sql_stmt_tests/st_y5.testcase \
	sql_stmt_tests/st_z1.testcase \
	sql_stmt_tests/st_z2.testcase \
	sql_stmt_tests/st_z3.testcase \
	sql_stmt_tests/st_z4.testcase \
	sql_stmt_tests/st_z5.testcase \
	sql_stmt_tests/st_z6.testcase \
	sql_stmt_tests/swapcoords1.testcase \
	sql_stmt_tests/swapcoords2.testcase \
	sql_stmt_tests/swapcoords3.testcase \
	sql_stmt_tests/swapcoords4.testcase \
	sql_stmt_tests/tan00.testcase \
	sql_stmt_tests/tan0.testcase \
	sql_stmt_tests/tanpi4.testcase \
	sql_stmt_tests/tan-pi.testcase \
	sql_stmt_tests/tanpi.testcase \
	sql_stmt_tests/tan-text.testcase \
	sql_stmt_tests/testdb1.sqlite\
	sql_stmt_tests/trivial.sqlite_RO\
	sql_stmt_tests/unaryunion1.testcase\
	sql_stmt_tests/unaryunion2.testcase\
	sql_stmt_tests/unaryunion3.testcase\
	sql_stmt_tests/unaryunion4.testcase\
	sql_stmt_tests/unaryunion5.testcase\
	sql_stmt_tests/unaryunion6.testcase\
	sql_stmt_tests/unaryunion7.testcase\
	sql_stmt_tests/unaryunion8.testcase\
	sql_stmt_tests/us_ch_m.testcase \
	sql_stmt_tests/us_ft_m.testcase \
	sql_stmt_tests/us_in_m.testcase \
	sql_stmt_tests/us_mi_m.testcase \
	sql_stmt_tests/us_yd_m.testcase \
	sql_stmt_tests/variance.testcase\
	sql_stmt_tests/yd_m.testcase







