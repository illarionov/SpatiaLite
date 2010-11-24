/* 
 vanuatuLexer.l -- Vanuatu WKT parser - FLEX config
  
 version 2.4, 2010 April 2

 Author: Sandro Furieri a.furieri@lqt.it

 ------------------------------------------------------------------------------
 
 Version: MPL 1.1/GPL 2.0/LGPL 2.1
 
 The contents of this file are subject to the Mozilla Public License Version
 1.1 (the "License"); you may not use this file except in compliance with
 the License. You may obtain a copy of the License at
 http://www.mozilla.org/MPL/
 
Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
for the specific language governing rights and limitations under the
License.

The Original Code is the SpatiaLite library

The Initial Developer of the Original Code is Alessandro Furieri
 
Portions created by the Initial Developer are Copyright (C) 2008
the Initial Developer. All Rights Reserved.

Contributor(s):
The Vanuatu Team - University of Toronto

Alternatively, the contents of this file may be used under the terms of
either the GNU General Public License Version 2 or later (the "GPL"), or
the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
in which case the provisions of the GPL or the LGPL are applicable instead
of those above. If you wish to allow use of your version of this file only
under the terms of either the GPL or the LGPL, and not to allow others to
use your version of this file under the terms of the MPL, indicate your
decision by deleting the provisions above and replace them with the notice
and other provisions required by the GPL or the LGPL. If you do not delete
the provisions above, a recipient may use your version of this file under
the terms of any one of the MPL, the GPL or the LGPL.
 
*/

/******************************************************************************
** The following code was created by Team Vanuatu of The University of Toronto.

Authors:
Ruppi Rana			ruppi.rana@gmail.com
Dev Tanna			dev.tanna@gmail.com
Elias Adum			elias.adum@gmail.com
Benton Hui			benton.hui@gmail.com
Abhayan Sundararajan		abhayan@gmail.com
Chee-Lun Michael Stephen Cho	cheelun.cho@gmail.com
Nikola Banovic			nikola.banovic@gmail.com
Yong Jian			yong.jian@utoronto.ca

Supervisor:
Greg Wilson			gvwilson@cs.toronto.ca

-------------------------------------------------------------------------------
*/

%option prefix="VanuatuWkt"

%{

/* For debugging purposes */
int line = 1, col = 1;

%}

