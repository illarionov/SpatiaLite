/*

 gg_geoJSON.c -- GeoJSON parser/lexer 
  
 version 2.4, 2011 May 16

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

int geoJSON_parse_error;

static int
geoJsonCheckValidity (gaiaGeomCollPtr geom)
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
geoJSON_setSrid (gaiaGeomCollPtr geom, int *srid)
{
/* setting up the SRID */
    if (!geom)
	return NULL;
    geom->Srid = *srid;
    return geom;
}

static gaiaGeomCollPtr
gaiaGeoJsonGeometryFromPoint (gaiaPointPtr point, int srid)
{
/* builds a GEOMETRY containing a POINT */
    gaiaGeomCollPtr geom = NULL;
    geom = gaiaAllocGeomColl ();
    geom->DeclaredType = GAIA_POINT;
    geom->Srid = srid;
    gaiaAddPointToGeomColl (geom, point->X, point->Y);
    gaiaFreePoint (point);
    return geom;
}

static gaiaGeomCollPtr
gaiaGeoJsonGeometryFromPointZ (gaiaPointPtr point, int srid)
{
/* builds a GEOMETRY containing a POINTZ */
    gaiaGeomCollPtr geom = NULL;
    geom = gaiaAllocGeomCollXYZ ();
    geom->DeclaredType = GAIA_POINTZ;
    geom->Srid = srid;
    gaiaAddPointToGeomCollXYZ (geom, point->X, point->Y, point->Z);
    gaiaFreePoint (point);
    return geom;
}

