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
correspondign section into "gg_wkt.c"
Search for "VANUATU_LEMON_H" in order to 
identify the appropriate place where
to paste the generated code.

3.2] vanuatuWkt.c
Please, copy the whole file content and
then replace [i.e. using cut&paste] the
correspondign section into "gg_wkt.c"
Search for "VANUATU_LEMON" in order to 
identify the appropriate place where
to paste the generated code.



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
