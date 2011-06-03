/* 
 kml.y -- KML parser - LEMON config
  
 version 2.4, 2011 June 2

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
 
Portions created by the Initial Developer are Copyright (C) 2011
the Initial Developer. All Rights Reserved.

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

// Tokens are void pointers (so we can cast them to whatever we want)
%token_type {void *}

// Output to stderr when stack overflows
%stack_overflow {
     fprintf(stderr,"Giving up.  Parser stack overflow\n");
}

// Increase this number if necessary
%stack_size 1000000

// Header files to be included in kml.c
%include {
}

// Set the return value of gaiaParseKml in the following pointer:
%extra_argument { gaiaGeomCollPtr *result }

// Invalid syntax (ie. no rules matched)
%syntax_error {
/* 
** when the LEMON parser encounters an error
** then this global variable is set 
*/
	kml_parse_error = 1;
	*result = NULL;
}
 
 /* This is to terminate with a new line */
 main ::= in.
 in ::= .
 in ::= in state KML_NEWLINE.
 
 state ::= program.
 
 /* 
 * program is the start node. All strings matched by this CFG must be one of 
 * geo_text (text describing a geometry)
 */
 
program ::= geo_text.

// geometries (2D):
geo_text ::= point(P). { *result = P; }				// P is a geometry collection containing a point
geo_text ::= linestring(L). { *result = L; }		// L is a geometry collection containing a linestring
geo_text ::= polygon(P). { *result = P; }		// P is a geometry collection containing a polygon
geo_text ::= geocoll(H). { *result = H; }		// H is a geometry collection created from user input

// Syntax for a "point" object:
// The functions called build a geometry collection from a gaiaPointPtr
point(P) ::= KML_START_POINT KML_START_COORDS point_coordxy(Q) KML_END_COORDS KML_END_POINT.
	{ P = kml_buildGeomFromPoint((gaiaPointPtr)Q); } // Point 

// Point coordinates in different dimensions.
// Create the point by calling the proper function in SpatiaLite :
point_coordxy(P) ::= coord(X) KML_COMMA coord(Y). 
	{ P = (void *) kml_point_xy((double *)X, (double *)Y); }

// All coordinates are assumed to be doubles (guaranteed by the flex tokenizer).
coord(A) ::= KML_NUM(B). { A = B; } 


// Rules to match an infinite number of points:
// Also links the generated gaiaPointPtrs together
extra_pointsxy(A) ::=  . { A = NULL; }
extra_pointsxy(A) ::= point_coordxy(P) extra_pointsxy(B).
	{ ((gaiaPointPtr)P)->Next = (gaiaPointPtr)B;  A = P; }


// Syntax for a "linestring" object:
// The functions called build a geometry collection from a gaiaLinestringPtr
linestring(L) ::= KML_START_LINESTRING KML_START_COORDS linestring_text(X) KML_END_COORDS KML_END_LINESTRING. 
	{ L = kml_buildGeomFromLinestring((gaiaLinestringPtr)X); }

// A valid linestring must have at least two vertices:
// The functions called build a gaiaLinestring from a linked list of points
linestring_text(L) ::= point_coordxy(P) point_coordxy(Q) extra_pointsxy(R).
	{ 
	   ((gaiaPointPtr)Q)->Next = (gaiaPointPtr)R; 
	   ((gaiaPointPtr)P)->Next = (gaiaPointPtr)Q;
	   L = (void *) kml_linestring_xy((gaiaPointPtr)P);
	}


// Syntax for a "polygon" object:
// The functions called build a geometry collection from a gaiaPolygonPtr
polygon(P) ::= KML_START_POLYGON polygon_text(X) KML_END_POLYGON.
	{ P = kml_buildGeomFromPolygon((gaiaPolygonPtr)X); }

