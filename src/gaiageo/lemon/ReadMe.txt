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
First some manual adjustement is required.
Open the source file, applying the following
substitutions:
ParseAlloc            -> vanuatuParseAlloc
ParseFree             -> vanuatuParseFree
ParseStackPeak        -> vanuatuParseStackPeak
Parse                 -> vanuatuParse
YYMINORTYPE           -> VANUATU_YYMINORTYPE
YY_CHAR               -> VANUATU_YY_CHAR
yyzerominor           -> vanuatu_yyzerominor
yy_action             -> vanuatu_yy_action
yy_lookahead          -> vanuatu_yy_lookahead
yy_shift_ofst         -> vanuatu_yy_shift_ofst
yy_reduce_ofst        -> vanuatu_yy_reduce_ofst
yy_default            -> vanuatu_yy_default
yyStackEntry          -> vanuatu_yyStackEntry
yyParser              -> vanuatu_yyParser
yy_destructor         -> vanuatu_yy_destructor
yy_pop_parser_stack   -> vanuatu_yy_pop_parser_stack
yy_find_shift_action  -> vanuatu_yy_find_shift_action
yy_find_reduce_action -> vanuatu_yy_find_reduce_action
yyStackOverflow       -> vanuatu_yyStackOverflow
yy_shift              -> vanuatu_yy_shift
yyRuleInfo            -> vanuatu_yyRuleInfo
yy_reduce             -> vanuatu_yy_reduce
yy_parse_failed       -> vanuatu_yy_parse_failed
yy_syntax_error       -> vanuatu_yy_syntax_error
yy_buffer_stack_top   -> vanuatu_yy_buffer_stack_top
yy_buffer_stack_max   -> vanuatu_yy_buffer_stack_max
yy_buffer_stack       -> vanuatu_yy_buffer_stack
yy_c_buf_p            -> vanuatu_yy_c_buf_p
yy_init               -> vanuatu_yy_init
yy_start              -> vanuatu_yy_start
yy_state_type         -> vanuatu_yy_state_type
yy_trans_info         -> vanuatu_yy_trans_info
yy_accept             -> vanuatu_yy_accept
yy_ec                 -> vanuatu_yy_ec
yy_meta               -> vanuatu_yy_meta
yy_base               -> vanuatu_yy_base
yy_def                -> vanuatu_yy_def
yy_nxt                -> vanuatu_yy_nxt
yy_chk                -> vanuatu_yy_chk
yy_fatal_error        -> vanuatu_yy_fatal_error
yy_init_globals       -> vanuatu_yy_init_globals
yy_get_next_buffer    -> vanuatu_yy_get_next_buffer
yy_get_previous_state -> vanuatu_yy_get_previous_state
yy_try_NUL_trans      -> vanuatu_yy_try_NUL_trans
yyunput               -> vanuatu_yyunput
input                 -> vanuatu_input

Then copy the whole file content and
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
First some manual adjustement is required.
Open the source file, applying the following
substitutions:
ParseAlloc            -> ewktParseAlloc
ParseFree             -> ewktParseFree
ParseStackPeak        -> ewktParseStackPeak
Parse                 -> ewktParse
YYMINORTYPE           -> EWKT_YYMINORTYPE
YY_CHAR               -> EWKT_YY_CHAR
yyzerominor           -> ewkt_yyzerominor
yy_action             -> ewkt_yy_action
yy_lookahead          -> ewkt_yy_lookahead
yy_shift_ofst         -> ewkt_yy_shift_ofst
yy_reduce_ofst        -> ewkt_yy_reduce_ofst
yy_default            -> ewkt_yy_default
yyStackEntry          -> ewkt_yyStackEntry
yyParser              -> ewkt_yyParser
yy_destructor         -> ewkt_yy_destructor
yy_pop_parser_stack   -> ewkt_yy_pop_parser_stack
yy_find_shift_action  -> ewkt_yy_find_shift_action
yy_find_reduce_action -> ewkt_yy_find_reduce_action
yyStackOverflow       -> ewkt_yyStackOverflow
yy_shift              -> ewkt_yy_shift
yyRuleInfo            -> ewkt_yyRuleInfo
yy_reduce             -> ewkt_yy_reduce
yy_parse_failed       -> ewkt_yy_parse_failed
yy_syntax_error       -> ewkt_yy_syntax_error
yy_buffer_stack_top   -> ewkt_yy_buffer_stack_top
yy_buffer_stack_max   -> ewkt_yy_buffer_stack_max
yy_buffer_stack       -> ewkt_yy_buffer_stack
yy_c_buf_p            -> ewkt_yy_c_buf_p
yy_init               -> ewkt_yy_init
yy_start              -> ewkt_yy_start
yy_state_type         -> ewkt_yy_state_type
yy_trans_info         -> ewkt_yy_trans_info
yy_accept             -> ewkt_yy_accept
yy_ec                 -> ewkt_yy_ec
yy_meta               -> ewkt_yy_meta
yy_base               -> ewkt_yy_base
yy_def                -> ewkt_yy_def
yy_nxt                -> ewkt_yy_nxt
yy_chk                -> ewkt_yy_chk
yy_fatal_error        -> ewkt_yy_fatal_error
yy_init_globals       -> ewkt_yy_init_globals
yy_get_next_buffer    -> ewkt_yy_get_next_buffer
yy_get_previous_state -> ewkt_yy_get_previous_state
yy_try_NUL_trans      -> ewkt_yy_try_NUL_trans
yyunput               -> ewkt_yyunput
input                 -> ewkt_input

