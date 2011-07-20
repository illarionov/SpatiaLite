/*

 gg_ewkt.c -- EWKT parser/lexer 
  
 version 3.0, 2011 July 20

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

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <assert.h>

#ifdef SPL_AMALGAMATION		/* spatialite-amalgamation */
#include <spatialite/sqlite3ext.h>
#else
#include <sqlite3ext.h>
#endif

#include <spatialite/gaiageo.h>

#ifdef _WIN32
#define strcasecmp	_stricmp
#define strncasecmp	_strnicmp
#define atoll	_atoi64
#endif /* not WIN32 */

int ewkt_parse_error;

static int
ewktCheckValidity (gaiaGeomCollPtr geom)
{
/* checks if this one is a degenerated geometry */
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    int ib;
    int entities = 0;
    pt = geom->FirstPoint;
    while (pt)
      {
	  /* checking points */
	  entities++;
	  pt = pt->Next;
      }
    ln = geom->FirstLinestring;
    while (ln)
      {
	  /* checking linestrings */
	  if (ln->Points < 2)
	      return 0;
	  entities++;
	  ln = ln->Next;
      }
    pg = geom->FirstPolygon;
    while (pg)
      {
	  /* checking polygons */
	  rng = pg->Exterior;
	  if (rng->Points < 4)
	      return 0;
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		rng = pg->Interiors + ib;
		if (rng->Points < 4)
		    return 0;
	    }
	  entities++;
	  pg = pg->Next;
      }
    if (!entities)
	return 0;
    return 1;
}

static gaiaGeomCollPtr
gaiaEwktGeometryFromPoint (gaiaPointPtr point)
{
/* builds a GEOMETRY containing a POINT */
    gaiaGeomCollPtr geom = NULL;
    geom = gaiaAllocGeomColl ();
    geom->DeclaredType = GAIA_POINT;
    gaiaAddPointToGeomColl (geom, point->X, point->Y);
    gaiaFreePoint (point);
    return geom;
}

static gaiaGeomCollPtr
gaiaEwktGeometryFromPointZ (gaiaPointPtr point)
{
/* builds a GEOMETRY containing a POINTZ */
    gaiaGeomCollPtr geom = NULL;
    geom = gaiaAllocGeomCollXYZ ();
    geom->DeclaredType = GAIA_POINTZ;
    gaiaAddPointToGeomCollXYZ (geom, point->X, point->Y, point->Z);
    gaiaFreePoint (point);
    return geom;
}

static gaiaGeomCollPtr
gaiaEwktGeometryFromPointM (gaiaPointPtr point)
{
/* builds a GEOMETRY containing a POINTM */
    gaiaGeomCollPtr geom = NULL;
    geom = gaiaAllocGeomCollXYM ();
    geom->DeclaredType = GAIA_POINTM;
    gaiaAddPointToGeomCollXYM (geom, point->X, point->Y, point->M);
    gaiaFreePoint (point);
    return geom;
}

static gaiaGeomCollPtr
gaiaEwktGeometryFromPointZM (gaiaPointPtr point)
{
/* builds a GEOMETRY containing a POINTZM */
    gaiaGeomCollPtr geom = NULL;
    geom = gaiaAllocGeomCollXYZM ();
    geom->DeclaredType = GAIA_POINTZM;
    gaiaAddPointToGeomCollXYZM (geom, point->X, point->Y, point->Z, point->M);
    gaiaFreePoint (point);
    return geom;
}

static gaiaGeomCollPtr
gaiaEwktGeometryFromLinestring (gaiaLinestringPtr line)
{
/* builds a GEOMETRY containing a LINESTRING */
    gaiaGeomCollPtr geom = NULL;
    gaiaLinestringPtr line2;
    int iv;
    double x;
    double y;
    geom = gaiaAllocGeomColl ();
    geom->DeclaredType = GAIA_LINESTRING;
    line2 = gaiaAddLinestringToGeomColl (geom, line->Points);
    for (iv = 0; iv < line2->Points; iv++)
      {
	  /* sets the POINTS for the exterior ring */
	  gaiaGetPoint (line->Coords, iv, &x, &y);
	  gaiaSetPoint (line2->Coords, iv, x, y);
      }
    gaiaFreeLinestring (line);
    return geom;
}

static gaiaGeomCollPtr
gaiaEwktGeometryFromLinestringZ (gaiaLinestringPtr line)
{
/* builds a GEOMETRY containing a LINESTRINGZ */
    gaiaGeomCollPtr geom = NULL;
    gaiaLinestringPtr line2;
    int iv;
    double x;
    double y;
    double z;
    geom = gaiaAllocGeomCollXYZ ();
    geom->DeclaredType = GAIA_LINESTRING;
    line2 = gaiaAddLinestringToGeomColl (geom, line->Points);
    for (iv = 0; iv < line2->Points; iv++)
      {
	  /* sets the POINTS for the exterior ring */
	  gaiaGetPointXYZ (line->Coords, iv, &x, &y, &z);
	  gaiaSetPointXYZ (line2->Coords, iv, x, y, z);
      }
    gaiaFreeLinestring (line);
    return geom;
}


static gaiaGeomCollPtr
gaiaEwktGeometryFromLinestringM (gaiaLinestringPtr line)
{
/* builds a GEOMETRY containing a LINESTRINGM */
    gaiaGeomCollPtr geom = NULL;
    gaiaLinestringPtr line2;
    int iv;
    double x;
    double y;
    double m;
    geom = gaiaAllocGeomCollXYM ();
    geom->DeclaredType = GAIA_LINESTRING;
    line2 = gaiaAddLinestringToGeomColl (geom, line->Points);
    for (iv = 0; iv < line2->Points; iv++)
      {
	  /* sets the POINTS for the exterior ring */
	  gaiaGetPointXYM (line->Coords, iv, &x, &y, &m);
	  gaiaSetPointXYM (line2->Coords, iv, x, y, m);
      }
    gaiaFreeLinestring (line);
    return geom;
}

static gaiaGeomCollPtr
gaiaEwktGeometryFromLinestringZM (gaiaLinestringPtr line)
{
/* builds a GEOMETRY containing a LINESTRINGZM */
    gaiaGeomCollPtr geom = NULL;
    gaiaLinestringPtr line2;
    int iv;
    double x;
    double y;
    double z;
    double m;
    geom = gaiaAllocGeomCollXYZM ();
    geom->DeclaredType = GAIA_LINESTRING;
    line2 = gaiaAddLinestringToGeomColl (geom, line->Points);
    for (iv = 0; iv < line2->Points; iv++)
      {
	  /* sets the POINTS for the exterior ring */
	  gaiaGetPointXYZM (line->Coords, iv, &x, &y, &z, &m);
	  gaiaSetPointXYZM (line2->Coords, iv, x, y, z, m);
      }
    gaiaFreeLinestring (line);
    return geom;
}

static gaiaPointPtr
ewkt_point_xy (double *x, double *y)
{
    return gaiaAllocPoint (*x, *y);
}

/* 
 * Creates a 3D (xyz) point in SpatiaLite
 * x, y, and z are pointers to doubles which represent the x, y, and z coordinates of the point to be created.
 * Returns a gaiaPointPtr representing the created point.
 *
 * Creates a 3D (xyz) point. This is a parser helper function which is called when 3D coordinates are encountered.
 * Parameters x, y, and z are pointers to doubles which represent the x, y, and z coordinates of the point to be created.
 * Returns a gaiaPointPtr pointing to the 3D created point.
 */
static gaiaPointPtr
ewkt_point_xyz (double *x, double *y, double *z)
{
    return gaiaAllocPointXYZ (*x, *y, *z);
}

/* 
 * Creates a 2D (xy) point with an m value which is a part of the linear reference system. This is a parser helper
 * function which is called when 2D   *coordinates with an m value are encountered.
 * Parameters x and y are pointers to doubles which represent the x and y coordinates of the point to be created.
 * Parameter m is a pointer to a double which represents the part of the linear reference system.
 * Returns a gaiaPointPtr pointing to the created 2D point with an m value.
 */
static gaiaPointPtr
ewkt_point_xym (double *x, double *y, double *m)
{
    return gaiaAllocPointXYM (*x, *y, *m);
}

/* 
 * Creates a 4D (xyz) point with an m value which is a part of the linear reference system. This is a parser helper
 * function which is called when  *4Dcoordinates with an m value are encountered
 * Parameters x, y, and z are pointers to doubles which represent the x, y, and z coordinates of the point to be created. 
 * Parameter m is a pointer to a double which represents the part of the linear reference system.
 * Returns a gaiaPointPtr pointing the created 4D point with an m value.
 */
gaiaPointPtr
ewkt_point_xyzm (double *x, double *y, double *z, double *m)
{
    return gaiaAllocPointXYZM (*x, *y, *z, *m);
}

/*
 * Builds a geometry collection from a point. The geometry collection should contain only one element ? the point. 
 * The correct geometry type must be *decided based on the point type. The parser should call this function when the 
 * ?POINT? WKT expression is encountered.
 * Parameter point is a pointer to a 2D, 3D, 2D with an m value, or 4D with an m value point.
 * Returns a geometry collection containing the point. The geometry must have FirstPoint and LastPoint  pointing to the
 * same place as point.  *DimensionModel must be the same as the model of the point and DimensionType must be GAIA_TYPE_POINT.
 */
static gaiaGeomCollPtr
ewkt_buildGeomFromPoint (gaiaPointPtr point)
{
    switch (point->DimensionModel)
      {
      case GAIA_XY:
	  return gaiaEwktGeometryFromPoint (point);
	  break;
      case GAIA_XY_Z:
	  return gaiaEwktGeometryFromPointZ (point);
	  break;
      case GAIA_XY_M:
	  return gaiaEwktGeometryFromPointM (point);
	  break;
      case GAIA_XY_Z_M:
	  return gaiaEwktGeometryFromPointZM (point);
	  break;
      }
    return NULL;
}

/* 
 * Creates a 2D (xy) linestring from a list of 2D points.
 *
 * Parameter first is a gaiaPointPtr to the first point in a linked list of points which define the linestring.
 * All of the points in the list must be 2D (xy) points. There must be at least 2 points in the list.
 *
 * Returns a pointer to linestring containing all of the points in the list.
 */
static gaiaLinestringPtr
ewkt_linestring_xy (gaiaPointPtr first)
{
    gaiaPointPtr p = first;
    gaiaPointPtr p_n;
    int points = 0;
    int i = 0;
    gaiaLinestringPtr linestring;

    while (p != NULL)
      {
	  p = p->Next;
	  points++;
      }

    linestring = gaiaAllocLinestring (points);

    p = first;
    while (p != NULL)
      {
	  gaiaSetPoint (linestring->Coords, i, p->X, p->Y);
	  p_n = p->Next;
	  gaiaFreePoint (p);
	  p = p_n;
	  i++;
      }

    return linestring;
}

/* 
 * Creates a 3D (xyz) linestring from a list of 3D points.
 *
 * Parameter first is a gaiaPointPtr to the first point in a linked list of points which define the linestring.
 * All of the points in the list must be 3D (xyz) points. There must be at least 2 points in the list.
 *
 * Returns a pointer to linestring containing all of the points in the list.
 */
static gaiaLinestringPtr
ewkt_linestring_xyz (gaiaPointPtr first)
{
    gaiaPointPtr p = first;
    gaiaPointPtr p_n;
    int points = 0;
    int i = 0;
    gaiaLinestringPtr linestring;

    while (p != NULL)
      {
	  p = p->Next;
	  points++;
      }

    linestring = gaiaAllocLinestringXYZ (points);

    p = first;
    while (p != NULL)
      {
	  gaiaSetPointXYZ (linestring->Coords, i, p->X, p->Y, p->Z);
	  p_n = p->Next;
	  gaiaFreePoint (p);
	  p = p_n;
	  i++;
      }

    return linestring;
}

/* 
 * Creates a 2D (xy) with m value linestring from a list of 2D with m value points.
 *
 * Parameter first is a gaiaPointPtr to the first point in a linked list of points which define the linestring.
 * All of the points in the list must be 2D (xy) with m value points. There must be at least 2 points in the list.
 *
 * Returns a pointer to linestring containing all of the points in the list.
 */
static gaiaLinestringPtr
ewkt_linestring_xym (gaiaPointPtr first)
{
    gaiaPointPtr p = first;
    gaiaPointPtr p_n;
    int points = 0;
    int i = 0;
    gaiaLinestringPtr linestring;

    while (p != NULL)
      {
	  p = p->Next;
	  points++;
      }

    linestring = gaiaAllocLinestringXYM (points);

    p = first;
    while (p != NULL)
      {
	  gaiaSetPointXYM (linestring->Coords, i, p->X, p->Y, p->M);
	  p_n = p->Next;
	  gaiaFreePoint (p);
	  p = p_n;
	  i++;
      }

    return linestring;
}

/* 
 * Creates a 4D (xyz) with m value linestring from a list of 4D with m value points.
 *
 * Parameter first is a gaiaPointPtr to the first point in a linked list of points which define the linestring.
 * All of the points in the list must be 4D (xyz) with m value points. There must be at least 2 points in the list.
 *
 * Returns a pointer to linestring containing all of the points in the list.
 */
static gaiaLinestringPtr
ewkt_linestring_xyzm (gaiaPointPtr first)
{
    gaiaPointPtr p = first;
    gaiaPointPtr p_n;
    int points = 0;
    int i = 0;
    gaiaLinestringPtr linestring;

    while (p != NULL)
      {
	  p = p->Next;
	  points++;
      }

    linestring = gaiaAllocLinestringXYZM (points);

    p = first;
    while (p != NULL)
      {
	  gaiaSetPointXYZM (linestring->Coords, i, p->X, p->Y, p->Z, p->M);
	  p_n = p->Next;
	  gaiaFreePoint (p);
	  p = p_n;
	  i++;
      }

    return linestring;
}

/*
 * Builds a geometry collection from a linestring.
 */
static gaiaGeomCollPtr
ewkt_buildGeomFromLinestring (gaiaLinestringPtr line)
{
    switch (line->DimensionModel)
      {
      case GAIA_XY:
	  return gaiaEwktGeometryFromLinestring (line);
	  break;
      case GAIA_XY_Z:
	  return gaiaEwktGeometryFromLinestringZ (line);
	  break;
      case GAIA_XY_M:
	  return gaiaEwktGeometryFromLinestringM (line);
	  break;
      case GAIA_XY_Z_M:
	  return gaiaEwktGeometryFromLinestringZM (line);
	  break;
      }
    return NULL;
}

/*
 * Helper function that determines the number of points in the linked list.
 */
static int
ewkt_count_points (gaiaPointPtr first)
{
    /* Counts the number of points in the ring. */
    gaiaPointPtr p = first;
    int numpoints = 0;
    while (p != NULL)
      {
	  numpoints++;
	  p = p->Next;
      }
    return numpoints;
}

/*
 * Creates a 2D (xy) ring in SpatiaLite
 *
 * first is a gaiaPointPtr to the first point in a linked list of points which define the polygon.
 * All of the points given to the function are 2D (xy) points. There will be at least 4 points in the list.
 *
 * Returns the ring defined by the points given to the function.
 */
static gaiaRingPtr
ewkt_ring_xy (gaiaPointPtr first)
{
    gaiaPointPtr p = first;
    gaiaPointPtr p_n;
    gaiaRingPtr ring = NULL;
    int numpoints;
    int index;

    /* If no pointers are given, return. */
    if (first == NULL)
	return NULL;

    /* Counts the number of points in the ring. */
    numpoints = ewkt_count_points (first);
    if (numpoints < 4)
	return NULL;

    /* Creates and allocates a ring structure. */
    ring = gaiaAllocRing (numpoints);
    if (ring == NULL)
	return NULL;

    /* Adds every point into the ring structure. */
    p = first;
    for (index = 0; index < numpoints; index++)
      {
	  gaiaSetPoint (ring->Coords, index, p->X, p->Y);
	  p_n = p->Next;
	  gaiaFreePoint (p);
	  p = p_n;
      }

    return ring;
}

/*
 * Creates a 3D (xyz) ring in SpatiaLite
 *
 * first is a gaiaPointPtr to the first point in a linked list of points which define the polygon.
 * All of the points given to the function are 3D (xyz) points. There will be at least 4 points in the list.
 *
 * Returns the ring defined by the points given to the function.
 */
static gaiaRingPtr
ewkt_ring_xyz (gaiaPointPtr first)
{
    gaiaPointPtr p = first;
    gaiaPointPtr p_n;
    gaiaRingPtr ring = NULL;
    int numpoints;
    int index;

    /* If no pointers are given, return. */
    if (first == NULL)
	return NULL;

    /* Counts the number of points in the ring. */
    numpoints = ewkt_count_points (first);
    if (numpoints < 4)
	return NULL;

    /* Creates and allocates a ring structure. */
    ring = gaiaAllocRingXYZ (numpoints);
    if (ring == NULL)
	return NULL;

    /* Adds every point into the ring structure. */
    p = first;
    for (index = 0; index < numpoints; index++)
      {
	  gaiaSetPointXYZ (ring->Coords, index, p->X, p->Y, p->Z);
	  p_n = p->Next;
	  gaiaFreePoint (p);
	  p = p_n;
      }

    return ring;
}

/*
 * Creates a 2D (xym) ring in SpatiaLite
 *
 * first is a gaiaPointPtr to the first point in a linked list of points which define the polygon.
 * All of the points given to the function are 2D (xym) points. There will be at least 4 points in the list.
 *
 * Returns the ring defined by the points given to the function.
 */
static gaiaRingPtr
ewkt_ring_xym (gaiaPointPtr first)
{
    gaiaPointPtr p = first;
    gaiaPointPtr p_n;
    gaiaRingPtr ring = NULL;
    int numpoints;
    int index;

    /* If no pointers are given, return. */
    if (first == NULL)
	return NULL;

    /* Counts the number of points in the ring. */
    numpoints = ewkt_count_points (first);
    if (numpoints < 4)
	return NULL;

    /* Creates and allocates a ring structure. */
    ring = gaiaAllocRingXYM (numpoints);
    if (ring == NULL)
	return NULL;

    /* Adds every point into the ring structure. */
    p = first;
    for (index = 0; index < numpoints; index++)
      {
	  gaiaSetPointXYM (ring->Coords, index, p->X, p->Y, p->M);
	  p_n = p->Next;
	  gaiaFreePoint (p);
	  p = p_n;
      }

    return ring;
}

/*
 * Creates a 3D (xyzm) ring in SpatiaLite
 *
 * first is a gaiaPointPtr to the first point in a linked list of points which define the polygon.
 * All of the points given to the function are 3D (xyzm) points. There will be at least 4 points in the list.
 *
 * Returns the ring defined by the points given to the function.
 */
static gaiaRingPtr
ewkt_ring_xyzm (gaiaPointPtr first)
{
    gaiaPointPtr p = first;
    gaiaPointPtr p_n;
    gaiaRingPtr ring = NULL;
    int numpoints;
    int index;

    /* If no pointers are given, return. */
    if (first == NULL)
	return NULL;

    /* Counts the number of points in the ring. */
    numpoints = ewkt_count_points (first);
    if (numpoints < 4)
	return NULL;

    /* Creates and allocates a ring structure. */
    ring = gaiaAllocRingXYZM (numpoints);
    if (ring == NULL)
	return NULL;

    /* Adds every point into the ring structure. */
    p = first;
    for (index = 0; index < numpoints; index++)
      {
	  gaiaSetPointXYZM (ring->Coords, index, p->X, p->Y, p->Z, p->M);
	  p_n = p->Next;
	  gaiaFreePoint (p);
	  p = p_n;
      }

    return ring;
}

/*
 * Helper function that will create any type of polygon (xy, xym, xyz, xyzm) in SpatiaLite.
 * 
 * first is a gaiaRingPtr to the first ring in a linked list of rings which define the polygon.
 * The first ring in the linked list is the external ring while the rest (if any) are internal rings.
 * All of the rings given to the function are of the same type. There will be at least 1 ring in the list.
 *
 * Returns the polygon defined by the rings given to the function.
 */
static gaiaPolygonPtr
ewkt_polygon_any_type (gaiaRingPtr first)
{
    gaiaRingPtr p;
    gaiaRingPtr p_n;
    gaiaPolygonPtr polygon;
    /* If no pointers are given, return. */
    if (first == NULL)
	return NULL;

    /* Creates and allocates a polygon structure with the exterior ring. */
    polygon = gaiaCreatePolygon (first);
    if (polygon == NULL)
	return NULL;

    /* Adds all interior rings into the polygon structure. */
    p = first;
    while (p != NULL)
      {
	  p_n = p->Next;
	  if (p == first)
	      gaiaFreeRing (p);
	  else
	      gaiaAddRingToPolyg (polygon, p);
	  p = p_n;
      }

    return polygon;
}

/* 
 * Creates a 2D (xy) polygon in SpatiaLite
 *
 * first is a gaiaRingPtr to the first ring in a linked list of rings which define the polygon.
 * The first ring in the linked list is the external ring while the rest (if any) are internal rings.
 * All of the rings given to the function are 2D (xy) rings. There will be at least 1 ring in the list.
 *
 * Returns the polygon defined by the rings given to the function.
 */
static gaiaPolygonPtr
ewkt_polygon_xy (gaiaRingPtr first)
{
    return ewkt_polygon_any_type (first);
}

/* 
 * Creates a 3D (xyz) polygon in SpatiaLite
 *
 * first is a gaiaRingPtr to the first ring in a linked list of rings which define the polygon.
 * The first ring in the linked list is the external ring while the rest (if any) are internal rings.
 * All of the rings given to the function are 3D (xyz) rings. There will be at least 1 ring in the list.
 *
 * Returns the polygon defined by the rings given to the function.
 */
static gaiaPolygonPtr
ewkt_polygon_xyz (gaiaRingPtr first)
{
    return ewkt_polygon_any_type (first);
}

/* 
 * Creates a 2D (xym) polygon in SpatiaLite
 *
 * first is a gaiaRingPtr to the first ring in a linked list of rings which define the polygon.
 * The first ring in the linked list is the external ring while the rest (if any) are internal rings.
 * All of the rings given to the function are 2D (xym) rings. There will be at least 1 ring in the list.
 *
 * Returns the polygon defined by the rings given to the function.
 */
static gaiaPolygonPtr
ewkt_polygon_xym (gaiaRingPtr first)
{
    return ewkt_polygon_any_type (first);
}

/* 
 * Creates a 3D (xyzm) polygon in SpatiaLite
 *
 * first is a gaiaRingPtr to the first ring in a linked list of rings which define the polygon.
 * The first ring in the linked list is the external ring while the rest (if any) are internal rings.
 * All of the rings given to the function are 3D (xyzm) rings. There will be at least 1 ring in the list.
 *
 * Returns the polygon defined by the rings given to the function.
 */
static gaiaPolygonPtr
ewkt_polygon_xyzm (gaiaRingPtr first)
{
    return ewkt_polygon_any_type (first);
}

/*
 * Builds a geometry collection from a polygon.
 * NOTE: This function may already be implemented in the SpatiaLite code base. If it is, make sure that we
 *              can use it (ie. it doesn't use any other variables or anything else set by Sandro's parser). If you find
 *              that we can use an existing function then ignore this one.
 */
static gaiaGeomCollPtr
ewkt_buildGeomFromPolygon (gaiaPolygonPtr polygon)
{
    gaiaGeomCollPtr geom = NULL;

    /* If no pointers are given, return. */
    if (polygon == NULL)
      {
	  return NULL;
      }

    /* Creates and allocates a geometry collection containing a multipoint. */
    switch (polygon->DimensionModel)
      {
      case GAIA_XY:
	  geom = gaiaAllocGeomColl ();
	  break;
      case GAIA_XY_Z:
	  geom = gaiaAllocGeomCollXYZ ();
	  break;
      case GAIA_XY_M:
	  geom = gaiaAllocGeomCollXYM ();
	  break;
      case GAIA_XY_Z_M:
	  geom = gaiaAllocGeomCollXYZM ();
	  break;
      }
    if (geom == NULL)
      {
	  return NULL;
      }
    geom->DeclaredType = GAIA_POLYGON;

    /* Stores the location of the first and last polygons in the linked list. */
    geom->FirstPolygon = polygon;
    while (polygon != NULL)
      {
	  geom->LastPolygon = polygon;
	  polygon = polygon->Next;
      }
    return geom;
}

/* 
 * Creates a 2D (xy) multipoint object in SpatiaLite
 *
 * first is a gaiaPointPtr to the first point in a linked list of points.
 * All of the points given to the function are 2D (xy) points. There will be at least 1 point in the list.
 *
 * Returns a geometry collection containing the created multipoint object.
 */