static gaiaGeomCollPtr
gaiaGeoJsonGeometryFromLinestring (gaiaLinestringPtr line, int srid)
{
/* builds a GEOMETRY containing a LINESTRING */
    gaiaGeomCollPtr geom = NULL;
    gaiaLinestringPtr line2;
    int iv;
    double x;
    double y;
    geom = gaiaAllocGeomColl ();
    geom->DeclaredType = GAIA_LINESTRING;
    geom->Srid = srid;
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
gaiaGeoJsonGeometryFromLinestringZ (gaiaLinestringPtr line, int srid)
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
    geom->Srid = srid;
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

static gaiaPointPtr
geoJSON_point_xy (double *x, double *y)
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
geoJSON_point_xyz (double *x, double *y, double *z)
{
    return gaiaAllocPointXYZ (*x, *y, *z);
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
geoJSON_buildGeomFromPoint (gaiaPointPtr point)
{
    switch (point->DimensionModel)
      {
      case GAIA_XY:
	  return gaiaGeoJsonGeometryFromPoint (point, -1);
	  break;
      case GAIA_XY_Z:
	  return gaiaGeoJsonGeometryFromPointZ (point, -1);
	  break;
      }
    return NULL;
}

static gaiaGeomCollPtr
geoJSON_buildGeomFromPointSrid (gaiaPointPtr point, int *srid)
{
    switch (point->DimensionModel)
      {
      case GAIA_XY:
	  return gaiaGeoJsonGeometryFromPoint (point, *srid);
	  break;
      case GAIA_XY_Z:
	  return gaiaGeoJsonGeometryFromPointZ (point, *srid);
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
geoJSON_linestring_xy (gaiaPointPtr first)
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
geoJSON_linestring_xyz (gaiaPointPtr first)
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
 * Builds a geometry collection from a linestring.
 */
static gaiaGeomCollPtr
geoJSON_buildGeomFromLinestring (gaiaLinestringPtr line)
{
    switch (line->DimensionModel)
      {
      case GAIA_XY:
	  return gaiaGeoJsonGeometryFromLinestring (line, -1);
	  break;
      case GAIA_XY_Z:
	  return gaiaGeoJsonGeometryFromLinestringZ (line, -1);
	  break;
      }
    return NULL;
}

static gaiaGeomCollPtr
geoJSON_buildGeomFromLinestringSrid (gaiaLinestringPtr line, int *srid)
{
    switch (line->DimensionModel)
      {
      case GAIA_XY:
	  return gaiaGeoJsonGeometryFromLinestring (line, *srid);
	  break;
      case GAIA_XY_Z:
	  return gaiaGeoJsonGeometryFromLinestringZ (line, *srid);
	  break;
      }
    return NULL;
}

/*
 * Helper function that determines the number of points in the linked list.
 */
static int
geoJSON_count_points (gaiaPointPtr first)
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
geoJSON_ring_xy (gaiaPointPtr first)
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
    numpoints = geoJSON_count_points (first);
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
geoJSON_ring_xyz (gaiaPointPtr first)
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
    numpoints = geoJSON_count_points (first);
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
 * Helper function that will create any type of polygon (xy, xyz) in SpatiaLite.
 * 
 * first is a gaiaRingPtr to the first ring in a linked list of rings which define the polygon.
 * The first ring in the linked list is the external ring while the rest (if any) are internal rings.
 * All of the rings given to the function are of the same type. There will be at least 1 ring in the list.
 *
 * Returns the polygon defined by the rings given to the function.
 */
static gaiaPolygonPtr
geoJSON_polygon_any_type (gaiaRingPtr first)
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
geoJSON_polygon_xy (gaiaRingPtr first)
{
    return geoJSON_polygon_any_type (first);
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
geoJSON_polygon_xyz (gaiaRingPtr first)
{
    return geoJSON_polygon_any_type (first);
}

/*
 * Builds a geometry collection from a polygon.
 * NOTE: This function may already be implemented in the SpatiaLite code base. If it is, make sure that we
 *              can use it (ie. it doesn't use any other variables or anything else set by Sandro's parser). If you find
 *              that we can use an existing function then ignore this one.
 */
static gaiaGeomCollPtr
geoJSON_buildGeomFromPolygon (gaiaPolygonPtr polygon)
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

static gaiaGeomCollPtr
geoJSON_buildGeomFromPolygonSrid (gaiaPolygonPtr polygon, int *srid)
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
      }
    if (geom == NULL)
      {
	  return NULL;
      }
    geom->DeclaredType = GAIA_POLYGON;
    geom->Srid = *srid;

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
geoJSON_multipoint_xy (gaiaPointPtr first)
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
geoJSON_multipoint_xyz (gaiaPointPtr first)
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
 * Creates a geometry collection containing 2D (xy) linestrings.
 * Parameter first is a gaiaLinestringPtr to the first linestring in a linked list of linestrings which should be added to the
 * collection. All of the *linestrings in the list must be 2D (xy) linestrings. There must be at least 1 linestring in the list.
 * Returns a pointer to the created geometry collection of 2D linestrings. The geometry must have FirstLinestring pointing to the
 * first linestring in the list pointed by first and LastLinestring pointing to the last element of the same list. DimensionModel
 * must be GAIA_XY and DimensionType must be *GAIA_TYPE_LINESTRING.
 */

static gaiaGeomCollPtr
geoJSON_multilinestring_xy (gaiaLinestringPtr first)
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
geoJSON_multilinestring_xyz (gaiaLinestringPtr first)
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
geoJSON_multipolygon_xy (gaiaPolygonPtr first)
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
geoJSON_multipolygon_xyz (gaiaPolygonPtr first)
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

static void
geoJSON_geomColl_common (gaiaGeomCollPtr org, gaiaGeomCollPtr dst)
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
geoJSON_geomColl_xy (gaiaGeomCollPtr first)
{
    gaiaGeomCollPtr geom = gaiaAllocGeomColl ();
    if (geom == NULL)
	return NULL;
    geom->DeclaredType = GAIA_GEOMETRYCOLLECTION;
    geom->DimensionModel = GAIA_XY;
    geoJSON_geomColl_common (first, geom);
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
geoJSON_geomColl_xyz (gaiaGeomCollPtr first)
{
    gaiaGeomCollPtr geom = gaiaAllocGeomColl ();
    if (geom == NULL)
	return NULL;
    geom->DeclaredType = GAIA_GEOMETRYCOLLECTION;
    geom->DimensionModel = GAIA_XY_Z;
    geoJSON_geomColl_common (first, geom);
    return geom;
}








/*
 GEOJSON_LEMON_H_START - LEMON generated header starts here 
*/

#define GEOJSON_NEWLINE                 1
#define GEOJSON_OPEN_BRACE              2
#define GEOJSON_TYPE                    3
#define GEOJSON_COLON                   4
#define GEOJSON_POINT                   5
#define GEOJSON_COMMA                   6
#define GEOJSON_COORDS                  7
#define GEOJSON_CLOSE_BRACE             8
#define GEOJSON_BBOX                    9
#define GEOJSON_OPEN_BRACKET           10
#define GEOJSON_CLOSE_BRACKET          11
#define GEOJSON_CRS                    12
#define GEOJSON_NAME                   13
#define GEOJSON_PROPS                  14
#define GEOJSON_NUM                    15
#define GEOJSON_SHORT_SRID             16
#define GEOJSON_LONG_SRID              17
#define GEOJSON_LINESTRING             18
#define GEOJSON_POLYGON                19
#define GEOJSON_MULTIPOINT             20
#define GEOJSON_MULTILINESTRING        21
#define GEOJSON_MULTIPOLYGON           22
#define GEOJSON_GEOMETRYCOLLECTION     23
#define GEOJSON_GEOMS                  24

/*
 GEOJSON_LEMON_H_END - LEMON generated header ends here 
*/









#ifndef YYSTYPE
typedef union
{
    double dval;
    int ival;
    struct symtab *symp;
} geoJSON_yystype;
#define YYSTYPE geoJSON_yystype
#define YYSTYPE_IS_TRIVIAL 1
#endif


/* extern YYSTYPE yylval; */
YYSTYPE GeoJsonLval;

/*
** CAVEAT: there is an incompatibility between LEMON and FLEX
** this macro resolves the issue
*/
#define yy_accept	yy_geo_json_accept










/*
 GEOJSON_LEMON_START - LEMON generated header starts here 
*/

/* Driver template for the LEMON parser generator.
** The author disclaims copyright to this source code.
*/
/* First off, code is included that follows the "include" declaration
** in the input grammar file. */
#include <stdio.h>
#line 55 "geoJSON.y"

#line 10 "geoJSON.c"
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
#define YYNOCODE 84
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE void *
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
} GEOJSON_YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 1000000
#endif
#define ParseARG_SDECL  gaiaGeomCollPtr *result ;
#define ParseARG_PDECL , gaiaGeomCollPtr *result 
#define ParseARG_FETCH  gaiaGeomCollPtr *result  = yypParser->result 
#define ParseARG_STORE yypParser->result  = result 
#define YYNSTATE 679
#define YYNRULE 159
#define YY_NO_ACTION      (YYNSTATE+YYNRULE+2)
#define YY_ACCEPT_ACTION  (YYNSTATE+YYNRULE+1)
#define YY_ERROR_ACTION   (YYNSTATE+YYNRULE)

/* The yyzerominor constant is used to initialize instances of
** YYMINORTYPE objects to zero. */
static const GEOJSON_YYMINORTYPE geoJSON_yyzerominor = { 0 };

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
static const YYACTIONTYPE geoJSON_yy_action[] = {
 /*     0 */   348,  535,  532,  586,  678,  676,  674,  672,  671,  670,
 /*    10 */   669,  668,  667,  666,  665,  664,  662,  473,  157,   32,
 /*    20 */   106,  105,   88,  403,  118,  121,  215,  679,  492,  484,
 /*    30 */   231,  206,  302,  371,  448,  525,  656,   87,  527,   86,
 /*    40 */   156,  466,  447,  468,  137,  443,  119,  228,  391,  283,
 /*    50 */   155,  154,  455,  153,  152,  304,  157,  317,  106,  105,
 /*    60 */   329,  450,   88,  471,  118,  121,  483,  313,  373,  203,
 /*    70 */   394,  589,  208,  406,  248,  495,  430,  260,  436,    3,
 /*    80 */   289,  267,  506,  478,  449,  372,  336,    2,  401,  333,
 /*    90 */   457,  339,  522,  524,  334,  285,  314,  429,  186,  353,
 /*   100 */    69,  359,  516,  272,  263,  409,  401,  316,  401,  387,
 /*   110 */   234,  379,  509,  511,  284,  116,  290,  508,  242,  237,
 /*   120 */   658,  401,  501,  503,  638,  637,  264,  496,  270,  489,
 /*   130 */   491,  401,  485,  505,  187,  184,  487,  272,  493,  479,
 /*   140 */   481,  410,  474,  416,  839,    1,  401,  315,  470,  190,
 /*   150 */   192,  467,  299,  521,  523,  462,  507,  188,  513,  194,
 /*   160 */   197,  221,  331,  351,  401,  202,  204,  330,  210,  212,
 /*   170 */   221,  324,  464,  488,  209,  335,  337,  587,  453,  269,
 /*   180 */   139,  427,  244,  340,  325,  327,  342,  401,  225,  451,
 /*   190 */   401,  347,  349,   67,  226,  223,  355,  357,  354,  239,
 /*   200 */   277,  320,  341,  122,  143,  401,  486,  469,   33,  362,
 /*   210 */   439,  452,  454,  401,  401,  367,  369,  368,  358,  444,
 /*   220 */   446,  370,  433,  375,  377,  401,  401,  220,   91,   96,
 /*   230 */   310,  236,  432,  434,  401,  251,   90,  424,  426,  401,
 /*   240 */    89,  418,  306,  308,   37,  411,  419,  298,  300,  385,
 /*   250 */   401,   43,  292,  383,  256,  258,  279,  412,  414,  386,
 /*   260 */   350,  408,  428,  401,  293,  147,  402,  404,  401,  390,
 /*   270 */   338,  262,  282,  286,  288,  397,  407,  413,  135,  401,
 /*   280 */   266,  528,  278,  280,  273,  312,  381,   93,  401,  130,
 /*   290 */    95,  142,  299,   92,  572,   97,  388,  209,   98,  651,
 /*   300 */   101,  269,    5,  529,  570,  392,  396,    4,  138,  405,
 /*   310 */   427,  117,  149,  415,  610,  244,  453,  243,  111,  240,
 /*   320 */   238,  115,  236,  109,  456,  133,  230,  232,  614,  126,
 /*   330 */   217,  458,  140,  227,  179,  123,  224,  131,  222,  134,
 /*   340 */   145,  220,  120,  119,  137,  219,  216,  218,  136,  552,
 /*   350 */     6,  533,  214,  634,  461,  146,  591,  201,  605,  196,
 /*   360 */   102,  470,  148,   38,  161,  675,  185,  519,  150,  632,
 /*   370 */   536,  673,  538,   11,   12,  520,  517,  518,   13,  514,
 /*   380 */   465,  540,  515,  328,   15,  504,  549,  542,  510,  512,
 /*   390 */   213,  332,  625,  541,  539,  677,  537,  183,  530,  160,
 /*   400 */   344,  543,  182,  631,  526,   25,  544,  626,   17,   31,
 /*   410 */    16,  624,   18,  499,  498,   24,  500,  623,  497,  195,
 /*   420 */   494,  191,  840,   27,  490,  116,  840,   23,  545,  482,
 /*   430 */   199,  840,  198,  546,  663,   28,  200,   19,  630,  617,
 /*   440 */   144,  246,  180,  622,  193,  621,  480,  159,   20,  254,
 /*   450 */   151,  618,  189,   21,   49,  547,  548,  158,   29,  620,
 /*   460 */   550,  840,  141,  205,   22,  477,  431,  168,  459,  211,
 /*   470 */   475,  476,  619,  425,  840,   34,  162,  472,  108,  633,
 /*   480 */   616,   26,  840,  658,  629,  170,  132,  502,  128,  551,
 /*   490 */    53,  129,  660,  463,  163,  125,  840,  255,  635,  124,
 /*   500 */   661,  207,  178,  840,  177,   35,  657,  659,  636,  127,
 /*   510 */   110,  840,  423,  553,   30,    8,  840,  554,  613,  615,
 /*   520 */   229,  563,   36,  556,  639,   39,  114,  555,  840,  175,
 /*   530 */   607,  113,  235,  840,  165,  641,  112,  460,  445,  840,
 /*   540 */   840,  640,  420,  557,  241,  435,  840,  558,  611,  438,
 /*   550 */   612,  606,    7,   40,  559,  840,  441,  245,  442,  840,
 /*   560 */   654,  249,  840,  560,  440,  252,  247,  840,   45,   42,
 /*   570 */   437,   48,  609,  253,  107,   41,  417,   44,  840,  608,
 /*   580 */   268,  257,  174,  250,  840,   56,  562,   55,  561,  564,
 /*   590 */   840,   47,   46,  233,  655,  840,  422,  261,  259,  421,
 /*   600 */    61,  265,  271,  566,  840,  565,  275,   57,  296,  276,
 /*   610 */   274,   50,  652,   65,  643,   10,  604,  326,  642,  181,
 /*   620 */   603,   99,  176,  653,   51,  389,   59,  840,  393,  319,
 /*   630 */   568,  567,  840,   52,  569,  602,  104,  840,  281,  291,
 /*   640 */   601,  400,  287,  399,  840,  840,  103,  840,  395,  644,
 /*   650 */   398,  840,   54,  295,  294,   60,  100,   63,  840,   62,
 /*   660 */   169,  840,  840,  599,  840,  600,    9,  840,  598,  571,
 /*   670 */    64,  297,  382,  164,  840,   58,   66,  650,  840,  840,
 /*   680 */    14,  649,  301,  378,  840,  573,  597,   94,  374,  840,
 /*   690 */   596,  840,  840,  307,  840,  840,  840,  574,   72,  305,
 /*   700 */   840,   68,  840,  840,  840,  384,  309,  575,  840,  595,
 /*   710 */   311,  376,   71,  840,   70,  303,  628,  840,  594,  167,
 /*   720 */   318,  840,  840,  366,  840,   75,  840,  576,  321,  578,
 /*   730 */    81,  840,  577,  323,  322,  361,  364,   73,   80,  645,
 /*   740 */   840,  840,  365,  166,  593,  356,  840,  840,  363,  360,
 /*   750 */    74,  592,  580,   85,  646,  840,   76,  173,  579,   83,
 /*   760 */   352,  840,  582,   84,   77,  648,  590,   82,  840,  627,
 /*   770 */   171,  581,  840,  588,  172,  583,  840,  584,  343,  840,
 /*   780 */   585,  840,  346,  647,  380,   78,  840,  531,  840,   79,
 /*   790 */   345,  840,  840,  840,  840,  534,
};
static const YYCODETYPE geoJSON_yy_lookahead[] = {
 /*     0 */    28,   29,   30,   31,   32,   33,   34,   35,   36,   37,
 /*    10 */    38,   39,   40,   41,   42,   43,   44,    5,   75,   10,
 /*    20 */    77,   78,   79,    9,   81,   82,   12,    0,    5,    2,
 /*    30 */    18,   19,   20,   21,   22,   23,   15,   79,   24,   81,
 /*    40 */    82,   18,   19,    7,   59,    9,   61,    7,   12,    9,
 /*    50 */    23,   75,   12,   77,   78,    7,   75,    9,   77,   78,
 /*    60 */    12,    7,   79,    9,   81,   82,   12,    5,    7,    5,
 /*    70 */     9,   80,    7,   12,    9,    9,    7,   12,    9,    6,
 /*    80 */    18,   19,   18,   19,   46,   55,   56,   10,   50,    7,
 /*    90 */    24,    9,   73,   74,    7,    9,    9,   47,   48,    7,
 /*   100 */    10,    9,   46,    2,   46,    7,   50,    9,   50,    7,
 /*   110 */    24,    9,   69,   70,    7,    6,    9,   46,   51,   52,
 /*   120 */    11,   50,   69,   70,   16,   17,    7,   46,    9,   69,
 /*   130 */    70,   50,   47,   48,   55,   56,    7,    2,    9,   69,
 /*   140 */    70,    7,   46,    9,   26,   27,   50,   54,   45,   55,
 /*   150 */    56,   45,   49,   69,   70,   49,    7,    7,    9,    9,
 /*   160 */    46,   45,   47,   48,   50,   55,   56,   45,   57,   58,
 /*   170 */    45,   49,   73,   74,   49,   63,   64,   80,   55,   56,
 /*   180 */    10,   57,   58,   46,   63,   64,   46,   50,   47,   48,
 /*   190 */    50,   63,   64,   10,   55,   56,   63,   64,   45,   73,
 /*   200 */    74,   46,   49,   57,   58,   50,   55,   56,   10,   46,
 /*   210 */    46,   69,   70,   50,   50,   63,   64,   73,   74,   65,
 /*   220 */    66,   46,   46,   65,   66,   50,   50,   45,   55,   56,
 /*   230 */    46,   49,   65,   66,   50,   46,   45,   65,   66,   50,
 /*   240 */    49,   45,   63,   64,   10,   49,   46,   57,   58,   45,
 /*   250 */    50,   10,   45,   49,   57,   58,   49,   65,   66,   46,
 /*   260 */    80,   47,   48,   50,   46,    6,   65,   66,   50,   47,
 /*   270 */    48,   47,   48,   57,   58,   46,   73,   74,    6,   50,
 /*   280 */    57,   58,   57,   58,   46,   53,   67,   10,   50,   10,
 /*   290 */    55,   10,   49,    6,   67,    6,   68,   49,   10,   50,
 /*   300 */    56,   56,   10,   73,   68,   50,   50,    6,   10,   76,
 /*   310 */    57,    6,   10,   76,   62,   58,   55,   54,   49,   49,
 /*   320 */    49,   10,   49,   61,   50,    6,   80,   62,   60,    6,
 /*   330 */    80,   71,   57,   53,   50,   45,   45,   10,   45,   45,
 /*   340 */     6,   45,   59,   61,   59,   50,   60,   50,   45,   71,
 /*   350 */    10,    8,   53,   53,   50,   49,   80,   50,    8,   50,
 /*   360 */     4,   45,   49,    4,    6,    8,   54,    7,   58,   54,
 /*   370 */     8,    8,    8,    4,    4,   24,   11,    6,   10,    4,
 /*   380 */    72,    8,    6,    2,    4,    3,   72,    8,    2,   11,
 /*   390 */     2,    6,    8,   76,   76,   74,   76,    6,   76,    4,
 /*   400 */     6,    8,    6,   11,    7,    4,    8,    8,    4,    4,
 /*   410 */    10,    8,   10,    7,    6,    4,    4,    8,   11,    4,
 /*   420 */     4,    7,   83,   10,    6,    6,   83,   10,    8,    7,
 /*   430 */     6,   83,   11,    8,    8,    4,    7,    4,    8,   11,
 /*   440 */     6,    2,    6,    8,    6,    8,    3,    6,    4,    7,
 /*   450 */     4,   11,    7,    4,    4,    8,    8,    4,    4,    8,
 /*   460 */     8,   83,    6,    6,    4,    7,   11,    4,    7,    3,
 /*   470 */    11,    6,    8,    6,   83,    4,    6,    4,    4,   11,
 /*   480 */    11,    4,   83,   11,    8,    6,    6,    6,    6,   11,
 /*   490 */     4,    6,    8,    6,    6,    6,   83,    6,    8,    6,
 /*   500 */     8,    7,    6,   83,    6,    4,   11,    8,    8,    6,
 /*   510 */     4,   83,    7,   11,    4,    4,   83,    8,   11,   11,
 /*   520 */     6,    8,    4,    8,    8,    4,    6,    8,   83,    6,
 /*   530 */     8,    6,    8,   83,    6,    8,    6,   11,    6,   83,
 /*   540 */    83,    8,   11,    8,    8,    7,   83,    8,   11,    4,
 /*   550 */     8,    8,    4,    4,    8,   83,    6,   13,    7,   83,
 /*   560 */     8,    4,   83,    8,   11,   11,    4,   83,   10,   10,
 /*   570 */     4,   10,    8,    6,    4,   10,    4,    4,   83,    8,
 /*   580 */     3,    7,    6,   14,   83,   10,    8,    4,    8,    8,
 /*   590 */    83,    4,    4,   11,    8,   83,    7,   13,    6,    6,
 /*   600 */    10,    4,    4,    8,   83,   11,    6,    4,    7,    7,
 /*   610 */    11,    4,    8,    4,    8,    4,    8,    3,    8,    6,
 /*   620 */     8,    4,    6,    8,    4,    4,    4,   83,    4,    4,
 /*   630 */    11,    8,   83,    4,    8,    8,    6,   83,    7,    4,
 /*   640 */     8,    7,    6,    6,   83,   83,    6,   83,    4,    8,
 /*   650 */    11,   83,   10,    6,   11,    4,    6,    4,   83,   10,
 /*   660 */     6,   83,   83,    8,   83,    8,   10,   83,    8,   11,
 /*   670 */     4,    7,   11,    6,   83,    4,    4,    8,   83,   83,
 /*   680 */    10,    8,    6,    6,   83,   11,    8,    4,    4,   83,
 /*   690 */     8,   83,   83,    7,   83,   83,   83,    8,    4,   11,
 /*   700 */    83,   10,   83,   83,   83,   24,    6,    8,   83,   11,
 /*   710 */     4,   24,   10,   83,    4,    4,    8,   83,   11,    6,
 /*   720 */     4,   83,   83,   11,   83,   10,   83,    8,   11,    8,
 /*   730 */    10,   83,    8,    7,    6,    6,    6,    4,    4,    8,
 /*   740 */    83,   83,    7,    4,    8,    7,   83,   83,   11,    4,
 /*   750 */    10,    8,    8,    4,    8,   83,    4,    6,    8,    4,
 /*   760 */     7,   83,    8,    4,    4,    8,    8,    4,   83,    8,
 /*   770 */     6,    1,   83,    8,    6,    8,   83,   11,    4,   83,
 /*   780 */     8,   83,   11,    8,    6,    4,   83,   11,   83,   10,
 /*   790 */     7,   83,   83,   83,   83,   11,
};
#define YY_SHIFT_USE_DFLT (-1)
#define YY_SHIFT_MAX 531
static const short geoJSON_yy_shift_ofst[] = {
 /*     0 */    -1,   27,  388,  381,  386,  386,  381,  108,  101,   21,
 /*    10 */   198,   77,  234,   21,   21,  234,   21,  234,   21,  234,
 /*    20 */   101,  198,  234,   21,  170,  198,  170,   21,  198,    9,
 /*    30 */    77,    9,  241,  170,  101,  198,   77,    9,  198,  234,
 /*    40 */    90,   21,   21,  170,   90,   21,   90,  170,   21,    9,
 /*    50 */    90,  101,   90,  135,   21,    9,   21,    9,   77,    9,
 /*    60 */   135,   21,   21,  170,  170,    9,  183,  170,   21,  198,
 /*    70 */    90,   21,   77,  183,   21,   21,  170,  183,  183,   21,
 /*    80 */   183,   21,  198,  183,  170,  101,   73,   73,   73,  259,
 /*    90 */   272,  287,  277,  279,  281,  287,  289,  288,  281,  288,
 /*   100 */    21,  289,  292,   21,   21,  301,  301,  298,  302,  305,
 /*   110 */   277,  259,  281,  281,  281,  281,   21,  311,   73,  305,
 /*   120 */   319,   73,  323,  272,  279,  279,  298,   21,  279,  279,
 /*   130 */    21,  279,   21,  327,  272,  279,  272,  319,  327,   21,
 /*   140 */   323,  281,   21,  334,   21,  302,  259,  281,  259,  311,
 /*   150 */   334,  279,  301,  301,  301,  340,   73,  301,   12,   65,
 /*   160 */    23,   14,   36,   40,   48,   54,   62,   61,   64,  102,
 /*   170 */    98,   92,   87,   82,   69,   66,  107,  119,   86,  109,
 /*   180 */   129,  134,  150,  149,  384,  392,  396,  399,  401,  405,
 /*   190 */   403,  411,  409,  414,  415,  413,  419,  421,  424,  429,
 /*   200 */   431,  434,  435,  438,  437,  445,  441,  449,  454,  456,
 /*   210 */   451,  463,  464,  466,  468,  471,  428,  440,  472,  480,
 /*   220 */   482,  485,  489,  490,  493,  496,  500,  469,  501,  494,
 /*   230 */   508,  488,  507,  514,  518,  516,  520,  524,  525,  527,
 /*   240 */   530,  533,  536,  537,  542,  548,  544,  439,  557,  558,
 /*   250 */   562,  554,  567,  442,  450,  569,  564,  474,  571,  574,
 /*   260 */   486,  491,  498,  582,  583,  584,  522,  592,  597,  543,
 /*   270 */   598,  575,  577,  599,  600,  602,  603,  606,  608,  610,
 /*   280 */   612,  617,  616,  621,  622,  625,  627,  631,  632,  636,
 /*   290 */   635,  590,  641,  643,  647,  601,  609,  653,  655,  657,
 /*   300 */   660,  664,  667,  670,  672,  676,  678,  683,  682,  686,
 /*   310 */   688,  691,  698,  700,  706,  707,  711,  716,  715,  720,
 /*   320 */   717,  728,  726,  734,  731,  736,  739,  743,  614,  749,
 /*   330 */   746,  751,  753,  755,  759,  758,  761,  765,  768,  774,
 /*   340 */   771,  775,  776,  779,  783,  781,  778,  772,  770,  767,
 /*   350 */   766,  764,  763,  760,  757,  754,  752,  750,  744,  745,
 /*   360 */   740,  738,  737,  730,  735,  733,  729,  724,  721,  719,
 /*   370 */   712,  713,  708,  710,  702,  699,  694,  689,  687,  684,
 /*   380 */   681,  674,  677,  673,  671,  669,  661,  666,  658,  656,
 /*   390 */   654,  651,  650,  649,  644,  642,  640,  639,  637,  634,
 /*   400 */   629,  630,  626,  624,  623,  619,  620,  615,  613,  611,
 /*   410 */   607,  604,  595,  586,  581,  594,  572,  561,  552,  531,
 /*   420 */   593,  589,  588,  587,  513,  505,  580,  578,  576,  479,
 /*   430 */   573,  467,  555,  455,  546,  570,  566,  565,  559,  553,
 /*   440 */   550,  551,  549,  545,  539,  538,  535,  532,  528,  526,
 /*   450 */   521,  523,  519,  515,  509,  511,  495,  510,  502,  506,
 /*   460 */   481,  503,  499,  461,  492,  478,  487,  484,  477,  476,
 /*   470 */   452,  473,  417,  470,  459,  465,  458,  460,  457,  448,
 /*   480 */   453,  447,  446,  444,  443,  436,  430,  433,  426,  425,
 /*   490 */   422,  420,  418,  416,  402,  412,  407,  408,  406,  404,
 /*   500 */   400,  398,  397,  393,  395,  391,  385,  380,  378,  379,
 /*   510 */   382,  373,  376,  375,  368,  351,  365,  371,  360,  370,
 /*   520 */   369,  364,  363,  362,  357,  358,  359,  356,  350,  343,
 /*   530 */   784,  394,
};
#define YY_REDUCE_USE_DFLT (-58)
#define YY_REDUCE_MAX 157
static const short geoJSON_yy_reduce_ofst[] = {
 /*     0 */   118,  -28,  -57,  -42,  -24,  -19,  -17,   67,   50,   38,
 /*    10 */    30,   19,   84,   56,   58,   43,   71,   53,   81,   60,
 /*    20 */    85,   79,   70,   96,  103,   94,  106,  114,  110,  111,
 /*    30 */    99,  124,  -15,  125,  141,  139,  126,  146,  151,  142,
 /*    40 */   154,  164,  176,  182,  167,  189,  172,  196,  200,  197,
 /*    50 */   192,  214,  201,  224,  229,  223,  238,  225,  203,  216,
 /*    60 */   222,  218,  213,  207,  204,  190,  179,  191,  184,  173,
 /*    70 */   158,  175,  144,  152,  163,  155,  153,  133,  128,  140,
 /*    80 */   121,  137,  123,  112,  122,  115,   -9,   97,  180,   93,
 /*    90 */   232,  219,  235,  116,  243,  227,  228,  244,  248,  245,
 /*   100 */   249,  236,  230,  255,  256,  233,  237,  253,  257,  252,
 /*   110 */   261,  263,  269,  270,  271,  273,  274,  262,  246,  265,
 /*   120 */   268,  250,  260,  280,  290,  291,  275,  284,  293,  294,
 /*   130 */   295,  296,  297,  283,  299,  303,  300,  286,  285,  304,
 /*   140 */   278,  306,  307,  308,  309,  310,  312,  313,  315,  282,
 /*   150 */   314,  316,  317,  318,  320,  321,  276,  322,
};
static const YYACTIONTYPE geoJSON_yy_default[] = {
 /*     0 */   680,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*    10 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*    20 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*    30 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*    40 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*    50 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*    60 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*    70 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*    80 */   838,  838,  838,  838,  838,  838,  828,  828,  828,  720,
 /*    90 */   718,  783,  838,  838,  838,  783,  786,  838,  838,  838,
 /*   100 */   838,  786,  838,  838,  838,  821,  821,  838,  838,  754,
 /*   110 */   838,  720,  838,  838,  838,  838,  838,  838,  828,  754,
 /*   120 */   751,  828,  801,  718,  838,  838,  838,  838,  838,  838,
 /*   130 */   838,  838,  838,  838,  718,  838,  718,  751,  838,  838,
 /*   140 */   801,  838,  838,  804,  838,  838,  720,  838,  720,  838,
 /*   150 */   804,  838,  821,  821,  821,  838,  828,  821,  838,  838,
 /*   160 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   170 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   180 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   190 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   200 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   210 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   220 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   230 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   240 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   250 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   260 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   270 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   280 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   290 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   300 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   310 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   320 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   330 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   340 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   350 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   360 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   370 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   380 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   390 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   400 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   410 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   420 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   430 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   440 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   450 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   460 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   470 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   480 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   490 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   500 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   510 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   520 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   530 */   838,  838,  683,  806,  818,  682,  799,  822,  793,  823,
 /*   540 */   797,  824,  791,  798,  792,  796,  790,  795,  789,  805,
 /*   550 */   832,  803,  802,  800,  794,  788,  834,  781,  775,  779,
 /*   560 */   773,  780,  836,  774,  778,  819,  772,  777,  820,  771,
 /*   570 */   787,  785,  784,  782,  776,  770,  767,  761,  807,  765,
 /*   580 */   813,  681,  759,  766,  825,  760,  684,  829,  764,  830,
 /*   590 */   758,  831,  763,  757,  769,  768,  762,  756,  747,  741,
 /*   600 */   833,  745,  739,  746,  740,  744,  835,  738,  743,  737,
 /*   610 */   755,  753,  837,  749,  752,  826,  750,  748,  827,  742,
 /*   620 */   736,  733,  727,  731,  725,  732,  726,  730,  724,  729,
 /*   630 */   723,  735,  721,  734,  719,  728,  722,  717,  716,  712,
 /*   640 */   711,  808,  709,  814,  703,  707,  701,  708,  702,  706,
 /*   650 */   700,  710,  705,  810,  699,  816,  715,  714,  713,  704,
 /*   660 */   698,  809,  697,  815,  696,  695,  694,  693,  692,  691,
 /*   670 */   690,  689,  688,  811,  687,  817,  686,  812,  685,
};
#define YY_SZ_ACTTAB (int)(sizeof(geoJSON_yy_action)/sizeof(geoJSON_yy_action[0]))

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
struct geoJSON_yyStackEntry {
  YYACTIONTYPE stateno;  /* The state-number */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  GEOJSON_YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};
typedef struct geoJSON_yyStackEntry geoJSON_yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct geoJSON_yyParser {
  int yyidx;                    /* Index of top element in stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyidxMax;                 /* Maximum value of yyidx */
#endif
  int yyerrcnt;                 /* Shifts left before out of the error */
  ParseARG_SDECL                /* A place to hold %extra_argument */
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  geoJSON_yyStackEntry *yystack;        /* The parser's stack */
#else
  geoJSON_yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
#endif
};
typedef struct geoJSON_yyParser geoJSON_yyParser;

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
  "$",             "GEOJSON_NEWLINE",  "GEOJSON_OPEN_BRACE",  "GEOJSON_TYPE",
  "GEOJSON_COLON",  "GEOJSON_POINT",  "GEOJSON_COMMA",  "GEOJSON_COORDS",
  "GEOJSON_CLOSE_BRACE",  "GEOJSON_BBOX",  "GEOJSON_OPEN_BRACKET",  "GEOJSON_CLOSE_BRACKET",
  "GEOJSON_CRS",   "GEOJSON_NAME",  "GEOJSON_PROPS",  "GEOJSON_NUM", 
  "GEOJSON_SHORT_SRID",  "GEOJSON_LONG_SRID",  "GEOJSON_LINESTRING",  "GEOJSON_POLYGON",
  "GEOJSON_MULTIPOINT",  "GEOJSON_MULTILINESTRING",  "GEOJSON_MULTIPOLYGON",  "GEOJSON_GEOMETRYCOLLECTION",
  "GEOJSON_GEOMS",  "error",         "main",          "in",          
  "state",         "program",       "geo_text",      "point",       
  "pointz",        "linestring",    "linestringz",   "polygon",     
  "polygonz",      "multipoint",    "multipointz",   "multilinestring",
  "multilinestringz",  "multipolygon",  "multipolygonz",  "geocoll",     
  "geocollz",      "point_coordxy",  "bbox",          "short_crs",   
  "long_crs",      "point_coordxyz",  "coord",         "short_srid",  
  "long_srid",     "extra_pointsxy",  "extra_pointsxyz",  "linestring_text",
  "linestring_textz",  "polygon_text",  "polygon_textz",  "ring",        
  "extra_rings",   "ringz",         "extra_ringsz",  "multipoint_text",
  "multipoint_textz",  "multilinestring_text",  "multilinestring_textz",  "multilinestring_text2",
  "multilinestring_textz2",  "multipolygon_text",  "multipolygon_textz",  "multipolygon_text2",
  "multipolygon_textz2",  "geocoll_text",  "geocoll_textz",  "coll_point",  
  "geocoll_text2",  "coll_linestring",  "coll_polygon",  "coll_pointz", 
  "geocoll_textz2",  "coll_linestringz",  "coll_polygonz",
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "main ::= in",
 /*   1 */ "in ::=",
 /*   2 */ "in ::= in state GEOJSON_NEWLINE",
 /*   3 */ "state ::= program",
 /*   4 */ "program ::= geo_text",
 /*   5 */ "geo_text ::= point",
 /*   6 */ "geo_text ::= pointz",
 /*   7 */ "geo_text ::= linestring",
 /*   8 */ "geo_text ::= linestringz",
 /*   9 */ "geo_text ::= polygon",
 /*  10 */ "geo_text ::= polygonz",
 /*  11 */ "geo_text ::= multipoint",
 /*  12 */ "geo_text ::= multipointz",
 /*  13 */ "geo_text ::= multilinestring",
 /*  14 */ "geo_text ::= multilinestringz",
 /*  15 */ "geo_text ::= multipolygon",
 /*  16 */ "geo_text ::= multipolygonz",
 /*  17 */ "geo_text ::= geocoll",
 /*  18 */ "geo_text ::= geocollz",
 /*  19 */ "point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE",
 /*  20 */ "point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE",
 /*  21 */ "point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE",
 /*  22 */ "point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE",
 /*  23 */ "point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE",
 /*  24 */ "point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE",
 /*  25 */ "pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE",
 /*  26 */ "pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE",
 /*  27 */ "pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE",
 /*  28 */ "pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE",
 /*  29 */ "point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE",
 /*  30 */ "point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE",
 /*  31 */ "bbox ::= coord GEOJSON_COMMA coord GEOJSON_COMMA coord GEOJSON_COMMA coord",
 /*  32 */ "short_crs ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_NAME GEOJSON_COMMA GEOJSON_PROPS GEOJSON_COLON GEOJSON_OPEN_BRACE GEOJSON_NAME GEOJSON_COLON short_srid GEOJSON_CLOSE_BRACE GEOJSON_CLOSE_BRACE",
 /*  33 */ "long_crs ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_NAME GEOJSON_COMMA GEOJSON_PROPS GEOJSON_COLON GEOJSON_OPEN_BRACE GEOJSON_NAME GEOJSON_COLON long_srid GEOJSON_CLOSE_BRACE GEOJSON_CLOSE_BRACE",
 /*  34 */ "point_coordxy ::= GEOJSON_OPEN_BRACKET coord GEOJSON_COMMA coord GEOJSON_CLOSE_BRACKET",
 /*  35 */ "point_coordxyz ::= GEOJSON_OPEN_BRACKET coord GEOJSON_COMMA coord GEOJSON_COMMA coord GEOJSON_CLOSE_BRACKET",
 /*  36 */ "coord ::= GEOJSON_NUM",
 /*  37 */ "short_srid ::= GEOJSON_SHORT_SRID",
 /*  38 */ "long_srid ::= GEOJSON_LONG_SRID",
 /*  39 */ "extra_pointsxy ::=",
 /*  40 */ "extra_pointsxy ::= GEOJSON_COMMA point_coordxy extra_pointsxy",
 /*  41 */ "extra_pointsxyz ::=",
 /*  42 */ "extra_pointsxyz ::= GEOJSON_COMMA point_coordxyz extra_pointsxyz",
 /*  43 */ "linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE",
 /*  44 */ "linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE",
 /*  45 */ "linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE",
 /*  46 */ "linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE",
 /*  47 */ "linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE",
 /*  48 */ "linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE",
 /*  49 */ "linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE",
 /*  50 */ "linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE",
 /*  51 */ "linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE",
 /*  52 */ "linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE",
 /*  53 */ "linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE",
 /*  54 */ "linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE",
 /*  55 */ "linestring_text ::= GEOJSON_OPEN_BRACKET point_coordxy GEOJSON_COMMA point_coordxy extra_pointsxy GEOJSON_CLOSE_BRACKET",
 /*  56 */ "linestring_textz ::= GEOJSON_OPEN_BRACKET point_coordxyz GEOJSON_COMMA point_coordxyz extra_pointsxyz GEOJSON_CLOSE_BRACKET",
 /*  57 */ "polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE",
 /*  58 */ "polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE",
 /*  59 */ "polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE",
 /*  60 */ "polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE",
 /*  61 */ "polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE",
 /*  62 */ "polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE",
 /*  63 */ "polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE",
 /*  64 */ "polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE",
 /*  65 */ "polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE",
 /*  66 */ "polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE",
 /*  67 */ "polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE",
 /*  68 */ "polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE",
 /*  69 */ "polygon_text ::= GEOJSON_OPEN_BRACKET ring extra_rings GEOJSON_CLOSE_BRACKET",
 /*  70 */ "polygon_textz ::= GEOJSON_OPEN_BRACKET ringz extra_ringsz GEOJSON_CLOSE_BRACKET",
 /*  71 */ "ring ::= GEOJSON_OPEN_BRACKET point_coordxy GEOJSON_COMMA point_coordxy GEOJSON_COMMA point_coordxy GEOJSON_COMMA point_coordxy extra_pointsxy GEOJSON_CLOSE_BRACKET",
 /*  72 */ "extra_rings ::=",
 /*  73 */ "extra_rings ::= GEOJSON_COMMA ring extra_rings",
 /*  74 */ "ringz ::= GEOJSON_OPEN_BRACKET point_coordxyz GEOJSON_COMMA point_coordxyz GEOJSON_COMMA point_coordxyz GEOJSON_COMMA point_coordxyz extra_pointsxyz GEOJSON_CLOSE_BRACKET",
 /*  75 */ "extra_ringsz ::=",
 /*  76 */ "extra_ringsz ::= GEOJSON_COMMA ringz extra_ringsz",
 /*  77 */ "multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE",
 /*  78 */ "multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE",
 /*  79 */ "multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE",
 /*  80 */ "multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE",
 /*  81 */ "multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE",
 /*  82 */ "multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE",
 /*  83 */ "multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE",
 /*  84 */ "multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE",
 /*  85 */ "multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE",
 /*  86 */ "multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE",
 /*  87 */ "multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE",
 /*  88 */ "multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE",
 /*  89 */ "multipoint_text ::= GEOJSON_OPEN_BRACKET point_coordxy extra_pointsxy GEOJSON_CLOSE_BRACKET",
 /*  90 */ "multipoint_textz ::= GEOJSON_OPEN_BRACKET point_coordxyz extra_pointsxyz GEOJSON_CLOSE_BRACKET",
 /*  91 */ "multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE",
 /*  92 */ "multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE",
 /*  93 */ "multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE",
 /*  94 */ "multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE",
 /*  95 */ "multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE",
 /*  96 */ "multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE",
 /*  97 */ "multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE",
 /*  98 */ "multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE",
 /*  99 */ "multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE",
 /* 100 */ "multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE",
 /* 101 */ "multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE",
 /* 102 */ "multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE",
 /* 103 */ "multilinestring_text ::= GEOJSON_OPEN_BRACKET linestring_text multilinestring_text2 GEOJSON_CLOSE_BRACKET",
 /* 104 */ "multilinestring_text2 ::=",
 /* 105 */ "multilinestring_text2 ::= GEOJSON_COMMA linestring_text multilinestring_text2",
 /* 106 */ "multilinestring_textz ::= GEOJSON_OPEN_BRACKET linestring_textz multilinestring_textz2 GEOJSON_CLOSE_BRACKET",
 /* 107 */ "multilinestring_textz2 ::=",
 /* 108 */ "multilinestring_textz2 ::= GEOJSON_COMMA linestring_textz multilinestring_textz2",
 /* 109 */ "multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE",
 /* 110 */ "multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE",
 /* 111 */ "multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE",
 /* 112 */ "multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE",
 /* 113 */ "multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE",
 /* 114 */ "multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE",
 /* 115 */ "multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE",
 /* 116 */ "multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE",
 /* 117 */ "multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE",
 /* 118 */ "multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE",
 /* 119 */ "multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE",
 /* 120 */ "multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE",
 /* 121 */ "multipolygon_text ::= GEOJSON_OPEN_BRACKET polygon_text multipolygon_text2 GEOJSON_CLOSE_BRACKET",
 /* 122 */ "multipolygon_text2 ::=",
 /* 123 */ "multipolygon_text2 ::= GEOJSON_COMMA polygon_text multipolygon_text2",
 /* 124 */ "multipolygon_textz ::= GEOJSON_OPEN_BRACKET polygon_textz multipolygon_textz2 GEOJSON_CLOSE_BRACKET",
 /* 125 */ "multipolygon_textz2 ::=",
 /* 126 */ "multipolygon_textz2 ::= GEOJSON_COMMA polygon_textz multipolygon_textz2",
 /* 127 */ "geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE",
 /* 128 */ "geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE",
 /* 129 */ "geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE",
 /* 130 */ "geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE",
 /* 131 */ "geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE",
 /* 132 */ "geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE",
 /* 133 */ "geocollz ::= GEOJSON_GEOMETRYCOLLECTION geocoll_textz",
 /* 134 */ "geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE",
 /* 135 */ "geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE",
 /* 136 */ "geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE",
 /* 137 */ "geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE",
 /* 138 */ "geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE",
 /* 139 */ "geocoll_text ::= GEOJSON_OPEN_BRACKET coll_point geocoll_text2 GEOJSON_CLOSE_BRACKET",
 /* 140 */ "geocoll_text ::= GEOJSON_OPEN_BRACKET coll_linestring geocoll_text2 GEOJSON_CLOSE_BRACKET",
 /* 141 */ "geocoll_text ::= GEOJSON_OPEN_BRACKET coll_polygon geocoll_text2 GEOJSON_CLOSE_BRACKET",
 /* 142 */ "geocoll_text2 ::=",
 /* 143 */ "geocoll_text2 ::= GEOJSON_COMMA coll_point geocoll_text2",
 /* 144 */ "geocoll_text2 ::= GEOJSON_COMMA coll_linestring geocoll_text2",
 /* 145 */ "geocoll_text2 ::= GEOJSON_COMMA coll_polygon geocoll_text2",
 /* 146 */ "geocoll_textz ::= GEOJSON_OPEN_BRACKET coll_pointz geocoll_textz2 GEOJSON_CLOSE_BRACKET",
 /* 147 */ "geocoll_textz ::= GEOJSON_OPEN_BRACKET coll_linestringz geocoll_textz2 GEOJSON_CLOSE_BRACKET",
 /* 148 */ "geocoll_textz ::= GEOJSON_OPEN_BRACKET coll_polygonz geocoll_textz2 GEOJSON_CLOSE_BRACKET",
 /* 149 */ "geocoll_textz2 ::=",
 /* 150 */ "geocoll_textz2 ::= GEOJSON_COMMA coll_pointz geocoll_textz2",
 /* 151 */ "geocoll_textz2 ::= GEOJSON_COMMA coll_linestringz geocoll_textz2",
 /* 152 */ "geocoll_textz2 ::= GEOJSON_COMMA coll_polygonz geocoll_textz2",
 /* 153 */ "coll_point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE",
 /* 154 */ "coll_pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE",
 /* 155 */ "coll_linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE",
 /* 156 */ "coll_linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE",
 /* 157 */ "coll_polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE",
 /* 158 */ "coll_polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.
*/
static void yyGrowStack(geoJSON_yyParser *p){
  int newSize;
  geoJSON_yyStackEntry *pNew;

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
void *geoJSONParseAlloc(void *(*mallocProc)(size_t)){
  geoJSON_yyParser *pParser;
  pParser = (geoJSON_yyParser*)(*mallocProc)( (size_t)sizeof(geoJSON_yyParser) );
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
static void geoJSON_yy_destructor(
  geoJSON_yyParser *yypParser,    /* The parser */
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  GEOJSON_YYMINORTYPE *yypminor   /* The object to be destroyed */
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
static int geoJSON_yy_pop_parser_stack(geoJSON_yyParser *pParser){
  YYCODETYPE yymajor;
  geoJSON_yyStackEntry *yytos = &pParser->yystack[pParser->yyidx];

  if( pParser->yyidx<0 ) return 0;
#ifndef NDEBUG
  if( yyTraceFILE && pParser->yyidx>=0 ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yymajor = yytos->major;
  geoJSON_yy_destructor(pParser, yymajor, &yytos->minor);
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
void geoJSONParseFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
  geoJSON_yyParser *pParser = (geoJSON_yyParser*)p;
  if( pParser==0 ) return;
  while( pParser->yyidx>=0 ) geoJSON_yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
  free(pParser->yystack);
#endif
  (*freeProc)((void*)pParser);
}

/*
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int geoJSONParseStackPeak(void *p){
  geoJSON_yyParser *pParser = (geoJSON_yyParser*)p;
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
static int geoJSON_yy_find_shift_action(
  geoJSON_yyParser *pParser,        /* The parser */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  int stateno = pParser->yystack[pParser->yyidx].stateno;
 
  if( stateno>YY_SHIFT_MAX || (i = geoJSON_yy_shift_ofst[stateno])==YY_SHIFT_USE_DFLT ){
    return geoJSON_yy_default[stateno];
  }
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
  if( i<0 || i>=YY_SZ_ACTTAB || geoJSON_yy_lookahead[i]!=iLookAhead ){
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
        return geoJSON_yy_find_shift_action(pParser, iFallback);
      }
#endif
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        if( j>=0 && j<YY_SZ_ACTTAB && geoJSON_yy_lookahead[j]==YYWILDCARD ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return geoJSON_yy_action[j];
        }
      }
#endif /* YYWILDCARD */
    }
    return geoJSON_yy_default[stateno];
  }else{
    return geoJSON_yy_action[i];
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
static int geoJSON_yy_find_reduce_action(
  int stateno,              /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
#ifdef YYERRORSYMBOL
  if( stateno>YY_REDUCE_MAX ){
    return geoJSON_yy_default[stateno];
  }
#else
  assert( stateno<=YY_REDUCE_MAX );
#endif
  i = geoJSON_yy_reduce_ofst[stateno];
  assert( i!=YY_REDUCE_USE_DFLT );
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
#ifdef YYERRORSYMBOL
  if( i<0 || i>=YY_SZ_ACTTAB || geoJSON_yy_lookahead[i]!=iLookAhead ){
    return geoJSON_yy_default[stateno];
  }
#else
  assert( i>=0 && i<YY_SZ_ACTTAB );
  assert( geoJSON_yy_lookahead[i]==iLookAhead );
#endif
  return geoJSON_yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void geoJSON_yyStackOverflow(geoJSON_yyParser *yypParser, GEOJSON_YYMINORTYPE *yypMinor){
   ParseARG_FETCH;
   yypParser->yyidx--;
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yypParser->yyidx>=0 ) geoJSON_yy_pop_parser_stack(yypParser);
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
#line 47 "geoJSON.y"

     fprintf(stderr,"Giving up.  Parser stack overflow\n");
#line 987 "geoJSON.c"
   ParseARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
** Perform a shift action.
*/
static void geoJSON_yy_shift(
  geoJSON_yyParser *yypParser,          /* The parser to be shifted */
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  GEOJSON_YYMINORTYPE *yypMinor         /* Pointer to the minor token to shift in */
){
  geoJSON_yyStackEntry *yytos;
  yypParser->yyidx++;
#ifdef YYTRACKMAXSTACKDEPTH
  if( yypParser->yyidx>yypParser->yyidxMax ){
    yypParser->yyidxMax = yypParser->yyidx;
  }
#endif
#if YYSTACKDEPTH>0 
  if( yypParser->yyidx>=YYSTACKDEPTH ){
    geoJSON_yyStackOverflow(yypParser, yypMinor);
    return;
  }
#else
  if( yypParser->yyidx>=yypParser->yystksz ){
    yyGrowStack(yypParser);
    if( yypParser->yyidx>=yypParser->yystksz ){
      geoJSON_yyStackOverflow(yypParser, yypMinor);
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
} geoJSON_yyRuleInfo[] = {
  { 26, 1 },
  { 27, 0 },
  { 27, 3 },
  { 28, 1 },
  { 29, 1 },
  { 30, 1 },
  { 30, 1 },
  { 30, 1 },
  { 30, 1 },
  { 30, 1 },
  { 30, 1 },
  { 30, 1 },
  { 30, 1 },
  { 30, 1 },
  { 30, 1 },
  { 30, 1 },
  { 30, 1 },
  { 30, 1 },
  { 30, 1 },
  { 31, 9 },
  { 31, 15 },
  { 31, 13 },
  { 31, 13 },
  { 31, 19 },
  { 31, 19 },
  { 32, 9 },
  { 32, 15 },
  { 32, 13 },
  { 32, 13 },
  { 31, 19 },
  { 31, 19 },
  { 46, 7 },
  { 47, 13 },
  { 48, 13 },
  { 45, 5 },
  { 49, 7 },
  { 50, 1 },
  { 51, 1 },
  { 52, 1 },
  { 53, 0 },
  { 53, 3 },
  { 54, 0 },
  { 54, 3 },
  { 33, 9 },
  { 33, 15 },
  { 33, 13 },
  { 33, 13 },
  { 33, 19 },
  { 33, 19 },
  { 34, 9 },
  { 34, 15 },
  { 34, 13 },
  { 34, 13 },
  { 34, 19 },
  { 34, 19 },
  { 55, 6 },
  { 56, 6 },
  { 35, 9 },
  { 35, 15 },
  { 35, 13 },
  { 35, 13 },
  { 35, 19 },
  { 35, 19 },
  { 36, 9 },
  { 36, 15 },
  { 36, 13 },
  { 36, 13 },
  { 36, 19 },
  { 36, 19 },
  { 57, 4 },
  { 58, 4 },
  { 59, 10 },
  { 60, 0 },
  { 60, 3 },
  { 61, 10 },
  { 62, 0 },
  { 62, 3 },
  { 37, 9 },
  { 37, 15 },
  { 37, 13 },
  { 37, 13 },
  { 37, 19 },
  { 37, 19 },
  { 38, 9 },
  { 38, 15 },
  { 38, 13 },
  { 38, 13 },
  { 38, 19 },
  { 38, 19 },
  { 63, 4 },
  { 64, 4 },
  { 39, 9 },
  { 39, 15 },
  { 39, 13 },
  { 39, 13 },
  { 39, 19 },
  { 39, 19 },
  { 40, 9 },
  { 40, 15 },
  { 40, 13 },
  { 40, 13 },
  { 40, 19 },
  { 40, 19 },
  { 65, 4 },
  { 67, 0 },
  { 67, 3 },
  { 66, 4 },
  { 68, 0 },
  { 68, 3 },
  { 41, 9 },
  { 41, 15 },
  { 41, 13 },
  { 41, 13 },
  { 41, 19 },
  { 41, 19 },
  { 42, 9 },
  { 42, 15 },
  { 42, 13 },
  { 42, 13 },
  { 42, 19 },
  { 42, 19 },
  { 69, 4 },
  { 71, 0 },
  { 71, 3 },
  { 70, 4 },
  { 72, 0 },
  { 72, 3 },
  { 43, 9 },
  { 43, 15 },
  { 43, 13 },
  { 43, 13 },
  { 43, 19 },
  { 43, 19 },
  { 44, 2 },
  { 44, 15 },
  { 44, 13 },
  { 44, 13 },
  { 44, 19 },
  { 44, 19 },
  { 73, 4 },
  { 73, 4 },
  { 73, 4 },
  { 76, 0 },
  { 76, 3 },
  { 76, 3 },
  { 76, 3 },
  { 74, 4 },
  { 74, 4 },
  { 74, 4 },
  { 80, 0 },
  { 80, 3 },
  { 80, 3 },
  { 80, 3 },
  { 75, 9 },
  { 79, 9 },
  { 77, 9 },
  { 81, 9 },
  { 78, 9 },
  { 82, 9 },
};

static void geoJSON_yy_accept(geoJSON_yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void geoJSON_yy_reduce(
  geoJSON_yyParser *yypParser,         /* The parser */
  int yyruleno                 /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  GEOJSON_YYMINORTYPE yygotominor;        /* The LHS of the rule reduced */
  geoJSON_yyStackEntry *yymsp;            /* The top of the parser's stack */
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
  yygotominor = geoJSON_yyzerominor;


  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
      case 5: /* geo_text ::= point */
      case 6: /* geo_text ::= pointz */ yytestcase(yyruleno==6);
      case 7: /* geo_text ::= linestring */ yytestcase(yyruleno==7);
      case 8: /* geo_text ::= linestringz */ yytestcase(yyruleno==8);
      case 9: /* geo_text ::= polygon */ yytestcase(yyruleno==9);
      case 10: /* geo_text ::= polygonz */ yytestcase(yyruleno==10);
      case 11: /* geo_text ::= multipoint */ yytestcase(yyruleno==11);
      case 12: /* geo_text ::= multipointz */ yytestcase(yyruleno==12);
      case 13: /* geo_text ::= multilinestring */ yytestcase(yyruleno==13);
      case 14: /* geo_text ::= multilinestringz */ yytestcase(yyruleno==14);
      case 15: /* geo_text ::= multipolygon */ yytestcase(yyruleno==15);
      case 16: /* geo_text ::= multipolygonz */ yytestcase(yyruleno==16);
      case 17: /* geo_text ::= geocoll */ yytestcase(yyruleno==17);
      case 18: /* geo_text ::= geocollz */ yytestcase(yyruleno==18);
#line 86 "geoJSON.y"
{ *result = yymsp[0].minor.yy0; }
#line 1273 "geoJSON.c"
        break;
      case 19: /* point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE */
      case 20: /* point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==20);
      case 25: /* pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==25);
      case 26: /* pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==26);
      case 153: /* coll_point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==153);
      case 154: /* coll_pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==154);
#line 106 "geoJSON.y"
{ yygotominor.yy0 = geoJSON_buildGeomFromPoint((gaiaPointPtr)yymsp[-1].minor.yy0); }
#line 1283 "geoJSON.c"
        break;
      case 21: /* point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE */
      case 22: /* point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==22);
      case 27: /* pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==27);
      case 28: /* pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==28);
#line 114 "geoJSON.y"
{ yygotominor.yy0 = geoJSON_buildGeomFromPointSrid((gaiaPointPtr)yymsp[-1].minor.yy0, (int *)yymsp[-5].minor.yy0); }
#line 1291 "geoJSON.c"
        break;
      case 23: /* point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE */
      case 24: /* point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==24);
      case 29: /* point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==29);
      case 30: /* point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==30);
#line 123 "geoJSON.y"
{ yygotominor.yy0 = geoJSON_buildGeomFromPointSrid((gaiaPointPtr)yymsp[-1].minor.yy0, (int *)yymsp[-11].minor.yy0); }
#line 1299 "geoJSON.c"
        break;
      case 32: /* short_crs ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_NAME GEOJSON_COMMA GEOJSON_PROPS GEOJSON_COLON GEOJSON_OPEN_BRACE GEOJSON_NAME GEOJSON_COLON short_srid GEOJSON_CLOSE_BRACE GEOJSON_CLOSE_BRACE */
      case 33: /* long_crs ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_NAME GEOJSON_COMMA GEOJSON_PROPS GEOJSON_COLON GEOJSON_OPEN_BRACE GEOJSON_NAME GEOJSON_COLON long_srid GEOJSON_CLOSE_BRACE GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==33);
#line 162 "geoJSON.y"
{ yygotominor.yy0 = yymsp[-2].minor.yy0; }
#line 1305 "geoJSON.c"
        break;
      case 34: /* point_coordxy ::= GEOJSON_OPEN_BRACKET coord GEOJSON_COMMA coord GEOJSON_CLOSE_BRACKET */
#line 173 "geoJSON.y"
{ yygotominor.yy0 = (void *) geoJSON_point_xy((double *)yymsp[-3].minor.yy0, (double *)yymsp[-1].minor.yy0); }
#line 1310 "geoJSON.c"
        break;
      case 35: /* point_coordxyz ::= GEOJSON_OPEN_BRACKET coord GEOJSON_COMMA coord GEOJSON_COMMA coord GEOJSON_CLOSE_BRACKET */
#line 175 "geoJSON.y"
{ yygotominor.yy0 = (void *) geoJSON_point_xyz((double *)yymsp[-5].minor.yy0, (double *)yymsp[-3].minor.yy0, (double *)yymsp[-1].minor.yy0); }
#line 1315 "geoJSON.c"
        break;
      case 36: /* coord ::= GEOJSON_NUM */
      case 37: /* short_srid ::= GEOJSON_SHORT_SRID */ yytestcase(yyruleno==37);
      case 38: /* long_srid ::= GEOJSON_LONG_SRID */ yytestcase(yyruleno==38);
      case 133: /* geocollz ::= GEOJSON_GEOMETRYCOLLECTION geocoll_textz */ yytestcase(yyruleno==133);
#line 178 "geoJSON.y"
{ yygotominor.yy0 = yymsp[0].minor.yy0; }
#line 1323 "geoJSON.c"
        break;
      case 39: /* extra_pointsxy ::= */
      case 41: /* extra_pointsxyz ::= */ yytestcase(yyruleno==41);
      case 72: /* extra_rings ::= */ yytestcase(yyruleno==72);
      case 75: /* extra_ringsz ::= */ yytestcase(yyruleno==75);
      case 104: /* multilinestring_text2 ::= */ yytestcase(yyruleno==104);
      case 107: /* multilinestring_textz2 ::= */ yytestcase(yyruleno==107);
      case 122: /* multipolygon_text2 ::= */ yytestcase(yyruleno==122);
      case 125: /* multipolygon_textz2 ::= */ yytestcase(yyruleno==125);
      case 142: /* geocoll_text2 ::= */ yytestcase(yyruleno==142);
      case 149: /* geocoll_textz2 ::= */ yytestcase(yyruleno==149);
#line 189 "geoJSON.y"
{ yygotominor.yy0 = NULL; }
#line 1337 "geoJSON.c"
        break;
      case 40: /* extra_pointsxy ::= GEOJSON_COMMA point_coordxy extra_pointsxy */
      case 42: /* extra_pointsxyz ::= GEOJSON_COMMA point_coordxyz extra_pointsxyz */ yytestcase(yyruleno==42);
#line 191 "geoJSON.y"
{ ((gaiaPointPtr)yymsp[-1].minor.yy0)->Next = (gaiaPointPtr)yymsp[0].minor.yy0;  yygotominor.yy0 = yymsp[-1].minor.yy0; }
#line 1343 "geoJSON.c"
        break;
      case 43: /* linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE */
      case 44: /* linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==44);
      case 49: /* linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==49);
      case 50: /* linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==50);
      case 155: /* coll_linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==155);
      case 156: /* coll_linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==156);
#line 202 "geoJSON.y"
{ yygotominor.yy0 = geoJSON_buildGeomFromLinestring((gaiaLinestringPtr)yymsp[-1].minor.yy0); }
#line 1353 "geoJSON.c"
        break;
      case 45: /* linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE */
      case 46: /* linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==46);
      case 51: /* linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==51);
      case 52: /* linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==52);
#line 210 "geoJSON.y"
{ yygotominor.yy0 = geoJSON_buildGeomFromLinestringSrid((gaiaLinestringPtr)yymsp[-1].minor.yy0, (int *)yymsp[-5].minor.yy0); }
#line 1361 "geoJSON.c"
        break;
      case 47: /* linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE */
      case 48: /* linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==48);
      case 53: /* linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==53);
      case 54: /* linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==54);
#line 219 "geoJSON.y"
{ yygotominor.yy0 = geoJSON_buildGeomFromLinestringSrid((gaiaLinestringPtr)yymsp[-1].minor.yy0, (int *)yymsp[-11].minor.yy0); }
#line 1369 "geoJSON.c"
        break;
      case 55: /* linestring_text ::= GEOJSON_OPEN_BRACKET point_coordxy GEOJSON_COMMA point_coordxy extra_pointsxy GEOJSON_CLOSE_BRACKET */
#line 254 "geoJSON.y"
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   ((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0;
	   yygotominor.yy0 = (void *) geoJSON_linestring_xy((gaiaPointPtr)yymsp[-4].minor.yy0);
	}
#line 1378 "geoJSON.c"
        break;
      case 56: /* linestring_textz ::= GEOJSON_OPEN_BRACKET point_coordxyz GEOJSON_COMMA point_coordxyz extra_pointsxyz GEOJSON_CLOSE_BRACKET */
#line 261 "geoJSON.y"
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   ((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0;
	   yygotominor.yy0 = (void *) geoJSON_linestring_xyz((gaiaPointPtr)yymsp[-4].minor.yy0);
	}
#line 1387 "geoJSON.c"
        break;
      case 57: /* polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE */
      case 58: /* polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==58);
      case 63: /* polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==63);
      case 64: /* polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==64);
      case 157: /* coll_polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==157);
      case 158: /* coll_polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==158);
#line 272 "geoJSON.y"
{ yygotominor.yy0 = geoJSON_buildGeomFromPolygon((gaiaPolygonPtr)yymsp[-1].minor.yy0); }
#line 1397 "geoJSON.c"
        break;
      case 59: /* polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE */
      case 60: /* polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==60);
      case 65: /* polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==65);
      case 66: /* polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==66);
#line 280 "geoJSON.y"
{ yygotominor.yy0 = geoJSON_buildGeomFromPolygonSrid((gaiaPolygonPtr)yymsp[-1].minor.yy0, (int *)yymsp[-5].minor.yy0); }
#line 1405 "geoJSON.c"
        break;
      case 61: /* polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE */
      case 62: /* polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==62);
      case 67: /* polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==67);
      case 68: /* polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==68);
#line 289 "geoJSON.y"
{ yygotominor.yy0 = geoJSON_buildGeomFromPolygonSrid((gaiaPolygonPtr)yymsp[-1].minor.yy0, (int *)yymsp[-11].minor.yy0); }
#line 1413 "geoJSON.c"
        break;
      case 69: /* polygon_text ::= GEOJSON_OPEN_BRACKET ring extra_rings GEOJSON_CLOSE_BRACKET */
#line 324 "geoJSON.y"
{ 
		((gaiaRingPtr)yymsp[-2].minor.yy0)->Next = (gaiaRingPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) geoJSON_polygon_xy((gaiaRingPtr)yymsp[-2].minor.yy0);
	}
#line 1421 "geoJSON.c"
        break;
      case 70: /* polygon_textz ::= GEOJSON_OPEN_BRACKET ringz extra_ringsz GEOJSON_CLOSE_BRACKET */
#line 330 "geoJSON.y"
{  
		((gaiaRingPtr)yymsp[-2].minor.yy0)->Next = (gaiaRingPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) geoJSON_polygon_xyz((gaiaRingPtr)yymsp[-2].minor.yy0);
	}
#line 1429 "geoJSON.c"
        break;
      case 71: /* ring ::= GEOJSON_OPEN_BRACKET point_coordxy GEOJSON_COMMA point_coordxy GEOJSON_COMMA point_coordxy GEOJSON_COMMA point_coordxy extra_pointsxy GEOJSON_CLOSE_BRACKET */
#line 339 "geoJSON.y"
{
		((gaiaPointPtr)yymsp[-8].minor.yy0)->Next = (gaiaPointPtr)yymsp[-6].minor.yy0; 
		((gaiaPointPtr)yymsp[-6].minor.yy0)->Next = (gaiaPointPtr)yymsp[-4].minor.yy0;
		((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0; 
		((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) geoJSON_ring_xy((gaiaPointPtr)yymsp[-8].minor.yy0);
	}
#line 1440 "geoJSON.c"
        break;
      case 73: /* extra_rings ::= GEOJSON_COMMA ring extra_rings */
      case 76: /* extra_ringsz ::= GEOJSON_COMMA ringz extra_ringsz */ yytestcase(yyruleno==76);
#line 350 "geoJSON.y"
{
		((gaiaRingPtr)yymsp[-1].minor.yy0)->Next = (gaiaRingPtr)yymsp[0].minor.yy0;
		yygotominor.yy0 = yymsp[-1].minor.yy0;
	}
#line 1449 "geoJSON.c"
        break;
      case 74: /* ringz ::= GEOJSON_OPEN_BRACKET point_coordxyz GEOJSON_COMMA point_coordxyz GEOJSON_COMMA point_coordxyz GEOJSON_COMMA point_coordxyz extra_pointsxyz GEOJSON_CLOSE_BRACKET */
#line 357 "geoJSON.y"
{
		((gaiaPointPtr)yymsp[-8].minor.yy0)->Next = (gaiaPointPtr)yymsp[-6].minor.yy0; 
		((gaiaPointPtr)yymsp[-6].minor.yy0)->Next = (gaiaPointPtr)yymsp[-4].minor.yy0;
		((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0; 
		((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) geoJSON_ring_xyz((gaiaPointPtr)yymsp[-8].minor.yy0);
	}
#line 1460 "geoJSON.c"
        break;
      case 77: /* multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE */
      case 78: /* multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==78);
      case 83: /* multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==83);
      case 84: /* multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==84);
      case 91: /* multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==91);
      case 92: /* multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==92);
      case 97: /* multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==97);
      case 98: /* multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==98);
      case 109: /* multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==109);
      case 110: /* multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==110);
      case 115: /* multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==115);
      case 116: /* multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==116);
      case 127: /* geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==127);
      case 128: /* geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==128);
      case 134: /* geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==134);
#line 377 "geoJSON.y"
{ yygotominor.yy0 = yymsp[-1].minor.yy0; }
#line 1479 "geoJSON.c"
        break;
      case 79: /* multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE */
      case 80: /* multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==80);
      case 85: /* multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==85);
      case 86: /* multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==86);
      case 93: /* multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==93);
      case 94: /* multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==94);
      case 99: /* multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==99);
      case 100: /* multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==100);
      case 111: /* multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==111);
      case 112: /* multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==112);
      case 117: /* multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==117);
      case 118: /* multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==118);
      case 129: /* geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==129);
      case 130: /* geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==130);
      case 135: /* geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==135);
      case 136: /* geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==136);
#line 385 "geoJSON.y"
{ yygotominor.yy0 = (void *) geoJSON_setSrid((gaiaGeomCollPtr)yymsp[-1].minor.yy0, (int *)yymsp[-5].minor.yy0); }
#line 1499 "geoJSON.c"
        break;
      case 81: /* multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE */
      case 82: /* multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==82);
      case 87: /* multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==87);
      case 88: /* multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==88);
      case 95: /* multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==95);
      case 96: /* multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==96);
      case 101: /* multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==101);
      case 102: /* multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==102);
      case 113: /* multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==113);
      case 114: /* multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==114);
      case 119: /* multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==119);
      case 120: /* multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==120);
      case 131: /* geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==131);
      case 132: /* geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==132);
      case 137: /* geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==137);
      case 138: /* geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==138);
#line 394 "geoJSON.y"
{ yygotominor.yy0 = (void *) geoJSON_setSrid((gaiaGeomCollPtr)yymsp[-1].minor.yy0, (int *)yymsp[-11].minor.yy0); }
#line 1519 "geoJSON.c"
        break;
      case 89: /* multipoint_text ::= GEOJSON_OPEN_BRACKET point_coordxy extra_pointsxy GEOJSON_CLOSE_BRACKET */
#line 429 "geoJSON.y"
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) geoJSON_multipoint_xy((gaiaPointPtr)yymsp[-2].minor.yy0);
	}
#line 1527 "geoJSON.c"
        break;
      case 90: /* multipoint_textz ::= GEOJSON_OPEN_BRACKET point_coordxyz extra_pointsxyz GEOJSON_CLOSE_BRACKET */
#line 434 "geoJSON.y"
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) geoJSON_multipoint_xyz((gaiaPointPtr)yymsp[-2].minor.yy0);
	}
#line 1535 "geoJSON.c"
        break;
      case 103: /* multilinestring_text ::= GEOJSON_OPEN_BRACKET linestring_text multilinestring_text2 GEOJSON_CLOSE_BRACKET */
#line 496 "geoJSON.y"
{ 
	   ((gaiaLinestringPtr)yymsp[-2].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) geoJSON_multilinestring_xy((gaiaLinestringPtr)yymsp[-2].minor.yy0);
	}
#line 1543 "geoJSON.c"
        break;
      case 105: /* multilinestring_text2 ::= GEOJSON_COMMA linestring_text multilinestring_text2 */
      case 108: /* multilinestring_textz2 ::= GEOJSON_COMMA linestring_textz multilinestring_textz2 */ yytestcase(yyruleno==108);
#line 504 "geoJSON.y"
{ ((gaiaLinestringPtr)yymsp[-1].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[0].minor.yy0;  yygotominor.yy0 = yymsp[-1].minor.yy0; }
#line 1549 "geoJSON.c"
        break;
      case 106: /* multilinestring_textz ::= GEOJSON_OPEN_BRACKET linestring_textz multilinestring_textz2 GEOJSON_CLOSE_BRACKET */
#line 507 "geoJSON.y"
{ 
	   ((gaiaLinestringPtr)yymsp[-2].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) geoJSON_multilinestring_xyz((gaiaLinestringPtr)yymsp[-2].minor.yy0);
	}
#line 1557 "geoJSON.c"
        break;
      case 121: /* multipolygon_text ::= GEOJSON_OPEN_BRACKET polygon_text multipolygon_text2 GEOJSON_CLOSE_BRACKET */
#line 573 "geoJSON.y"
{ 
	   ((gaiaPolygonPtr)yymsp[-2].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) geoJSON_multipolygon_xy((gaiaPolygonPtr)yymsp[-2].minor.yy0);
	}
#line 1565 "geoJSON.c"
        break;
      case 123: /* multipolygon_text2 ::= GEOJSON_COMMA polygon_text multipolygon_text2 */
      case 126: /* multipolygon_textz2 ::= GEOJSON_COMMA polygon_textz multipolygon_textz2 */ yytestcase(yyruleno==126);
#line 581 "geoJSON.y"
{ ((gaiaPolygonPtr)yymsp[-1].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[0].minor.yy0;  yygotominor.yy0 = yymsp[-1].minor.yy0; }
#line 1571 "geoJSON.c"
        break;
      case 124: /* multipolygon_textz ::= GEOJSON_OPEN_BRACKET polygon_textz multipolygon_textz2 GEOJSON_CLOSE_BRACKET */
#line 584 "geoJSON.y"
{ 
	   ((gaiaPolygonPtr)yymsp[-2].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) geoJSON_multipolygon_xyz((gaiaPolygonPtr)yymsp[-2].minor.yy0);
	}
#line 1579 "geoJSON.c"
        break;
      case 139: /* geocoll_text ::= GEOJSON_OPEN_BRACKET coll_point geocoll_text2 GEOJSON_CLOSE_BRACKET */
      case 140: /* geocoll_text ::= GEOJSON_OPEN_BRACKET coll_linestring geocoll_text2 GEOJSON_CLOSE_BRACKET */ yytestcase(yyruleno==140);
      case 141: /* geocoll_text ::= GEOJSON_OPEN_BRACKET coll_polygon geocoll_text2 GEOJSON_CLOSE_BRACKET */ yytestcase(yyruleno==141);
#line 648 "geoJSON.y"
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) geoJSON_geomColl_xy((gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
#line 1589 "geoJSON.c"
        break;
      case 143: /* geocoll_text2 ::= GEOJSON_COMMA coll_point geocoll_text2 */
      case 144: /* geocoll_text2 ::= GEOJSON_COMMA coll_linestring geocoll_text2 */ yytestcase(yyruleno==144);
      case 145: /* geocoll_text2 ::= GEOJSON_COMMA coll_polygon geocoll_text2 */ yytestcase(yyruleno==145);
      case 150: /* geocoll_textz2 ::= GEOJSON_COMMA coll_pointz geocoll_textz2 */ yytestcase(yyruleno==150);
      case 151: /* geocoll_textz2 ::= GEOJSON_COMMA coll_linestringz geocoll_textz2 */ yytestcase(yyruleno==151);
      case 152: /* geocoll_textz2 ::= GEOJSON_COMMA coll_polygonz geocoll_textz2 */ yytestcase(yyruleno==152);
#line 668 "geoJSON.y"
{
		((gaiaGeomCollPtr)yymsp[-1].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[0].minor.yy0;
		yygotominor.yy0 = yymsp[-1].minor.yy0;
	}
#line 1602 "geoJSON.c"
        break;
      case 146: /* geocoll_textz ::= GEOJSON_OPEN_BRACKET coll_pointz geocoll_textz2 GEOJSON_CLOSE_BRACKET */
      case 147: /* geocoll_textz ::= GEOJSON_OPEN_BRACKET coll_linestringz geocoll_textz2 GEOJSON_CLOSE_BRACKET */ yytestcase(yyruleno==147);
      case 148: /* geocoll_textz ::= GEOJSON_OPEN_BRACKET coll_polygonz geocoll_textz2 GEOJSON_CLOSE_BRACKET */ yytestcase(yyruleno==148);
#line 687 "geoJSON.y"
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) geoJSON_geomColl_xyz((gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
#line 1612 "geoJSON.c"
        break;
      default:
      /* (0) main ::= in */ yytestcase(yyruleno==0);
      /* (1) in ::= */ yytestcase(yyruleno==1);
      /* (2) in ::= in state GEOJSON_NEWLINE */ yytestcase(yyruleno==2);
      /* (3) state ::= program */ yytestcase(yyruleno==3);
      /* (4) program ::= geo_text */ yytestcase(yyruleno==4);
      /* (31) bbox ::= coord GEOJSON_COMMA coord GEOJSON_COMMA coord GEOJSON_COMMA coord */ yytestcase(yyruleno==31);
        break;
  };
  yygoto = geoJSON_yyRuleInfo[yyruleno].lhs;
  yysize = geoJSON_yyRuleInfo[yyruleno].nrhs;
  yypParser->yyidx -= yysize;
  yyact = geoJSON_yy_find_reduce_action(yymsp[-yysize].stateno,(YYCODETYPE)yygoto);
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
      geoJSON_yy_shift(yypParser,yyact,yygoto,&yygotominor);
    }
  }else{
    assert( yyact == YYNSTATE + YYNRULE + 1 );
    geoJSON_yy_accept(yypParser);
  }
}

/*
** The following code executes when the parse fails
*/
#ifndef YYNOERRORRECOVERY
static void geoJSON_yy_parse_failed(
  geoJSON_yyParser *yypParser           /* The parser */
){
  ParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) geoJSON_yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
static void geoJSON_yy_syntax_error(
  geoJSON_yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  GEOJSON_YYMINORTYPE yyminor            /* The minor type of the error token */
){
  ParseARG_FETCH;
#define TOKEN (yyminor.yy0)
#line 62 "geoJSON.y"

/* 
** when the LEMON parser encounters an error
** then this global variable is set 
*/
	geoJSON_parse_error = 1;
	*result = NULL;
#line 1688 "geoJSON.c"
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void geoJSON_yy_accept(
  geoJSON_yyParser *yypParser           /* The parser */
){
  ParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) geoJSON_yy_pop_parser_stack(yypParser);
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
void geoJSONParse(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  ParseTOKENTYPE yyminor       /* The value for the token */
  ParseARG_PDECL               /* Optional %extra_argument parameter */
){
  GEOJSON_YYMINORTYPE yyminorunion;
  int yyact;            /* The parser action. */
  int yyendofinput;     /* True if we are at the end of input */
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  geoJSON_yyParser *yypParser;  /* The parser */

  /* (re)initialize the parser, if necessary */
  yypParser = (geoJSON_yyParser*)yyp;
  if( yypParser->yyidx<0 ){
#if YYSTACKDEPTH<=0
    if( yypParser->yystksz <=0 ){
      /*memset(&yyminorunion, 0, sizeof(yyminorunion));*/
      yyminorunion = geoJSON_yyzerominor;
      geoJSON_yyStackOverflow(yypParser, &yyminorunion);
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
    yyact = geoJSON_yy_find_shift_action(yypParser,(YYCODETYPE)yymajor);
    if( yyact<YYNSTATE ){
      assert( !yyendofinput );  /* Impossible to shift the $ token */
      geoJSON_yy_shift(yypParser,yyact,yymajor,&yyminorunion);
      yypParser->yyerrcnt--;
      yymajor = YYNOCODE;
    }else if( yyact < YYNSTATE + YYNRULE ){
      geoJSON_yy_reduce(yypParser,yyact-YYNSTATE);
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
        geoJSON_yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yymx = yypParser->yystack[yypParser->yyidx].major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        geoJSON_yy_destructor(yypParser, (YYCODETYPE)yymajor,&yyminorunion);
        yymajor = YYNOCODE;
      }else{
         while(
          yypParser->yyidx >= 0 &&
          yymx != YYERRORSYMBOL &&
          (yyact = geoJSON_yy_find_reduce_action(
                        yypParser->yystack[yypParser->yyidx].stateno,
                        YYERRORSYMBOL)) >= YYNSTATE
        ){
          geoJSON_yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yyidx < 0 || yymajor==0 ){
          geoJSON_yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
          geoJSON_yy_parse_failed(yypParser);
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          GEOJSON_YYMINORTYPE u2;
          u2.YYERRSYMDT = 0;
          geoJSON_yy_shift(yypParser,yyact,YYERRORSYMBOL,&u2);
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
      geoJSON_yy_syntax_error(yypParser,yymajor,yyminorunion);
      geoJSON_yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
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
        geoJSON_yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yypParser->yyerrcnt = 3;
      geoJSON_yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      if( yyendofinput ){
        geoJSON_yy_parse_failed(yypParser);
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && yypParser->yyidx>=0 );
  return;
}


/*
 GEOJSON_LEMON_END - LEMON generated code ends here 
*/

















/*
** CAVEAT: there is an incompatibility between LEMON and FLEX
** this macro resolves the issue
*/
#undef yy_accept
















/*
 GEOJSON_FLEX_START - FLEX generated code starts here 
*/

#line 3 "lex.GeoJson.c"

#define  YY_INT_ALIGNED short int

/* A lexical scanner generated by flex */

#define yy_create_buffer GeoJson_create_buffer
#define yy_delete_buffer GeoJson_delete_buffer
#define yy_flex_debug GeoJson_flex_debug
#define yy_init_buffer GeoJson_init_buffer
#define yy_flush_buffer GeoJson_flush_buffer
#define yy_load_buffer_state GeoJson_load_buffer_state
#define yy_switch_to_buffer GeoJson_switch_to_buffer
#define yyin GeoJsonin
#define yyleng GeoJsonleng
#define yylex GeoJsonlex
#define yylineno GeoJsonlineno
#define yyout GeoJsonout
#define yyrestart GeoJsonrestart
#define yytext GeoJsontext
#define yywrap GeoJsonwrap
#define yyalloc GeoJsonalloc
#define yyrealloc GeoJsonrealloc
#define yyfree GeoJsonfree

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
#define YY_NEW_FILE GeoJsonrestart(GeoJsonin  )

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

extern int GeoJsonleng;

extern FILE *GeoJsonin, *GeoJsonout;

#define EOB_ACT_CONTINUE_SCAN 0
#define EOB_ACT_END_OF_FILE 1
#define EOB_ACT_LAST_MATCH 2

    #define YY_LESS_LINENO(n)
    
/* Return all but the first "n" matched characters back to the input stream. */
#define yyless(n) \
	do \
		{ \
		/* Undo effects of setting up GeoJsontext. */ \
        int yyless_macro_arg = (n); \
        YY_LESS_LINENO(yyless_macro_arg);\
		*yy_cp = (yy_hold_char); \
		YY_RESTORE_YY_MORE_OFFSET \
		(yy_c_buf_p) = yy_cp = yy_bp + yyless_macro_arg - YY_MORE_ADJ; \
		YY_DO_BEFORE_ACTION; /* set up GeoJsontext again */ \
		} \
	while ( 0 )

#define unput(c) geoJSON_yyunput( c, (yytext_ptr)  )

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
	 * (via GeoJsonrestart()), so that the user can continue scanning by
	 * just pointing GeoJsonin at a new input file.
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

/* yy_hold_char holds the character lost when GeoJsontext is formed. */
static char yy_hold_char;
static int yy_n_chars;		/* number of characters read into yy_ch_buf */
int GeoJsonleng;

/* Points to current character in buffer. */
static char *yy_c_buf_p = (char *) 0;
static int yy_init = 0;		/* whether we need to initialize */
static int yy_start = 0;	/* start state number */

/* Flag which is used to allow GeoJsonwrap()'s to do buffer switches
 * instead of setting up a fresh GeoJsonin.  A bit of a hack ...
 */
static int yy_did_buffer_switch_on_eof;

void GeoJsonrestart (FILE *input_file  );
void GeoJson_switch_to_buffer (YY_BUFFER_STATE new_buffer  );
YY_BUFFER_STATE GeoJson_create_buffer (FILE *file,int size  );
void GeoJson_delete_buffer (YY_BUFFER_STATE b  );
void GeoJson_flush_buffer (YY_BUFFER_STATE b  );
void GeoJsonpush_buffer_state (YY_BUFFER_STATE new_buffer  );
void GeoJsonpop_buffer_state (void );

static void GeoJsonensure_buffer_stack (void );
static void GeoJson_load_buffer_state (void );
static void GeoJson_init_buffer (YY_BUFFER_STATE b,FILE *file  );

#define YY_FLUSH_BUFFER GeoJson_flush_buffer(YY_CURRENT_BUFFER )

YY_BUFFER_STATE GeoJson_scan_buffer (char *base,yy_size_t size  );
YY_BUFFER_STATE GeoJson_scan_string (yyconst char *yy_str  );
YY_BUFFER_STATE GeoJson_scan_bytes (yyconst char *bytes,int len  );

void *GeoJsonalloc (yy_size_t  );
void *GeoJsonrealloc (void *,yy_size_t  );
void GeoJsonfree (void *  );

#define yy_new_buffer GeoJson_create_buffer

#define yy_set_interactive(is_interactive) \
	{ \
	if ( ! YY_CURRENT_BUFFER ){ \
        GeoJsonensure_buffer_stack (); \
		YY_CURRENT_BUFFER_LVALUE =    \
            GeoJson_create_buffer(GeoJsonin,YY_BUF_SIZE ); \
	} \
	YY_CURRENT_BUFFER_LVALUE->yy_is_interactive = is_interactive; \
	}

#define yy_set_bol(at_bol) \
	{ \
	if ( ! YY_CURRENT_BUFFER ){\
        GeoJsonensure_buffer_stack (); \
		YY_CURRENT_BUFFER_LVALUE =    \
            GeoJson_create_buffer(GeoJsonin,YY_BUF_SIZE ); \
	} \
	YY_CURRENT_BUFFER_LVALUE->yy_at_bol = at_bol; \
	}

#define YY_AT_BOL() (YY_CURRENT_BUFFER_LVALUE->yy_at_bol)

/* Begin user sect3 */

typedef unsigned char YY_CHAR;

FILE *GeoJsonin = (FILE *) 0, *GeoJsonout = (FILE *) 0;

typedef int yy_state_type;

extern int GeoJsonlineno;

int GeoJsonlineno = 1;

extern char *GeoJsontext;
#define yytext_ptr GeoJsontext

static yy_state_type yy_get_previous_state (void );
static yy_state_type yy_try_NUL_trans (yy_state_type current_state  );
static int yy_get_next_buffer (void );
static void yy_fatal_error (yyconst char msg[]  );

/* Done after the current pattern has been matched and before the
 * corresponding action - sets up GeoJsontext.
 */
#define YY_DO_BEFORE_ACTION \
	(yytext_ptr) = yy_bp; \
	GeoJsonleng = (size_t) (yy_cp - yy_bp); \
	(yy_hold_char) = *yy_cp; \
	*yy_cp = '\0'; \
	(yy_c_buf_p) = yy_cp;

#define YY_NUM_RULES 27
#define YY_END_OF_BUFFER 28
/* This struct is not used in this scanner,
   but its presence is necessary. */
struct yy_trans_info
	{
	flex_int32_t yy_verify;
	flex_int32_t yy_nxt;
	};
static yyconst flex_int16_t yy_accept[195] =
    {   0,
        0,    0,   28,   26,   24,   25,   26,   26,    4,   26,
        1,    5,    8,    9,    6,    7,    4,    5,    8,    9,
        6,    7,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    1,    4,    0,    0,    1,    1,
        1,    5,    8,    9,    6,    7,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    1,
        1,    1,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    1,    1,    0,    0,
        0,    0,    0,    0,    0,    0,   16,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,   13,    0,

        0,   14,    0,   10,    0,    0,    0,    0,    0,    0,
        0,   17,    0,    0,    0,    0,    0,    2,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,   19,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,   18,    0,   20,    0,
        0,   12,   15,    0,    0,    0,    0,   11,    0,    0,
        0,   22,    0,    0,    0,    0,    0,    0,    0,    0,
       21,    0,    0,    0,    0,    0,   23,    0,    0,    0,
        0,    0,    3,    0

    } ;

static yyconst flex_int32_t yy_ec[256] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    2,    3,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    2,    1,    4,    1,    1,    1,    1,    1,    1,
        1,    1,    5,    6,    7,    8,    1,    9,    9,    9,
        9,    9,    9,    9,    9,    9,    9,   10,    1,    1,
        1,    1,    1,    1,    1,    1,   11,    1,   12,    1,
       13,    1,    1,    1,    1,   14,   15,    1,    1,   16,
        1,    1,   17,    1,    1,    1,    1,    1,    1,    1,
       18,    1,   19,    1,    1,    1,   20,   21,   22,   23,

       24,   25,   26,    1,   27,    1,    1,   28,   29,   30,
       31,   32,    1,   33,   34,   35,   36,    1,    1,   37,
       38,    1,   39,    1,   40,    1,    1,    1,    1,    1,
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

static yyconst flex_int32_t yy_meta[41] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1
    } ;

static yyconst flex_int16_t yy_base[196] =
    {   0,
        0,   39,  223,  224,  224,   11,   68,  213,  218,   11,
        3,  217,  216,  215,  214,  213,  212,  211,  210,  209,
      208,  207,  193,  184,  180,  170,  174,  183,    0,  179,
      182,  168,  162,  166,   14,  224,    0,    1,   16,  189,
       18,  224,  224,  224,  224,  224,  180,  165,  165,  166,
        7,  162,  161,  157,  159,  160,  157,  155,  156,  176,
      175,  174,  169,  152,  156,  144,  148,  139,  139,  142,
      170,  144,  148,  139,  146,  159,  159,  158,  156,  141,
      147,  136,  127,  135,  156,  136,  224,  134,  153,  132,
      151,  123,   29,  118,  117,   38,  147,  119,  224,  122,

      113,  224,  114,  224,  120,  136,   51,  111,  110,  115,
      110,  224,  110,  109,  105,  102,  114,  224,   97,  107,
      103,   34,  128,  111,  103,  102,  118,  116,   96,  101,
       94,   85,  224,   87,   97,   96,   96,   87,   91,   99,
       80,   88,   89,   78,   77,   86,   81,  104,   72,  102,
       74,   68,   97,   95,   72,   68,  224,   62,  224,   63,
       88,  224,  224,   81,   64,   60,   82,  224,   63,   55,
       46,  224,   42,   39,   47,   38,   44,   66,   59,   37,
      224,   55,   36,   40,   49,   30,  224,   19,   18,   56,
        7,   55,  224,  224,    0

    } ;

static yyconst flex_int16_t yy_def[196] =
    {   0,
      195,  195,  194,  194,  194,  194,  194,  194,  194,    6,
      194,  194,  194,  194,  194,  194,  194,  194,  194,  194,
      194,  194,  194,  194,  194,  194,  194,  194,  194,  194,
      194,  194,  194,  194,  194,  194,    6,  194,  194,  194,
      194,  194,  194,  194,  194,  194,  194,  194,  194,  194,
      194,  194,  194,  194,  194,  194,  194,  194,  194,  194,
      194,  194,  194,  194,  194,  194,  194,  194,  194,  194,
      194,  194,  194,  194,  194,  194,  194,  194,  194,  194,
      194,  194,  194,  194,  194,  194,  194,  194,  194,  194,
      194,  194,  194,  194,  194,  194,  194,  194,  194,  194,

      194,  194,  194,  194,  194,  194,  194,  194,  194,  194,
      194,  194,  194,  194,  194,  194,  194,  194,  194,  194,
      194,  194,  194,  194,  194,  194,  194,  194,  194,  194,
      194,  194,  194,  194,  194,  194,  194,  194,  194,  194,
      194,  194,  194,  194,  194,  194,  194,  194,  194,  194,
      194,  194,  194,  194,  194,  194,  194,  194,  194,  194,
      194,  194,  194,  194,  194,  194,  194,  194,  194,  194,
      194,  194,  194,  194,  194,  194,  194,  194,  194,  194,
      194,  194,  194,  194,  194,  194,  194,  194,  194,  194,
      194,  194,  194,    0,  194

    } ;

static yyconst flex_int16_t yy_nxt[265] =
    {   0,
        4,    5,    6,    7,    8,    9,   10,  194,   11,   12,
       40,   41,   23,   37,   38,  192,   17,   13,   14,   39,
       18,   60,   35,   61,   39,   40,   41,  190,   19,   20,
       53,  189,   54,   67,   68,  106,   34,  107,   15,   16,
        5,    6,    7,    8,    9,   10,  188,   11,   12,   21,
       22,  110,  187,  111,  118,  186,   13,   14,  193,  107,
      131,  132,  191,  192,  192,  185,  184,  183,  182,  181,
      180,  179,  178,  177,  176,  175,  174,   15,   16,   23,
       24,   25,   26,   27,  173,  172,  171,  170,   28,   29,
      169,  168,  167,   30,  166,  165,  164,   31,  163,   32,

      162,  161,   33,   34,  160,  159,  158,  157,  156,  155,
      154,  153,  152,  151,  150,  149,  148,  147,  146,  145,
      144,  143,  142,  141,  140,  139,  138,  137,  136,  135,
      134,  133,  130,  129,  128,  127,  126,  125,  124,  123,
      122,  121,  120,  119,  107,  117,  116,  115,  114,  113,
      112,  109,  108,  105,  104,  103,  102,  101,  100,   99,
       98,   97,   96,   95,   94,   93,   78,   77,   92,   91,
       90,   89,   88,   87,   86,   85,   84,   83,   82,   81,
       80,   79,   62,   78,   77,   76,   75,   74,   73,   72,
       71,   70,   69,   66,   65,   64,   63,   62,   59,   58,

       57,   56,   55,   52,   51,   50,   49,   48,   47,   46,
       45,   44,   43,   42,   36,   46,   45,   44,   43,   42,
       36,   35,  194,    3,  194,  194,  194,  194,  194,  194,
      194,  194,  194,  194,  194,  194,  194,  194,  194,  194,
      194,  194,  194,  194,  194,  194,  194,  194,  194,  194,
      194,  194,  194,  194,  194,  194,  194,  194,  194,  194,
      194,  194,  194,  194
    } ;

static yyconst flex_int16_t yy_chk[265] =
    {   0,
      195,    1,    1,    1,    1,    1,    1,    0,    1,    1,
       11,   11,   38,   10,   10,  191,    6,    1,    1,   10,
        6,   35,   35,   39,   39,   41,   41,  189,    6,    6,
       29,  188,   29,   51,   51,   93,   38,   93,    1,    1,
        2,    2,    2,    2,    2,    2,  186,    2,    2,    6,
        6,   96,  185,   96,  107,  184,    2,    2,  192,  107,
      122,  122,  190,  192,  190,  183,  182,  180,  179,  178,
      177,  176,  175,  174,  173,  171,  170,    2,    2,    7,
        7,    7,    7,    7,  169,  167,  166,  165,    7,    7,
      164,  161,  160,    7,  158,  156,  155,    7,  154,    7,

      153,  152,    7,    7,  151,  150,  149,  148,  147,  146,
      145,  144,  143,  142,  141,  140,  139,  138,  137,  136,
      135,  134,  132,  131,  130,  129,  128,  127,  126,  125,
      124,  123,  121,  120,  119,  117,  116,  115,  114,  113,
      111,  110,  109,  108,  106,  105,  103,  101,  100,   98,
       97,   95,   94,   92,   91,   90,   89,   88,   86,   85,
       84,   83,   82,   81,   80,   79,   78,   77,   76,   75,
       74,   73,   72,   71,   70,   69,   68,   67,   66,   65,
       64,   63,   62,   61,   60,   59,   58,   57,   56,   55,
       54,   53,   52,   50,   49,   48,   47,   40,   34,   33,

       32,   31,   30,   28,   27,   26,   25,   24,   23,   22,
       21,   20,   19,   18,   17,   16,   15,   14,   13,   12,
        9,    8,    3,  194,  194,  194,  194,  194,  194,  194,
      194,  194,  194,  194,  194,  194,  194,  194,  194,  194,
      194,  194,  194,  194,  194,  194,  194,  194,  194,  194,
      194,  194,  194,  194,  194,  194,  194,  194,  194,  194,
      194,  194,  194,  194
    } ;

static yy_state_type yy_last_accepting_state;
static char *yy_last_accepting_cpos;

extern int GeoJson_flex_debug;
int GeoJson_flex_debug = 0;

/* The intent behind this definition is that it'll catch
 * any uses of REJECT which flex missed.
 */
#define REJECT reject_used_but_not_detected
#define yymore() yymore_used_but_not_detected
#define YY_MORE_ADJ 0
#define YY_RESTORE_YY_MORE_OFFSET
char *GeoJsontext;
/* 
 geoJsonLexer.l -- GeoJSON parser - FLEX config
  
 version 2.4, 2011 May 16

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
int geoJSON_line = 1, geoJSON_col = 1;

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

int GeoJsonlex_destroy (void );

int GeoJsonget_debug (void );

void GeoJsonset_debug (int debug_flag  );

YY_EXTRA_TYPE GeoJsonget_extra (void );

void GeoJsonset_extra (YY_EXTRA_TYPE user_defined  );

FILE *GeoJsonget_in (void );

void GeoJsonset_in  (FILE * in_str  );

FILE *GeoJsonget_out (void );

void GeoJsonset_out  (FILE * out_str  );

int GeoJsonget_leng (void );

char *GeoJsonget_text (void );

int GeoJsonget_lineno (void );

void GeoJsonset_lineno (int line_number  );

/* Macros after this point can all be overridden by user definitions in
 * section 1.
 */

#ifndef YY_SKIP_YYWRAP
#ifdef __cplusplus
extern "C" int GeoJsonwrap (void );
#else
extern int GeoJsonwrap (void );
#endif
#endif

    static void geoJSON_yyunput (int c,char *buf_ptr  );
    
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
#define ECHO do { if (fwrite( GeoJsontext, GeoJsonleng, 1, GeoJsonout )) {} } while (0)
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
			     (c = getc( GeoJsonin )) != EOF && c != '\n'; ++n ) \
			buf[n] = (char) c; \
		if ( c == '\n' ) \
			buf[n++] = (char) c; \
		if ( c == EOF && ferror( GeoJsonin ) ) \
			YY_FATAL_ERROR( "input in flex scanner failed" ); \
		result = n; \
		} \
	else \
		{ \
		errno=0; \
		while ( (result = fread(buf, 1, max_size, GeoJsonin))==0 && ferror(GeoJsonin)) \
			{ \
			if( errno != EINTR) \
				{ \
				YY_FATAL_ERROR( "input in flex scanner failed" ); \
				break; \
				} \
			errno=0; \
			clearerr(GeoJsonin); \
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

extern int GeoJsonlex (void);

#define YY_DECL int GeoJsonlex (void)
#endif /* !YY_DECL */

/* Code executed at the beginning of each rule, after GeoJsontext and GeoJsonleng
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

		if ( ! GeoJsonin )
			GeoJsonin = stdin;

		if ( ! GeoJsonout )
			GeoJsonout = stdout;

		if ( ! YY_CURRENT_BUFFER ) {
			GeoJsonensure_buffer_stack ();
			YY_CURRENT_BUFFER_LVALUE =
				GeoJson_create_buffer(GeoJsonin,YY_BUF_SIZE );
		}

		GeoJson_load_buffer_state( );
		}

	while ( 1 )		/* loops until end-of-file is reached */
		{
		yy_cp = (yy_c_buf_p);

		/* Support of GeoJsontext. */
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
				if ( yy_current_state >= 195 )
					yy_c = yy_meta[(unsigned int) yy_c];
				}
			yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
			++yy_cp;
			}
		while ( yy_base[yy_current_state] != 224 );

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
{ geoJSON_col += (int) strlen(GeoJsontext);  GeoJsonLval.dval = atof(GeoJsontext); return GEOJSON_NUM; }
	YY_BREAK
case 2:
YY_RULE_SETUP
{ geoJSON_col += (int) strlen(GeoJsontext);  GeoJsonLval.ival = atoi(GeoJsontext+6); return GEOJSON_SHORT_SRID; }
	YY_BREAK
case 3:
YY_RULE_SETUP
{ geoJSON_col += (int) strlen(GeoJsontext);  GeoJsonLval.ival = atoi(GeoJsontext+22); return GEOJSON_LONG_SRID; }
	YY_BREAK
case 4:
/* rule 4 can match eol */
YY_RULE_SETUP
{ GeoJsonLval.dval = 0; return GEOJSON_COMMA; }
	YY_BREAK
case 5:
/* rule 5 can match eol */
YY_RULE_SETUP
{ GeoJsonLval.dval = 0; return GEOJSON_COLON; }
	YY_BREAK
case 6:
/* rule 6 can match eol */
YY_RULE_SETUP
{ GeoJsonLval.dval = 0; return GEOJSON_OPEN_BRACE; }
	YY_BREAK
case 7:
/* rule 7 can match eol */
YY_RULE_SETUP
{ GeoJsonLval.dval = 0; return GEOJSON_CLOSE_BRACE; }
	YY_BREAK
case 8:
/* rule 8 can match eol */
YY_RULE_SETUP
{ GeoJsonLval.dval = 0; return GEOJSON_OPEN_BRACKET; }
	YY_BREAK
case 9:
/* rule 9 can match eol */
YY_RULE_SETUP
{ GeoJsonLval.dval = 0; return GEOJSON_CLOSE_BRACKET; }
	YY_BREAK
case 10:
YY_RULE_SETUP
{ GeoJsonLval.dval = 0; return GEOJSON_TYPE; }
	YY_BREAK
case 11:
YY_RULE_SETUP
{ GeoJsonLval.dval = 0; return GEOJSON_COORDS; }
	YY_BREAK
case 12:
YY_RULE_SETUP
{ GeoJsonLval.dval = 0; return GEOJSON_GEOMS; }
	YY_BREAK
case 13:
YY_RULE_SETUP
{ GeoJsonLval.dval = 0; return GEOJSON_BBOX; }
	YY_BREAK
case 14:
YY_RULE_SETUP
{ GeoJsonLval.dval = 0; return GEOJSON_NAME; }
	YY_BREAK
case 15:
YY_RULE_SETUP
{ GeoJsonLval.dval = 0; return GEOJSON_PROPS; }
	YY_BREAK
case 16:
YY_RULE_SETUP
{ GeoJsonLval.dval = 0; return GEOJSON_CRS; }
	YY_BREAK
case 17:
YY_RULE_SETUP
{ GeoJsonLval.dval = 0; return GEOJSON_POINT; }
	YY_BREAK
case 18:
YY_RULE_SETUP
{ GeoJsonLval.dval = 0; return GEOJSON_LINESTRING; }
	YY_BREAK
case 19:
YY_RULE_SETUP
{ GeoJsonLval.dval = 0; return GEOJSON_POLYGON; }
	YY_BREAK
case 20:
YY_RULE_SETUP
{ GeoJsonLval.dval = 0; return GEOJSON_MULTIPOINT; }
	YY_BREAK
case 21:
YY_RULE_SETUP
{ GeoJsonLval.dval = 0; return GEOJSON_MULTILINESTRING; }
	YY_BREAK
case 22:
YY_RULE_SETUP
{ GeoJsonLval.dval = 0; return GEOJSON_MULTIPOLYGON; }
	YY_BREAK
case 23:
YY_RULE_SETUP
{ GeoJsonLval.dval = 0; return GEOJSON_GEOMETRYCOLLECTION; }
	YY_BREAK
case 24:
YY_RULE_SETUP
{ geoJSON_col += (int) strlen(GeoJsontext); }               /* ignore but count white space */
	YY_BREAK
case 25:
/* rule 25 can match eol */
YY_RULE_SETUP
{ geoJSON_col = 0; ++geoJSON_line; return GEOJSON_NEWLINE; }
	YY_BREAK
case 26:
YY_RULE_SETUP
{ geoJSON_col += (int) strlen(GeoJsontext); return -1; }
	YY_BREAK
case 27:
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
			 * just pointed GeoJsonin at a new source and called
			 * GeoJsonlex().  If so, then we have to assure
			 * consistency between YY_CURRENT_BUFFER and our
			 * globals.  Here is the right place to do so, because
			 * this is the first action (other than possibly a
			 * back-up) that will match for the new input source.
			 */
			(yy_n_chars) = YY_CURRENT_BUFFER_LVALUE->yy_n_chars;
			YY_CURRENT_BUFFER_LVALUE->yy_input_file = GeoJsonin;
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

				if ( GeoJsonwrap( ) )
					{
					/* Note: because we've taken care in
					 * yy_get_next_buffer() to have set up
					 * GeoJsontext, we can now set up
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
} /* end of GeoJsonlex */

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
					GeoJsonrealloc((void *) b->yy_ch_buf,b->yy_buf_size + 2  );
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
			GeoJsonrestart(GeoJsonin  );
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
		YY_CURRENT_BUFFER_LVALUE->yy_ch_buf = (char *) GeoJsonrealloc((void *) YY_CURRENT_BUFFER_LVALUE->yy_ch_buf,new_size  );
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
			if ( yy_current_state >= 195 )
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
		if ( yy_current_state >= 195 )
			yy_c = yy_meta[(unsigned int) yy_c];
		}
	yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
	yy_is_jam = (yy_current_state == 194);

	return yy_is_jam ? 0 : yy_current_state;
}

    static void geoJSON_yyunput (int c, register char * yy_bp )
{
	register char *yy_cp;
    
    yy_cp = (yy_c_buf_p);

	/* undo effects of setting up GeoJsontext */
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
					GeoJsonrestart(GeoJsonin );

					/*FALLTHROUGH*/

				case EOB_ACT_END_OF_FILE:
					{
					if ( GeoJsonwrap( ) )
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
	*(yy_c_buf_p) = '\0';	/* preserve GeoJsontext */
	(yy_hold_char) = *++(yy_c_buf_p);

	return c;
}
#endif	/* ifndef YY_NO_INPUT */

/** Immediately switch to a different input stream.
 * @param input_file A readable stream.
 * 
 * @note This function does not reset the start condition to @c INITIAL .
 */
    void GeoJsonrestart  (FILE * input_file )
{
    
	if ( ! YY_CURRENT_BUFFER ){
        GeoJsonensure_buffer_stack ();
		YY_CURRENT_BUFFER_LVALUE =
            GeoJson_create_buffer(GeoJsonin,YY_BUF_SIZE );
	}

	GeoJson_init_buffer(YY_CURRENT_BUFFER,input_file );
	GeoJson_load_buffer_state( );
}

/** Switch to a different input buffer.
 * @param new_buffer The new input buffer.
 * 
 */
    void GeoJson_switch_to_buffer  (YY_BUFFER_STATE  new_buffer )
{
    
	/* TODO. We should be able to replace this entire function body
	 * with
	 *		GeoJsonpop_buffer_state();
	 *		GeoJsonpush_buffer_state(new_buffer);
     */
	GeoJsonensure_buffer_stack ();
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
	GeoJson_load_buffer_state( );

	/* We don't actually know whether we did this switch during
	 * EOF (GeoJsonwrap()) processing, but the only time this flag
	 * is looked at is after GeoJsonwrap() is called, so it's safe
	 * to go ahead and always set it.
	 */
	(yy_did_buffer_switch_on_eof) = 1;
}

static void GeoJson_load_buffer_state  (void)
{
    	(yy_n_chars) = YY_CURRENT_BUFFER_LVALUE->yy_n_chars;
	(yytext_ptr) = (yy_c_buf_p) = YY_CURRENT_BUFFER_LVALUE->yy_buf_pos;
	GeoJsonin = YY_CURRENT_BUFFER_LVALUE->yy_input_file;
	(yy_hold_char) = *(yy_c_buf_p);
}

/** Allocate and initialize an input buffer state.
 * @param file A readable stream.
 * @param size The character buffer size in bytes. When in doubt, use @c YY_BUF_SIZE.
 * 
 * @return the allocated buffer state.
 */
    YY_BUFFER_STATE GeoJson_create_buffer  (FILE * file, int  size )
{
	YY_BUFFER_STATE b;
    
	b = (YY_BUFFER_STATE) GeoJsonalloc(sizeof( struct yy_buffer_state )  );
	if ( ! b )
		YY_FATAL_ERROR( "out of dynamic memory in GeoJson_create_buffer()" );

	b->yy_buf_size = size;

	/* yy_ch_buf has to be 2 characters longer than the size given because
	 * we need to put in 2 end-of-buffer characters.
	 */
	b->yy_ch_buf = (char *) GeoJsonalloc(b->yy_buf_size + 2  );
	if ( ! b->yy_ch_buf )
		YY_FATAL_ERROR( "out of dynamic memory in GeoJson_create_buffer()" );

	b->yy_is_our_buffer = 1;

	GeoJson_init_buffer(b,file );

	return b;
}

/** Destroy the buffer.
 * @param b a buffer created with GeoJson_create_buffer()
 * 
 */
    void GeoJson_delete_buffer (YY_BUFFER_STATE  b )
{
    
	if ( ! b )
		return;

	if ( b == YY_CURRENT_BUFFER ) /* Not sure if we should pop here. */
		YY_CURRENT_BUFFER_LVALUE = (YY_BUFFER_STATE) 0;

	if ( b->yy_is_our_buffer )
		GeoJsonfree((void *) b->yy_ch_buf  );

	GeoJsonfree((void *) b  );
}

#ifndef __cplusplus
extern int isatty (int );
#endif /* __cplusplus */
    
/* Initializes or reinitializes a buffer.
 * This function is sometimes called more than once on the same buffer,
 * such as during a GeoJsonrestart() or at EOF.
 */
    static void GeoJson_init_buffer  (YY_BUFFER_STATE  b, FILE * file )

{
	int oerrno = errno;
    
	GeoJson_flush_buffer(b );

	b->yy_input_file = file;
	b->yy_fill_buffer = 1;

    /* If b is the current buffer, then GeoJson_init_buffer was _probably_
     * called from GeoJsonrestart() or through yy_get_next_buffer.
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
    void GeoJson_flush_buffer (YY_BUFFER_STATE  b )
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
		GeoJson_load_buffer_state( );
}

/** Pushes the new state onto the stack. The new state becomes
 *  the current state. This function will allocate the stack
 *  if necessary.
 *  @param new_buffer The new state.
 *  
 */
void GeoJsonpush_buffer_state (YY_BUFFER_STATE new_buffer )
{
    	if (new_buffer == NULL)
		return;

	GeoJsonensure_buffer_stack();

	/* This block is copied from GeoJson_switch_to_buffer. */
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

	/* copied from GeoJson_switch_to_buffer. */
	GeoJson_load_buffer_state( );
	(yy_did_buffer_switch_on_eof) = 1;
}

/** Removes and deletes the top of the stack, if present.
 *  The next element becomes the new top.
 *  
 */
void GeoJsonpop_buffer_state (void)
{
    	if (!YY_CURRENT_BUFFER)
		return;

	GeoJson_delete_buffer(YY_CURRENT_BUFFER );
	YY_CURRENT_BUFFER_LVALUE = NULL;
	if ((yy_buffer_stack_top) > 0)
		--(yy_buffer_stack_top);

	if (YY_CURRENT_BUFFER) {
		GeoJson_load_buffer_state( );
		(yy_did_buffer_switch_on_eof) = 1;
	}
}

/* Allocates the stack if it does not exist.
 *  Guarantees space for at least one push.
 */
static void GeoJsonensure_buffer_stack (void)
{
	int num_to_alloc;
    
	if (!(yy_buffer_stack)) {

		/* First allocation is just for 2 elements, since we don't know if this
		 * scanner will even need a stack. We use 2 instead of 1 to avoid an
		 * immediate realloc on the next call.
         */
		num_to_alloc = 1;
		(yy_buffer_stack) = (struct yy_buffer_state**)GeoJsonalloc
								(num_to_alloc * sizeof(struct yy_buffer_state*)
								);
		if ( ! (yy_buffer_stack) )
			YY_FATAL_ERROR( "out of dynamic memory in GeoJsonensure_buffer_stack()" );
								  
		memset((yy_buffer_stack), 0, num_to_alloc * sizeof(struct yy_buffer_state*));
				
		(yy_buffer_stack_max) = num_to_alloc;
		(yy_buffer_stack_top) = 0;
		return;
	}

	if ((yy_buffer_stack_top) >= ((yy_buffer_stack_max)) - 1){

		/* Increase the buffer to prepare for a possible push. */
		int grow_size = 8 /* arbitrary grow size */;

		num_to_alloc = (yy_buffer_stack_max) + grow_size;
		(yy_buffer_stack) = (struct yy_buffer_state**)GeoJsonrealloc
								((yy_buffer_stack),
								num_to_alloc * sizeof(struct yy_buffer_state*)
								);
		if ( ! (yy_buffer_stack) )
			YY_FATAL_ERROR( "out of dynamic memory in GeoJsonensure_buffer_stack()" );

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
YY_BUFFER_STATE GeoJson_scan_buffer  (char * base, yy_size_t  size )
{
	YY_BUFFER_STATE b;
    
	if ( size < 2 ||
	     base[size-2] != YY_END_OF_BUFFER_CHAR ||
	     base[size-1] != YY_END_OF_BUFFER_CHAR )
		/* They forgot to leave room for the EOB's. */
		return 0;

	b = (YY_BUFFER_STATE) GeoJsonalloc(sizeof( struct yy_buffer_state )  );
	if ( ! b )
		YY_FATAL_ERROR( "out of dynamic memory in GeoJson_scan_buffer()" );

	b->yy_buf_size = size - 2;	/* "- 2" to take care of EOB's */
	b->yy_buf_pos = b->yy_ch_buf = base;
	b->yy_is_our_buffer = 0;
	b->yy_input_file = 0;
	b->yy_n_chars = b->yy_buf_size;
	b->yy_is_interactive = 0;
	b->yy_at_bol = 1;
	b->yy_fill_buffer = 0;
	b->yy_buffer_status = YY_BUFFER_NEW;

	GeoJson_switch_to_buffer(b  );

	return b;
}

/** Setup the input buffer state to scan a string. The next call to GeoJsonlex() will
 * scan from a @e copy of @a str.
 * @param yystr a NUL-terminated string to scan
 * 
 * @return the newly allocated buffer state object.
 * @note If you want to scan bytes that may contain NUL values, then use
 *       GeoJson_scan_bytes() instead.
 */
YY_BUFFER_STATE GeoJson_scan_string (yyconst char * yystr )
{
    
	return GeoJson_scan_bytes(yystr,strlen(yystr) );
}

/** Setup the input buffer state to scan the given bytes. The next call to GeoJsonlex() will
 * scan from a @e copy of @a bytes.
 * @param yybytes the byte buffer to scan
 * @param _yybytes_len the number of bytes in the buffer pointed to by @a bytes.
 * 
 * @return the newly allocated buffer state object.
 */
YY_BUFFER_STATE GeoJson_scan_bytes  (yyconst char * yybytes, int  _yybytes_len )
{
	YY_BUFFER_STATE b;
	char *buf;
	yy_size_t n;
	int i;
    
	/* Get memory for full buffer, including space for trailing EOB's. */
	n = _yybytes_len + 2;
	buf = (char *) GeoJsonalloc(n  );
	if ( ! buf )
		YY_FATAL_ERROR( "out of dynamic memory in GeoJson_scan_bytes()" );

	for ( i = 0; i < _yybytes_len; ++i )
		buf[i] = yybytes[i];

	buf[_yybytes_len] = buf[_yybytes_len+1] = YY_END_OF_BUFFER_CHAR;

	b = GeoJson_scan_buffer(buf,n );
	if ( ! b )
		YY_FATAL_ERROR( "bad buffer in GeoJson_scan_bytes()" );

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
		/* Undo effects of setting up GeoJsontext. */ \
        int yyless_macro_arg = (n); \
        YY_LESS_LINENO(yyless_macro_arg);\
		GeoJsontext[GeoJsonleng] = (yy_hold_char); \
		(yy_c_buf_p) = GeoJsontext + yyless_macro_arg; \
		(yy_hold_char) = *(yy_c_buf_p); \
		*(yy_c_buf_p) = '\0'; \
		GeoJsonleng = yyless_macro_arg; \
		} \
	while ( 0 )

/* Accessor  methods (get/set functions) to struct members. */

/** Get the current line number.
 * 
 */
int GeoJsonget_lineno  (void)
{
        
    return GeoJsonlineno;
}

/** Get the input stream.
 * 
 */
FILE *GeoJsonget_in  (void)
{
        return GeoJsonin;
}

/** Get the output stream.
 * 
 */
FILE *GeoJsonget_out  (void)
{
        return GeoJsonout;
}

/** Get the length of the current token.
 * 
 */
int GeoJsonget_leng  (void)
{
        return GeoJsonleng;
}

/** Get the current token.
 * 
 */

char *GeoJsonget_text  (void)
{
        return GeoJsontext;
}

/** Set the current line number.
 * @param line_number
 * 
 */
void GeoJsonset_lineno (int  line_number )
{
    
    GeoJsonlineno = line_number;
}

/** Set the input stream. This does not discard the current
 * input buffer.
 * @param in_str A readable stream.
 * 
 * @see GeoJson_switch_to_buffer
 */
void GeoJsonset_in (FILE *  in_str )
{
        GeoJsonin = in_str ;
}

void GeoJsonset_out (FILE *  out_str )
{
        GeoJsonout = out_str ;
}

int GeoJsonget_debug  (void)
{
        return GeoJson_flex_debug;
}

void GeoJsonset_debug (int  bdebug )
{
        GeoJson_flex_debug = bdebug ;
}

static int yy_init_globals (void)
{
        /* Initialization is the same as for the non-reentrant scanner.
     * This function is called from GeoJsonlex_destroy(), so don't allocate here.
     */

    (yy_buffer_stack) = 0;
    (yy_buffer_stack_top) = 0;
    (yy_buffer_stack_max) = 0;
    (yy_c_buf_p) = (char *) 0;
    (yy_init) = 0;
    (yy_start) = 0;

/* Defined in main.c */
#ifdef YY_STDINIT
    GeoJsonin = stdin;
    GeoJsonout = stdout;
#else
    GeoJsonin = (FILE *) 0;
    GeoJsonout = (FILE *) 0;
#endif

    /* For future reference: Set errno on error, since we are called by
     * GeoJsonlex_init()
     */
    return 0;
}

/* GeoJsonlex_destroy is for both reentrant and non-reentrant scanners. */
int GeoJsonlex_destroy  (void)
{
    
    /* Pop the buffer stack, destroying each element. */
	while(YY_CURRENT_BUFFER){
		GeoJson_delete_buffer(YY_CURRENT_BUFFER  );
		YY_CURRENT_BUFFER_LVALUE = NULL;
		GeoJsonpop_buffer_state();
	}

	/* Destroy the stack itself. */
	GeoJsonfree((yy_buffer_stack) );
	(yy_buffer_stack) = NULL;

    /* Reset the globals. This is important in a non-reentrant scanner so the next time
     * GeoJsonlex() is called, initialization will occur. */
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

void *GeoJsonalloc (yy_size_t  size )
{
	return (void *) malloc( size );
}

void *GeoJsonrealloc  (void * ptr, yy_size_t  size )
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

void GeoJsonfree (void * ptr )
{
	free( (char *) ptr );	/* see GeoJsonrealloc() for (char *) cast */
}

#define YYTABLES_NAME "yytables"

/**
 * reset the line and column count
 *
 *
 */
void geoJSON_reset_lexer(void)
{

  geoJSON_line = 1;
  geoJSON_col  = 1;

}

/**
 * GeoJsonError() is invoked when the lexer or the parser encounter
 * an error. The error message is passed via *s
 *
 *
 */
void GeoJsonError(char *s)
{
  printf("error: %s at line: %d col: %d\n",s,geoJSON_line,geoJSON_col);

}

int GeoJsonwrap(void)
{
  return 1;
}


/* 
 GEOJSON_FLEX_END - FLEX generated code ends here 
*/



/*
** This is a linked-list struct to store all the values for each token.
** All tokens will have a value of 0, except tokens denoted as NUM.
** NUM tokens are geometry coordinates and will contain the floating
** point number.
*/
typedef struct geoJsonFlexTokenStruct
{
    double value;
    struct geoJsonFlexTokenStruct *Next;
} geoJsonFlexToken;

/*
** Function to clean up the linked-list of token values.
*/
static int
geoJSON_cleanup (geoJsonFlexToken * token)
{
    geoJsonFlexToken *ptok;
    geoJsonFlexToken *ptok_n;
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

gaiaGeomCollPtr
gaiaParseGeoJSON (const unsigned char *dirty_buffer)
{
    void *pParser = geoJSONParseAlloc (malloc);
    /* Linked-list of token values */
    geoJsonFlexToken *tokens = malloc (sizeof (geoJsonFlexToken));
    /* Pointer to the head of the list */
    geoJsonFlexToken *head = tokens;
    int yv;
    gaiaGeomCollPtr result = NULL;

    geoJSON_parse_error = 0;

    GeoJson_scan_string ((char *) dirty_buffer);

    /*
       / Keep tokenizing until we reach the end
       / yylex() will return the next matching Token for us.
     */
    while ((yv = yylex ()) != 0)
      {
	  if (yv == -1)
	    {
		return NULL;
	    }
	  tokens->Next = malloc (sizeof (geoJsonFlexToken));
	  /*
	     /GeoJsonLval is a global variable from FLEX.
	     /GeoJsonLval is defined in geoJsonLexglobal.h
	   */
	  tokens->Next->value = GeoJsonLval.dval;
	  /* Pass the token to the wkt parser created from lemon */
	  geoJSONParse (pParser, yv, &(tokens->Next->value), &result);
	  tokens = tokens->Next;
      }
    /* This denotes the end of a line as well as the end of the parser */
    geoJSONParse (pParser, GEOJSON_NEWLINE, 0, &result);
    geoJSONParseFree (pParser, free);
    GeoJsonlex_destroy ();

    /* Assigning the token as the end to avoid seg faults while cleaning */
    tokens->Next = NULL;
    geoJSON_cleanup (head);

    if (geoJSON_parse_error)
      {
	  if (result)
	      gaiaFreeGeomColl (result);
	  return NULL;
      }

    if (!result)
	return NULL;
    if (!geoJsonCheckValidity (result))
      {
	  gaiaFreeGeomColl (result);
	  return NULL;
      }

    gaiaMbrGeometry (result);

    return result;
}


/*
** CAVEAT: we must now undefine any Flex own macro
*/
#undef YYNOCODE
#undef YYNSTATE
#undef YYNRULE
#undef YY_SHIFT_MAX
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
#undef yy_accept
#undef yy_create_buffer
#undef yy_delete_buffer
#undef yy_flex_debug
#undef yy_init_buffer
#undef yy_flush_buffer
#undef yy_load_buffer_state
#undef yy_switch_to_buffer
#undef yyin
#undef yyleng
#undef yylex
#undef yylineno
#undef yyout
#undef yyrestart
#undef yytext
#undef yywrap
#undef yyalloc
#undef yyrealloc
#undef yyfree
#undef yyless
#undef yy_new_buffer
#undef yy_set_interactive
#undef yy_set_bol
#undef yytext_ptr
#undef unput
#undef YYSTYPE

