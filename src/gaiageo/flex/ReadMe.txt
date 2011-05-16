The Vanuatu WKT lexer:
======================
1) in order to make any change to the lexer
you have to edit first the definitions file
vanuatuLexer.l

2) then run:
flex -L vanuatuLexer.l

3) a source file named "lex.VanuatuWkt.c" will be
generated during the above step.
Please, copy the whole file content and
then replace [i.e. using cut&paste] the
correspondign section into "gg_vanuatu.c"
Search for "VANUATU_FLEX" in order to
identify the appropriate place where
to paste the generated code.




The EWKT lexer:
================
1) in order to make any change to the lexer
you have to edit first the definitions file
ewktLexer.l

2) then run:
flex -L ewktLexer.l

3) a source file named "lex.Ewkt.c" will be
generated during the above step.
Please, copy the whole file content and
then replace [i.e. using cut&paste] the
correspondign section into "gg_ewkt.c"
Search for "EWKT_FLEX" in order to 
identify the appropriate place where
to paste the generated code.