static gaiaGeomCollPtr
ewkt_multipoint_xy (gaiaPointPtr first)
{
    gaiaPointPtr p = first;
    gaiaPointPtr p_n;
    gaiaGeomCollPtr geom = NULL;

    /* If no pointers are given, return. */
    if (first == NULL)
	return NULL;

    /* Creates and allocates a geometry collection containing a multipoint. */
    geom = gaiaAllocGeomColl ();
    if (geom == NULL)
	return NULL;
    geom->DeclaredType = GAIA_MULTIPOINT;

    /* For every 2D (xy) point, add it to the geometry collection. */
    while (p != NULL)
      {
	  gaiaAddPointToGeomColl (geom, p->X, p->Y);
	  p_n = p->Next;
	  gaiaFreePoint (p);
	  p = p_n;
      }
    return geom;
}

/* 
 * Creates a 3D (xyz) multipoint object in SpatiaLite
 *
 * first is a gaiaPointPtr to the first point in a linked list of points.
 * All of the points given to the function are 3D (xyz) points. There will be at least 1 point in the list.
 *
 * Returns a geometry collection containing the created multipoint object.
 */
static gaiaGeomCollPtr
ewkt_multipoint_xyz (gaiaPointPtr first)
{
    gaiaPointPtr p = first;
    gaiaPointPtr p_n;
    gaiaGeomCollPtr geom = NULL;

    /* If no pointers are given, return. */
    if (first == NULL)
	return NULL;

    /* Creates and allocates a geometry collection containing a multipoint. */
    geom = gaiaAllocGeomCollXYZ ();
    if (geom == NULL)
	return NULL;
    geom->DeclaredType = GAIA_MULTIPOINT;

    /* For every 3D (xyz) point, add it to the geometry collection. */
    while (p != NULL)
      {
	  gaiaAddPointToGeomCollXYZ (geom, p->X, p->Y, p->Z);
	  p_n = p->Next;
	  gaiaFreePoint (p);
	  p = p_n;
      }
    return geom;
}

/* 
 * Creates a 2D (xym) multipoint object in SpatiaLite
 *
 * first is a gaiaPointPtr to the first point in a linked list of points.
 * All of the points given to the function are 2D (xym) points. There will be at least 1 point in the list.
 *
 * Returns a geometry collection containing the created multipoint object.
 */
static gaiaGeomCollPtr
ewkt_multipoint_xym (gaiaPointPtr first)
{
    gaiaPointPtr p = first;
    gaiaPointPtr p_n;
    gaiaGeomCollPtr geom = NULL;

    /* If no pointers are given, return. */
    if (first == NULL)
	return NULL;

    /* Creates and allocates a geometry collection containing a multipoint. */
    geom = gaiaAllocGeomCollXYM ();
    if (geom == NULL)
	return NULL;
    geom->DeclaredType = GAIA_MULTIPOINT;

    /* For every 2D (xym) point, add it to the geometry collection. */
    while (p != NULL)
      {
	  gaiaAddPointToGeomCollXYM (geom, p->X, p->Y, p->M);
	  p_n = p->Next;
	  gaiaFreePoint (p);
	  p = p_n;
      }
    return geom;
}

/* 
 * Creates a 3D (xyzm) multipoint object in SpatiaLite
 *
 * first is a gaiaPointPtr to the first point in a linked list of points which define the linestring.
 * All of the points given to the function are 3D (xyzm) points. There will be at least 1 point in the list.
 *
 * Returns a geometry collection containing the created multipoint object.
 */
static gaiaGeomCollPtr
ewkt_multipoint_xyzm (gaiaPointPtr first)
{
    gaiaPointPtr p = first;
    gaiaPointPtr p_n;
    gaiaGeomCollPtr geom = NULL;

    /* If no pointers are given, return. */
    if (first == NULL)
	return NULL;

    /* Creates and allocates a geometry collection containing a multipoint. */
    geom = gaiaAllocGeomCollXYZM ();
    if (geom == NULL)
	return NULL;
    geom->DeclaredType = GAIA_MULTIPOINT;

    /* For every 3D (xyzm) point, add it to the geometry collection. */
    while (p != NULL)
      {
	  gaiaAddPointToGeomCollXYZM (geom, p->X, p->Y, p->Z, p->M);
	  p_n = p->Next;
	  gaiaFreePoint (p);
	  p = p_n;
      }
    return geom;
}

/*
 * Creates a geometry collection containing 2D (xy) linestrings.
 * Parameter first is a gaiaLinestringPtr to the first linestring in a linked list of linestrings which should be added to the
 * collection. All of the *linestrings in the list must be 2D (xy) linestrings. There must be at least 1 linestring in the list.
 * Returns a pointer to the created geometry collection of 2D linestrings. The geometry must have FirstLinestring pointing to the
 * first linestring in the list pointed by first and LastLinestring pointing to the last element of the same list. DimensionModel
 * must be GAIA_XY and DimensionType must be *GAIA_TYPE_LINESTRING.
 */

static gaiaGeomCollPtr
ewkt_multilinestring_xy (gaiaLinestringPtr first)
{
    gaiaLinestringPtr p = first;
    gaiaLinestringPtr p_n;
    gaiaLinestringPtr new_line;
    gaiaGeomCollPtr a = gaiaAllocGeomColl ();
    a->DeclaredType = GAIA_MULTILINESTRING;
    a->DimensionModel = GAIA_XY;

    while (p)
      {
	  new_line = gaiaAddLinestringToGeomColl (a, p->Points);
	  gaiaCopyLinestringCoords (new_line, p);
	  p_n = p->Next;
	  gaiaFreeLinestring (p);
	  p = p_n;
      }

    return a;
}

/* 
 * Returns a geometry collection containing the created multilinestring object (?).
 * Creates a geometry collection containing 3D (xyz) linestrings.
 * Parameter first is a gaiaLinestringPtr to the first linestring in a linked list of linestrings which should be added to the
 * collection. All of the *linestrings in the list must be 3D (xyz) linestrings. There must be at least 1 linestring in the list.
 * Returns a pointer to the created geometry collection of 3D linestrings. The geometry must have FirstLinestring pointing to the
 * first linestring in the *list pointed by first and LastLinestring pointing to the last element of the same list. DimensionModel
 * must be GAIA_XYZ and DimensionType must be *GAIA_TYPE_LINESTRING.
 */
static gaiaGeomCollPtr
ewkt_multilinestring_xyz (gaiaLinestringPtr first)
{
    gaiaLinestringPtr p = first;
    gaiaLinestringPtr p_n;
    gaiaLinestringPtr new_line;
    gaiaGeomCollPtr a = gaiaAllocGeomCollXYZ ();
    a->DeclaredType = GAIA_MULTILINESTRING;
    a->DimensionModel = GAIA_XY_Z;

    while (p)
      {
	  new_line = gaiaAddLinestringToGeomColl (a, p->Points);
	  gaiaCopyLinestringCoords (new_line, p);
	  p_n = p->Next;
	  gaiaFreeLinestring (p);
	  p = p_n;
      }
    return a;
}

/* 
 * Creates a geometry collection containing 2D (xy) with m value linestrings.
 * Parameter first is a gaiaLinestringPtr to the first linestring in a linked list of linestrings which should be added to the
 * collection. All of the  *linestrings in the list must be 2D (xy) with m value linestrings. There must be at least 1 linestring 
 * in the list.
 * Returns a pointer to the created geometry collection of 2D with m value linestrings. The geometry must have FirstLinestring
 * pointing to the first *linestring in the list pointed by first and LastLinestring pointing to the last element of the same list. 
 * DimensionModel must be GAIA_XYM and *DimensionType must be GAIA_TYPE_LINESTRING.
 */
static gaiaGeomCollPtr
ewkt_multilinestring_xym (gaiaLinestringPtr first)
{
    gaiaLinestringPtr p = first;
    gaiaLinestringPtr p_n;
    gaiaLinestringPtr new_line;
    gaiaGeomCollPtr a = gaiaAllocGeomCollXYM ();
    a->DeclaredType = GAIA_MULTILINESTRING;
    a->DimensionModel = GAIA_XY_M;

    while (p)
      {
	  new_line = gaiaAddLinestringToGeomColl (a, p->Points);
	  gaiaCopyLinestringCoords (new_line, p);
	  p_n = p->Next;
	  gaiaFreeLinestring (p);
	  p = p_n;
      }

    return a;
}

/* 
 * Creates a geometry collection containing 4D (xyz) with m value linestrings.
 * Parameter first is a gaiaLinestringPtr to the first linestring in a linked list of linestrings which should be added to the
 * collection. All of the *linestrings in the list must be 4D (xyz) with m value linestrings. There must be at least 1 linestring
 * in the list.
 * Returns a pointer to the created geometry collection of 4D with m value linestrings. The geometry must have FirstLinestring
 * pointing to the first *linestring in the list pointed by first and LastLinestring pointing to the last element of the same list. 
 * DimensionModel must be GAIA_XYZM and *DimensionType must be GAIA_TYPE_LINESTRING.
 */
static gaiaGeomCollPtr
ewkt_multilinestring_xyzm (gaiaLinestringPtr first)
{
    gaiaLinestringPtr p = first;
    gaiaLinestringPtr p_n;
    gaiaLinestringPtr new_line;
    gaiaGeomCollPtr a = gaiaAllocGeomCollXYZM ();
    a->DeclaredType = GAIA_MULTILINESTRING;
    a->DimensionModel = GAIA_XY_Z_M;

    while (p)
      {
	  new_line = gaiaAddLinestringToGeomColl (a, p->Points);
	  gaiaCopyLinestringCoords (new_line, p);
	  p_n = p->Next;
	  gaiaFreeLinestring (p);
	  p = p_n;
      }
    return a;
}

/* 
 * Creates a geometry collection containing 2D (xy) polygons.
 *
 * Parameter first is a gaiaPolygonPtr to the first polygon in a linked list of polygons which should 
 * be added to the collection. All of the polygons in the list must be 2D (xy) polygons. There must be 
 * at least 1 polygon in the list.
 *
 * Returns a pointer to the created geometry collection of 2D polygons. The geometry must have 
 * FirstPolygon pointing to the first polygon in the list pointed by first and LastPolygon pointing 
 * to the last element of the same list. DimensionModel must be GAIA_XY and DimensionType must 
 * be GAIA_TYPE_POLYGON.
 *
 */
static gaiaGeomCollPtr
ewkt_multipolygon_xy (gaiaPolygonPtr first)
{
    gaiaPolygonPtr p = first;
    gaiaPolygonPtr p_n;
    int i = 0;
    gaiaPolygonPtr new_polyg;
    gaiaRingPtr i_ring;
    gaiaRingPtr o_ring;
    gaiaGeomCollPtr geom = gaiaAllocGeomColl ();

    geom->DeclaredType = GAIA_MULTIPOLYGON;

    while (p)
      {
	  i_ring = p->Exterior;
	  new_polyg =
	      gaiaAddPolygonToGeomColl (geom, i_ring->Points, p->NumInteriors);
	  o_ring = new_polyg->Exterior;
	  gaiaCopyRingCoords (o_ring, i_ring);

	  for (i = 0; i < new_polyg->NumInteriors; i++)
	    {
		i_ring = p->Interiors + i;
		o_ring = gaiaAddInteriorRing (new_polyg, i, i_ring->Points);
		gaiaCopyRingCoords (o_ring, i_ring);
	    }

	  p_n = p->Next;
	  gaiaFreePolygon (p);
	  p = p_n;
      }

    return geom;
}

/* 
 * Creates a geometry collection containing 3D (xyz) polygons.
 *
 * Parameter first is a gaiaPolygonPtr to the first polygon in a linked list of polygons which should be 
 * added to the collection. All of the polygons in the list must be 3D (xyz) polygons. There must be at
 * least 1 polygon in the list.
 *
 * Returns a pointer to the created geometry collection of 3D polygons. The geometry must have 
 * FirstPolygon pointing to the first polygon in the list pointed by first and LastPolygon pointing to 
 * the last element of the same list. DimensionModel must be GAIA_XYZ and DimensionType must 
 * be GAIA_TYPE_POLYGON.
 *
 */
static gaiaGeomCollPtr
ewkt_multipolygon_xyz (gaiaPolygonPtr first)
{
    gaiaPolygonPtr p = first;
    gaiaPolygonPtr p_n;
    int i = 0;
    gaiaPolygonPtr new_polyg;
    gaiaRingPtr i_ring;
    gaiaRingPtr o_ring;
    gaiaGeomCollPtr geom = gaiaAllocGeomCollXYZ ();

    geom->DeclaredType = GAIA_MULTIPOLYGON;

    while (p)
      {
	  i_ring = p->Exterior;
	  new_polyg =
	      gaiaAddPolygonToGeomColl (geom, i_ring->Points, p->NumInteriors);
	  o_ring = new_polyg->Exterior;
	  gaiaCopyRingCoords (o_ring, i_ring);

	  for (i = 0; i < new_polyg->NumInteriors; i++)
	    {
		i_ring = p->Interiors + i;
		o_ring = gaiaAddInteriorRing (new_polyg, i, i_ring->Points);
		gaiaCopyRingCoords (o_ring, i_ring);
	    }

	  p_n = p->Next;
	  gaiaFreePolygon (p);
	  p = p_n;
      }

    return geom;
}

/* 
 * Creates a geometry collection containing 2D (xy) with m value polygons.
 *
 * Parameter first is a gaiaPolygonPtr to the first polygon in a linked list of polygons which should
 * be added to the collection. All of the polygons in the list must be 2D (xy) with m value polygons.
 * There must be at least 1 polygon in the list.
 *
 * Returns a pointer to the created geometry collection of 2D with m value polygons. The geometry 
 * must have FirstPolygon pointing to the first polygon in the list pointed by first and LastPolygon 
 * pointing to the last element of the same list. DimensionModel must be GAIA_XYM and DimensionType 
 * must be GAIA_TYPE_POLYGON.
 *
 */
static gaiaGeomCollPtr
ewkt_multipolygon_xym (gaiaPolygonPtr first)
{
    gaiaPolygonPtr p = first;
    gaiaPolygonPtr p_n;
    int i = 0;
    gaiaPolygonPtr new_polyg;
    gaiaRingPtr i_ring;
    gaiaRingPtr o_ring;
    gaiaGeomCollPtr geom = gaiaAllocGeomCollXYM ();

    geom->DeclaredType = GAIA_MULTIPOLYGON;

    while (p)
      {
	  i_ring = p->Exterior;
	  new_polyg =
	      gaiaAddPolygonToGeomColl (geom, i_ring->Points, p->NumInteriors);
	  o_ring = new_polyg->Exterior;
	  gaiaCopyRingCoords (o_ring, i_ring);

	  for (i = 0; i < new_polyg->NumInteriors; i++)
	    {
		i_ring = p->Interiors + i;
		o_ring = gaiaAddInteriorRing (new_polyg, i, i_ring->Points);
		gaiaCopyRingCoords (o_ring, i_ring);
	    }

	  p_n = p->Next;
	  gaiaFreePolygon (p);
	  p = p_n;
      }

    return geom;
}

/*
 * Creates a geometry collection containing 4D (xyz) with m value polygons.
 *
 * Parameter first is a gaiaPolygonPtr to the first polygon in a linked list of polygons which should be 
 * added to the collection. All of the polygons in the list must be 4D (xyz) with m value polygons. 
 * There must be at least 1 polygon in the list.
 *
 * Returns a pointer to the created geometry collection of 4D with m value polygons. The geometry must 
 * have FirstPolygon pointing to the first polygon in the list pointed by first and LastPolygon pointing 
//  * to the last element of the same list. DimensionModel must be GAIA_XYZM and DimensionType must 
 * be GAIA_TYPE_POLYGON.
 *
 */
static gaiaGeomCollPtr
ewkt_multipolygon_xyzm (gaiaPolygonPtr first)
{
    gaiaPolygonPtr p = first;
    gaiaPolygonPtr p_n;
    int i = 0;
    gaiaPolygonPtr new_polyg;
    gaiaRingPtr i_ring;
    gaiaRingPtr o_ring;
    gaiaGeomCollPtr geom = gaiaAllocGeomCollXYZM ();

    geom->DeclaredType = GAIA_MULTIPOLYGON;

    while (p)
      {
	  i_ring = p->Exterior;
	  new_polyg =
	      gaiaAddPolygonToGeomColl (geom, i_ring->Points, p->NumInteriors);
	  o_ring = new_polyg->Exterior;
	  gaiaCopyRingCoords (o_ring, i_ring);

	  for (i = 0; i < new_polyg->NumInteriors; i++)
	    {
		i_ring = p->Interiors + i;
		o_ring = gaiaAddInteriorRing (new_polyg, i, i_ring->Points);
		gaiaCopyRingCoords (o_ring, i_ring);
	    }

	  p_n = p->Next;
	  gaiaFreePolygon (p);
	  p = p_n;
      }

    return geom;
}

static void
ewkt_geomColl_common (gaiaGeomCollPtr org, gaiaGeomCollPtr dst)
{
/* 
/ helper function: xfers entities between the Origin and Destination 
/ Sandro Furieri: 2010 October 12
*/
    gaiaGeomCollPtr p = org;
    gaiaGeomCollPtr p_n;
    gaiaPointPtr pt;
    gaiaPointPtr pt_n;
    gaiaLinestringPtr ln;
    gaiaLinestringPtr ln_n;
    gaiaPolygonPtr pg;
    gaiaPolygonPtr pg_n;
    while (p)
      {
	  pt = p->FirstPoint;
	  while (pt)
	    {
		pt_n = pt->Next;
		pt->Next = NULL;
		if (dst->FirstPoint == NULL)
		    dst->FirstPoint = pt;
		if (dst->LastPoint != NULL)
		    dst->LastPoint->Next = pt;
		dst->LastPoint = pt;
		pt = pt_n;
	    }
	  ln = p->FirstLinestring;
	  while (ln)
	    {
		ln_n = ln->Next;
		ln->Next = NULL;
		if (dst->FirstLinestring == NULL)
		    dst->FirstLinestring = ln;
		if (dst->LastLinestring != NULL)
		    dst->LastLinestring->Next = ln;
		dst->LastLinestring = ln;
		ln = ln_n;
	    }
	  pg = p->FirstPolygon;
	  while (pg)
	    {
		pg_n = pg->Next;
		pg->Next = NULL;
		if (dst->FirstPolygon == NULL)
		    dst->FirstPolygon = pg;
		if (dst->LastPolygon != NULL)
		    dst->LastPolygon->Next = pg;
		dst->LastPolygon = pg;
		pg = pg_n;
	    }
	  p_n = p->Next;
	  p->FirstPoint = NULL;
	  p->LastPoint = NULL;
	  p->FirstLinestring = NULL;
	  p->LastLinestring = NULL;
	  p->FirstPolygon = NULL;
	  p->LastPolygon = NULL;
	  gaiaFreeGeomColl (p);
	  p = p_n;
      }
}

/* Creates a 2D (xy) geometry collection in SpatiaLite
 *
 * first is the first geometry collection in a linked list of geometry collections.
 * Each geometry collection represents a single type of object (eg. one could be a POINT, 
 * another could be a LINESTRING, another could be a MULTILINESTRING, etc.).
 *
 * The type of object represented by any geometry collection is stored in the declaredType 
 * field of its struct. For example, if first->declaredType = GAIA_POINT, then first represents a point.
 * If first->declaredType = GAIA_MULTIPOINT, then first represents a multipoint.
 *
 * NOTE: geometry collections cannot contain other geometry collections (have to confirm this 
 * with Sandro).
 *
 * The goal of this function is to take the information from all of the structs in the linked list and 
 * return one geomColl struct containing all of that information.
 *
 * The integers used for 'declaredType' are defined in gaiageo.h. In this function, the only values 
 * contained in 'declaredType' that will be encountered will be:
 *
 *	GAIA_POINT, GAIA_LINESTRING, GAIA_POLYGON, 
 *	GAIA_MULTIPOINT, GAIA_MULTILINESTRING, GAIA_MULTIPOLYGON
 */
static gaiaGeomCollPtr
ewkt_geomColl_xy (gaiaGeomCollPtr first)
{
    gaiaGeomCollPtr geom = gaiaAllocGeomColl ();
    if (geom == NULL)
	return NULL;
    geom->DeclaredType = GAIA_GEOMETRYCOLLECTION;
    geom->DimensionModel = GAIA_XY;
    ewkt_geomColl_common (first, geom);
    return geom;
}

/*
 * See geomColl_xy for description.
 *
 * The only difference between this function and geomColl_xy is that the 'declaredType' field of the structs
 * in the linked list for this function will only contain the following types:
 *
 *	GAIA_POINTZ, GAIA_LINESTRINGZ, GAIA_POLYGONZ,
 * 	GAIA_MULTIPOINTZ, GAIA_MULTILINESTRINGZ, GAIA_MULTIPOLYGONZ
 */
static gaiaGeomCollPtr
ewkt_geomColl_xyz (gaiaGeomCollPtr first)
{
    gaiaGeomCollPtr geom = gaiaAllocGeomColl ();
    if (geom == NULL)
	return NULL;
    geom->DeclaredType = GAIA_GEOMETRYCOLLECTION;
    geom->DimensionModel = GAIA_XY_Z;
    ewkt_geomColl_common (first, geom);
    return geom;
}

/*
 * See geomColl_xy for description.
 *
 * The only difference between this function and geomColl_xy is that the 'declaredType' field of the structs
 * in the linked list for this function will only contain the following types:
 *
 *	GAIA_POINTM, GAIA_LINESTRINGM, GAIA_POLYGONM,
 * 	GAIA_MULTIPOINTM, GAIA_MULTILINESTRINGM, GAIA_MULTIPOLYGONM
 */
static gaiaGeomCollPtr
ewkt_geomColl_xym (gaiaGeomCollPtr first)
{
    gaiaGeomCollPtr geom = gaiaAllocGeomColl ();
    if (geom == NULL)
	return NULL;
    geom->DeclaredType = GAIA_GEOMETRYCOLLECTION;
    geom->DimensionModel = GAIA_XY_M;
    ewkt_geomColl_common (first, geom);
    return geom;
}

/*
 * See geomColl_xy for description.
 *
 * The only difference between this function and geomColl_xy is that the 'declaredType' field of the structs
 * in the linked list for this function will only contain the following types:
 *
 *	GAIA_POINTZM, GAIA_LINESTRINGZM, GAIA_POLYGONZM,
 * 	GAIA_MULTIPOINTZM, GAIA_MULTILINESTRINGZM, GAIA_MULTIPOLYGONZM
 */
static gaiaGeomCollPtr
ewkt_geomColl_xyzm (gaiaGeomCollPtr first)
{
    gaiaGeomCollPtr geom = gaiaAllocGeomColl ();
    if (geom == NULL)
	return NULL;
    geom->DeclaredType = GAIA_GEOMETRYCOLLECTION;
    geom->DimensionModel = GAIA_XY_Z_M;
    ewkt_geomColl_common (first, geom);
    return geom;
}



/*
** CAVEAT: we must redefine any Lemon/Flex own macro
*/
#define YYMINORTYPE		EWKT_MINORTYPE
#define YY_CHAR			EWKT_YY_CHAR
#define	input			ewkt_input
#define ParseAlloc		ewktParseAlloc
#define ParseFree		ewktParseFree
#define ParseStackPeak		ewktParseStackPeak
#define Parse			ewktParse
#define yyStackEntry		ewkt_yyStackEntry
#define yyzerominor		ewkt_yyzerominor
#define yy_accept		ewkt_yy_accept
#define yy_action		ewkt_yy_action
#define yy_base			ewkt_yy_base
#define yy_buffer_stack		ewkt_yy_buffer_stack
#define yy_buffer_stack_max	ewkt_yy_buffer_stack_max
#define yy_buffer_stack_top	ewkt_yy_buffer_stack_top
#define yy_c_buf_p		ewkt_yy_c_buf_p
#define yy_chk			ewkt_yy_chk
#define yy_def			ewkt_yy_def
#define yy_default		ewkt_yy_default
#define yy_destructor		ewkt_yy_destructor
#define yy_ec			ewkt_yy_ec
#define yy_fatal_error		ewkt_yy_fatal_error
#define yy_find_reduce_action	ewkt_yy_find_reduce_action
#define yy_find_shift_action	ewkt_yy_find_shift_action
#define yy_get_next_buffer	ewkt_yy_get_next_buffer
#define yy_get_previous_state	ewkt_yy_get_previous_state
#define yy_init			ewkt_yy_init
#define yy_init_globals		ewkt_yy_init_globals
#define yy_lookahead		ewkt_yy_lookahead
#define yy_meta			ewkt_yy_meta
#define yy_nxt			ewkt_yy_nxt
#define yy_parse_failed		ewkt_yy_parse_failed
#define yy_pop_parser_stack	ewkt_yy_pop_parser_stack
#define yy_reduce		ewkt_yy_reduce
#define yy_reduce_ofst		ewkt_yy_reduce_ofst
#define yy_shift		ewkt_yy_shift
#define yy_shift_ofst		ewkt_yy_shift_ofst
#define yy_start		ewkt_yy_start
#define yy_state_type		ewkt_yy_state_type
#define yy_syntax_error		ewkt_yy_syntax_error
#define yy_trans_info		ewkt_yy_trans_info
#define yy_try_NUL_trans	ewkt_yy_try_NUL_trans
#define yyParser		ewkt_yyParser
#define yyStackEntry		ewkt_yyStackEntry
#define yyStackOverflow		ewkt_yyStackOverflow
#define yyRuleInfo		ewkt_yyRuleInfo
#define yyunput			ewkt_yyunput
#define yyzerominor		ewkt_yyzerominor
#define yyTraceFILE		ewkt_yyTraceFILE
#define yyTracePrompt		ewkt_yyTracePrompt
#define yyTokenName		ewkt_yyTokenName
#define yyRuleName		ewkt_yyRuleName
#define ParseTrace		ewkt_ParseTrace