/**
*  The main string-token matcher.
*  The lower case part is probably not needed.  We should really be converting 
*  The string to all uppercase/lowercase to make it case iNsEnSiTiVe.
*  What Flex will do is, For the input string, beginning from the front, Flex
*  will try to match with any of the defined tokens from below.  Flex will 
*  then match the string of longest length.  Suppose the string is: POINT ZM,
*  Flex would match both POINT Z and POINT ZM, but since POINT ZM is the longer
*  of the two tokens, FLEX will match POINT ZM.
*/
%%
-?[0-9]+|-?[0-9]+"."[0-9]*|"+"?[0-9]+|"+"?[0-9]+"."[0-9]*       	{ col += (int) strlen(yytext);  VanuatuWktlval.dval = atof(yytext); return VANUATU_NUM; }
","										{ VanuatuWktlval.dval = 0; return VANUATU_COMMA; }
"("										{ VanuatuWktlval.dval = 0; return VANUATU_OPEN_BRACKET; }
")"										{ VanuatuWktlval.dval = 0; return VANUATU_CLOSE_BRACKET; }
(?i:"point")								{ VanuatuWktlval.dval = 0; return VANUATU_POINT; }
(?i:"point"[ ]?"z")							{ VanuatuWktlval.dval = 0; return VANUATU_POINT_Z; }
(?i:"point"[ ]?"m")							{ VanuatuWktlval.dval = 0; return VANUATU_POINT_M; }
(?i:"point"[ ]?"zm")							{ VanuatuWktlval.dval = 0; return VANUATU_POINT_ZM; }
(?i:"linestring")							{ VanuatuWktlval.dval = 0; return VANUATU_LINESTRING; }
(?i:"linestring"[ ]?"z")						{ VanuatuWktlval.dval = 0; return VANUATU_LINESTRING_Z; }
(?i:"linestring"[ ]?"m")				{ VanuatuWktlval.dval = 0; return VANUATU_LINESTRING_M; }
(?i:"linestring"[ ]?"zm")				{ VanuatuWktlval.dval = 0; return VANUATU_LINESTRING_ZM; }
(?i:"polygon")								{ VanuatuWktlval.dval = 0; return VANUATU_POLYGON; }
(?i:"polygon"[ ]?"z")					{ VanuatuWktlval.dval = 0; return VANUATU_POLYGON_Z; }
(?i:"polygon"[ ]?"m")					{ VanuatuWktlval.dval = 0; return VANUATU_POLYGON_M; }
(?i:"polygon"[ ]?"zm")				{ VanuatuWktlval.dval = 0; return VANUATU_POLYGON_ZM; }
(?i:"multipoint")							{ VanuatuWktlval.dval = 0; return VANUATU_MULTIPOINT; }
(?i:"multipoint"[ ]?"z")				{ VanuatuWktlval.dval = 0; return VANUATU_MULTIPOINT_Z; }
(?i:"multipoint"[ ]?"m")				{ VanuatuWktlval.dval = 0; return VANUATU_MULTIPOINT_M; }
(?i:"multipoint"[ ]?"zm")			{ VanuatuWktlval.dval = 0; return VANUATU_MULTIPOINT_ZM; }
(?i:"multilinestring")						{ VanuatuWktlval.dval = 0; return VANUATU_MULTILINESTRING; }
(?i:"multilinestring"[ ]?"z")		{ VanuatuWktlval.dval = 0; return VANUATU_MULTILINESTRING_Z; }
(?i:"multilinestring"[ ]?"m")		{ VanuatuWktlval.dval = 0; return VANUATU_MULTILINESTRING_M; }
(?i:"multilinestring"[ ]?"zm")		{ VanuatuWktlval.dval = 0; return VANUATU_MULTILINESTRING_ZM; }	
(?i:"multipolygon")							{ VanuatuWktlval.dval = 0; return VANUATU_MULTIPOLYGON; }
(?i:"multipolygon"[ ]?"z")			{ VanuatuWktlval.dval = 0; return VANUATU_MULTIPOLYGON_Z; }
(?i:"multipolygon"[ ]?"m")			{ VanuatuWktlval.dval = 0; return VANUATU_MULTIPOLYGON_M; }
(?i:"multipolygon"[ ]?"zm")		{ VanuatuWktlval.dval = 0; return VANUATU_MULTIPOLYGON_ZM; }
(?i:"geometrycollection")					{ VanuatuWktlval.dval = 0; return VANUATU_GEOMETRYCOLLECTION; }
(?i:"geometrycollection"[ ]?"z")	{ VanuatuWktlval.dval = 0; return VANUATU_GEOMETRYCOLLECTION_Z; }
(?i:"geometrycollection"[ ]?"m")	{ VanuatuWktlval.dval = 0; return VANUATU_GEOMETRYCOLLECTION_M; }
(?i:"geometrycollection"[ ]?"zm")	{ VanuatuWktlval.dval = 0; return VANUATU_GEOMETRYCOLLECTION_ZM; }

[ \t]   { col += (int) strlen(yytext); }               /* ignore but count white space */

\n      { col = 0; ++line; return VANUATU_NEWLINE; }

.       { col += (int) strlen(yytext); return -1; }
%%
/**
 * reset the line and column count
 *
 *
 */
void reset_lexer(void)
{

  line = 1;
  col  = 1;

}

/**
 * VanuatuWkterror() is invoked when the lexer or the parser encounter
 * an error. The error message is passed via *s
 *
 *
 */
void VanuatuWkterror(char *s)
{
  printf("error: %s at line: %d col: %d\n",s,line,col);

}

int yywrap(void)
{
  return 1;
}

/******************************************************************************
** This is the end of the code that was created by Team Vanuatu
** of The University of Toronto.

Authors:
Ruppi Rana			ruppi.rana@gmail.com
Dev Tanna			dev.tanna@gmail.com
Elias Adum			elias.adum@gmail.com
Benton Hui			benton.hui@gmail.com
Abhayan Sundararajan		abhayan@gmail.com
Chee-Lun Michael Stephen Cho	cheelun.cho@gmail.com
Nikola Banovic			nikola.banovic@gmail.com
Yong Jian			yong.jian@utoronto.ca

Supervisor:
Greg Wilson			gvwilson@cs.toronto.ca

-------------------------------------------------------------------------------
*/
