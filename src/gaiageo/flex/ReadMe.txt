1) in order to make any change to the lexer
you have to edit first the definitions file
vanuatuLexer.l

2) then run:
flex -L vanuatuLexer.l

3) a source file named "lex.yy.c" will be
generated during the above step.
Please, copy the whole file content and
then replace [i.e. using cut&paste] the
correspondign section into "gg_wkt.c"
Search for "VANUATU_FLEX" in order to 
identify the appropriate place where
to paste the generated code.