Then copy the whole file content and
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
First some manual adjustement is required.
Open the source file, applying the following
substitutions:
ParseAlloc            -> geoJSONParseAlloc
ParseFree             -> geoJSONParseFree
ParseStackPeak        -> geoJSONParseStackPeak
Parse                 -> geoJSONParse
YYMINORTYPE           -> GEOJSON_YYMINORTYPE
YY_CHAR               -> GEOJSON_YY_CHAR
yyzerominor           -> geoJSON_yyzerominor
yy_action             -> geoJSON_yy_action
yy_lookahead          -> geoJSON_yy_lookahead
yy_shift_ofst         -> geoJSON_yy_shift_ofst
yy_reduce_ofst        -> geoJSON_yy_reduce_ofst
yy_default            -> geoJSON_yy_default
yyStackEntry          -> geoJSON_yyStackEntry
yyParser              -> geoJSON_yyParser
yy_destructor         -> geoJSON_yy_destructor
yy_pop_parser_stack   -> geoJSON_yy_pop_parser_stack
yy_find_shift_action  -> geoJSON_yy_find_shift_action
yy_find_reduce_action -> geoJSON_yy_find_reduce_action
yyStackOverflow       -> geoJSON_yyStackOverflow
yy_shift              -> geoJSON_yy_shift
yyRuleInfo            -> geoJSON_yyRuleInfo
yy_reduce             -> geoJSON_yy_reduce
yy_parse_failed       -> geoJSON_yy_parse_failed
yy_syntax_error       -> geoJSON_yy_syntax_error
yy_buffer_stack_top   -> geoJSON_yy_buffer_stack_top
yy_buffer_stack_max   -> geoJSON_yy_buffer_stack_max
yy_buffer_stack       -> geoJSON_yy_buffer_stack
yy_c_buf_p            -> geoJSON_yy_c_buf_p
yy_init               -> geoJSON_yy_init
yy_start              -> geoJSON_yy_start
yy_state_type         -> geoJSON_yy_state_type
yy_trans_info         -> geoJSON_yy_trans_info
yy_accept             -> geoJSON_yy_accept
yy_ec                 -> geoJSON_yy_ec
yy_meta               -> geoJSON_yy_meta
yy_base               -> geoJSON_yy_base
yy_def                -> geoJSON_yy_def
yy_nxt                -> geoJSON_yy_nxt
yy_chk                -> geoJSON_yy_chk
yy_fatal_error        -> geoJSON_yy_fatal_error
yy_init_globals       -> geoJSON_yy_init_globals
yy_get_next_buffer    -> geoJSON_yy_get_next_buffer
yy_get_previous_state -> geoJSON_yy_get_previous_state
yy_try_NUL_trans      -> geoJSON_yy_try_NUL_trans
yyunput               -> geoJSON_yyunput
input                 -> geoJSON_input

Then copy the whole file content and
replace [i.e. using cut&paste] the
correspondign section into "gg_geoJSON.c"
Search for "GEOJSON_LEMON" in order to 
identify the appropriate place where
to paste the generated code.