/*
 EWKT_LEMON_H_START - LEMON generated header starts here 
*/
#define EWKT_NEWLINE                    1
#define EWKT_POINT                      2
#define EWKT_OPEN_BRACKET               3
#define EWKT_CLOSE_BRACKET              4
#define EWKT_POINT_M                    5
#define EWKT_NUM                        6
#define EWKT_COMMA                      7
#define EWKT_LINESTRING                 8
#define EWKT_LINESTRING_M               9
#define EWKT_POLYGON                   10
#define EWKT_POLYGON_M                 11
#define EWKT_MULTIPOINT                12
#define EWKT_MULTIPOINT_M              13
#define EWKT_MULTILINESTRING           14
#define EWKT_MULTILINESTRING_M         15
#define EWKT_MULTIPOLYGON              16
#define EWKT_MULTIPOLYGON_M            17
#define EWKT_GEOMETRYCOLLECTION        18
#define EWKT_GEOMETRYCOLLECTION_M      19
/*
 EWKT_LEMON_H_END - LEMON generated header ends here 
*/



typedef union
{
    double dval;
    struct symtab *symp;
} ewkt_yystype;
#define YYSTYPE ewkt_yystype

/* extern YYSTYPE yylval; */
YYSTYPE EwktLval;



/*
 EWKT_LEMON_START - LEMON generated code starts here 
*/

/* Driver template for the LEMON parser generator.
** The author disclaims copyright to this source code.
*/
/* First off, code is included that follows the "include" declaration
** in the input grammar file. */
#include <stdio.h>

/* Next is all token values, in a form suitable for use by makeheaders.
** This section will be null unless lemon is run with the -m switch.
*/
/* 
** These constants (all generated automatically by the parser generator)
** specify the various kinds of tokens (terminals) that the parser
** understands. 
**
** Each symbol here is a terminal symbol in the grammar.
*/
/* Make sure the INTERFACE macro is defined.
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/* The next thing included is series of defines which control
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 terminals
**                       and nonterminals.  "int" is used otherwise.
**    YYNOCODE           is a number of type YYCODETYPE which corresponds
**                       to no legal terminal or nonterminal number.  This
**                       number is used to fill in empty slots of the hash 
**                       table.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       have fall-back values which should be used if the
**                       original value of the token will not parse.
**    YYACTIONTYPE       is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 rules and
**                       states combined.  "int" is used otherwise.
**    ParseTOKENTYPE     is the data type used for minor tokens given 
**                       directly to the parser from the tokenizer.
**    YYMINORTYPE        is the data type used for all minor tokens.
**                       This is typically a union of many types, one of
**                       which is ParseTOKENTYPE.  The entry in the union
**                       for base tokens is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    ParseARG_SDECL     A static variable declaration for the %extra_argument
**    ParseARG_PDECL     A parameter declaration for the %extra_argument
**    ParseARG_STORE     Code to store %extra_argument into yypParser
**    ParseARG_FETCH     Code to extract %extra_argument from yypParser
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
*/
#define YYCODETYPE unsigned char
#define YYNOCODE 109
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE void *
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 1000000
#endif
#define ParseARG_SDECL  gaiaGeomCollPtr *result ;
#define ParseARG_PDECL , gaiaGeomCollPtr *result 
#define ParseARG_FETCH  gaiaGeomCollPtr *result  = yypParser->result 
#define ParseARG_STORE yypParser->result  = result 
#define YYNSTATE 350
#define YYNRULE 151
#define YY_NO_ACTION      (YYNSTATE+YYNRULE+2)
#define YY_ACCEPT_ACTION  (YYNSTATE+YYNRULE+1)
#define YY_ERROR_ACTION   (YYNSTATE+YYNRULE)

/* The yyzerominor constant is used to initialize instances of
** YYMINORTYPE objects to zero. */
static const YYMINORTYPE yyzerominor = { 0 };

/* Define the yytestcase() macro to be a no-op if is not already defined
** otherwise.
**
** Applications can choose to define yytestcase() in the %include section
** to a macro that can assist in verifying code coverage.  For production
** code the yytestcase() macro should be turned off.  But it is useful
** for testing.
*/
#ifndef yytestcase
# define yytestcase(X)
#endif


/* Next are the tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.  
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N < YYNSTATE                  Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   YYNSTATE <= N < YYNSTATE+YYNRULE   Reduce by rule N-YYNSTATE.
**
**   N == YYNSTATE+YYNRULE              A syntax error has occurred.
**
**   N == YYNSTATE+YYNRULE+1            The parser accepts its input.
**
**   N == YYNSTATE+YYNRULE+2            No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as
**
**      yy_action[ yy_shift_ofst[S] + X ]
**
** If the index value yy_shift_ofst[S]+X is out of range or if the value
** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X or if yy_shift_ofst[S]
** is equal to YY_SHIFT_USE_DFLT, it means that the action is not in the table
** and that yy_default[S] should be used instead.  
**
** The formula above is for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used in place of
** YY_SHIFT_USE_DFLT.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
*/
static const YYACTIONTYPE yy_action[] = {
 /*     0 */   159,  222,  223,  224,  225,  226,  227,  228,  229,  230,
 /*    10 */   231,  232,  233,  234,  235,  236,  237,  238,  239,  240,
 /*    20 */   241,  242,  243,  244,  245,  246,  247,  248,  249,  250,
 /*    30 */   251,  252,  350,  257,  160,  161,  162,  164,  163,   54,
 /*    40 */    12,   71,   13,   86,   15,   95,   16,  106,   18,  123,
 /*    50 */    20,  152,  128,  136,  144,  134,  142,  150,  135,  143,
 /*    60 */   151,  166,  168,    4,  170,   54,  175,  180,   55,  185,
 /*    70 */    54,   92,   93,  160,   94,   54,  207,  131,   14,   12,
 /*    80 */   132,   13,  129,  133,  130,  211,  139,   64,  215,  140,
 /*    90 */    62,  137,  141,  138,  145,  147,  146,   56,  148,  164,
 /*   100 */    74,  149,    5,   71,   78,   86,   82,  153,  157,  158,
 /*   110 */   154,  155,  156,  260,    6,  261,  262,  502,    1,  274,
 /*   120 */    60,  275,  276,  290,   59,  291,  292,  165,  298,   57,
 /*   130 */   299,  300,   97,   65,  100,  103,   62,  310,  256,  311,
 /*   140 */   312,  111,  167,  115,  119,  264,  322,   61,  323,  324,
 /*   150 */   172,   59,   57,   69,   66,   70,   66,   72,   73,   57,
 /*   160 */    57,  175,  176,  177,   76,   59,   59,   59,   59,  180,
 /*   170 */   181,   17,   62,   62,  182,   80,   19,   62,   62,  185,
 /*   180 */    66,  186,   66,  187,   66,   84,   66,  190,  191,   57,
 /*   190 */    57,  192,   90,   57,   57,   96,  166,   57,  168,    2,
 /*   200 */    59,   62,  170,   66,  161,   23,  162,   58,   59,   62,
 /*   210 */   163,   66,  259,  263,   63,  266,   25,   27,   67,  169,
 /*   220 */   267,   68,   28,  171,  269,   30,  173,  272,  271,   75,
 /*   230 */    31,  178,   79,   35,   39,   83,  183,  174,   87,   77,
 /*   240 */   188,   43,  279,   89,  195,   47,  193,  179,   81,  286,
 /*   250 */   194,  196,  282,  197,   98,  184,   85,   88,  285,  189,
 /*   260 */    91,   48,  289,   99,  101,   49,  102,  104,   50,  198,
 /*   270 */   296,  105,  107,  108,  109,  112,  110,  113,   74,  302,
 /*   280 */   114,  117,  116,  199,  304,  118,  200,  307,  306,  120,
 /*   290 */   201,  121,  309,   78,  122,  124,  126,    7,   82,   10,
 /*   300 */     8,  202,  125,  314,  260,    9,  127,  274,   11,  221,
 /*   310 */     3,  203,  316,  253,  261,  204,  254,  319,  255,  275,
 /*   320 */   318,   21,   22,  205,  258,  265,  321,  276,  262,   24,
 /*   330 */   268,   26,  270,  273,   29,  277,  206,   32,  326,   33,
 /*   340 */    34,  278,  280,   36,  281,  327,   37,  328,  208,  209,
 /*   350 */    38,  283,  210,   40,  284,   41,  332,  333,  334,  212,
 /*   360 */   213,   42,  214,  287,   44,   45,  338,   46,  288,  293,
 /*   370 */   339,  340,  343,  216,  294,  295,  297,  217,  218,  301,
 /*   380 */   303,  345,  346,  305,  347,  219,  220,  308,  313,  315,
 /*   390 */   317,  320,  325,   51,  503,  329,  330,  331,  335,   52,
 /*   400 */   503,  336,  337,   53,  503,  503,  341,  342,  344,  503,
 /*   410 */   503,  348,  349,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    23,   24,   25,   26,   27,   28,   29,   30,   31,   32,
 /*    10 */    33,   34,   35,   36,   37,   38,   39,   40,   41,   42,
 /*    20 */    43,   44,   45,   46,   47,   48,   49,   50,   51,   52,
 /*    30 */    53,   54,    0,    6,    2,   55,   56,    5,   58,   59,
 /*    40 */     8,    9,   10,   11,   12,   13,   14,   15,   16,   17,
 /*    50 */    18,   19,   27,   28,   29,   30,   31,   32,   33,   34,
 /*    60 */    35,   55,   56,    3,   58,   59,   55,   56,   59,   58,
 /*    70 */    59,   55,   56,    2,   58,   59,    2,   27,    3,    8,
 /*    80 */    30,   10,    8,   33,   10,    2,   28,   56,    2,   31,
 /*    90 */    59,    8,   34,   10,    8,   29,   10,   59,   32,    5,
 /*   100 */    72,   35,    3,    9,   76,   11,   78,   48,   49,   50,
 /*   110 */    48,   49,   50,   64,    3,   66,   67,   21,   22,   68,
 /*   120 */    55,   70,   71,   80,   59,   82,   83,   57,   84,   59,
 /*   130 */    86,   87,   64,   56,   66,   67,   59,   92,   59,   94,
 /*   140 */    95,   68,   60,   70,   71,   60,  100,   55,  102,  103,
 /*   150 */    57,   59,   59,   58,   59,   58,   59,   57,   57,   59,
 /*   160 */    59,   55,   55,   55,   55,   59,   59,   59,   59,   56,
 /*   170 */    56,    3,   59,   59,   56,   56,    3,   59,   59,   58,
 /*   180 */    59,   58,   59,   58,   59,   58,   59,   57,   57,   59,
 /*   190 */    59,   57,   57,   59,   59,   57,   55,   59,   56,    3,
 /*   200 */    59,   59,   58,   59,   55,    7,   56,   59,   59,   59,
 /*   210 */    58,   59,   59,   59,   59,   59,    7,    7,   59,   62,
 /*   220 */    62,   59,    3,   63,   63,    7,   61,   61,   65,    7,
 /*   230 */     3,   60,    7,    3,    3,    7,   62,   73,    3,   72,
 /*   240 */    63,    3,   73,    7,   62,    3,   61,   77,   76,   69,
 /*   250 */    60,   63,   77,   61,    7,   79,   78,   74,   79,   75,
 /*   260 */    74,    3,   75,   64,    7,    3,   66,    7,    3,   88,
 /*   270 */    81,   67,    3,   65,    7,    7,   65,    3,   72,   88,
 /*   280 */    68,    3,    7,   90,   90,   70,   91,   85,   91,    7,
 /*   290 */    89,    3,   89,   76,   71,    3,    7,    7,   78,    3,
 /*   300 */     7,   96,   69,   96,   64,    7,   69,   68,    7,    1,
 /*   310 */     3,   98,   98,    4,   66,   99,    4,   93,    4,   70,
 /*   320 */    99,    3,    7,   97,    4,    4,   97,   71,   67,    7,
 /*   330 */     4,    7,    4,    4,    7,    4,  104,    7,  104,    7,
 /*   340 */     7,    4,    4,    7,    4,  104,    7,  104,  104,  104,
 /*   350 */     7,    4,  106,    7,    4,    7,  106,  106,  106,  106,
 /*   360 */   106,    7,  107,    4,    7,    7,  107,    7,    4,    4,
 /*   370 */   107,  107,  101,  107,    4,    4,    4,  107,  105,    4,
 /*   380 */     4,  105,  105,    4,  105,  105,  105,    4,    4,    4,
 /*   390 */     4,    4,    4,    3,  108,    4,    4,    4,    4,    3,
 /*   400 */   108,    4,    4,    3,  108,  108,    4,    4,    4,  108,
 /*   410 */   108,    4,    4,
};
#define YY_SHIFT_USE_DFLT (-1)
#define YY_SHIFT_MAX 220
static const short yy_shift_ofst[] = {
 /*     0 */    -1,   32,   71,   27,   27,   27,   27,   74,   83,   86,
 /*    10 */    94,   94,   60,   75,   99,  111,  168,   60,  173,   75,
 /*    20 */   196,   27,   27,   27,   27,   27,   27,   27,   27,   27,
 /*    30 */    27,   27,   27,   27,   27,   27,   27,   27,   27,   27,
 /*    40 */    27,   27,   27,   27,   27,   27,   27,   27,   27,   27,
 /*    50 */    27,   27,   27,   27,   27,   27,   27,   27,   27,   27,
 /*    60 */   198,  198,   27,   27,  209,  209,   27,   27,   27,  210,
 /*    70 */   210,  219,  218,  218,  222,  227,  198,  222,  225,  230,
 /*    80 */   209,  225,  228,  231,  210,  228,  235,  238,  236,  238,
 /*    90 */   218,  236,  198,  209,  210,  242,  218,  247,  258,  247,
 /*   100 */   257,  262,  257,  260,  265,  260,  269,  219,  267,  219,
 /*   110 */   267,  268,  274,  227,  268,  275,  278,  230,  275,  282,
 /*   120 */   288,  231,  282,  292,  235,  289,  235,  289,  290,  258,
 /*   130 */   274,  290,  290,  290,  290,  290,  293,  262,  278,  293,
 /*   140 */   293,  293,  293,  293,  298,  265,  288,  298,  298,  298,
 /*   150 */   298,  298,  296,  301,  301,  301,  301,  301,  301,  308,
 /*   160 */   307,  309,  312,  314,  318,  320,  315,  321,  322,  326,
 /*   170 */   324,  328,  327,  329,  331,  330,  332,  333,  337,  338,
 /*   180 */   336,  339,  343,  340,  347,  346,  348,  354,  350,  359,
 /*   190 */   357,  358,  360,  364,  365,  370,  371,  372,  375,  376,
 /*   200 */   379,  383,  384,  385,  386,  387,  388,  390,  391,  392,
 /*   210 */   393,  396,  394,  397,  398,  400,  402,  403,  404,  407,
 /*   220 */   408,
};
#define YY_REDUCE_USE_DFLT (-24)
#define YY_REDUCE_MAX 158
static const short yy_reduce_ofst[] = {
 /*     0 */    96,  -23,   25,  -20,    6,   11,   16,   50,   58,   66,
 /*    10 */    59,   62,   49,   51,   28,   43,   44,   68,   45,   73,
 /*    20 */    46,   70,   65,   92,   31,   77,   95,   97,   93,  100,
 /*    30 */   101,  106,  107,  108,  109,  113,  114,  118,  119,  121,
 /*    40 */   123,  125,  127,  130,  131,  134,  135,  138,  141,  142,
 /*    50 */   144,  149,  150,  152,    9,   38,   79,  148,  153,  154,
 /*    60 */    82,   85,  155,  156,  157,  158,  159,  162,   79,  160,
 /*    70 */   161,  163,  165,  166,  164,  167,  171,  169,  170,  172,
 /*    80 */   174,  175,  176,  178,  177,  179,  180,  183,  184,  186,
 /*    90 */   185,  187,  190,  182,  188,  189,  192,  181,  199,  191,
 /*   100 */   193,  200,  194,  195,  204,  197,  202,  208,  201,  211,
 /*   110 */   203,  205,  212,  206,  207,  213,  215,  217,  214,  216,
 /*   120 */   223,  220,  221,  224,  233,  226,  237,  229,  232,  240,
 /*   130 */   239,  234,  241,  243,  244,  245,  246,  248,  249,  250,
 /*   140 */   251,  252,  253,  254,  255,  261,  256,  259,  263,  264,
 /*   150 */   266,  270,  271,  273,  276,  277,  279,  280,  281,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   351,  501,  501,  501,  501,  501,  501,  501,  501,  501,
 /*    10 */   501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
 /*    20 */   501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
 /*    30 */   501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
 /*    40 */   501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
 /*    50 */   501,  501,  501,  501,  501,  388,  390,  501,  501,  501,
 /*    60 */   393,  393,  501,  501,  397,  397,  501,  501,  501,  399,
 /*    70 */   399,  501,  395,  395,  418,  501,  393,  418,  424,  501,
 /*    80 */   397,  424,  427,  501,  399,  427,  501,  501,  421,  501,
 /*    90 */   395,  421,  393,  397,  399,  501,  395,  442,  501,  442,
 /*   100 */   448,  501,  448,  451,  501,  451,  501,  501,  445,  501,
 /*   110 */   445,  458,  501,  501,  458,  464,  501,  501,  464,  467,
 /*   120 */   501,  501,  467,  501,  501,  461,  501,  461,  476,  501,
 /*   130 */   501,  476,  476,  476,  476,  476,  490,  501,  501,  490,
 /*   140 */   490,  490,  490,  490,  497,  501,  501,  497,  497,  497,
 /*   150 */   497,  497,  501,  483,  483,  483,  483,  483,  483,  501,
 /*   160 */   501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
 /*   170 */   501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
 /*   180 */   501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
 /*   190 */   501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
 /*   200 */   501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
 /*   210 */   501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
 /*   220 */   501,  352,  353,  354,  355,  356,  357,  358,  359,  360,
 /*   230 */   361,  362,  363,  364,  365,  366,  367,  368,  369,  370,
 /*   240 */   371,  372,  373,  374,  375,  376,  377,  378,  379,  380,
 /*   250 */   381,  382,  383,  384,  385,  387,  391,  392,  386,  389,
 /*   260 */   401,  403,  404,  388,  394,  405,  390,  398,  407,  400,
 /*   270 */   408,  402,  396,  406,  409,  411,  412,  413,  417,  419,
 /*   280 */   415,  423,  425,  416,  426,  428,  410,  414,  420,  422,
 /*   290 */   429,  431,  432,  433,  435,  436,  430,  434,  437,  439,
 /*   300 */   440,  441,  443,  447,  449,  450,  452,  438,  444,  446,
 /*   310 */   453,  455,  456,  457,  459,  463,  465,  466,  468,  454,
 /*   320 */   460,  462,  469,  471,  472,  473,  477,  478,  479,  474,
 /*   330 */   475,  487,  491,  492,  493,  488,  489,  494,  498,  499,
 /*   340 */   500,  495,  496,  470,  480,  484,  485,  486,  481,  482,
};
#define YY_SZ_ACTTAB (int)(sizeof(yy_action)/sizeof(yy_action[0]))

/* The next table maps tokens into fallback tokens.  If a construct
** like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammar, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
*/
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
*/
struct yyStackEntry {
  YYACTIONTYPE stateno;  /* The state-number */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  int yyidx;                    /* Index of top element in stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyidxMax;                 /* Maximum value of yyidx */
#endif
  int yyerrcnt;                 /* Shifts left before out of the error */
  ParseARG_SDECL                /* A place to hold %extra_argument */
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  yyStackEntry *yystack;        /* The parser's stack */
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
#endif
};
typedef struct yyParser yyParser;