// A valid polygon must have at least the outer ring:
// The functions called build a gaiaPolygonPtr from a linked list of gaiaRingPtrs
polygon_text(P) ::= outer_ring(R) inner_rings(E).
	{ 
		((gaiaRingPtr)R)->Next = (gaiaRingPtr)E;
		P = (void *) kml_polygon_xy((gaiaRingPtr)R);
	}

// A valid outer ring must have at least 4 points
// The functions called build a gaiaRingPtr from a linked list of gaiaPointPtrs
outer_ring(R) ::= KML_START_OUTER KML_START_RING KML_START_COORDS point_coordxy(A) point_coordxy(B) point_coordxy(C) point_coordxy(D) extra_pointsxy(E) KML_END_COORDS KML_END_RING KML_END_OUTER.
	{
		((gaiaPointPtr)A)->Next = (gaiaPointPtr)B; 
		((gaiaPointPtr)B)->Next = (gaiaPointPtr)C;
		((gaiaPointPtr)C)->Next = (gaiaPointPtr)D; 
		((gaiaPointPtr)D)->Next = (gaiaPointPtr)E;
		R = (void *) kml_ring_xy((gaiaPointPtr)A);
	}

// A valid inner ring must have at least 4 points
// The functions called build a gaiaRingPtr from a linked list of gaiaPointPtrs
inner_ring(R) ::= KML_START_INNER KML_START_RING KML_START_COORDS point_coordxy(A) point_coordxy(B) point_coordxy(C) point_coordxy(D) extra_pointsxy(E) KML_END_COORDS KML_END_RING KML_END_INNER.
	{
		((gaiaPointPtr)A)->Next = (gaiaPointPtr)B; 
		((gaiaPointPtr)B)->Next = (gaiaPointPtr)C;
		((gaiaPointPtr)C)->Next = (gaiaPointPtr)D; 
		((gaiaPointPtr)D)->Next = (gaiaPointPtr)E;
		R = (void *) kml_ring_xy((gaiaPointPtr)A);
	}

// To match more than one 2D ring:
inner_rings(R) ::=  . { R = NULL; }
inner_rings(R) ::= inner_ring(S) inner_rings(T).
	{
		((gaiaRingPtr)S)->Next = (gaiaRingPtr)T;
		R = S;
	}


// Syntax for a "geometrycollection" object:
// X in the following lines refers to a geometry collection generated based on user input
geocoll(G) ::= KML_START_MULTI geocoll_text(X) KML_END_MULTI. { G = X; }

// Geometry collections can contain any number of points, linestrings, or polygons (but at least one):
geocoll_text(G) ::= point(P) geocoll_text2(X).
	{ 
		((gaiaGeomCollPtr)P)->Next = (gaiaGeomCollPtr)X;
		G = (void *) kml_geomColl_xy((gaiaGeomCollPtr)P);
	}
	
geocoll_text(G) ::= linestring(L) geocoll_text2(X).
	{ 
		((gaiaGeomCollPtr)L)->Next = (gaiaGeomCollPtr)X;
		G = (void *) kml_geomColl_xy((gaiaGeomCollPtr)L);
	}
	
geocoll_text(G) ::= polygon(P) geocoll_text2(X).
	{ 
		((gaiaGeomCollPtr)P)->Next = (gaiaGeomCollPtr)X;
		G = (void *) kml_geomColl_xy((gaiaGeomCollPtr)P);
	}

// Extra points, linestrings, or polygons
geocoll_text2(X) ::=  . { X = NULL; }
geocoll_text2(X) ::= point(P) geocoll_text2(Y).
	{
		((gaiaGeomCollPtr)P)->Next = (gaiaGeomCollPtr)Y;
		X = P;
	}
	
geocoll_text2(X) ::= linestring(L) geocoll_text2(Y).
	{
		((gaiaGeomCollPtr)L)->Next = (gaiaGeomCollPtr)Y;
		X = L;
	}
	
geocoll_text2(X) ::= polygon(P) geocoll_text2(Y).
	{
		((gaiaGeomCollPtr)P)->Next = (gaiaGeomCollPtr)Y;
		X = P;
	}
