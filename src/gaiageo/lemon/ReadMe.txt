What is the LEMON parser ?
==========================
Please see: http://www.hwaci.com/sw/lemon/

in a very few words: the LEMON parser is
internally used by the SQLite's development
team, so using it for SpatiaLite as well
seems to be fully appropriate.


How to get the LEMON parser executable:
=======================================
you can get the latest LEMON sources consulting
the above URL: anyway, for the sake of simplicity,
you can directly found a copy of LEMON sources
into the "lemon_src" directory.

in order to get the LEMON executable you have
to build it by yourself: that's not at all difficult,
simply type:

cd lemon_src
gcc lemon.c -o lemon




The Vanuatu WKT parser:
=======================
1) in order to make any change to the  WKT parser
you have to edit first the definitions file
vanuatuWkt.y

2) then run:
lemon -l vanuatuWkt.y

3) during the above step the following files will be
generated:
vanuatuWkt.c [the C code implementing the parser]
vanuatuWkt.h [C header file]
vanuatuWkt.out [check file - useful for debugging]

3.1] vanuatuWkt.h
Please, copy the whole file content and
then replace [i.e. using cut&paste] the
correspondign section into "gg_vanuatu.c"
Search for "VANUATU_LEMON_H" in order to
identify the appropriate place where
to paste the generated code.

3.2] vanuatuWkt.c
Please, copy the whole file content and
replace [i.e. using cut&paste] the
correspondign section into "gg_vanuatu.c"
Search for "VANUATU_LEMON" in order to
identify the appropriate place where
to paste the generated code.




The EWKT parser:
================
1) in order to make any change to the  EWKT parser
you have to edit first the definitions file
Ewkt.y

2) then run:
lemon -l Ewkt.y

3) during the above step the following files will be
generated:
Ewkt.c [the C code implementing the parser]
Ewkt.h [C header file]
Ewkt.out [check file - useful for debugging]

3.1] Ewkt.h
Please, copy the whole file content and
then replace [i.e. using cut&paste] the
correspondign section into "gg_ewkt.c"
Search for "EWKT_LEMON_H" in order to 
identify the appropriate place where
to paste the generated code.

3.2] Ewkt.c
Please, copy the whole file content and
replace [i.e. using cut&paste] the
correspondign section into "gg_ewkt.c"
Search for "EWKT_LEMON" in order to 
identify the appropriate place where
to paste the generated code.




The GeoJSON parser:
================
1) in order to make any change to the  GeoJSON parser
you have to edit first the definitions file
geoJSON.y

2) then run:
lemon -l geoJSOM.y

3) during the above step the following files will be
generated:
geoJSON.c [the C code implementing the parser]
geoJSON.h [C header file]
geoJSON.out [check file - useful for debugging]

3.1] geoJSON.h
Please, copy the whole file content and
then replace [i.e. using cut&paste] the
correspondign section into "gg_geoJSON.c"
Search for "GEOJSON_LEMON_H" in order to 
identify the appropriate place where
to paste the generated code.

3.2] geoJSON.c
Please, copy the whole file content and
replace [i.e. using cut&paste] the
correspondign section into "gg_geoJSON.c"
Search for "GEOJSON_LEMON" in order to 
identify the appropriate place where
to paste the generated code.




The KML parser:
================
1) in order to make any change to the  KML parser
you have to edit first the definitions file
Kml.y

2) then run:
lemon -l Kml.y

3) during the above step the following files will be
generated:
Kml.c [the C code implementing the parser]
Kml.h [C header file]
Kml.out [check file - useful for debugging]

3.1] Kml.h
Please, copy the whole file content and
then replace [i.e. using cut&paste] the
correspondign section into "gg_kml.c"
Search for "KML_LEMON_H" in order to 
identify the appropriate place where
to paste the generated code.

3.2] Kml.c
Please, copy the whole file content and
replace [i.e. using cut&paste] the
correspondign section into "gg_kml.c"
Search for "KML_LEMON" in order to 
identify the appropriate place where
to paste the generated code.