#ifndef NDEBUG
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
/* 
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL 
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void ParseTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = { 
  "$",             "EWKT_NEWLINE",  "EWKT_POINT",    "EWKT_OPEN_BRACKET",
  "EWKT_CLOSE_BRACKET",  "EWKT_POINT_M",  "EWKT_NUM",      "EWKT_COMMA",  
  "EWKT_LINESTRING",  "EWKT_LINESTRING_M",  "EWKT_POLYGON",  "EWKT_POLYGON_M",
  "EWKT_MULTIPOINT",  "EWKT_MULTIPOINT_M",  "EWKT_MULTILINESTRING",  "EWKT_MULTILINESTRING_M",
  "EWKT_MULTIPOLYGON",  "EWKT_MULTIPOLYGON_M",  "EWKT_GEOMETRYCOLLECTION",  "EWKT_GEOMETRYCOLLECTION_M",
  "error",         "main",          "in",            "state",       
  "program",       "geo_text",      "geo_textm",     "point",       
  "pointz",        "pointzm",       "linestring",    "linestringz", 
  "linestringzm",  "polygon",       "polygonz",      "polygonzm",   
  "multipoint",    "multipointz",   "multipointzm",  "multilinestring",
  "multilinestringz",  "multilinestringzm",  "multipolygon",  "multipolygonz",
  "multipolygonzm",  "geocoll",       "geocollz",      "geocollzm",   
  "pointm",        "linestringm",   "polygonm",      "multipointm", 
  "multilinestringm",  "multipolygonm",  "geocollm",      "point_coordxy",
  "point_coordxyz",  "point_coordxym",  "point_coordxyzm",  "coord",       
  "extra_pointsxy",  "extra_pointsxym",  "extra_pointsxyz",  "extra_pointsxyzm",
  "linestring_text",  "linestring_textm",  "linestring_textz",  "linestring_textzm",
  "polygon_text",  "polygon_textm",  "polygon_textz",  "polygon_textzm",
  "ring",          "extra_rings",   "ringm",         "extra_ringsm",
  "ringz",         "extra_ringsz",  "ringzm",        "extra_ringszm",
  "multipoint_text",  "multipoint_textm",  "multipoint_textz",  "multipoint_textzm",
  "multilinestring_text",  "multilinestring_textm",  "multilinestring_textz",  "multilinestring_textzm",
  "multilinestring_text2",  "multilinestring_textm2",  "multilinestring_textz2",  "multilinestring_textzm2",
  "multipolygon_text",  "multipolygon_textm",  "multipolygon_textz",  "multipolygon_textzm",
  "multipolygon_text2",  "multipolygon_textm2",  "multipolygon_textz2",  "multipolygon_textzm2",
  "geocoll_text",  "geocoll_textm",  "geocoll_textz",  "geocoll_textzm",
  "geocoll_text2",  "geocoll_textm2",  "geocoll_textz2",  "geocoll_textzm2",
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "main ::= in",
 /*   1 */ "in ::=",
 /*   2 */ "in ::= in state EWKT_NEWLINE",
 /*   3 */ "state ::= program",
 /*   4 */ "program ::= geo_text",
 /*   5 */ "program ::= geo_textm",
 /*   6 */ "geo_text ::= point",
 /*   7 */ "geo_text ::= pointz",
 /*   8 */ "geo_text ::= pointzm",
 /*   9 */ "geo_text ::= linestring",
 /*  10 */ "geo_text ::= linestringz",
 /*  11 */ "geo_text ::= linestringzm",
 /*  12 */ "geo_text ::= polygon",
 /*  13 */ "geo_text ::= polygonz",
 /*  14 */ "geo_text ::= polygonzm",
 /*  15 */ "geo_text ::= multipoint",
 /*  16 */ "geo_text ::= multipointz",
 /*  17 */ "geo_text ::= multipointzm",
 /*  18 */ "geo_text ::= multilinestring",
 /*  19 */ "geo_text ::= multilinestringz",
 /*  20 */ "geo_text ::= multilinestringzm",
 /*  21 */ "geo_text ::= multipolygon",
 /*  22 */ "geo_text ::= multipolygonz",
 /*  23 */ "geo_text ::= multipolygonzm",
 /*  24 */ "geo_text ::= geocoll",
 /*  25 */ "geo_text ::= geocollz",
 /*  26 */ "geo_text ::= geocollzm",
 /*  27 */ "geo_textm ::= pointm",
 /*  28 */ "geo_textm ::= linestringm",
 /*  29 */ "geo_textm ::= polygonm",
 /*  30 */ "geo_textm ::= multipointm",
 /*  31 */ "geo_textm ::= multilinestringm",
 /*  32 */ "geo_textm ::= multipolygonm",
 /*  33 */ "geo_textm ::= geocollm",
 /*  34 */ "point ::= EWKT_POINT EWKT_OPEN_BRACKET point_coordxy EWKT_CLOSE_BRACKET",
 /*  35 */ "pointz ::= EWKT_POINT EWKT_OPEN_BRACKET point_coordxyz EWKT_CLOSE_BRACKET",
 /*  36 */ "pointm ::= EWKT_POINT_M EWKT_OPEN_BRACKET point_coordxym EWKT_CLOSE_BRACKET",
 /*  37 */ "pointzm ::= EWKT_POINT EWKT_OPEN_BRACKET point_coordxyzm EWKT_CLOSE_BRACKET",
 /*  38 */ "point_coordxy ::= coord coord",
 /*  39 */ "point_coordxym ::= coord coord coord",
 /*  40 */ "point_coordxyz ::= coord coord coord",
 /*  41 */ "point_coordxyzm ::= coord coord coord coord",
 /*  42 */ "coord ::= EWKT_NUM",
 /*  43 */ "extra_pointsxy ::=",
 /*  44 */ "extra_pointsxy ::= EWKT_COMMA point_coordxy extra_pointsxy",
 /*  45 */ "extra_pointsxym ::=",
 /*  46 */ "extra_pointsxym ::= EWKT_COMMA point_coordxym extra_pointsxym",
 /*  47 */ "extra_pointsxyz ::=",
 /*  48 */ "extra_pointsxyz ::= EWKT_COMMA point_coordxyz extra_pointsxyz",
 /*  49 */ "extra_pointsxyzm ::=",
 /*  50 */ "extra_pointsxyzm ::= EWKT_COMMA point_coordxyzm extra_pointsxyzm",
 /*  51 */ "linestring ::= EWKT_LINESTRING linestring_text",
 /*  52 */ "linestringm ::= EWKT_LINESTRING_M linestring_textm",
 /*  53 */ "linestringz ::= EWKT_LINESTRING linestring_textz",
 /*  54 */ "linestringzm ::= EWKT_LINESTRING linestring_textzm",
 /*  55 */ "linestring_text ::= EWKT_OPEN_BRACKET point_coordxy EWKT_COMMA point_coordxy extra_pointsxy EWKT_CLOSE_BRACKET",
 /*  56 */ "linestring_textm ::= EWKT_OPEN_BRACKET point_coordxym EWKT_COMMA point_coordxym extra_pointsxym EWKT_CLOSE_BRACKET",
 /*  57 */ "linestring_textz ::= EWKT_OPEN_BRACKET point_coordxyz EWKT_COMMA point_coordxyz extra_pointsxyz EWKT_CLOSE_BRACKET",
 /*  58 */ "linestring_textzm ::= EWKT_OPEN_BRACKET point_coordxyzm EWKT_COMMA point_coordxyzm extra_pointsxyzm EWKT_CLOSE_BRACKET",
 /*  59 */ "polygon ::= EWKT_POLYGON polygon_text",
 /*  60 */ "polygonm ::= EWKT_POLYGON_M polygon_textm",
 /*  61 */ "polygonz ::= EWKT_POLYGON polygon_textz",
 /*  62 */ "polygonzm ::= EWKT_POLYGON polygon_textzm",
 /*  63 */ "polygon_text ::= EWKT_OPEN_BRACKET ring extra_rings EWKT_CLOSE_BRACKET",
 /*  64 */ "polygon_textm ::= EWKT_OPEN_BRACKET ringm extra_ringsm EWKT_CLOSE_BRACKET",
 /*  65 */ "polygon_textz ::= EWKT_OPEN_BRACKET ringz extra_ringsz EWKT_CLOSE_BRACKET",
 /*  66 */ "polygon_textzm ::= EWKT_OPEN_BRACKET ringzm extra_ringszm EWKT_CLOSE_BRACKET",
 /*  67 */ "ring ::= EWKT_OPEN_BRACKET point_coordxy EWKT_COMMA point_coordxy EWKT_COMMA point_coordxy EWKT_COMMA point_coordxy extra_pointsxy EWKT_CLOSE_BRACKET",
 /*  68 */ "extra_rings ::=",
 /*  69 */ "extra_rings ::= EWKT_COMMA ring extra_rings",
 /*  70 */ "ringm ::= EWKT_OPEN_BRACKET point_coordxym EWKT_COMMA point_coordxym EWKT_COMMA point_coordxym EWKT_COMMA point_coordxym extra_pointsxym EWKT_CLOSE_BRACKET",
 /*  71 */ "extra_ringsm ::=",
 /*  72 */ "extra_ringsm ::= EWKT_COMMA ringm extra_ringsm",
 /*  73 */ "ringz ::= EWKT_OPEN_BRACKET point_coordxyz EWKT_COMMA point_coordxyz EWKT_COMMA point_coordxyz EWKT_COMMA point_coordxyz extra_pointsxyz EWKT_CLOSE_BRACKET",
 /*  74 */ "extra_ringsz ::=",
 /*  75 */ "extra_ringsz ::= EWKT_COMMA ringz extra_ringsz",
 /*  76 */ "ringzm ::= EWKT_OPEN_BRACKET point_coordxyzm EWKT_COMMA point_coordxyzm EWKT_COMMA point_coordxyzm EWKT_COMMA point_coordxyzm extra_pointsxyzm EWKT_CLOSE_BRACKET",
 /*  77 */ "extra_ringszm ::=",
 /*  78 */ "extra_ringszm ::= EWKT_COMMA ringzm extra_ringszm",
 /*  79 */ "multipoint ::= EWKT_MULTIPOINT multipoint_text",
 /*  80 */ "multipointm ::= EWKT_MULTIPOINT_M multipoint_textm",
 /*  81 */ "multipointz ::= EWKT_MULTIPOINT multipoint_textz",
 /*  82 */ "multipointzm ::= EWKT_MULTIPOINT multipoint_textzm",
 /*  83 */ "multipoint_text ::= EWKT_OPEN_BRACKET point_coordxy extra_pointsxy EWKT_CLOSE_BRACKET",
 /*  84 */ "multipoint_textm ::= EWKT_OPEN_BRACKET point_coordxym extra_pointsxym EWKT_CLOSE_BRACKET",
 /*  85 */ "multipoint_textz ::= EWKT_OPEN_BRACKET point_coordxyz extra_pointsxyz EWKT_CLOSE_BRACKET",
 /*  86 */ "multipoint_textzm ::= EWKT_OPEN_BRACKET point_coordxyzm extra_pointsxyzm EWKT_CLOSE_BRACKET",
 /*  87 */ "multilinestring ::= EWKT_MULTILINESTRING multilinestring_text",
 /*  88 */ "multilinestringm ::= EWKT_MULTILINESTRING_M multilinestring_textm",
 /*  89 */ "multilinestringz ::= EWKT_MULTILINESTRING multilinestring_textz",
 /*  90 */ "multilinestringzm ::= EWKT_MULTILINESTRING multilinestring_textzm",
 /*  91 */ "multilinestring_text ::= EWKT_OPEN_BRACKET linestring_text multilinestring_text2 EWKT_CLOSE_BRACKET",
 /*  92 */ "multilinestring_text2 ::=",
 /*  93 */ "multilinestring_text2 ::= EWKT_COMMA linestring_text multilinestring_text2",
 /*  94 */ "multilinestring_textm ::= EWKT_OPEN_BRACKET linestring_textm multilinestring_textm2 EWKT_CLOSE_BRACKET",
 /*  95 */ "multilinestring_textm2 ::=",
 /*  96 */ "multilinestring_textm2 ::= EWKT_COMMA linestring_textm multilinestring_textm2",
 /*  97 */ "multilinestring_textz ::= EWKT_OPEN_BRACKET linestring_textz multilinestring_textz2 EWKT_CLOSE_BRACKET",
 /*  98 */ "multilinestring_textz2 ::=",
 /*  99 */ "multilinestring_textz2 ::= EWKT_COMMA linestring_textz multilinestring_textz2",
 /* 100 */ "multilinestring_textzm ::= EWKT_OPEN_BRACKET linestring_textzm multilinestring_textzm2 EWKT_CLOSE_BRACKET",
 /* 101 */ "multilinestring_textzm2 ::=",
 /* 102 */ "multilinestring_textzm2 ::= EWKT_COMMA linestring_textzm multilinestring_textzm2",
 /* 103 */ "multipolygon ::= EWKT_MULTIPOLYGON multipolygon_text",
 /* 104 */ "multipolygonm ::= EWKT_MULTIPOLYGON_M multipolygon_textm",
 /* 105 */ "multipolygonz ::= EWKT_MULTIPOLYGON multipolygon_textz",
 /* 106 */ "multipolygonzm ::= EWKT_MULTIPOLYGON multipolygon_textzm",
 /* 107 */ "multipolygon_text ::= EWKT_OPEN_BRACKET polygon_text multipolygon_text2 EWKT_CLOSE_BRACKET",
 /* 108 */ "multipolygon_text2 ::=",
 /* 109 */ "multipolygon_text2 ::= EWKT_COMMA polygon_text multipolygon_text2",
 /* 110 */ "multipolygon_textm ::= EWKT_OPEN_BRACKET polygon_textm multipolygon_textm2 EWKT_CLOSE_BRACKET",
 /* 111 */ "multipolygon_textm2 ::=",
 /* 112 */ "multipolygon_textm2 ::= EWKT_COMMA polygon_textm multipolygon_textm2",
 /* 113 */ "multipolygon_textz ::= EWKT_OPEN_BRACKET polygon_textz multipolygon_textz2 EWKT_CLOSE_BRACKET",
 /* 114 */ "multipolygon_textz2 ::=",
 /* 115 */ "multipolygon_textz2 ::= EWKT_COMMA polygon_textz multipolygon_textz2",
 /* 116 */ "multipolygon_textzm ::= EWKT_OPEN_BRACKET polygon_textzm multipolygon_textzm2 EWKT_CLOSE_BRACKET",
 /* 117 */ "multipolygon_textzm2 ::=",
 /* 118 */ "multipolygon_textzm2 ::= EWKT_COMMA polygon_textzm multipolygon_textzm2",
 /* 119 */ "geocoll ::= EWKT_GEOMETRYCOLLECTION geocoll_text",
 /* 120 */ "geocollm ::= EWKT_GEOMETRYCOLLECTION_M geocoll_textm",
 /* 121 */ "geocollz ::= EWKT_GEOMETRYCOLLECTION geocoll_textz",
 /* 122 */ "geocollzm ::= EWKT_GEOMETRYCOLLECTION geocoll_textzm",
 /* 123 */ "geocoll_text ::= EWKT_OPEN_BRACKET point geocoll_text2 EWKT_CLOSE_BRACKET",
 /* 124 */ "geocoll_text ::= EWKT_OPEN_BRACKET linestring geocoll_text2 EWKT_CLOSE_BRACKET",
 /* 125 */ "geocoll_text ::= EWKT_OPEN_BRACKET polygon geocoll_text2 EWKT_CLOSE_BRACKET",
 /* 126 */ "geocoll_text2 ::=",
 /* 127 */ "geocoll_text2 ::= EWKT_COMMA point geocoll_text2",
 /* 128 */ "geocoll_text2 ::= EWKT_COMMA linestring geocoll_text2",
 /* 129 */ "geocoll_text2 ::= EWKT_COMMA polygon geocoll_text2",
 /* 130 */ "geocoll_textm ::= EWKT_OPEN_BRACKET pointm geocoll_textm2 EWKT_CLOSE_BRACKET",
 /* 131 */ "geocoll_textm ::= EWKT_OPEN_BRACKET linestringm geocoll_textm2 EWKT_CLOSE_BRACKET",
 /* 132 */ "geocoll_textm ::= EWKT_OPEN_BRACKET polygonm geocoll_textm2 EWKT_CLOSE_BRACKET",
 /* 133 */ "geocoll_textm2 ::=",
 /* 134 */ "geocoll_textm2 ::= EWKT_COMMA pointm geocoll_textm2",
 /* 135 */ "geocoll_textm2 ::= EWKT_COMMA linestringm geocoll_textm2",
 /* 136 */ "geocoll_textm2 ::= EWKT_COMMA polygonm geocoll_textm2",
 /* 137 */ "geocoll_textz ::= EWKT_OPEN_BRACKET pointz geocoll_textz2 EWKT_CLOSE_BRACKET",
 /* 138 */ "geocoll_textz ::= EWKT_OPEN_BRACKET linestringz geocoll_textz2 EWKT_CLOSE_BRACKET",
 /* 139 */ "geocoll_textz ::= EWKT_OPEN_BRACKET polygonz geocoll_textz2 EWKT_CLOSE_BRACKET",
 /* 140 */ "geocoll_textz2 ::=",
 /* 141 */ "geocoll_textz2 ::= EWKT_COMMA pointz geocoll_textz2",
 /* 142 */ "geocoll_textz2 ::= EWKT_COMMA linestringz geocoll_textz2",
 /* 143 */ "geocoll_textz2 ::= EWKT_COMMA polygonz geocoll_textz2",
 /* 144 */ "geocoll_textzm ::= EWKT_OPEN_BRACKET pointzm geocoll_textzm2 EWKT_CLOSE_BRACKET",
 /* 145 */ "geocoll_textzm ::= EWKT_OPEN_BRACKET linestringzm geocoll_textzm2 EWKT_CLOSE_BRACKET",
 /* 146 */ "geocoll_textzm ::= EWKT_OPEN_BRACKET polygonzm geocoll_textzm2 EWKT_CLOSE_BRACKET",
 /* 147 */ "geocoll_textzm2 ::=",
 /* 148 */ "geocoll_textzm2 ::= EWKT_COMMA pointzm geocoll_textzm2",
 /* 149 */ "geocoll_textzm2 ::= EWKT_COMMA linestringzm geocoll_textzm2",
 /* 150 */ "geocoll_textzm2 ::= EWKT_COMMA polygonzm geocoll_textzm2",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.
*/
static void yyGrowStack(yyParser *p){
  int newSize;
  yyStackEntry *pNew;

  newSize = p->yystksz*2 + 100;
  pNew = realloc(p->yystack, newSize*sizeof(pNew[0]));
  if( pNew ){
    p->yystack = pNew;
    p->yystksz = newSize;
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows to %d entries!\n",
              yyTracePrompt, p->yystksz);
    }
#endif
  }
}
#endif

/* 
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to Parse and ParseFree.
*/
void *ParseAlloc(void *(*mallocProc)(size_t)){
  yyParser *pParser;
  pParser = (yyParser*)(*mallocProc)( (size_t)sizeof(yyParser) );
  if( pParser ){
    pParser->yyidx = -1;
#ifdef YYTRACKMAXSTACKDEPTH
    pParser->yyidxMax = 0;
#endif
#if YYSTACKDEPTH<=0
    pParser->yystack = NULL;
    pParser->yystksz = 0;
    yyGrowStack(pParser);
#endif
  }
  return pParser;
}

/* The following function deletes the value associated with a
** symbol.  The symbol can be either a terminal or nonterminal.
** "yymajor" is the symbol code, and "yypminor" is a pointer to
** the value.
*/
static void yy_destructor(
  yyParser *yypParser,    /* The parser */
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  YYMINORTYPE *yypminor   /* The object to be destroyed */
){
  ParseARG_FETCH;
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is 
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are not used
    ** inside the C code.
    */
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
**
** Return the major token number for the symbol popped.
*/
static int yy_pop_parser_stack(yyParser *pParser){
  YYCODETYPE yymajor;
  yyStackEntry *yytos = &pParser->yystack[pParser->yyidx];

  if( pParser->yyidx<0 ) return 0;
#ifndef NDEBUG
  if( yyTraceFILE && pParser->yyidx>=0 ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yymajor = yytos->major;
  yy_destructor(pParser, yymajor, &yytos->minor);
  pParser->yyidx--;
  return yymajor;
}

/* 
** Deallocate and destroy a parser.  Destructors are all called for
** all stack elements before shutting the parser down.
**
** Inputs:
** <ul>
** <li>  A pointer to the parser.  This should be a pointer
**       obtained from ParseAlloc.
** <li>  A pointer to a function used to reclaim memory obtained
**       from malloc.
** </ul>
*/
void ParseFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
  yyParser *pParser = (yyParser*)p;
  if( pParser==0 ) return;
  while( pParser->yyidx>=0 ) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
  free(pParser->yystack);
#endif
  (*freeProc)((void*)pParser);
}

/*
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int ParseStackPeak(void *p){
  yyParser *pParser = (yyParser*)p;
  return pParser->yyidxMax;
}
#endif

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_shift_action(
  yyParser *pParser,        /* The parser */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  int stateno = pParser->yystack[pParser->yyidx].stateno;
 
  if( stateno>YY_SHIFT_MAX || (i = yy_shift_ofst[stateno])==YY_SHIFT_USE_DFLT ){
    return yy_default[stateno];
  }
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    if( iLookAhead>0 ){
#ifdef YYFALLBACK
      YYCODETYPE iFallback;            /* Fallback token */
      if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
             && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
             yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
        }
#endif
        return yy_find_shift_action(pParser, iFallback);
      }
#endif
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        if( j>=0 && j<YY_SZ_ACTTAB && yy_lookahead[j]==YYWILDCARD ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
    }
    return yy_default[stateno];
  }else{
    return yy_action[i];
  }
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_reduce_action(
  int stateno,              /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
#ifdef YYERRORSYMBOL
  if( stateno>YY_REDUCE_MAX ){
    return yy_default[stateno];
  }
#else
  assert( stateno<=YY_REDUCE_MAX );
#endif
  i = yy_reduce_ofst[stateno];
  assert( i!=YY_REDUCE_USE_DFLT );
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
#ifdef YYERRORSYMBOL
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }
#else
  assert( i>=0 && i<YY_SZ_ACTTAB );
  assert( yy_lookahead[i]==iLookAhead );
#endif
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void yyStackOverflow(yyParser *yypParser, YYMINORTYPE *yypMinor){
   ParseARG_FETCH;
   yypParser->yyidx--;
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */

     fprintf(stderr,"Giving up.  Parser stack overflow\n");
   ParseARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  YYMINORTYPE *yypMinor         /* Pointer to the minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yyidx++;
#ifdef YYTRACKMAXSTACKDEPTH
  if( yypParser->yyidx>yypParser->yyidxMax ){
    yypParser->yyidxMax = yypParser->yyidx;
  }
#endif
#if YYSTACKDEPTH>0 
  if( yypParser->yyidx>=YYSTACKDEPTH ){
    yyStackOverflow(yypParser, yypMinor);
    return;
  }
#else
  if( yypParser->yyidx>=yypParser->yystksz ){
    yyGrowStack(yypParser);
    if( yypParser->yyidx>=yypParser->yystksz ){
      yyStackOverflow(yypParser, yypMinor);
      return;
    }
  }
#endif
  yytos = &yypParser->yystack[yypParser->yyidx];
  yytos->stateno = (YYACTIONTYPE)yyNewState;
  yytos->major = (YYCODETYPE)yyMajor;
  yytos->minor = *yypMinor;
#ifndef NDEBUG
  if( yyTraceFILE && yypParser->yyidx>0 ){
    int i;
    fprintf(yyTraceFILE,"%sShift %d\n",yyTracePrompt,yyNewState);
    fprintf(yyTraceFILE,"%sStack:",yyTracePrompt);
    for(i=1; i<=yypParser->yyidx; i++)
      fprintf(yyTraceFILE," %s",yyTokenName[yypParser->yystack[i].major]);
    fprintf(yyTraceFILE,"\n");
  }
#endif
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct {
  YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
  { 21, 1 },
  { 22, 0 },
  { 22, 3 },
  { 23, 1 },
  { 24, 1 },
  { 24, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 26, 1 },
  { 26, 1 },
  { 26, 1 },
  { 26, 1 },
  { 26, 1 },
  { 26, 1 },
  { 26, 1 },
  { 27, 4 },
  { 28, 4 },
  { 48, 4 },
  { 29, 4 },
  { 55, 2 },
  { 57, 3 },
  { 56, 3 },
  { 58, 4 },
  { 59, 1 },
  { 60, 0 },
  { 60, 3 },
  { 61, 0 },
  { 61, 3 },
  { 62, 0 },
  { 62, 3 },
  { 63, 0 },
  { 63, 3 },
  { 30, 2 },
  { 49, 2 },
  { 31, 2 },
  { 32, 2 },
  { 64, 6 },
  { 65, 6 },
  { 66, 6 },
  { 67, 6 },
  { 33, 2 },
  { 50, 2 },
  { 34, 2 },
  { 35, 2 },
  { 68, 4 },
  { 69, 4 },
  { 70, 4 },
  { 71, 4 },
  { 72, 10 },
  { 73, 0 },
  { 73, 3 },
  { 74, 10 },
  { 75, 0 },
  { 75, 3 },
  { 76, 10 },
  { 77, 0 },
  { 77, 3 },
  { 78, 10 },
  { 79, 0 },
  { 79, 3 },
  { 36, 2 },
  { 51, 2 },
  { 37, 2 },
  { 38, 2 },
  { 80, 4 },
  { 81, 4 },
  { 82, 4 },
  { 83, 4 },
  { 39, 2 },
  { 52, 2 },
  { 40, 2 },
  { 41, 2 },
  { 84, 4 },
  { 88, 0 },
  { 88, 3 },
  { 85, 4 },
  { 89, 0 },
  { 89, 3 },
  { 86, 4 },
  { 90, 0 },
  { 90, 3 },
  { 87, 4 },
  { 91, 0 },
  { 91, 3 },
  { 42, 2 },
  { 53, 2 },
  { 43, 2 },
  { 44, 2 },
  { 92, 4 },
  { 96, 0 },
  { 96, 3 },
  { 93, 4 },
  { 97, 0 },
  { 97, 3 },
  { 94, 4 },
  { 98, 0 },
  { 98, 3 },
  { 95, 4 },
  { 99, 0 },
  { 99, 3 },
  { 45, 2 },
  { 54, 2 },
  { 46, 2 },
  { 47, 2 },
  { 100, 4 },
  { 100, 4 },
  { 100, 4 },
  { 104, 0 },
  { 104, 3 },
  { 104, 3 },
  { 104, 3 },
  { 101, 4 },
  { 101, 4 },
  { 101, 4 },
  { 105, 0 },
  { 105, 3 },
  { 105, 3 },
  { 105, 3 },
  { 102, 4 },
  { 102, 4 },
  { 102, 4 },
  { 106, 0 },
  { 106, 3 },
  { 106, 3 },
  { 106, 3 },
  { 103, 4 },
  { 103, 4 },
  { 103, 4 },
  { 107, 0 },
  { 107, 3 },
  { 107, 3 },
  { 107, 3 },
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void yy_reduce(
  yyParser *yypParser,         /* The parser */
  int yyruleno                 /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  YYMINORTYPE yygotominor;        /* The LHS of the rule reduced */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  ParseARG_FETCH;
  yymsp = &yypParser->yystack[yypParser->yyidx];
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno>=0 
        && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    fprintf(yyTraceFILE, "%sReduce [%s].\n", yyTracePrompt,
      yyRuleName[yyruleno]);
  }
#endif /* NDEBUG */

  /* Silence complaints from purify about yygotominor being uninitialized
  ** in some cases when it is copied into the stack after the following
  ** switch.  yygotominor is uninitialized when a rule reduces that does
  ** not set the value of its left-hand side nonterminal.  Leaving the
  ** value of the nonterminal uninitialized is utterly harmless as long
  ** as the value is never used.  So really the only thing this code
  ** accomplishes is to quieten purify.  
  **
  ** 2007-01-16:  The wireshark project (www.wireshark.org) reports that
  ** without this code, their parser segfaults.  I'm not sure what there
  ** parser is doing to make this happen.  This is the second bug report
  ** from wireshark this week.  Clearly they are stressing Lemon in ways
  ** that it has not been previously stressed...  (SQLite ticket #2172)
  */
  /*memset(&yygotominor, 0, sizeof(yygotominor));*/
  yygotominor = yyzerominor;


  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
      case 6: /* geo_text ::= point */
      case 7: /* geo_text ::= pointz */ yytestcase(yyruleno==7);
      case 8: /* geo_text ::= pointzm */ yytestcase(yyruleno==8);
      case 9: /* geo_text ::= linestring */ yytestcase(yyruleno==9);
      case 10: /* geo_text ::= linestringz */ yytestcase(yyruleno==10);
      case 11: /* geo_text ::= linestringzm */ yytestcase(yyruleno==11);
      case 12: /* geo_text ::= polygon */ yytestcase(yyruleno==12);
      case 13: /* geo_text ::= polygonz */ yytestcase(yyruleno==13);
      case 14: /* geo_text ::= polygonzm */ yytestcase(yyruleno==14);
      case 15: /* geo_text ::= multipoint */ yytestcase(yyruleno==15);
      case 16: /* geo_text ::= multipointz */ yytestcase(yyruleno==16);
      case 17: /* geo_text ::= multipointzm */ yytestcase(yyruleno==17);
      case 18: /* geo_text ::= multilinestring */ yytestcase(yyruleno==18);
      case 19: /* geo_text ::= multilinestringz */ yytestcase(yyruleno==19);
      case 20: /* geo_text ::= multilinestringzm */ yytestcase(yyruleno==20);
      case 21: /* geo_text ::= multipolygon */ yytestcase(yyruleno==21);
      case 22: /* geo_text ::= multipolygonz */ yytestcase(yyruleno==22);
      case 23: /* geo_text ::= multipolygonzm */ yytestcase(yyruleno==23);
      case 24: /* geo_text ::= geocoll */ yytestcase(yyruleno==24);
      case 25: /* geo_text ::= geocollz */ yytestcase(yyruleno==25);
      case 26: /* geo_text ::= geocollzm */ yytestcase(yyruleno==26);
      case 27: /* geo_textm ::= pointm */ yytestcase(yyruleno==27);
      case 28: /* geo_textm ::= linestringm */ yytestcase(yyruleno==28);
      case 29: /* geo_textm ::= polygonm */ yytestcase(yyruleno==29);
      case 30: /* geo_textm ::= multipointm */ yytestcase(yyruleno==30);
      case 31: /* geo_textm ::= multilinestringm */ yytestcase(yyruleno==31);
      case 32: /* geo_textm ::= multipolygonm */ yytestcase(yyruleno==32);
      case 33: /* geo_textm ::= geocollm */ yytestcase(yyruleno==33);
{ *result = yymsp[0].minor.yy0; }
        break;
      case 34: /* point ::= EWKT_POINT EWKT_OPEN_BRACKET point_coordxy EWKT_CLOSE_BRACKET */
      case 35: /* pointz ::= EWKT_POINT EWKT_OPEN_BRACKET point_coordxyz EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==35);
      case 37: /* pointzm ::= EWKT_POINT EWKT_OPEN_BRACKET point_coordxyzm EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==37);
{ yygotominor.yy0 = ewkt_buildGeomFromPoint((gaiaPointPtr)yymsp[-1].minor.yy0); }
        break;
      case 36: /* pointm ::= EWKT_POINT_M EWKT_OPEN_BRACKET point_coordxym EWKT_CLOSE_BRACKET */
{ yygotominor.yy0 = ewkt_buildGeomFromPoint((gaiaPointPtr)yymsp[-1].minor.yy0);  }
        break;
      case 38: /* point_coordxy ::= coord coord */
{ yygotominor.yy0 = (void *) ewkt_point_xy((double *)yymsp[-1].minor.yy0, (double *)yymsp[0].minor.yy0); }
        break;
      case 39: /* point_coordxym ::= coord coord coord */
{ yygotominor.yy0 = (void *) ewkt_point_xym((double *)yymsp[-2].minor.yy0, (double *)yymsp[-1].minor.yy0, (double *)yymsp[0].minor.yy0); }
        break;
      case 40: /* point_coordxyz ::= coord coord coord */
{ yygotominor.yy0 = (void *) ewkt_point_xyz((double *)yymsp[-2].minor.yy0, (double *)yymsp[-1].minor.yy0, (double *)yymsp[0].minor.yy0); }
        break;
      case 41: /* point_coordxyzm ::= coord coord coord coord */
{ yygotominor.yy0 = (void *) ewkt_point_xyzm((double *)yymsp[-3].minor.yy0, (double *)yymsp[-2].minor.yy0, (double *)yymsp[-1].minor.yy0, (double *)yymsp[0].minor.yy0); }
        break;
      case 42: /* coord ::= EWKT_NUM */
      case 79: /* multipoint ::= EWKT_MULTIPOINT multipoint_text */ yytestcase(yyruleno==79);
      case 80: /* multipointm ::= EWKT_MULTIPOINT_M multipoint_textm */ yytestcase(yyruleno==80);
      case 81: /* multipointz ::= EWKT_MULTIPOINT multipoint_textz */ yytestcase(yyruleno==81);
      case 82: /* multipointzm ::= EWKT_MULTIPOINT multipoint_textzm */ yytestcase(yyruleno==82);
      case 87: /* multilinestring ::= EWKT_MULTILINESTRING multilinestring_text */ yytestcase(yyruleno==87);
      case 88: /* multilinestringm ::= EWKT_MULTILINESTRING_M multilinestring_textm */ yytestcase(yyruleno==88);
      case 89: /* multilinestringz ::= EWKT_MULTILINESTRING multilinestring_textz */ yytestcase(yyruleno==89);
      case 90: /* multilinestringzm ::= EWKT_MULTILINESTRING multilinestring_textzm */ yytestcase(yyruleno==90);
      case 103: /* multipolygon ::= EWKT_MULTIPOLYGON multipolygon_text */ yytestcase(yyruleno==103);
      case 104: /* multipolygonm ::= EWKT_MULTIPOLYGON_M multipolygon_textm */ yytestcase(yyruleno==104);
      case 105: /* multipolygonz ::= EWKT_MULTIPOLYGON multipolygon_textz */ yytestcase(yyruleno==105);
      case 106: /* multipolygonzm ::= EWKT_MULTIPOLYGON multipolygon_textzm */ yytestcase(yyruleno==106);
      case 119: /* geocoll ::= EWKT_GEOMETRYCOLLECTION geocoll_text */ yytestcase(yyruleno==119);
      case 120: /* geocollm ::= EWKT_GEOMETRYCOLLECTION_M geocoll_textm */ yytestcase(yyruleno==120);
      case 121: /* geocollz ::= EWKT_GEOMETRYCOLLECTION geocoll_textz */ yytestcase(yyruleno==121);
      case 122: /* geocollzm ::= EWKT_GEOMETRYCOLLECTION geocoll_textzm */ yytestcase(yyruleno==122);
{ yygotominor.yy0 = yymsp[0].minor.yy0; }
        break;
      case 43: /* extra_pointsxy ::= */
      case 45: /* extra_pointsxym ::= */ yytestcase(yyruleno==45);
      case 47: /* extra_pointsxyz ::= */ yytestcase(yyruleno==47);
      case 49: /* extra_pointsxyzm ::= */ yytestcase(yyruleno==49);
      case 68: /* extra_rings ::= */ yytestcase(yyruleno==68);
      case 71: /* extra_ringsm ::= */ yytestcase(yyruleno==71);
      case 74: /* extra_ringsz ::= */ yytestcase(yyruleno==74);
      case 77: /* extra_ringszm ::= */ yytestcase(yyruleno==77);
      case 92: /* multilinestring_text2 ::= */ yytestcase(yyruleno==92);
      case 95: /* multilinestring_textm2 ::= */ yytestcase(yyruleno==95);
      case 98: /* multilinestring_textz2 ::= */ yytestcase(yyruleno==98);
      case 101: /* multilinestring_textzm2 ::= */ yytestcase(yyruleno==101);
      case 108: /* multipolygon_text2 ::= */ yytestcase(yyruleno==108);
      case 111: /* multipolygon_textm2 ::= */ yytestcase(yyruleno==111);
      case 114: /* multipolygon_textz2 ::= */ yytestcase(yyruleno==114);
      case 117: /* multipolygon_textzm2 ::= */ yytestcase(yyruleno==117);
      case 126: /* geocoll_text2 ::= */ yytestcase(yyruleno==126);
      case 133: /* geocoll_textm2 ::= */ yytestcase(yyruleno==133);
      case 140: /* geocoll_textz2 ::= */ yytestcase(yyruleno==140);
      case 147: /* geocoll_textzm2 ::= */ yytestcase(yyruleno==147);
{ yygotominor.yy0 = NULL; }
        break;
      case 44: /* extra_pointsxy ::= EWKT_COMMA point_coordxy extra_pointsxy */
      case 46: /* extra_pointsxym ::= EWKT_COMMA point_coordxym extra_pointsxym */ yytestcase(yyruleno==46);
      case 48: /* extra_pointsxyz ::= EWKT_COMMA point_coordxyz extra_pointsxyz */ yytestcase(yyruleno==48);
      case 50: /* extra_pointsxyzm ::= EWKT_COMMA point_coordxyzm extra_pointsxyzm */ yytestcase(yyruleno==50);
{ ((gaiaPointPtr)yymsp[-1].minor.yy0)->Next = (gaiaPointPtr)yymsp[0].minor.yy0;  yygotominor.yy0 = yymsp[-1].minor.yy0; }
        break;
      case 51: /* linestring ::= EWKT_LINESTRING linestring_text */
      case 52: /* linestringm ::= EWKT_LINESTRING_M linestring_textm */ yytestcase(yyruleno==52);
      case 53: /* linestringz ::= EWKT_LINESTRING linestring_textz */ yytestcase(yyruleno==53);
      case 54: /* linestringzm ::= EWKT_LINESTRING linestring_textzm */ yytestcase(yyruleno==54);
{ yygotominor.yy0 = ewkt_buildGeomFromLinestring((gaiaLinestringPtr)yymsp[0].minor.yy0); }
        break;
      case 55: /* linestring_text ::= EWKT_OPEN_BRACKET point_coordxy EWKT_COMMA point_coordxy extra_pointsxy EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   ((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0;
	   yygotominor.yy0 = (void *) ewkt_linestring_xy((gaiaPointPtr)yymsp[-4].minor.yy0);
	}
        break;
      case 56: /* linestring_textm ::= EWKT_OPEN_BRACKET point_coordxym EWKT_COMMA point_coordxym extra_pointsxym EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   ((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0;
	   yygotominor.yy0 = (void *) ewkt_linestring_xym((gaiaPointPtr)yymsp[-4].minor.yy0);
	}
        break;
      case 57: /* linestring_textz ::= EWKT_OPEN_BRACKET point_coordxyz EWKT_COMMA point_coordxyz extra_pointsxyz EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   ((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0;
	   yygotominor.yy0 = (void *) ewkt_linestring_xyz((gaiaPointPtr)yymsp[-4].minor.yy0);
	}
        break;
      case 58: /* linestring_textzm ::= EWKT_OPEN_BRACKET point_coordxyzm EWKT_COMMA point_coordxyzm extra_pointsxyzm EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   ((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0;
	   yygotominor.yy0 = (void *) ewkt_linestring_xyzm((gaiaPointPtr)yymsp[-4].minor.yy0);
	}
        break;
      case 59: /* polygon ::= EWKT_POLYGON polygon_text */
      case 60: /* polygonm ::= EWKT_POLYGON_M polygon_textm */ yytestcase(yyruleno==60);
      case 61: /* polygonz ::= EWKT_POLYGON polygon_textz */ yytestcase(yyruleno==61);
      case 62: /* polygonzm ::= EWKT_POLYGON polygon_textzm */ yytestcase(yyruleno==62);
{ yygotominor.yy0 = ewkt_buildGeomFromPolygon((gaiaPolygonPtr)yymsp[0].minor.yy0); }
        break;
      case 63: /* polygon_text ::= EWKT_OPEN_BRACKET ring extra_rings EWKT_CLOSE_BRACKET */
{ 
		((gaiaRingPtr)yymsp[-2].minor.yy0)->Next = (gaiaRingPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) ewkt_polygon_xy((gaiaRingPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 64: /* polygon_textm ::= EWKT_OPEN_BRACKET ringm extra_ringsm EWKT_CLOSE_BRACKET */
{ 
		((gaiaRingPtr)yymsp[-2].minor.yy0)->Next = (gaiaRingPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) ewkt_polygon_xym((gaiaRingPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 65: /* polygon_textz ::= EWKT_OPEN_BRACKET ringz extra_ringsz EWKT_CLOSE_BRACKET */
{  
		((gaiaRingPtr)yymsp[-2].minor.yy0)->Next = (gaiaRingPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) ewkt_polygon_xyz((gaiaRingPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 66: /* polygon_textzm ::= EWKT_OPEN_BRACKET ringzm extra_ringszm EWKT_CLOSE_BRACKET */
{ 
		((gaiaRingPtr)yymsp[-2].minor.yy0)->Next = (gaiaRingPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) ewkt_polygon_xyzm((gaiaRingPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 67: /* ring ::= EWKT_OPEN_BRACKET point_coordxy EWKT_COMMA point_coordxy EWKT_COMMA point_coordxy EWKT_COMMA point_coordxy extra_pointsxy EWKT_CLOSE_BRACKET */
{
		((gaiaPointPtr)yymsp[-8].minor.yy0)->Next = (gaiaPointPtr)yymsp[-6].minor.yy0; 
		((gaiaPointPtr)yymsp[-6].minor.yy0)->Next = (gaiaPointPtr)yymsp[-4].minor.yy0;
		((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0; 
		((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) ewkt_ring_xy((gaiaPointPtr)yymsp[-8].minor.yy0);
	}
        break;
      case 69: /* extra_rings ::= EWKT_COMMA ring extra_rings */
      case 72: /* extra_ringsm ::= EWKT_COMMA ringm extra_ringsm */ yytestcase(yyruleno==72);
      case 75: /* extra_ringsz ::= EWKT_COMMA ringz extra_ringsz */ yytestcase(yyruleno==75);
      case 78: /* extra_ringszm ::= EWKT_COMMA ringzm extra_ringszm */ yytestcase(yyruleno==78);
{
		((gaiaRingPtr)yymsp[-1].minor.yy0)->Next = (gaiaRingPtr)yymsp[0].minor.yy0;
		yygotominor.yy0 = yymsp[-1].minor.yy0;
	}
        break;
      case 70: /* ringm ::= EWKT_OPEN_BRACKET point_coordxym EWKT_COMMA point_coordxym EWKT_COMMA point_coordxym EWKT_COMMA point_coordxym extra_pointsxym EWKT_CLOSE_BRACKET */
{
		((gaiaPointPtr)yymsp[-8].minor.yy0)->Next = (gaiaPointPtr)yymsp[-6].minor.yy0; 
		((gaiaPointPtr)yymsp[-6].minor.yy0)->Next = (gaiaPointPtr)yymsp[-4].minor.yy0;
		((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0; 
		((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) ewkt_ring_xym((gaiaPointPtr)yymsp[-8].minor.yy0);
	}
        break;
      case 73: /* ringz ::= EWKT_OPEN_BRACKET point_coordxyz EWKT_COMMA point_coordxyz EWKT_COMMA point_coordxyz EWKT_COMMA point_coordxyz extra_pointsxyz EWKT_CLOSE_BRACKET */
{
		((gaiaPointPtr)yymsp[-8].minor.yy0)->Next = (gaiaPointPtr)yymsp[-6].minor.yy0; 
		((gaiaPointPtr)yymsp[-6].minor.yy0)->Next = (gaiaPointPtr)yymsp[-4].minor.yy0;
		((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0; 
		((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) ewkt_ring_xyz((gaiaPointPtr)yymsp[-8].minor.yy0);
	}
        break;
      case 76: /* ringzm ::= EWKT_OPEN_BRACKET point_coordxyzm EWKT_COMMA point_coordxyzm EWKT_COMMA point_coordxyzm EWKT_COMMA point_coordxyzm extra_pointsxyzm EWKT_CLOSE_BRACKET */
{
		((gaiaPointPtr)yymsp[-8].minor.yy0)->Next = (gaiaPointPtr)yymsp[-6].minor.yy0; 
		((gaiaPointPtr)yymsp[-6].minor.yy0)->Next = (gaiaPointPtr)yymsp[-4].minor.yy0;
		((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0; 
		((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) ewkt_ring_xyzm((gaiaPointPtr)yymsp[-8].minor.yy0);
	}
        break;
      case 83: /* multipoint_text ::= EWKT_OPEN_BRACKET point_coordxy extra_pointsxy EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) ewkt_multipoint_xy((gaiaPointPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 84: /* multipoint_textm ::= EWKT_OPEN_BRACKET point_coordxym extra_pointsxym EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) ewkt_multipoint_xym((gaiaPointPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 85: /* multipoint_textz ::= EWKT_OPEN_BRACKET point_coordxyz extra_pointsxyz EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) ewkt_multipoint_xyz((gaiaPointPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 86: /* multipoint_textzm ::= EWKT_OPEN_BRACKET point_coordxyzm extra_pointsxyzm EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) ewkt_multipoint_xyzm((gaiaPointPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 91: /* multilinestring_text ::= EWKT_OPEN_BRACKET linestring_text multilinestring_text2 EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaLinestringPtr)yymsp[-2].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) ewkt_multilinestring_xy((gaiaLinestringPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 93: /* multilinestring_text2 ::= EWKT_COMMA linestring_text multilinestring_text2 */
      case 96: /* multilinestring_textm2 ::= EWKT_COMMA linestring_textm multilinestring_textm2 */ yytestcase(yyruleno==96);
      case 99: /* multilinestring_textz2 ::= EWKT_COMMA linestring_textz multilinestring_textz2 */ yytestcase(yyruleno==99);
      case 102: /* multilinestring_textzm2 ::= EWKT_COMMA linestring_textzm multilinestring_textzm2 */ yytestcase(yyruleno==102);
{ ((gaiaLinestringPtr)yymsp[-1].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[0].minor.yy0;  yygotominor.yy0 = yymsp[-1].minor.yy0; }
        break;
      case 94: /* multilinestring_textm ::= EWKT_OPEN_BRACKET linestring_textm multilinestring_textm2 EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaLinestringPtr)yymsp[-2].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) ewkt_multilinestring_xym((gaiaLinestringPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 97: /* multilinestring_textz ::= EWKT_OPEN_BRACKET linestring_textz multilinestring_textz2 EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaLinestringPtr)yymsp[-2].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) ewkt_multilinestring_xyz((gaiaLinestringPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 100: /* multilinestring_textzm ::= EWKT_OPEN_BRACKET linestring_textzm multilinestring_textzm2 EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaLinestringPtr)yymsp[-2].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) ewkt_multilinestring_xyzm((gaiaLinestringPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 107: /* multipolygon_text ::= EWKT_OPEN_BRACKET polygon_text multipolygon_text2 EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPolygonPtr)yymsp[-2].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) ewkt_multipolygon_xy((gaiaPolygonPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 109: /* multipolygon_text2 ::= EWKT_COMMA polygon_text multipolygon_text2 */
      case 112: /* multipolygon_textm2 ::= EWKT_COMMA polygon_textm multipolygon_textm2 */ yytestcase(yyruleno==112);
      case 115: /* multipolygon_textz2 ::= EWKT_COMMA polygon_textz multipolygon_textz2 */ yytestcase(yyruleno==115);
      case 118: /* multipolygon_textzm2 ::= EWKT_COMMA polygon_textzm multipolygon_textzm2 */ yytestcase(yyruleno==118);
{ ((gaiaPolygonPtr)yymsp[-1].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[0].minor.yy0;  yygotominor.yy0 = yymsp[-1].minor.yy0; }
        break;
      case 110: /* multipolygon_textm ::= EWKT_OPEN_BRACKET polygon_textm multipolygon_textm2 EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPolygonPtr)yymsp[-2].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) ewkt_multipolygon_xym((gaiaPolygonPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 113: /* multipolygon_textz ::= EWKT_OPEN_BRACKET polygon_textz multipolygon_textz2 EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPolygonPtr)yymsp[-2].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) ewkt_multipolygon_xyz((gaiaPolygonPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 116: /* multipolygon_textzm ::= EWKT_OPEN_BRACKET polygon_textzm multipolygon_textzm2 EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPolygonPtr)yymsp[-2].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) ewkt_multipolygon_xyzm((gaiaPolygonPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 123: /* geocoll_text ::= EWKT_OPEN_BRACKET point geocoll_text2 EWKT_CLOSE_BRACKET */
      case 124: /* geocoll_text ::= EWKT_OPEN_BRACKET linestring geocoll_text2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==124);
      case 125: /* geocoll_text ::= EWKT_OPEN_BRACKET polygon geocoll_text2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==125);
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) ewkt_geomColl_xy((gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 127: /* geocoll_text2 ::= EWKT_COMMA point geocoll_text2 */
      case 128: /* geocoll_text2 ::= EWKT_COMMA linestring geocoll_text2 */ yytestcase(yyruleno==128);
      case 129: /* geocoll_text2 ::= EWKT_COMMA polygon geocoll_text2 */ yytestcase(yyruleno==129);
      case 134: /* geocoll_textm2 ::= EWKT_COMMA pointm geocoll_textm2 */ yytestcase(yyruleno==134);
      case 135: /* geocoll_textm2 ::= EWKT_COMMA linestringm geocoll_textm2 */ yytestcase(yyruleno==135);
      case 136: /* geocoll_textm2 ::= EWKT_COMMA polygonm geocoll_textm2 */ yytestcase(yyruleno==136);
      case 141: /* geocoll_textz2 ::= EWKT_COMMA pointz geocoll_textz2 */ yytestcase(yyruleno==141);
      case 142: /* geocoll_textz2 ::= EWKT_COMMA linestringz geocoll_textz2 */ yytestcase(yyruleno==142);
      case 143: /* geocoll_textz2 ::= EWKT_COMMA polygonz geocoll_textz2 */ yytestcase(yyruleno==143);
      case 148: /* geocoll_textzm2 ::= EWKT_COMMA pointzm geocoll_textzm2 */ yytestcase(yyruleno==148);
      case 149: /* geocoll_textzm2 ::= EWKT_COMMA linestringzm geocoll_textzm2 */ yytestcase(yyruleno==149);
      case 150: /* geocoll_textzm2 ::= EWKT_COMMA polygonzm geocoll_textzm2 */ yytestcase(yyruleno==150);
{
		((gaiaGeomCollPtr)yymsp[-1].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[0].minor.yy0;
		yygotominor.yy0 = yymsp[-1].minor.yy0;
	}
        break;
      case 130: /* geocoll_textm ::= EWKT_OPEN_BRACKET pointm geocoll_textm2 EWKT_CLOSE_BRACKET */
      case 131: /* geocoll_textm ::= EWKT_OPEN_BRACKET linestringm geocoll_textm2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==131);
      case 132: /* geocoll_textm ::= EWKT_OPEN_BRACKET polygonm geocoll_textm2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==132);
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) ewkt_geomColl_xym((gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 137: /* geocoll_textz ::= EWKT_OPEN_BRACKET pointz geocoll_textz2 EWKT_CLOSE_BRACKET */
      case 138: /* geocoll_textz ::= EWKT_OPEN_BRACKET linestringz geocoll_textz2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==138);
      case 139: /* geocoll_textz ::= EWKT_OPEN_BRACKET polygonz geocoll_textz2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==139);
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) ewkt_geomColl_xyz((gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 144: /* geocoll_textzm ::= EWKT_OPEN_BRACKET pointzm geocoll_textzm2 EWKT_CLOSE_BRACKET */
      case 145: /* geocoll_textzm ::= EWKT_OPEN_BRACKET linestringzm geocoll_textzm2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==145);
      case 146: /* geocoll_textzm ::= EWKT_OPEN_BRACKET polygonzm geocoll_textzm2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==146);
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) ewkt_geomColl_xyzm((gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      default:
      /* (0) main ::= in */ yytestcase(yyruleno==0);
      /* (1) in ::= */ yytestcase(yyruleno==1);
      /* (2) in ::= in state EWKT_NEWLINE */ yytestcase(yyruleno==2);
      /* (3) state ::= program */ yytestcase(yyruleno==3);
      /* (4) program ::= geo_text */ yytestcase(yyruleno==4);
      /* (5) program ::= geo_textm */ yytestcase(yyruleno==5);
        break;
  };
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yypParser->yyidx -= yysize;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,(YYCODETYPE)yygoto);
  if( yyact < YYNSTATE ){
#ifdef NDEBUG
    /* If we are not debugging and the reduce action popped at least
    ** one element off the stack, then we can push the new element back
    ** onto the stack here, and skip the stack overflow test in yy_shift().
    ** That gives a significant speed improvement. */
    if( yysize ){
      yypParser->yyidx++;
      yymsp -= yysize-1;
      yymsp->stateno = (YYACTIONTYPE)yyact;
      yymsp->major = (YYCODETYPE)yygoto;
      yymsp->minor = yygotominor;
    }else
#endif
    {
      yy_shift(yypParser,yyact,yygoto,&yygotominor);
    }
  }else{
    assert( yyact == YYNSTATE + YYNRULE + 1 );
    yy_accept(yypParser);
  }
}

/*
** The following code executes when the parse fails
*/
#ifndef YYNOERRORRECOVERY
static void yy_parse_failed(
  yyParser *yypParser           /* The parser */
){
  ParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  YYMINORTYPE yyminor            /* The minor type of the error token */
){
  ParseARG_FETCH;
#define TOKEN (yyminor.yy0)

/* 
** when the LEMON parser encounters an error
** then this global variable is set 
*/
	ewkt_parse_error = 1;
	*result = NULL;
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  ParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "ParseAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
void Parse(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  ParseTOKENTYPE yyminor       /* The value for the token */
  ParseARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  int yyact;            /* The parser action. */
  int yyendofinput;     /* True if we are at the end of input */
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  yyParser *yypParser;  /* The parser */

  /* (re)initialize the parser, if necessary */
  yypParser = (yyParser*)yyp;
  if( yypParser->yyidx<0 ){
#if YYSTACKDEPTH<=0
    if( yypParser->yystksz <=0 ){
      /*memset(&yyminorunion, 0, sizeof(yyminorunion));*/
      yyminorunion = yyzerominor;
      yyStackOverflow(yypParser, &yyminorunion);
      return;
    }
#endif
    yypParser->yyidx = 0;
    yypParser->yyerrcnt = -1;
    yypParser->yystack[0].stateno = 0;
    yypParser->yystack[0].major = 0;
  }
  yyminorunion.yy0 = yyminor;
  yyendofinput = (yymajor==0);
  ParseARG_STORE;

#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput %s\n",yyTracePrompt,yyTokenName[yymajor]);
  }
#endif

  do{
    yyact = yy_find_shift_action(yypParser,(YYCODETYPE)yymajor);
    if( yyact<YYNSTATE ){
      assert( !yyendofinput );  /* Impossible to shift the $ token */
      yy_shift(yypParser,yyact,yymajor,&yyminorunion);
      yypParser->yyerrcnt--;
      yymajor = YYNOCODE;
    }else if( yyact < YYNSTATE + YYNRULE ){
      yy_reduce(yypParser,yyact-YYNSTATE);
    }else{
      assert( yyact == YY_ERROR_ACTION );
#ifdef YYERRORSYMBOL
      int yymx;
#endif
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".  
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yypParser->yyerrcnt<0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yymx = yypParser->yystack[yypParser->yyidx].major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(yypParser, (YYCODETYPE)yymajor,&yyminorunion);
        yymajor = YYNOCODE;
      }else{
         while(
          yypParser->yyidx >= 0 &&
          yymx != YYERRORSYMBOL &&
          (yyact = yy_find_reduce_action(
                        yypParser->yystack[yypParser->yyidx].stateno,
                        YYERRORSYMBOL)) >= YYNSTATE
        ){
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yyidx < 0 || yymajor==0 ){
          yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          YYMINORTYPE u2;
          u2.YYERRSYMDT = 0;
          yy_shift(yypParser,yyact,YYERRORSYMBOL,&u2);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
#elif defined(YYNOERRORRECOVERY)
      /* If the YYNOERRORRECOVERY macro is defined, then do not attempt to
      ** do any kind of error recovery.  Instead, simply invoke the syntax
      ** error routine and continue going as if nothing had happened.
      **
      ** Applications can set this macro (for example inside %include) if
      ** they intend to abandon the parse upon the first syntax error seen.
      */
      yy_syntax_error(yypParser,yymajor,yyminorunion);
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      yymajor = YYNOCODE;
      
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yypParser->yyerrcnt<=0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && yypParser->yyidx>=0 );
  return;
}


/*
 EWKT_LEMON_END - LEMON generated code ends here 
*/

















/*
** CAVEAT: there is an incompatibility between LEMON and FLEX
** this macro resolves the issue
*/
#undef yy_accept
#define yy_accept	yy_ewkt_flex_accept




/*
 EWKT_FLEX_START - FLEX generated code starts here 
*/

#line 3 "lex.Ewkt.c"

#define  YY_INT_ALIGNED short int

/* A lexical scanner generated by flex */

#define yy_create_buffer Ewkt_create_buffer
#define yy_delete_buffer Ewkt_delete_buffer
#define yy_flex_debug Ewkt_flex_debug
#define yy_init_buffer Ewkt_init_buffer
#define yy_flush_buffer Ewkt_flush_buffer
#define yy_load_buffer_state Ewkt_load_buffer_state
#define yy_switch_to_buffer Ewkt_switch_to_buffer
#define yyin Ewktin
#define yyleng Ewktleng
#define yylex Ewktlex
#define yylineno Ewktlineno
#define yyout Ewktout
#define yyrestart Ewktrestart
#define yytext Ewkttext
#define yywrap Ewktwrap
#define yyalloc Ewktalloc
#define yyrealloc Ewktrealloc
#define yyfree Ewktfree

#define FLEX_SCANNER
#define YY_FLEX_MAJOR_VERSION 2
#define YY_FLEX_MINOR_VERSION 5
#define YY_FLEX_SUBMINOR_VERSION 35
#if YY_FLEX_SUBMINOR_VERSION > 0
#define FLEX_BETA
#endif

/* First, we deal with  platform-specific or compiler-specific issues. */

/* begin standard C headers. */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

/* end standard C headers. */

/* flex integer type definitions */

#ifndef FLEXINT_H
#define FLEXINT_H

/* C99 systems have <inttypes.h>. Non-C99 systems may or may not. */

#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L

/* C99 says to define __STDC_LIMIT_MACROS before including stdint.h,
 * if you want the limit (max/min) macros for int types. 
 */
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS 1
#endif

#include <inttypes.h>
typedef int8_t flex_int8_t;
typedef uint8_t flex_uint8_t;
typedef int16_t flex_int16_t;
typedef uint16_t flex_uint16_t;
typedef int32_t flex_int32_t;
typedef uint32_t flex_uint32_t;
#else
typedef signed char flex_int8_t;
typedef short int flex_int16_t;
typedef int flex_int32_t;
typedef unsigned char flex_uint8_t; 
typedef unsigned short int flex_uint16_t;
typedef unsigned int flex_uint32_t;

/* Limits of integral types. */
#ifndef INT8_MIN
#define INT8_MIN               (-128)
#endif
#ifndef INT16_MIN
#define INT16_MIN              (-32767-1)
#endif
#ifndef INT32_MIN
#define INT32_MIN              (-2147483647-1)
#endif
#ifndef INT8_MAX
#define INT8_MAX               (127)
#endif
#ifndef INT16_MAX
#define INT16_MAX              (32767)
#endif
#ifndef INT32_MAX
#define INT32_MAX              (2147483647)
#endif
#ifndef UINT8_MAX
#define UINT8_MAX              (255U)
#endif
#ifndef UINT16_MAX
#define UINT16_MAX             (65535U)
#endif
#ifndef UINT32_MAX
#define UINT32_MAX             (4294967295U)
#endif

#endif /* ! C99 */

#endif /* ! FLEXINT_H */

#ifdef __cplusplus

/* The "const" storage-class-modifier is valid. */
#define YY_USE_CONST

#else	/* ! __cplusplus */

/* C99 requires __STDC__ to be defined as 1. */
#if defined (__STDC__)

#define YY_USE_CONST

#endif	/* defined (__STDC__) */
#endif	/* ! __cplusplus */

#ifdef YY_USE_CONST
#define yyconst const
#else
#define yyconst
#endif

/* Returned upon end-of-file. */
#define YY_NULL 0

/* Promotes a possibly negative, possibly signed char to an unsigned
 * integer for use as an array index.  If the signed char is negative,
 * we want to instead treat it as an 8-bit unsigned char, hence the
 * double cast.
 */
#define YY_SC_TO_UI(c) ((unsigned int) (unsigned char) c)

/* Enter a start condition.  This macro really ought to take a parameter,
 * but we do it the disgusting crufty way forced on us by the ()-less
 * definition of BEGIN.
 */
#define BEGIN (yy_start) = 1 + 2 *

/* Translate the current start state into a value that can be later handed
 * to BEGIN to return to the state.  The YYSTATE alias is for lex
 * compatibility.
 */
#define YY_START (((yy_start) - 1) / 2)
#define YYSTATE YY_START

/* Action number for EOF rule of a given start state. */
#define YY_STATE_EOF(state) (YY_END_OF_BUFFER + state + 1)

/* Special action meaning "start processing a new file". */
#define YY_NEW_FILE Ewktrestart(Ewktin  )

#define YY_END_OF_BUFFER_CHAR 0

/* Size of default input buffer. */
#ifndef YY_BUF_SIZE
#ifdef __ia64__
/* On IA-64, the buffer size is 16k, not 8k.
 * Moreover, YY_BUF_SIZE is 2*YY_READ_BUF_SIZE in the general case.
 * Ditto for the __ia64__ case accordingly.
 */
#define YY_BUF_SIZE 32768
#else
#define YY_BUF_SIZE 16384
#endif /* __ia64__ */
#endif

/* The state buf must be large enough to hold one state per character in the main buffer.
 */
#define YY_STATE_BUF_SIZE   ((YY_BUF_SIZE + 2) * sizeof(yy_state_type))

#ifndef YY_TYPEDEF_YY_BUFFER_STATE
#define YY_TYPEDEF_YY_BUFFER_STATE
typedef struct yy_buffer_state *YY_BUFFER_STATE;
#endif

extern int Ewktleng;

extern FILE *Ewktin, *Ewktout;

#define EOB_ACT_CONTINUE_SCAN 0
#define EOB_ACT_END_OF_FILE 1
#define EOB_ACT_LAST_MATCH 2

    #define YY_LESS_LINENO(n)
    
/* Return all but the first "n" matched characters back to the input stream. */
#define yyless(n) \
	do \
		{ \
		/* Undo effects of setting up Ewkttext. */ \
        int yyless_macro_arg = (n); \
        YY_LESS_LINENO(yyless_macro_arg);\
		*yy_cp = (yy_hold_char); \
		YY_RESTORE_YY_MORE_OFFSET \
		(yy_c_buf_p) = yy_cp = yy_bp + yyless_macro_arg - YY_MORE_ADJ; \
		YY_DO_BEFORE_ACTION; /* set up Ewkttext again */ \
		} \
	while ( 0 )

#define unput(c) yyunput( c, (yytext_ptr)  )

#ifndef YY_TYPEDEF_YY_SIZE_T
#define YY_TYPEDEF_YY_SIZE_T
typedef size_t yy_size_t;
#endif

#ifndef YY_STRUCT_YY_BUFFER_STATE
#define YY_STRUCT_YY_BUFFER_STATE
struct yy_buffer_state
	{
	FILE *yy_input_file;

	char *yy_ch_buf;		/* input buffer */
	char *yy_buf_pos;		/* current position in input buffer */

	/* Size of input buffer in bytes, not including room for EOB
	 * characters.
	 */
	yy_size_t yy_buf_size;

	/* Number of characters read into yy_ch_buf, not including EOB
	 * characters.
	 */
	int yy_n_chars;

	/* Whether we "own" the buffer - i.e., we know we created it,
	 * and can realloc() it to grow it, and should free() it to
	 * delete it.
	 */
	int yy_is_our_buffer;

	/* Whether this is an "interactive" input source; if so, and
	 * if we're using stdio for input, then we want to use getc()
	 * instead of fread(), to make sure we stop fetching input after
	 * each newline.
	 */
	int yy_is_interactive;

	/* Whether we're considered to be at the beginning of a line.
	 * If so, '^' rules will be active on the next match, otherwise
	 * not.
	 */
	int yy_at_bol;

    int yy_bs_lineno; /**< The line count. */
    int yy_bs_column; /**< The column count. */
    
	/* Whether to try to fill the input buffer when we reach the
	 * end of it.
	 */
	int yy_fill_buffer;

	int yy_buffer_status;

#define YY_BUFFER_NEW 0
#define YY_BUFFER_NORMAL 1
	/* When an EOF's been seen but there's still some text to process
	 * then we mark the buffer as YY_EOF_PENDING, to indicate that we
	 * shouldn't try reading from the input source any more.  We might
	 * still have a bunch of tokens to match, though, because of
	 * possible backing-up.
	 *
	 * When we actually see the EOF, we change the status to "new"
	 * (via Ewktrestart()), so that the user can continue scanning by
	 * just pointing Ewktin at a new input file.
	 */
#define YY_BUFFER_EOF_PENDING 2

	};
#endif /* !YY_STRUCT_YY_BUFFER_STATE */

/* Stack of input buffers. */
static size_t yy_buffer_stack_top = 0; /**< index of top of stack. */
static size_t yy_buffer_stack_max = 0; /**< capacity of stack. */
static YY_BUFFER_STATE * yy_buffer_stack = 0; /**< Stack as an array. */

/* We provide macros for accessing buffer states in case in the
 * future we want to put the buffer states in a more general
 * "scanner state".
 *
 * Returns the top of the stack, or NULL.
 */
#define YY_CURRENT_BUFFER ( (yy_buffer_stack) \
                          ? (yy_buffer_stack)[(yy_buffer_stack_top)] \
                          : NULL)

/* Same as previous macro, but useful when we know that the buffer stack is not
 * NULL or when we need an lvalue. For internal use only.
 */
#define YY_CURRENT_BUFFER_LVALUE (yy_buffer_stack)[(yy_buffer_stack_top)]

/* yy_hold_char holds the character lost when Ewkttext is formed. */
static char yy_hold_char;
static int yy_n_chars;		/* number of characters read into yy_ch_buf */
int Ewktleng;

/* Points to current character in buffer. */
static char *yy_c_buf_p = (char *) 0;
static int yy_init = 0;		/* whether we need to initialize */
static int yy_start = 0;	/* start state number */

/* Flag which is used to allow Ewktwrap()'s to do buffer switches
 * instead of setting up a fresh Ewktin.  A bit of a hack ...
 */
static int yy_did_buffer_switch_on_eof;

void Ewktrestart (FILE *input_file  );
void Ewkt_switch_to_buffer (YY_BUFFER_STATE new_buffer  );
YY_BUFFER_STATE Ewkt_create_buffer (FILE *file,int size  );
void Ewkt_delete_buffer (YY_BUFFER_STATE b  );
void Ewkt_flush_buffer (YY_BUFFER_STATE b  );
void Ewktpush_buffer_state (YY_BUFFER_STATE new_buffer  );
void Ewktpop_buffer_state (void );

static void Ewktensure_buffer_stack (void );
static void Ewkt_load_buffer_state (void );
static void Ewkt_init_buffer (YY_BUFFER_STATE b,FILE *file  );

#define YY_FLUSH_BUFFER Ewkt_flush_buffer(YY_CURRENT_BUFFER )

YY_BUFFER_STATE Ewkt_scan_buffer (char *base,yy_size_t size  );
YY_BUFFER_STATE Ewkt_scan_string (yyconst char *yy_str  );
YY_BUFFER_STATE Ewkt_scan_bytes (yyconst char *bytes,int len  );

void *Ewktalloc (yy_size_t  );
void *Ewktrealloc (void *,yy_size_t  );
void Ewktfree (void *  );

#define yy_new_buffer Ewkt_create_buffer

#define yy_set_interactive(is_interactive) \
	{ \
	if ( ! YY_CURRENT_BUFFER ){ \
        Ewktensure_buffer_stack (); \
		YY_CURRENT_BUFFER_LVALUE =    \
            Ewkt_create_buffer(Ewktin,YY_BUF_SIZE ); \
	} \
	YY_CURRENT_BUFFER_LVALUE->yy_is_interactive = is_interactive; \
	}

#define yy_set_bol(at_bol) \
	{ \
	if ( ! YY_CURRENT_BUFFER ){\
        Ewktensure_buffer_stack (); \
		YY_CURRENT_BUFFER_LVALUE =    \
            Ewkt_create_buffer(Ewktin,YY_BUF_SIZE ); \
	} \
	YY_CURRENT_BUFFER_LVALUE->yy_at_bol = at_bol; \
	}

#define YY_AT_BOL() (YY_CURRENT_BUFFER_LVALUE->yy_at_bol)

/* Begin user sect3 */

typedef unsigned char YY_CHAR;

FILE *Ewktin = (FILE *) 0, *Ewktout = (FILE *) 0;

typedef int yy_state_type;

extern int Ewktlineno;

int Ewktlineno = 1;

extern char *Ewkttext;
#define yytext_ptr Ewkttext

static yy_state_type yy_get_previous_state (void );
static yy_state_type yy_try_NUL_trans (yy_state_type current_state  );
static int yy_get_next_buffer (void );
static void yy_fatal_error (yyconst char msg[]  );

/* Done after the current pattern has been matched and before the
 * corresponding action - sets up Ewkttext.
 */
#define YY_DO_BEFORE_ACTION \
	(yytext_ptr) = yy_bp; \
	Ewktleng = (size_t) (yy_cp - yy_bp); \
	(yy_hold_char) = *yy_cp; \
	*yy_cp = '\0'; \
	(yy_c_buf_p) = yy_cp;

#define YY_NUM_RULES 22
#define YY_END_OF_BUFFER 23
/* This struct is not used in this scanner,
   but its presence is necessary. */
struct yy_trans_info
	{
	flex_int32_t yy_verify;
	flex_int32_t yy_nxt;
	};
static yyconst flex_int16_t yy_accept[93] =
    {   0,
        0,    0,   23,   21,   19,   20,    3,    4,   21,    2,
       21,    1,   21,   21,   21,   21,    1,    1,    1,    1,
        0,    0,    0,    0,    1,    1,    1,    0,    0,    0,
        0,    0,    1,    1,    0,    0,    0,    0,    0,    0,
        0,    0,    5,    0,    0,    0,    0,    0,    6,    0,
        0,    0,    0,    0,    9,    0,    0,    0,    0,    0,
       10,    0,    0,    0,    0,    0,    0,    7,    0,   11,
        0,    0,    8,    0,   12,    0,    0,    0,   15,    0,
        0,   16,    0,    0,    0,   13,    0,   14,    0,   17,
       18,    0

    } ;

static yyconst flex_int32_t yy_ec[256] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    2,    3,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    2,    1,    1,    1,    1,    1,    1,    1,    4,
        5,    1,    6,    7,    8,    9,    1,   10,   10,   10,
       10,   10,   10,   10,   10,   10,   10,    1,    1,    1,
        1,    1,    1,    1,    1,    1,   11,    1,   12,    1,
       13,    1,   14,    1,    1,   15,   16,   17,   18,   19,
        1,   20,   21,   22,   23,    1,    1,    1,   24,    1,
        1,    1,    1,    1,    1,    1,    1,    1,   25,    1,

       26,    1,   27,    1,   28,    1,    1,   29,   30,   31,
       32,   33,    1,   34,   35,   36,   37,    1,    1,    1,
       38,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,

        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1
    } ;

static yyconst flex_int32_t yy_meta[39] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1
    } ;

static yyconst flex_int16_t yy_base[93] =
    {   0,
        0,    0,  187,  188,  188,  188,  188,  188,  174,  188,
      173,   30,   29,   28,   20,   26,   36,   38,  172,   40,
       33,   35,   38,   45,  171,  166,  165,   38,   49,   40,
       46,   40,  164,  163,   57,   49,   57,   50,   66,   58,
       59,   72,   66,   70,   69,   70,   78,   79,  188,   81,
       75,   86,   90,   94,   94,  104,   99,  105,  101,   95,
      188,  102,  112,  105,  105,  115,  120,  120,  115,  122,
      125,  129,  188,  125,  188,  129,  135,  134,  137,  143,
      138,  188,  134,  150,  150,  149,  148,  188,  154,  156,
      188,  188

    } ;

static yyconst flex_int16_t yy_def[93] =
    {   0,
       92,    1,   92,   92,   92,   92,   92,   92,   92,   92,
       92,   92,   92,   92,   92,   92,   92,   92,   92,   92,
       92,   92,   92,   92,   92,   92,   92,   92,   92,   92,
       92,   92,   92,   92,   92,   92,   92,   92,   92,   92,
       92,   92,   92,   92,   92,   92,   92,   92,   92,   92,
       92,   92,   92,   92,   92,   92,   92,   92,   92,   92,
       92,   92,   92,   92,   92,   92,   92,   92,   92,   92,
       92,   92,   92,   92,   92,   92,   92,   92,   92,   92,
       92,   92,   92,   92,   92,   92,   92,   92,   92,   92,
       92,    0

    } ;

static yyconst flex_int16_t yy_nxt[227] =
    {   0,
        4,    5,    6,    7,    8,    9,   10,   11,    4,   12,
        4,    4,   13,    4,   14,   15,    4,    4,   16,    4,
        4,    4,    4,    4,    4,    4,   13,    4,   14,   15,
        4,    4,   16,    4,    4,    4,    4,    4,   19,   20,
       21,   22,   23,   24,   25,   17,   26,   18,   19,   20,
       28,   29,   30,   35,   21,   22,   23,   24,   31,   32,
       36,   37,   38,   39,   28,   29,   30,   35,   40,   41,
       42,   43,   31,   32,   36,   37,   38,   39,   44,   45,
       46,   49,   40,   41,   42,   43,   47,   50,   51,   52,
       48,   53,   44,   45,   46,   49,   54,   55,   56,   57,

       47,   50,   51,   52,   48,   53,   58,   59,   60,   61,
       54,   55,   56,   57,   62,   63,   64,   65,   66,   67,
       58,   59,   60,   61,   68,   69,   70,   71,   62,   63,
       64,   65,   66,   67,   72,   73,   74,   75,   68,   69,
       70,   71,   76,   77,   78,   79,   80,   81,   72,   73,
       74,   75,   82,   83,   84,   85,   76,   77,   78,   79,
       80,   81,   86,   87,   88,   89,   82,   83,   84,   85,
       90,   91,   34,   33,   27,   34,   86,   87,   88,   89,
       33,   27,   18,   17,   90,   91,   92,    3,   92,   92,
       92,   92,   92,   92,   92,   92,   92,   92,   92,   92,

       92,   92,   92,   92,   92,   92,   92,   92,   92,   92,
       92,   92,   92,   92,   92,   92,   92,   92,   92,   92,
       92,   92,   92,   92,   92,   92
    } ;

static yyconst flex_int16_t yy_chk[227] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,   12,   12,
       13,   14,   15,   16,   17,   17,   18,   18,   20,   20,
       21,   22,   23,   28,   13,   14,   15,   16,   24,   24,
       29,   30,   31,   32,   21,   22,   23,   28,   35,   36,
       37,   38,   24,   24,   29,   30,   31,   32,   39,   40,
       41,   43,   35,   36,   37,   38,   42,   44,   45,   46,
       42,   47,   39,   40,   41,   43,   48,   50,   51,   52,

       42,   44,   45,   46,   42,   47,   53,   54,   54,   55,
       48,   50,   51,   52,   56,   57,   58,   59,   60,   62,
       53,   54,   54,   55,   63,   64,   65,   66,   56,   57,
       58,   59,   60,   62,   67,   68,   69,   70,   63,   64,
       65,   66,   71,   72,   74,   76,   77,   78,   67,   68,
       69,   70,   79,   80,   81,   83,   71,   72,   74,   76,
       77,   78,   84,   85,   86,   87,   79,   80,   81,   83,
       89,   90,   34,   33,   27,   26,   84,   85,   86,   87,
       25,   19,   11,    9,   89,   90,    3,   92,   92,   92,
       92,   92,   92,   92,   92,   92,   92,   92,   92,   92,

       92,   92,   92,   92,   92,   92,   92,   92,   92,   92,
       92,   92,   92,   92,   92,   92,   92,   92,   92,   92,
       92,   92,   92,   92,   92,   92
    } ;

static yy_state_type yy_last_accepting_state;
static char *yy_last_accepting_cpos;

extern int Ewkt_flex_debug;
int Ewkt_flex_debug = 0;

/* The intent behind this definition is that it'll catch
 * any uses of REJECT which flex missed.
 */
#define REJECT reject_used_but_not_detected
#define yymore() yymore_used_but_not_detected
#define YY_MORE_ADJ 0
#define YY_RESTORE_YY_MORE_OFFSET
char *Ewkttext;
/* 
 EwktLexer.l -- EWKT parser - FLEX config
  
 version 2.4, 2011 May 14

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

/* For debugging purposes */
int ewkt_line = 1, ewkt_col = 1;

/**
*  The main string-token matcher.
*  The lower case part is probably not needed.  We should really be converting 
*  The string to all uppercase/lowercase to make it case iNsEnSiTiVe.
*  What Flex will do is, For the input string, beginning from the front, Flex
*  will try to match with any of the defined tokens from below.  Flex will 
*  then match the string of longest length.  Suppose the string is: POINTM,
*  Flex would match both POINT and POINTM, but since POINTM is the longer
*  of the two tokens, FLEX will match POINTM.
*/

#define INITIAL 0

#ifndef YY_NO_UNISTD_H
/* Special case for "unistd.h", since it is non-ANSI. We include it way
 * down here because we want the user's section 1 to have been scanned first.
 * The user has a chance to override it with an option.
 */
#include <unistd.h>
#endif

#ifndef YY_EXTRA_TYPE
#define YY_EXTRA_TYPE void *
#endif

static int yy_init_globals (void );

/* Accessor methods to globals.
   These are made visible to non-reentrant scanners for convenience. */

int Ewktlex_destroy (void );

int Ewktget_debug (void );

void Ewktset_debug (int debug_flag  );

YY_EXTRA_TYPE Ewktget_extra (void );

void Ewktset_extra (YY_EXTRA_TYPE user_defined  );

FILE *Ewktget_in (void );

void Ewktset_in  (FILE * in_str  );

FILE *Ewktget_out (void );

void Ewktset_out  (FILE * out_str  );

int Ewktget_leng (void );

char *Ewktget_text (void );

int Ewktget_lineno (void );

void Ewktset_lineno (int line_number  );

/* Macros after this point can all be overridden by user definitions in
 * section 1.
 */

#ifndef YY_SKIP_YYWRAP
#ifdef __cplusplus
extern "C" int Ewktwrap (void );
#else
extern int Ewktwrap (void );
#endif
#endif

    static void yyunput (int c,char *buf_ptr  );
    
#ifndef yytext_ptr
static void yy_flex_strncpy (char *,yyconst char *,int );
#endif

#ifdef YY_NEED_STRLEN
static int yy_flex_strlen (yyconst char * );
#endif

#ifndef YY_NO_INPUT

#ifdef __cplusplus
static int yyinput (void );
#else
static int input (void );
#endif

#endif

/* Amount of stuff to slurp up with each read. */
#ifndef YY_READ_BUF_SIZE
#ifdef __ia64__
/* On IA-64, the buffer size is 16k, not 8k */
#define YY_READ_BUF_SIZE 16384
#else
#define YY_READ_BUF_SIZE 8192
#endif /* __ia64__ */
#endif

/* Copy whatever the last rule matched to the standard output. */
#ifndef ECHO
/* This used to be an fputs(), but since the string might contain NUL's,
 * we now use fwrite().
 */
#define ECHO do { if (fwrite( Ewkttext, Ewktleng, 1, Ewktout )) {} } while (0)
#endif

/* Gets input and stuffs it into "buf".  number of characters read, or YY_NULL,
 * is returned in "result".
 */
#ifndef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( YY_CURRENT_BUFFER_LVALUE->yy_is_interactive ) \
		{ \
		int c = '*'; \
		size_t n; \
		for ( n = 0; n < max_size && \
			     (c = getc( Ewktin )) != EOF && c != '\n'; ++n ) \
			buf[n] = (char) c; \
		if ( c == '\n' ) \
			buf[n++] = (char) c; \
		if ( c == EOF && ferror( Ewktin ) ) \
			YY_FATAL_ERROR( "input in flex scanner failed" ); \
		result = n; \
		} \
	else \
		{ \
		errno=0; \
		while ( (result = fread(buf, 1, max_size, Ewktin))==0 && ferror(Ewktin)) \
			{ \
			if( errno != EINTR) \
				{ \
				YY_FATAL_ERROR( "input in flex scanner failed" ); \
				break; \
				} \
			errno=0; \
			clearerr(Ewktin); \
			} \
		}\
\

#endif

/* No semi-colon after return; correct usage is to write "yyterminate();" -
 * we don't want an extra ';' after the "return" because that will cause
 * some compilers to complain about unreachable statements.
 */
#ifndef yyterminate
#define yyterminate() return YY_NULL
#endif

/* Number of entries by which start-condition stack grows. */
#ifndef YY_START_STACK_INCR
#define YY_START_STACK_INCR 25
#endif

/* Report a fatal error. */
#ifndef YY_FATAL_ERROR
#define YY_FATAL_ERROR(msg) yy_fatal_error( msg )
#endif

/* end tables serialization structures and prototypes */

/* Default declaration of generated scanner - a define so the user can
 * easily add parameters.
 */
#ifndef YY_DECL
#define YY_DECL_IS_OURS 1

extern int Ewktlex (void);

#define YY_DECL int Ewktlex (void)
#endif /* !YY_DECL */

/* Code executed at the beginning of each rule, after Ewkttext and Ewktleng
 * have been set up.
 */
#ifndef YY_USER_ACTION
#define YY_USER_ACTION
#endif

/* Code executed at the end of each rule. */
#ifndef YY_BREAK
#define YY_BREAK break;
#endif

#define YY_RULE_SETUP \
	YY_USER_ACTION

/** The main scanner function which does all the work.
 */
YY_DECL
{
	register yy_state_type yy_current_state;
	register char *yy_cp, *yy_bp;
	register int yy_act;
    
	if ( !(yy_init) )
		{
		(yy_init) = 1;

#ifdef YY_USER_INIT
		YY_USER_INIT;
#endif

		if ( ! (yy_start) )
			(yy_start) = 1;	/* first start state */

		if ( ! Ewktin )
			Ewktin = stdin;

		if ( ! Ewktout )
			Ewktout = stdout;

		if ( ! YY_CURRENT_BUFFER ) {
			Ewktensure_buffer_stack ();
			YY_CURRENT_BUFFER_LVALUE =
				Ewkt_create_buffer(Ewktin,YY_BUF_SIZE );
		}

		Ewkt_load_buffer_state( );
		}

	while ( 1 )		/* loops until end-of-file is reached */
		{
		yy_cp = (yy_c_buf_p);

		/* Support of Ewkttext. */
		*yy_cp = (yy_hold_char);

		/* yy_bp points to the position in yy_ch_buf of the start of
		 * the current run.
		 */
		yy_bp = yy_cp;

		yy_current_state = (yy_start);
yy_match:
		do
			{
			register YY_CHAR yy_c = yy_ec[YY_SC_TO_UI(*yy_cp)];
			if ( yy_accept[yy_current_state] )
				{
				(yy_last_accepting_state) = yy_current_state;
				(yy_last_accepting_cpos) = yy_cp;
				}
			while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
				{
				yy_current_state = (int) yy_def[yy_current_state];
				if ( yy_current_state >= 93 )
					yy_c = yy_meta[(unsigned int) yy_c];
				}
			yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
			++yy_cp;
			}
		while ( yy_base[yy_current_state] != 188 );

yy_find_action:
		yy_act = yy_accept[yy_current_state];
		if ( yy_act == 0 )
			{ /* have to back up */
			yy_cp = (yy_last_accepting_cpos);
			yy_current_state = (yy_last_accepting_state);
			yy_act = yy_accept[yy_current_state];
			}

		YY_DO_BEFORE_ACTION;

do_action:	/* This label is used only to access EOF actions. */

		switch ( yy_act )
	{ /* beginning of action switch */
			case 0: /* must back up */
			/* undo the effects of YY_DO_BEFORE_ACTION */
			*yy_cp = (yy_hold_char);
			yy_cp = (yy_last_accepting_cpos);
			yy_current_state = (yy_last_accepting_state);
			goto yy_find_action;

case 1:
YY_RULE_SETUP
{ ewkt_col += (int) strlen(Ewkttext);  EwktLval.dval = atof(Ewkttext); return EWKT_NUM; }
	YY_BREAK
case 2:
YY_RULE_SETUP
{ EwktLval.dval = 0; return EWKT_COMMA; }
	YY_BREAK
case 3:
YY_RULE_SETUP
{ EwktLval.dval = 0; return EWKT_OPEN_BRACKET; }
	YY_BREAK
case 4:
YY_RULE_SETUP
{ EwktLval.dval = 0; return EWKT_CLOSE_BRACKET; }
	YY_BREAK
case 5:
YY_RULE_SETUP
{ EwktLval.dval = 0; return EWKT_POINT; }
	YY_BREAK
case 6:
YY_RULE_SETUP
{ EwktLval.dval = 0; return EWKT_POINT_M; }
	YY_BREAK
case 7:
YY_RULE_SETUP
{ EwktLval.dval = 0; return EWKT_LINESTRING; }
	YY_BREAK
case 8:
YY_RULE_SETUP
{ EwktLval.dval = 0; return EWKT_LINESTRING_M; }
	YY_BREAK
case 9:
YY_RULE_SETUP
{ EwktLval.dval = 0; return EWKT_POLYGON; }
	YY_BREAK
case 10:
YY_RULE_SETUP
{ EwktLval.dval = 0; return EWKT_POLYGON_M; }
	YY_BREAK
case 11:
YY_RULE_SETUP
{ EwktLval.dval = 0; return EWKT_MULTIPOINT; }
	YY_BREAK
case 12:
YY_RULE_SETUP
{ EwktLval.dval = 0; return EWKT_MULTIPOINT_M; }
	YY_BREAK
case 13:
YY_RULE_SETUP
{ EwktLval.dval = 0; return EWKT_MULTILINESTRING; }
	YY_BREAK
case 14:
YY_RULE_SETUP
{ EwktLval.dval = 0; return EWKT_MULTILINESTRING_M; }
	YY_BREAK
case 15:
YY_RULE_SETUP
{ EwktLval.dval = 0; return EWKT_MULTIPOLYGON; }
	YY_BREAK
case 16:
YY_RULE_SETUP
{ EwktLval.dval = 0; return EWKT_MULTIPOLYGON_M; }
	YY_BREAK
case 17:
YY_RULE_SETUP
{ EwktLval.dval = 0; return EWKT_GEOMETRYCOLLECTION; }
	YY_BREAK
case 18:
YY_RULE_SETUP
{ EwktLval.dval = 0; return EWKT_GEOMETRYCOLLECTION_M; }
	YY_BREAK
case 19:
YY_RULE_SETUP
{ ewkt_col += (int) strlen(Ewkttext); }               /* ignore but count white space */
	YY_BREAK
case 20:
/* rule 20 can match eol */
YY_RULE_SETUP
{ ewkt_col = 0; ++ewkt_line; }
	YY_BREAK
case 21:
YY_RULE_SETUP
{ ewkt_col += (int) strlen(Ewkttext); return -1; }
	YY_BREAK
case 22:
YY_RULE_SETUP
ECHO;
	YY_BREAK
case YY_STATE_EOF(INITIAL):
	yyterminate();

	case YY_END_OF_BUFFER:
		{
		/* Amount of text matched not including the EOB char. */
		int yy_amount_of_matched_text = (int) (yy_cp - (yytext_ptr)) - 1;

		/* Undo the effects of YY_DO_BEFORE_ACTION. */
		*yy_cp = (yy_hold_char);
		YY_RESTORE_YY_MORE_OFFSET

		if ( YY_CURRENT_BUFFER_LVALUE->yy_buffer_status == YY_BUFFER_NEW )
			{
			/* We're scanning a new file or input source.  It's
			 * possible that this happened because the user
			 * just pointed Ewktin at a new source and called
			 * Ewktlex().  If so, then we have to assure
			 * consistency between YY_CURRENT_BUFFER and our
			 * globals.  Here is the right place to do so, because
			 * this is the first action (other than possibly a
			 * back-up) that will match for the new input source.
			 */
			(yy_n_chars) = YY_CURRENT_BUFFER_LVALUE->yy_n_chars;
			YY_CURRENT_BUFFER_LVALUE->yy_input_file = Ewktin;
			YY_CURRENT_BUFFER_LVALUE->yy_buffer_status = YY_BUFFER_NORMAL;
			}

		/* Note that here we test for yy_c_buf_p "<=" to the position
		 * of the first EOB in the buffer, since yy_c_buf_p will
		 * already have been incremented past the NUL character
		 * (since all states make transitions on EOB to the
		 * end-of-buffer state).  Contrast this with the test
		 * in input().
		 */
		if ( (yy_c_buf_p) <= &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[(yy_n_chars)] )
			{ /* This was really a NUL. */
			yy_state_type yy_next_state;

			(yy_c_buf_p) = (yytext_ptr) + yy_amount_of_matched_text;

			yy_current_state = yy_get_previous_state(  );

			/* Okay, we're now positioned to make the NUL
			 * transition.  We couldn't have
			 * yy_get_previous_state() go ahead and do it
			 * for us because it doesn't know how to deal
			 * with the possibility of jamming (and we don't
			 * want to build jamming into it because then it
			 * will run more slowly).
			 */

			yy_next_state = yy_try_NUL_trans( yy_current_state );

			yy_bp = (yytext_ptr) + YY_MORE_ADJ;

			if ( yy_next_state )
				{
				/* Consume the NUL. */
				yy_cp = ++(yy_c_buf_p);
				yy_current_state = yy_next_state;
				goto yy_match;
				}

			else
				{
				yy_cp = (yy_c_buf_p);
				goto yy_find_action;
				}
			}

		else switch ( yy_get_next_buffer(  ) )
			{
			case EOB_ACT_END_OF_FILE:
				{
				(yy_did_buffer_switch_on_eof) = 0;

				if ( Ewktwrap( ) )
					{
					/* Note: because we've taken care in
					 * yy_get_next_buffer() to have set up
					 * Ewkttext, we can now set up
					 * yy_c_buf_p so that if some total
					 * hoser (like flex itself) wants to
					 * call the scanner after we return the
					 * YY_NULL, it'll still work - another
					 * YY_NULL will get returned.
					 */
					(yy_c_buf_p) = (yytext_ptr) + YY_MORE_ADJ;

					yy_act = YY_STATE_EOF(YY_START);
					goto do_action;
					}

				else
					{
					if ( ! (yy_did_buffer_switch_on_eof) )
						YY_NEW_FILE;
					}
				break;
				}

			case EOB_ACT_CONTINUE_SCAN:
				(yy_c_buf_p) =
					(yytext_ptr) + yy_amount_of_matched_text;

				yy_current_state = yy_get_previous_state(  );

				yy_cp = (yy_c_buf_p);
				yy_bp = (yytext_ptr) + YY_MORE_ADJ;
				goto yy_match;

			case EOB_ACT_LAST_MATCH:
				(yy_c_buf_p) =
				&YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[(yy_n_chars)];

				yy_current_state = yy_get_previous_state(  );

				yy_cp = (yy_c_buf_p);
				yy_bp = (yytext_ptr) + YY_MORE_ADJ;
				goto yy_find_action;
			}
		break;
		}

	default:
		YY_FATAL_ERROR(
			"fatal flex scanner internal error--no action found" );
	} /* end of action switch */
		} /* end of scanning one token */
} /* end of Ewktlex */

/* yy_get_next_buffer - try to read in a new buffer
 *
 * Returns a code representing an action:
 *	EOB_ACT_LAST_MATCH -
 *	EOB_ACT_CONTINUE_SCAN - continue scanning from current position
 *	EOB_ACT_END_OF_FILE - end of file
 */
static int yy_get_next_buffer (void)
{
    	register char *dest = YY_CURRENT_BUFFER_LVALUE->yy_ch_buf;
	register char *source = (yytext_ptr);
	register int number_to_move, i;
	int ret_val;

	if ( (yy_c_buf_p) > &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[(yy_n_chars) + 1] )
		YY_FATAL_ERROR(
		"fatal flex scanner internal error--end of buffer missed" );

	if ( YY_CURRENT_BUFFER_LVALUE->yy_fill_buffer == 0 )
		{ /* Don't try to fill the buffer, so this is an EOF. */
		if ( (yy_c_buf_p) - (yytext_ptr) - YY_MORE_ADJ == 1 )
			{
			/* We matched a single character, the EOB, so
			 * treat this as a final EOF.
			 */
			return EOB_ACT_END_OF_FILE;
			}

		else
			{
			/* We matched some text prior to the EOB, first
			 * process it.
			 */
			return EOB_ACT_LAST_MATCH;
			}
		}

	/* Try to read more data. */

	/* First move last chars to start of buffer. */
	number_to_move = (int) ((yy_c_buf_p) - (yytext_ptr)) - 1;

	for ( i = 0; i < number_to_move; ++i )
		*(dest++) = *(source++);

	if ( YY_CURRENT_BUFFER_LVALUE->yy_buffer_status == YY_BUFFER_EOF_PENDING )
		/* don't do the read, it's not guaranteed to return an EOF,
		 * just force an EOF
		 */
		YY_CURRENT_BUFFER_LVALUE->yy_n_chars = (yy_n_chars) = 0;

	else
		{
			int num_to_read =
			YY_CURRENT_BUFFER_LVALUE->yy_buf_size - number_to_move - 1;

		while ( num_to_read <= 0 )
			{ /* Not enough room in the buffer - grow it. */

			/* just a shorter name for the current buffer */
			YY_BUFFER_STATE b = YY_CURRENT_BUFFER;

			int yy_c_buf_p_offset =
				(int) ((yy_c_buf_p) - b->yy_ch_buf);

			if ( b->yy_is_our_buffer )
				{
				int new_size = b->yy_buf_size * 2;

				if ( new_size <= 0 )
					b->yy_buf_size += b->yy_buf_size / 8;
				else
					b->yy_buf_size *= 2;

				b->yy_ch_buf = (char *)
					/* Include room in for 2 EOB chars. */
					Ewktrealloc((void *) b->yy_ch_buf,b->yy_buf_size + 2  );
				}
			else
				/* Can't grow it, we don't own it. */
				b->yy_ch_buf = 0;

			if ( ! b->yy_ch_buf )
				YY_FATAL_ERROR(
				"fatal error - scanner input buffer overflow" );

			(yy_c_buf_p) = &b->yy_ch_buf[yy_c_buf_p_offset];

			num_to_read = YY_CURRENT_BUFFER_LVALUE->yy_buf_size -
						number_to_move - 1;

			}

		if ( num_to_read > YY_READ_BUF_SIZE )
			num_to_read = YY_READ_BUF_SIZE;

		/* Read in more data. */
		YY_INPUT( (&YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[number_to_move]),
			(yy_n_chars), (size_t) num_to_read );

		YY_CURRENT_BUFFER_LVALUE->yy_n_chars = (yy_n_chars);
		}

	if ( (yy_n_chars) == 0 )
		{
		if ( number_to_move == YY_MORE_ADJ )
			{
			ret_val = EOB_ACT_END_OF_FILE;
			Ewktrestart(Ewktin  );
			}

		else
			{
			ret_val = EOB_ACT_LAST_MATCH;
			YY_CURRENT_BUFFER_LVALUE->yy_buffer_status =
				YY_BUFFER_EOF_PENDING;
			}
		}

	else
		ret_val = EOB_ACT_CONTINUE_SCAN;

	if ((yy_size_t) ((yy_n_chars) + number_to_move) > YY_CURRENT_BUFFER_LVALUE->yy_buf_size) {
		/* Extend the array by 50%, plus the number we really need. */
		yy_size_t new_size = (yy_n_chars) + number_to_move + ((yy_n_chars) >> 1);
		YY_CURRENT_BUFFER_LVALUE->yy_ch_buf = (char *) Ewktrealloc((void *) YY_CURRENT_BUFFER_LVALUE->yy_ch_buf,new_size  );
		if ( ! YY_CURRENT_BUFFER_LVALUE->yy_ch_buf )
			YY_FATAL_ERROR( "out of dynamic memory in yy_get_next_buffer()" );
	}

	(yy_n_chars) += number_to_move;
	YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[(yy_n_chars)] = YY_END_OF_BUFFER_CHAR;
	YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[(yy_n_chars) + 1] = YY_END_OF_BUFFER_CHAR;

	(yytext_ptr) = &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[0];

	return ret_val;
}

/* yy_get_previous_state - get the state just before the EOB char was reached */

    static yy_state_type yy_get_previous_state (void)
{
	register yy_state_type yy_current_state;
	register char *yy_cp;
    
	yy_current_state = (yy_start);

	for ( yy_cp = (yytext_ptr) + YY_MORE_ADJ; yy_cp < (yy_c_buf_p); ++yy_cp )
		{
		register YY_CHAR yy_c = (*yy_cp ? yy_ec[YY_SC_TO_UI(*yy_cp)] : 1);
		if ( yy_accept[yy_current_state] )
			{
			(yy_last_accepting_state) = yy_current_state;
			(yy_last_accepting_cpos) = yy_cp;
			}
		while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
			{
			yy_current_state = (int) yy_def[yy_current_state];
			if ( yy_current_state >= 93 )
				yy_c = yy_meta[(unsigned int) yy_c];
			}
		yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
		}

	return yy_current_state;
}

/* yy_try_NUL_trans - try to make a transition on the NUL character
 *
 * synopsis
 *	next_state = yy_try_NUL_trans( current_state );
 */
    static yy_state_type yy_try_NUL_trans  (yy_state_type yy_current_state )
{
	register int yy_is_jam;
    	register char *yy_cp = (yy_c_buf_p);

	register YY_CHAR yy_c = 1;
	if ( yy_accept[yy_current_state] )
		{
		(yy_last_accepting_state) = yy_current_state;
		(yy_last_accepting_cpos) = yy_cp;
		}
	while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
		{
		yy_current_state = (int) yy_def[yy_current_state];
		if ( yy_current_state >= 93 )
			yy_c = yy_meta[(unsigned int) yy_c];
		}
	yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
	yy_is_jam = (yy_current_state == 92);

	return yy_is_jam ? 0 : yy_current_state;
}

    static void yyunput (int c, register char * yy_bp )
{
	register char *yy_cp;
    
    yy_cp = (yy_c_buf_p);

	/* undo effects of setting up Ewkttext */
	*yy_cp = (yy_hold_char);

	if ( yy_cp < YY_CURRENT_BUFFER_LVALUE->yy_ch_buf + 2 )
		{ /* need to shift things up to make room */
		/* +2 for EOB chars. */
		register int number_to_move = (yy_n_chars) + 2;
		register char *dest = &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[
					YY_CURRENT_BUFFER_LVALUE->yy_buf_size + 2];
		register char *source =
				&YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[number_to_move];

		while ( source > YY_CURRENT_BUFFER_LVALUE->yy_ch_buf )
			*--dest = *--source;

		yy_cp += (int) (dest - source);
		yy_bp += (int) (dest - source);
		YY_CURRENT_BUFFER_LVALUE->yy_n_chars =
			(yy_n_chars) = YY_CURRENT_BUFFER_LVALUE->yy_buf_size;

		if ( yy_cp < YY_CURRENT_BUFFER_LVALUE->yy_ch_buf + 2 )
			YY_FATAL_ERROR( "flex scanner push-back overflow" );
		}

	*--yy_cp = (char) c;

	(yytext_ptr) = yy_bp;
	(yy_hold_char) = *yy_cp;
	(yy_c_buf_p) = yy_cp;
}

#ifndef YY_NO_INPUT
#ifdef __cplusplus
    static int yyinput (void)
#else
    static int input  (void)
#endif

{
	int c;
    
	*(yy_c_buf_p) = (yy_hold_char);

	if ( *(yy_c_buf_p) == YY_END_OF_BUFFER_CHAR )
		{
		/* yy_c_buf_p now points to the character we want to return.
		 * If this occurs *before* the EOB characters, then it's a
		 * valid NUL; if not, then we've hit the end of the buffer.
		 */
		if ( (yy_c_buf_p) < &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[(yy_n_chars)] )
			/* This was really a NUL. */
			*(yy_c_buf_p) = '\0';

		else
			{ /* need more input */
			int offset = (yy_c_buf_p) - (yytext_ptr);
			++(yy_c_buf_p);

			switch ( yy_get_next_buffer(  ) )
				{
				case EOB_ACT_LAST_MATCH:
					/* This happens because yy_g_n_b()
					 * sees that we've accumulated a
					 * token and flags that we need to
					 * try matching the token before
					 * proceeding.  But for input(),
					 * there's no matching to consider.
					 * So convert the EOB_ACT_LAST_MATCH
					 * to EOB_ACT_END_OF_FILE.
					 */

					/* Reset buffer status. */
					Ewktrestart(Ewktin );

					/*FALLTHROUGH*/

				case EOB_ACT_END_OF_FILE:
					{
					if ( Ewktwrap( ) )
						return EOF;

					if ( ! (yy_did_buffer_switch_on_eof) )
						YY_NEW_FILE;
#ifdef __cplusplus
					return yyinput();
#else
					return input();
#endif
					}

				case EOB_ACT_CONTINUE_SCAN:
					(yy_c_buf_p) = (yytext_ptr) + offset;
					break;
				}
			}
		}

	c = *(unsigned char *) (yy_c_buf_p);	/* cast for 8-bit char's */
	*(yy_c_buf_p) = '\0';	/* preserve Ewkttext */
	(yy_hold_char) = *++(yy_c_buf_p);

	return c;
}
#endif	/* ifndef YY_NO_INPUT */

/** Immediately switch to a different input stream.
 * @param input_file A readable stream.
 * 
 * @note This function does not reset the start condition to @c INITIAL .
 */
    void Ewktrestart  (FILE * input_file )
{
    
	if ( ! YY_CURRENT_BUFFER ){
        Ewktensure_buffer_stack ();
		YY_CURRENT_BUFFER_LVALUE =
            Ewkt_create_buffer(Ewktin,YY_BUF_SIZE );
	}

	Ewkt_init_buffer(YY_CURRENT_BUFFER,input_file );
	Ewkt_load_buffer_state( );
}

/** Switch to a different input buffer.
 * @param new_buffer The new input buffer.
 * 
 */
    void Ewkt_switch_to_buffer  (YY_BUFFER_STATE  new_buffer )
{
    
	/* TODO. We should be able to replace this entire function body
	 * with
	 *		Ewktpop_buffer_state();
	 *		Ewktpush_buffer_state(new_buffer);
     */
	Ewktensure_buffer_stack ();
	if ( YY_CURRENT_BUFFER == new_buffer )
		return;

	if ( YY_CURRENT_BUFFER )
		{
		/* Flush out information for old buffer. */
		*(yy_c_buf_p) = (yy_hold_char);
		YY_CURRENT_BUFFER_LVALUE->yy_buf_pos = (yy_c_buf_p);
		YY_CURRENT_BUFFER_LVALUE->yy_n_chars = (yy_n_chars);
		}

	YY_CURRENT_BUFFER_LVALUE = new_buffer;
	Ewkt_load_buffer_state( );

	/* We don't actually know whether we did this switch during
	 * EOF (Ewktwrap()) processing, but the only time this flag
	 * is looked at is after Ewktwrap() is called, so it's safe
	 * to go ahead and always set it.
	 */
	(yy_did_buffer_switch_on_eof) = 1;
}

static void Ewkt_load_buffer_state  (void)
{
    	(yy_n_chars) = YY_CURRENT_BUFFER_LVALUE->yy_n_chars;
	(yytext_ptr) = (yy_c_buf_p) = YY_CURRENT_BUFFER_LVALUE->yy_buf_pos;
	Ewktin = YY_CURRENT_BUFFER_LVALUE->yy_input_file;
	(yy_hold_char) = *(yy_c_buf_p);
}

/** Allocate and initialize an input buffer state.
 * @param file A readable stream.
 * @param size The character buffer size in bytes. When in doubt, use @c YY_BUF_SIZE.
 * 
 * @return the allocated buffer state.
 */
    YY_BUFFER_STATE Ewkt_create_buffer  (FILE * file, int  size )
{
	YY_BUFFER_STATE b;
    
	b = (YY_BUFFER_STATE) Ewktalloc(sizeof( struct yy_buffer_state )  );
	if ( ! b )
		YY_FATAL_ERROR( "out of dynamic memory in Ewkt_create_buffer()" );

	b->yy_buf_size = size;

	/* yy_ch_buf has to be 2 characters longer than the size given because
	 * we need to put in 2 end-of-buffer characters.
	 */
	b->yy_ch_buf = (char *) Ewktalloc(b->yy_buf_size + 2  );
	if ( ! b->yy_ch_buf )
		YY_FATAL_ERROR( "out of dynamic memory in Ewkt_create_buffer()" );

	b->yy_is_our_buffer = 1;

	Ewkt_init_buffer(b,file );

	return b;
}

/** Destroy the buffer.
 * @param b a buffer created with Ewkt_create_buffer()
 * 
 */
    void Ewkt_delete_buffer (YY_BUFFER_STATE  b )
{
    
	if ( ! b )
		return;

	if ( b == YY_CURRENT_BUFFER ) /* Not sure if we should pop here. */
		YY_CURRENT_BUFFER_LVALUE = (YY_BUFFER_STATE) 0;

	if ( b->yy_is_our_buffer )
		Ewktfree((void *) b->yy_ch_buf  );

	Ewktfree((void *) b  );
}

#ifndef __cplusplus
extern int isatty (int );
#endif /* __cplusplus */
    
/* Initializes or reinitializes a buffer.
 * This function is sometimes called more than once on the same buffer,
 * such as during a Ewktrestart() or at EOF.
 */
    static void Ewkt_init_buffer  (YY_BUFFER_STATE  b, FILE * file )

{
	int oerrno = errno;
    
	Ewkt_flush_buffer(b );

	b->yy_input_file = file;
	b->yy_fill_buffer = 1;

    /* If b is the current buffer, then Ewkt_init_buffer was _probably_
     * called from Ewktrestart() or through yy_get_next_buffer.
     * In that case, we don't want to reset the lineno or column.
     */
    if (b != YY_CURRENT_BUFFER){
        b->yy_bs_lineno = 1;
        b->yy_bs_column = 0;
    }

        b->yy_is_interactive = file ? (isatty( fileno(file) ) > 0) : 0;
    
	errno = oerrno;
}

/** Discard all buffered characters. On the next scan, YY_INPUT will be called.
 * @param b the buffer state to be flushed, usually @c YY_CURRENT_BUFFER.
 * 
 */
    void Ewkt_flush_buffer (YY_BUFFER_STATE  b )
{
    	if ( ! b )
		return;

	b->yy_n_chars = 0;

	/* We always need two end-of-buffer characters.  The first causes
	 * a transition to the end-of-buffer state.  The second causes
	 * a jam in that state.
	 */
	b->yy_ch_buf[0] = YY_END_OF_BUFFER_CHAR;
	b->yy_ch_buf[1] = YY_END_OF_BUFFER_CHAR;

	b->yy_buf_pos = &b->yy_ch_buf[0];

	b->yy_at_bol = 1;
	b->yy_buffer_status = YY_BUFFER_NEW;

	if ( b == YY_CURRENT_BUFFER )
		Ewkt_load_buffer_state( );
}

/** Pushes the new state onto the stack. The new state becomes
 *  the current state. This function will allocate the stack
 *  if necessary.
 *  @param new_buffer The new state.
 *  
 */
void Ewktpush_buffer_state (YY_BUFFER_STATE new_buffer )
{
    	if (new_buffer == NULL)
		return;

	Ewktensure_buffer_stack();

	/* This block is copied from Ewkt_switch_to_buffer. */
	if ( YY_CURRENT_BUFFER )
		{
		/* Flush out information for old buffer. */
		*(yy_c_buf_p) = (yy_hold_char);
		YY_CURRENT_BUFFER_LVALUE->yy_buf_pos = (yy_c_buf_p);
		YY_CURRENT_BUFFER_LVALUE->yy_n_chars = (yy_n_chars);
		}

	/* Only push if top exists. Otherwise, replace top. */
	if (YY_CURRENT_BUFFER)
		(yy_buffer_stack_top)++;
	YY_CURRENT_BUFFER_LVALUE = new_buffer;

	/* copied from Ewkt_switch_to_buffer. */
	Ewkt_load_buffer_state( );
	(yy_did_buffer_switch_on_eof) = 1;
}

/** Removes and deletes the top of the stack, if present.
 *  The next element becomes the new top.
 *  
 */
void Ewktpop_buffer_state (void)
{
    	if (!YY_CURRENT_BUFFER)
		return;

	Ewkt_delete_buffer(YY_CURRENT_BUFFER );
	YY_CURRENT_BUFFER_LVALUE = NULL;
	if ((yy_buffer_stack_top) > 0)
		--(yy_buffer_stack_top);

	if (YY_CURRENT_BUFFER) {
		Ewkt_load_buffer_state( );
		(yy_did_buffer_switch_on_eof) = 1;
	}
}

/* Allocates the stack if it does not exist.
 *  Guarantees space for at least one push.
 */
static void Ewktensure_buffer_stack (void)
{
	int num_to_alloc;
    
	if (!(yy_buffer_stack)) {

		/* First allocation is just for 2 elements, since we don't know if this
		 * scanner will even need a stack. We use 2 instead of 1 to avoid an
		 * immediate realloc on the next call.
         */
		num_to_alloc = 1;
		(yy_buffer_stack) = (struct yy_buffer_state**)Ewktalloc
								(num_to_alloc * sizeof(struct yy_buffer_state*)
								);
		if ( ! (yy_buffer_stack) )
			YY_FATAL_ERROR( "out of dynamic memory in Ewktensure_buffer_stack()" );
								  
		memset((yy_buffer_stack), 0, num_to_alloc * sizeof(struct yy_buffer_state*));
				
		(yy_buffer_stack_max) = num_to_alloc;
		(yy_buffer_stack_top) = 0;
		return;
	}

	if ((yy_buffer_stack_top) >= ((yy_buffer_stack_max)) - 1){

		/* Increase the buffer to prepare for a possible push. */
		int grow_size = 8 /* arbitrary grow size */;

		num_to_alloc = (yy_buffer_stack_max) + grow_size;
		(yy_buffer_stack) = (struct yy_buffer_state**)Ewktrealloc
								((yy_buffer_stack),
								num_to_alloc * sizeof(struct yy_buffer_state*)
								);
		if ( ! (yy_buffer_stack) )
			YY_FATAL_ERROR( "out of dynamic memory in Ewktensure_buffer_stack()" );

		/* zero only the new slots.*/
		memset((yy_buffer_stack) + (yy_buffer_stack_max), 0, grow_size * sizeof(struct yy_buffer_state*));
		(yy_buffer_stack_max) = num_to_alloc;
	}
}

/** Setup the input buffer state to scan directly from a user-specified character buffer.
 * @param base the character buffer
 * @param size the size in bytes of the character buffer
 * 
 * @return the newly allocated buffer state object. 
 */
YY_BUFFER_STATE Ewkt_scan_buffer  (char * base, yy_size_t  size )
{
	YY_BUFFER_STATE b;
    
	if ( size < 2 ||
	     base[size-2] != YY_END_OF_BUFFER_CHAR ||
	     base[size-1] != YY_END_OF_BUFFER_CHAR )
		/* They forgot to leave room for the EOB's. */
		return 0;

	b = (YY_BUFFER_STATE) Ewktalloc(sizeof( struct yy_buffer_state )  );
	if ( ! b )
		YY_FATAL_ERROR( "out of dynamic memory in Ewkt_scan_buffer()" );

	b->yy_buf_size = size - 2;	/* "- 2" to take care of EOB's */
	b->yy_buf_pos = b->yy_ch_buf = base;
	b->yy_is_our_buffer = 0;
	b->yy_input_file = 0;
	b->yy_n_chars = b->yy_buf_size;
	b->yy_is_interactive = 0;
	b->yy_at_bol = 1;
	b->yy_fill_buffer = 0;
	b->yy_buffer_status = YY_BUFFER_NEW;

	Ewkt_switch_to_buffer(b  );

	return b;
}

/** Setup the input buffer state to scan a string. The next call to Ewktlex() will
 * scan from a @e copy of @a str.
 * @param yystr a NUL-terminated string to scan
 * 
 * @return the newly allocated buffer state object.
 * @note If you want to scan bytes that may contain NUL values, then use
 *       Ewkt_scan_bytes() instead.
 */
YY_BUFFER_STATE Ewkt_scan_string (yyconst char * yystr )
{
    
	return Ewkt_scan_bytes(yystr,strlen(yystr) );
}

/** Setup the input buffer state to scan the given bytes. The next call to Ewktlex() will
 * scan from a @e copy of @a bytes.
 * @param yybytes the byte buffer to scan
 * @param _yybytes_len the number of bytes in the buffer pointed to by @a bytes.
 * 
 * @return the newly allocated buffer state object.
 */
YY_BUFFER_STATE Ewkt_scan_bytes  (yyconst char * yybytes, int  _yybytes_len )
{
	YY_BUFFER_STATE b;
	char *buf;
	yy_size_t n;
	int i;
    
	/* Get memory for full buffer, including space for trailing EOB's. */
	n = _yybytes_len + 2;
	buf = (char *) Ewktalloc(n  );
	if ( ! buf )
		YY_FATAL_ERROR( "out of dynamic memory in Ewkt_scan_bytes()" );

	for ( i = 0; i < _yybytes_len; ++i )
		buf[i] = yybytes[i];

	buf[_yybytes_len] = buf[_yybytes_len+1] = YY_END_OF_BUFFER_CHAR;

	b = Ewkt_scan_buffer(buf,n );
	if ( ! b )
		YY_FATAL_ERROR( "bad buffer in Ewkt_scan_bytes()" );

	/* It's okay to grow etc. this buffer, and we should throw it
	 * away when we're done.
	 */
	b->yy_is_our_buffer = 1;

	return b;
}

#ifndef YY_EXIT_FAILURE
#define YY_EXIT_FAILURE 2
#endif

static void yy_fatal_error (yyconst char* msg )
{
    	(void) fprintf( stderr, "%s\n", msg );
	exit( YY_EXIT_FAILURE );
}

/* Redefine yyless() so it works in section 3 code. */

#undef yyless
#define yyless(n) \
	do \
		{ \
		/* Undo effects of setting up Ewkttext. */ \
        int yyless_macro_arg = (n); \
        YY_LESS_LINENO(yyless_macro_arg);\
		Ewkttext[Ewktleng] = (yy_hold_char); \
		(yy_c_buf_p) = Ewkttext + yyless_macro_arg; \
		(yy_hold_char) = *(yy_c_buf_p); \
		*(yy_c_buf_p) = '\0'; \
		Ewktleng = yyless_macro_arg; \
		} \
	while ( 0 )

/* Accessor  methods (get/set functions) to struct members. */

/** Get the current line number.
 * 
 */
int Ewktget_lineno  (void)
{
        
    return Ewktlineno;
}

/** Get the input stream.
 * 
 */
FILE *Ewktget_in  (void)
{
        return Ewktin;
}

/** Get the output stream.
 * 
 */
FILE *Ewktget_out  (void)
{
        return Ewktout;
}

/** Get the length of the current token.
 * 
 */
int Ewktget_leng  (void)
{
        return Ewktleng;
}

/** Get the current token.
 * 
 */

char *Ewktget_text  (void)
{
        return Ewkttext;
}

/** Set the current line number.
 * @param line_number
 * 
 */
void Ewktset_lineno (int  line_number )
{
    
    Ewktlineno = line_number;
}

/** Set the input stream. This does not discard the current
 * input buffer.
 * @param in_str A readable stream.
 * 
 * @see Ewkt_switch_to_buffer
 */
void Ewktset_in (FILE *  in_str )
{
        Ewktin = in_str ;
}

void Ewktset_out (FILE *  out_str )
{
        Ewktout = out_str ;
}

int Ewktget_debug  (void)
{
        return Ewkt_flex_debug;
}

void Ewktset_debug (int  bdebug )
{
        Ewkt_flex_debug = bdebug ;
}

static int yy_init_globals (void)
{
        /* Initialization is the same as for the non-reentrant scanner.
     * This function is called from Ewktlex_destroy(), so don't allocate here.
     */

    (yy_buffer_stack) = 0;
    (yy_buffer_stack_top) = 0;
    (yy_buffer_stack_max) = 0;
    (yy_c_buf_p) = (char *) 0;
    (yy_init) = 0;
    (yy_start) = 0;

/* Defined in main.c */
#ifdef YY_STDINIT
    Ewktin = stdin;
    Ewktout = stdout;
#else
    Ewktin = (FILE *) 0;
    Ewktout = (FILE *) 0;
#endif

    /* For future reference: Set errno on error, since we are called by
     * Ewktlex_init()
     */
    return 0;
}

/* Ewktlex_destroy is for both reentrant and non-reentrant scanners. */
int Ewktlex_destroy  (void)
{
    
    /* Pop the buffer stack, destroying each element. */
	while(YY_CURRENT_BUFFER){
		Ewkt_delete_buffer(YY_CURRENT_BUFFER  );
		YY_CURRENT_BUFFER_LVALUE = NULL;
		Ewktpop_buffer_state();
	}

	/* Destroy the stack itself. */
	Ewktfree((yy_buffer_stack) );
	(yy_buffer_stack) = NULL;

    /* Reset the globals. This is important in a non-reentrant scanner so the next time
     * Ewktlex() is called, initialization will occur. */
    yy_init_globals( );

    return 0;
}

/*
 * Internal utility routines.
 */

#ifndef yytext_ptr
static void yy_flex_strncpy (char* s1, yyconst char * s2, int n )
{
	register int i;
	for ( i = 0; i < n; ++i )
		s1[i] = s2[i];
}
#endif

#ifdef YY_NEED_STRLEN
static int yy_flex_strlen (yyconst char * s )
{
	register int n;
	for ( n = 0; s[n]; ++n )
		;

	return n;
}
#endif

void *Ewktalloc (yy_size_t  size )
{
	return (void *) malloc( size );
}

void *Ewktrealloc  (void * ptr, yy_size_t  size )
{
	/* The cast to (char *) in the following accommodates both
	 * implementations that use char* generic pointers, and those
	 * that use void* generic pointers.  It works with the latter
	 * because both ANSI C and C++ allow castless assignment from
	 * any pointer type to void*, and deal with argument conversions
	 * as though doing an assignment.
	 */
	return (void *) realloc( (char *) ptr, size );
}

void Ewktfree (void * ptr )
{
	free( (char *) ptr );	/* see Ewktrealloc() for (char *) cast */
}

#define YYTABLES_NAME "yytables"

/**
 * reset the line and column count
 *
 *
 */
void ewkt_reset_lexer(void)
{

  ewkt_line = 1;
  ewkt_col  = 1;

}

/**
 * EwktError() is invoked when the lexer or the parser encounter
 * an error. The error message is passed via *s
 *
 *
 */
void EwktError(char *s)
{
  printf("error: %s at line: %d col: %d\n",s,ewkt_line,ewkt_col);

}

int Ewktwrap(void)
{
  return 1;
}


/* 
 EWKT_FLEX_END - FLEX generated code ends here 
*/



/*
** This is a linked-list struct to store all the values for each token.
** All tokens will have a value of 0, except tokens denoted as NUM.
** NUM tokens are geometry coordinates and will contain the floating
** point number.
*/
typedef struct ewktFlexTokenStruct
{
    double value;
    struct ewktFlexTokenStruct *Next;
} ewktFlexToken;

/*
** Function to clean up the linked-list of token values.
*/
static int
ewkt_cleanup (ewktFlexToken * token)
{
    ewktFlexToken *ptok;
    ewktFlexToken *ptok_n;
    if (token == NULL)
	return 0;
    ptok = token;
    while (ptok)
      {
	  ptok_n = ptok->Next;
	  free (ptok);
	  ptok = ptok_n;
      }
    return 0;
}

static int
findEwktSrid (const char *buffer, int *base_offset)
{
/* attempting to identify the EWKT SRID */
    char dummy[1024];
    char *out;
    const char *in = buffer;
    int end = -1;
    int i;
    *base_offset = 0;

    while (*in != '\0')
      {
	  /* searching the first semi-colon [srid delimiter] */
	  if (*in == ';')
	    {
		end = in - buffer;
		break;
	    }
	  in++;
      }
    if (end < 0)
	return -1;
    in = buffer;
    out = dummy;
    for (i = 0; i < end; i++)
      {
	  /* normalizing whitespaces */
	  char c = *in;
	  if (c == ' ' || c == '\t' || c == '\n')
	    {
		in++;
		continue;
	    }
	  *out++ = *in++;
      }
    *out = '\0';
    if (strncasecmp (dummy, "SRID=", 5) != 0)
	return -1;
    for (i = 5; i < (int) strlen (dummy); i++)
      {
	  if (i == 5)
	    {
		if (dummy[i] == '-' || dummy[i] == '+')
		    continue;
	    }
	  if (dummy[i] >= '0' && dummy[i] <= '9')
	      continue;
	  return -1;
      }
    *base_offset = end + 1;
    return atoi (dummy + 5);
}

gaiaGeomCollPtr
gaiaParseEWKT (const unsigned char *dirty_buffer)
{
    void *pParser = ParseAlloc (malloc);
    /* Linked-list of token values */
    ewktFlexToken *tokens = malloc (sizeof (ewktFlexToken));
    /* Pointer to the head of the list */
    ewktFlexToken *head = tokens;
    int yv;
    int srid;
    int base_offset;
    gaiaGeomCollPtr result = NULL;

    tokens->Next = NULL;
    ewkt_parse_error = 0;

    srid = findEwktSrid ((char *) dirty_buffer, &base_offset);
    Ewkt_scan_string ((char *) dirty_buffer + base_offset);

    /*
       / Keep tokenizing until we reach the end
       / yylex() will return the next matching Token for us.
     */
    while ((yv = yylex ()) != 0)
      {
	  if (yv == -1)
	    {
		ewkt_parse_error = 1;
		break;
	    }
	  tokens->Next = malloc (sizeof (ewktFlexToken));
	  tokens->Next->Next = NULL;
	  /*
	     /EwktLval is a global variable from FLEX.
	     /EwktLval is defined in ewktLexglobal.h
	   */
	  tokens->Next->value = EwktLval.dval;
	  /* Pass the token to the wkt parser created from lemon */
	  Parse (pParser, yv, &(tokens->Next->value), &result);
	  tokens = tokens->Next;
      }
    /* This denotes the end of a line as well as the end of the parser */
    Parse (pParser, EWKT_NEWLINE, 0, &result);
    ParseFree (pParser, free);
    Ewktlex_destroy ();

    /* Assigning the token as the end to avoid seg faults while cleaning */
    tokens->Next = NULL;
    ewkt_cleanup (head);

    if (ewkt_parse_error)
      {
	  if (result)
	      gaiaFreeGeomColl (result);
	  return NULL;
      }

    if (!result)
	return NULL;
    if (!ewktCheckValidity (result))
      {
	  gaiaFreeGeomColl (result);
	  return NULL;
      }

    gaiaMbrGeometry (result);
    result->Srid = srid;

    return result;
}


/*
** CAVEAT: we must now undefine any Lemon/Flex own macro
*/
#undef YYNOCODE
#undef YYNSTATE
#undef YYNRULE
#undef YY_SHIFT_MAX
#undef YY_SHIFT_USE_DFLT
#undef YY_REDUCE_USE_DFLT
#undef YY_REDUCE_MAX
#undef YY_FLUSH_BUFFER
#undef YY_DO_BEFORE_ACTION
#undef YY_NUM_RULES
#undef YY_END_OF_BUFFER
#undef YY_END_FILE
#undef YYACTIONTYPE
#undef YY_SZ_ACTTAB
#undef YY_NEW_FILE
#undef BEGIN
#undef YY_START
#undef YY_CURRENT_BUFFER
#undef YY_CURRENT_BUFFER_LVALUE
#undef YY_STATE_BUF_SIZE
#undef YY_DECL
#undef YY_FATAL_ERROR
#undef YYMINORTYPE
#undef YY_CHAR
#undef YYSTYPE
#undef input
#undef ParseAlloc
#undef ParseFree
#undef ParseStackPeak
#undef Parse
#undef yyalloc
#undef yyfree
#undef yyin
#undef yyleng
#undef yyless
#undef yylex
#undef yylineno
#undef yyout
#undef yyrealloc
#undef yyrestart
#undef yyStackEntry
#undef yytext
#undef yywrap
#undef yyzerominor
#undef yy_accept
#undef yy_action
#undef yy_base
#undef yy_buffer_stack
#undef yy_buffer_stack_max
#undef yy_buffer_stack_top
#undef yy_c_buf_p
#undef yy_chk
#undef yy_create_buffer
#undef yy_def
#undef yy_default
#undef yy_delete_buffer
#undef yy_destructor
#undef yy_ec
#undef yy_fatal_error
#undef yy_find_reduce_action
#undef yy_find_shift_action
#undef yy_flex_debug
#undef yy_flush_buffer
#undef yy_get_next_buffer
#undef yy_get_previous_state
#undef yy_init
#undef yy_init_buffer
#undef yy_init_globals
#undef yy_load_buffer
#undef yy_load_buffer_state
#undef yy_lookahead
#undef yy_meta
#undef yy_new_buffer
#undef yy_nxt
#undef yy_parse_failed
#undef yy_pop_parser_stack
#undef yy_reduce
#undef yy_reduce_ofst
#undef yy_set_bol
#undef yy_set_interactive
#undef yy_shift
#undef yy_shift_ofst
#undef yy_start
#undef yy_state_type
#undef yy_switch_to_buffer
#undef yy_syntax_error
#undef yy_trans_info
#undef yy_try_NUL_trans
#undef yyParser
#undef yyStackEntry
#undef yyStackOverflow
#undef yyRuleInfo
#undef yytext_ptr
#undef yyunput
#undef yyzerominor
#undef ParseARG_SDECL
#undef ParseARG_PDECL
#undef ParseARG_FETCH
#undef ParseARG_STORE
#undef REJECT
#undef yymore
#undef YY_MORE_ADJ
#undef YY_RESTORE_YY_MORE_OFFSET
#undef YY_LESS_LINENO
#undef yyTracePrompt
#undef yyTraceFILE
#undef yyTokenName
#undef yyRuleName
#undef ParseTrace
