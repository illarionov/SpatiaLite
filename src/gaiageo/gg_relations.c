/*

 gg_relations.c -- Gaia spatial relations
    
 version 2.4, 2009 September 17

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

#if defined(_WIN32) && !defined(__MINGW32__)
/* MSVC strictly requires this include [off_t] */
#include <sys/types.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef OMIT_GEOS		/* including GEOS */
#include <geos_c.h>
#endif

#ifdef SPL_AMALGAMATION	/* spatialite-amalgamation */
#include <spatialite/sqlite3ext.h>
#else
#include <sqlite3ext.h>
#endif	

#include <spatialite/gaiageo.h>

/* GLOBAL variables */
char gaia_geos_error_msg[2048];
char gaia_geos_warning_msg[2048];

GAIAGEO_DECLARE void
gaiaResetGeosMsg ()
{
/* resets the GEOS error and warning messages */
    *gaia_geos_error_msg = '\0';
    *gaia_geos_warning_msg = '\0';
}

GAIAGEO_DECLARE const char *
gaiaGetGeosErrorMsg ()
{
/* return the latest GEOS error message */
    return gaia_geos_error_msg;
}

GAIAGEO_DECLARE const char *
gaiaGetGeosWarningMsg ()
{
/* return the latest GEOS error message */
    return gaia_geos_warning_msg;
}

GAIAGEO_DECLARE void
gaiaSetGeosErrorMsg (const char *msg)
{
/* return the latest GEOS error message */
    strcpy (gaia_geos_error_msg, msg);
}

GAIAGEO_DECLARE void
gaiaSetGeosWarningMsg (const char *msg)
{
/* return the latest GEOS error message */
    strcpy (gaia_geos_warning_msg, msg);
}

static int
check_point (double *coords, int points, double x, double y)
{
/* checks if [X,Y] point is defined into this coordinate array [Linestring or Ring] */
    int iv;
    double xx;
    double yy;
    for (iv = 0; iv < points; iv++)
      {
	  gaiaGetPoint (coords, iv, &xx, &yy);
	  if (xx == x && yy == y)
	      return 1;
      }
    return 0;
}

GAIAGEO_DECLARE int
gaiaLinestringEquals (gaiaLinestringPtr line1, gaiaLinestringPtr line2)
{
/* checks if two Linestrings are "spatially equal" */
    int iv;
    double x;
    double y;
    if (line1->Points != line2->Points)
	return 0;
    for (iv = 0; iv < line1->Points; iv++)
      {
	  gaiaGetPoint (line1->Coords, iv, &x, &y);
	  if (!check_point (line2->Coords, line2->Points, x, y))
	      return 0;
      }
    return 1;
}

GAIAGEO_DECLARE int
gaiaPolygonEquals (gaiaPolygonPtr polyg1, gaiaPolygonPtr polyg2)
{
/* checks if two Polygons are "spatially equal" */
    int ib;
    int ib2;
    int iv;
    int ok2;
    double x;
    double y;
    gaiaRingPtr ring1;
    gaiaRingPtr ring2;
    if (polyg1->NumInteriors != polyg2->NumInteriors)
	return 0;
/* checking the EXTERIOR RINGs */
    ring1 = polyg1->Exterior;
    ring2 = polyg2->Exterior;
    if (ring1->Points != ring2->Points)
	return 0;
    for (iv = 0; iv < ring1->Points; iv++)
      {
	  gaiaGetPoint (ring1->Coords, iv, &x, &y);
	  if (!check_point (ring2->Coords, ring2->Points, x, y))
	      return 0;
      }
    for (ib = 0; ib < polyg1->NumInteriors; ib++)
      {
	  /* checking the INTERIOR RINGS */
	  int ok = 0;
	  ring1 = polyg1->Interiors + ib;
	  for (ib2 = 0; ib2 < polyg2->NumInteriors; ib2++)
	    {
		ok2 = 1;
		ring2 = polyg2->Interiors + ib2;
		for (iv = 0; iv < ring1->Points; iv++)
		  {
		      gaiaGetPoint (ring1->Coords, iv, &x, &y);
		      if (!check_point (ring2->Coords, ring2->Points, x, y))
			{
			    ok2 = 0;
			    break;
			}
		  }
		if (ok2)
		  {
		      ok = 1;
		      break;
		  }
	    }
	  if (!ok)
	      return 0;
      }
    return 1;
}

#ifndef OMIT_GEOS		/* including GEOS */

GAIAGEO_DECLARE int
gaiaGeomCollEquals (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2)
{
/* checks if two Geometries are "spatially equal" */
    int ret;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    if (!geom1 || !geom2)
	return -1;
    g1 = gaiaToGeos (geom1);
    g2 = gaiaToGeos (geom2);
    ret = GEOSEquals (g1, g2);
    GEOSGeom_destroy (g1);
    GEOSGeom_destroy (g2);
    return ret;
}

GAIAGEO_DECLARE int
gaiaGeomCollIntersects (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2)
{
/* checks if two Geometries do "spatially intersects" */
    int ret;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    if (!geom1 || !geom2)
	return -1;
    g1 = gaiaToGeos (geom1);
    g2 = gaiaToGeos (geom2);
    ret = GEOSIntersects (g1, g2);
    GEOSGeom_destroy (g1);
    GEOSGeom_destroy (g2);
    return ret;
}

GAIAGEO_DECLARE int
gaiaGeomCollDisjoint (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2)
{
/* checks if two Geometries are "spatially disjoint" */
    int ret;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    if (!geom1 || !geom2)
	return -1;
    g1 = gaiaToGeos (geom1);
    g2 = gaiaToGeos (geom2);
    ret = GEOSDisjoint (g1, g2);
    GEOSGeom_destroy (g1);
    GEOSGeom_destroy (g2);
    return ret;
}

GAIAGEO_DECLARE int
gaiaGeomCollOverlaps (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2)
{
/* checks if two Geometries do "spatially overlaps" */
    int ret;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    if (!geom1 || !geom2)
	return -1;
    g1 = gaiaToGeos (geom1);
    g2 = gaiaToGeos (geom2);
    ret = GEOSOverlaps (g1, g2);
    GEOSGeom_destroy (g1);
    GEOSGeom_destroy (g2);
    return ret;
}

GAIAGEO_DECLARE int
gaiaGeomCollCrosses (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2)
{
/* checks if two Geometries do "spatially crosses" */
    int ret;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    if (!geom1 || !geom2)
	return -1;
    g1 = gaiaToGeos (geom1);
    g2 = gaiaToGeos (geom2);
    ret = GEOSCrosses (g1, g2);
    GEOSGeom_destroy (g1);
    GEOSGeom_destroy (g2);
    return ret;
}

GAIAGEO_DECLARE int
gaiaGeomCollTouches (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2)
{
/* checks if two Geometries do "spatially touches" */
    int ret;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    if (!geom1 || !geom2)
	return -1;
    g1 = gaiaToGeos (geom1);
    g2 = gaiaToGeos (geom2);
    ret = GEOSTouches (g1, g2);
    GEOSGeom_destroy (g1);
    GEOSGeom_destroy (g2);
    return ret;
}

GAIAGEO_DECLARE int
gaiaGeomCollWithin (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2)
{
/* checks if GEOM-1 is completely contained within GEOM-2 */
    int ret;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    if (!geom1 || !geom2)
	return -1;
    g1 = gaiaToGeos (geom1);
    g2 = gaiaToGeos (geom2);
    ret = GEOSWithin (g1, g2);
    GEOSGeom_destroy (g1);
    GEOSGeom_destroy (g2);
    return ret;
}

GAIAGEO_DECLARE int
gaiaGeomCollContains (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2)
{
/* checks if GEOM-1 completely contains GEOM-2 */
    int ret;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    if (!geom1 || !geom2)
	return -1;
    g1 = gaiaToGeos (geom1);
    g2 = gaiaToGeos (geom2);
    ret = GEOSContains (g1, g2);
    GEOSGeom_destroy (g1);
    GEOSGeom_destroy (g2);
    return ret;
}

GAIAGEO_DECLARE int
gaiaGeomCollRelate (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2,
		    const char *pattern)
{
/* checks if if GEOM-1 and GEOM-2 have a spatial relationship as specified by the pattern Matrix */
    int ret;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    if (!geom1 || !geom2)
	return -1;
    g1 = gaiaToGeos (geom1);
    g2 = gaiaToGeos (geom2);
    ret = GEOSRelatePattern (g1, g2, pattern);
    GEOSGeom_destroy (g1);
    GEOSGeom_destroy (g2);
    return ret;
}

GAIAGEO_DECLARE int
gaiaGeomCollLength (gaiaGeomCollPtr geom, double *xlength)
{
/* computes the total length for this Geometry */
    double length;
    int ret;
    GEOSGeometry *g;
    if (!geom)
	return 0;
    g = gaiaToGeos (geom);
    ret = GEOSLength (g, &length);
    GEOSGeom_destroy (g);
    if (ret)
	*xlength = length;
    return ret;
}

GAIAGEO_DECLARE int
gaiaGeomCollArea (gaiaGeomCollPtr geom, double *xarea)
{
/* computes the total area for this Geometry */
    double area;
    int ret;
    GEOSGeometry *g;
    if (!geom)
	return 0;
    g = gaiaToGeos (geom);
    ret = GEOSArea (g, &area);
    GEOSGeom_destroy (g);
    if (ret)
	*xarea = area;
    return ret;
}

GAIAGEO_DECLARE int
gaiaGeomCollDistance (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2,
		      double *xdist)
{
/* computes the minimum distance intercurring between GEOM-1 and GEOM-2 */
    double dist;
    int ret;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    if (!geom1 || !geom2)
	return 0;
    g1 = gaiaToGeos (geom1);
    g2 = gaiaToGeos (geom2);
    ret = GEOSDistance (g1, g2, &dist);
    GEOSGeom_destroy (g1);
    GEOSGeom_destroy (g2);
    if (ret)
	*xdist = dist;
    return ret;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaGeometryIntersection (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2)
{
/* builds a new geometry representing the "spatial intersection" of GEOM-1 and GEOM-2 */
    gaiaGeomCollPtr geo;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    GEOSGeometry *g3;
    if (!geom1 || !geom2)
	return NULL;
    g1 = gaiaToGeos (geom1);
    g2 = gaiaToGeos (geom2);
    g3 = GEOSIntersection (g1, g2);
    GEOSGeom_destroy (g1);
    GEOSGeom_destroy (g2);
    if (!g3)
	return NULL;
    if (geom1->DimensionModel == GAIA_XY_Z)
	geo = gaiaFromGeos_XYZ (g3);
    else if (geom1->DimensionModel == GAIA_XY_M)
	geo = gaiaFromGeos_XYM (g3);
    else if (geom1->DimensionModel == GAIA_XY_Z_M)
	geo = gaiaFromGeos_XYZM (g3);
    else
	geo = gaiaFromGeos_XY (g3);
    GEOSGeom_destroy (g3);
    if (geo == NULL)
	return NULL;
    geo->Srid = geom1->Srid;
    return geo;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaGeometryUnion (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2)
{
/* builds a new geometry representing the "spatial union" of GEOM-1 and GEOM-2 */
    gaiaGeomCollPtr geo;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    GEOSGeometry *g3;
    if (!geom1 || !geom2)
	return NULL;
    g1 = gaiaToGeos (geom1);
    g2 = gaiaToGeos (geom2);
    g3 = GEOSUnion (g1, g2);
    GEOSGeom_destroy (g1);
    GEOSGeom_destroy (g2);
    if (geom1->DimensionModel == GAIA_XY_Z)
	geo = gaiaFromGeos_XYZ (g3);
    else if (geom1->DimensionModel == GAIA_XY_M)
	geo = gaiaFromGeos_XYM (g3);
    else if (geom1->DimensionModel == GAIA_XY_Z_M)
	geo = gaiaFromGeos_XYZM (g3);
    else
	geo = gaiaFromGeos_XY (g3);
    GEOSGeom_destroy (g3);
    if (geo == NULL)
	return NULL;
    geo->Srid = geom1->Srid;
    if (geo->DeclaredType == GAIA_POINT &&
	geom1->DeclaredType == GAIA_MULTIPOINT)
	geo->DeclaredType = GAIA_MULTIPOINT;
    if (geo->DeclaredType == GAIA_LINESTRING &&
	geom1->DeclaredType == GAIA_MULTILINESTRING)
	geo->DeclaredType = GAIA_MULTILINESTRING;
    if (geo->DeclaredType == GAIA_POLYGON &&
	geom1->DeclaredType == GAIA_MULTIPOLYGON)
	geo->DeclaredType = GAIA_MULTIPOLYGON;
    return geo;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaGeometryDifference (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2)
{
/* builds a new geometry representing the "spatial difference" of GEOM-1 and GEOM-2 */
    gaiaGeomCollPtr geo;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    GEOSGeometry *g3;
    if (!geom1 || !geom2)
	return NULL;
    g1 = gaiaToGeos (geom1);
    g2 = gaiaToGeos (geom2);
    g3 = GEOSDifference (g1, g2);
    GEOSGeom_destroy (g1);
    GEOSGeom_destroy (g2);
    if (!g3)
	return NULL;
    if (geom1->DimensionModel == GAIA_XY_Z)
	geo = gaiaFromGeos_XYZ (g3);
    else if (geom1->DimensionModel == GAIA_XY_M)
	geo = gaiaFromGeos_XYM (g3);
    else if (geom1->DimensionModel == GAIA_XY_Z_M)
	geo = gaiaFromGeos_XYZM (g3);
    else
	geo = gaiaFromGeos_XY (g3);
    GEOSGeom_destroy (g3);
    if (geo == NULL)
	return NULL;
    geo->Srid = geom1->Srid;
    return geo;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaGeometrySymDifference (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2)
{
/* builds a new geometry representing the "spatial symmetric difference" of GEOM-1 and GEOM-2 */
    gaiaGeomCollPtr geo;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    GEOSGeometry *g3;
    if (!geom1 || !geom2)
	return NULL;
    g1 = gaiaToGeos (geom1);
    g2 = gaiaToGeos (geom2);
    g3 = GEOSSymDifference (g1, g2);
    GEOSGeom_destroy (g1);
    GEOSGeom_destroy (g2);
    if (!g3)
	return NULL;
    if (geom1->DimensionModel == GAIA_XY_Z)
	geo = gaiaFromGeos_XYZ (g3);
    else if (geom1->DimensionModel == GAIA_XY_M)
	geo = gaiaFromGeos_XYM (g3);
    else if (geom1->DimensionModel == GAIA_XY_Z_M)
	geo = gaiaFromGeos_XYZM (g3);
    else
	geo = gaiaFromGeos_XY (g3);
    GEOSGeom_destroy (g3);
    if (geo == NULL)
	return NULL;
    geo->Srid = geom1->Srid;
    return geo;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaBoundary (gaiaGeomCollPtr geom)
{
/* builds a new geometry representing the conbinatorial boundary of GEOM */
    gaiaGeomCollPtr geo;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    if (!geom)
	return NULL;
    g1 = gaiaToGeos (geom);
    g2 = GEOSBoundary (g1);
    GEOSGeom_destroy (g1);
    if (!g2)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	geo = gaiaFromGeos_XYZ (g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	geo = gaiaFromGeos_XYM (g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	geo = gaiaFromGeos_XYZM (g2);
    else
	geo = gaiaFromGeos_XY (g2);
    GEOSGeom_destroy (g2);
    if (geo == NULL)
	return NULL;
    geo->Srid = geom->Srid;
    return geo;
}

GAIAGEO_DECLARE int
gaiaGeomCollCentroid (gaiaGeomCollPtr geom, double *x, double *y)
{
/* returns a Point representing the centroid for this Geometry */
    gaiaGeomCollPtr geo;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    if (!geom)
	return 0;
    g1 = gaiaToGeos (geom);
    g2 = GEOSGetCentroid (g1);
    GEOSGeom_destroy (g1);
    if (!g2)
	return 0;
    if (geom->DimensionModel == GAIA_XY_Z)
	geo = gaiaFromGeos_XYZ (g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	geo = gaiaFromGeos_XYM (g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	geo = gaiaFromGeos_XYZM (g2);
    else
	geo = gaiaFromGeos_XY (g2);
    GEOSGeom_destroy (g2);
    if (geo == NULL)
	return 0;
    if (geo->FirstPoint)
      {
	  *x = geo->FirstPoint->X;
	  *y = geo->FirstPoint->Y;
	  gaiaFreeGeomColl (geo);
	  return 1;
      }
    gaiaFreeGeomColl (geo);
    return 0;
}

GAIAGEO_DECLARE int
gaiaGetPointOnSurface (gaiaGeomCollPtr geom, double *x, double *y)
{
/* returns a Point guaranteed to lie on the Surface */
    gaiaGeomCollPtr geo;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    if (!geom)
	return 0;
    g1 = gaiaToGeos (geom);
    g2 = GEOSPointOnSurface (g1);
    GEOSGeom_destroy (g1);
    if (!g2)
	return 0;
    if (geom->DimensionModel == GAIA_XY_Z)
	geo = gaiaFromGeos_XYZ (g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	geo = gaiaFromGeos_XYM (g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	geo = gaiaFromGeos_XYZM (g2);
    else
	geo = gaiaFromGeos_XY (g2);
    GEOSGeom_destroy (g2);
    if (geo == NULL)
	return 0;
    if (geo->FirstPoint)
      {
	  *x = geo->FirstPoint->X;
	  *y = geo->FirstPoint->Y;
	  gaiaFreeGeomColl (geo);
	  return 1;
      }
    gaiaFreeGeomColl (geo);
    return 0;
}

GAIAGEO_DECLARE int
gaiaIsSimple (gaiaGeomCollPtr geom)
{
/* checks if this GEOMETRYCOLLECTION is a simple one */
    int ret;
    GEOSGeometry *g;
    if (!geom)
	return -1;
    g = gaiaToGeos (geom);
    ret = GEOSisSimple (g);
    GEOSGeom_destroy (g);
    if (ret == 2)
	return -1;
    return ret;
}

GAIAGEO_DECLARE int
gaiaIsRing (gaiaLinestringPtr line)
{
/* checks if this LINESTRING can be a valid RING */
    gaiaGeomCollPtr geo;
    gaiaLinestringPtr line2;
    int ret;
    int iv;
    double x;
    double y;
    double z;
    double m;
    GEOSGeometry *g;
    if (!line)
	return -1;
    if (line->DimensionModel == GAIA_XY_Z)
	geo = gaiaAllocGeomCollXYZ ();
    else if (line->DimensionModel == GAIA_XY_M)
	geo = gaiaAllocGeomCollXYM ();
    else if (line->DimensionModel == GAIA_XY_Z_M)
	geo = gaiaAllocGeomCollXYZM ();
    else
	geo = gaiaAllocGeomColl ();
    line2 = gaiaAddLinestringToGeomColl (geo, line->Points);
    for (iv = 0; iv < line2->Points; iv++)
      {
	  z = 0.0;
	  m = 0.0;
	  if (line->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (line->Coords, iv, &x, &y, &z);
	    }
	  else if (line->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (line->Coords, iv, &x, &y, &m);
	    }
	  else if (line->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (line->Coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (line->Coords, iv, &x, &y);
	    }
	  if (line2->DimensionModel == GAIA_XY_Z)
	    {
		gaiaSetPointXYZ (line2->Coords, iv, x, y, z);
	    }
	  else if (line2->DimensionModel == GAIA_XY_M)
	    {
		gaiaSetPointXYM (line2->Coords, iv, x, y, m);
	    }
	  else if (line2->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaSetPointXYZM (line2->Coords, iv, x, y, z, m);
	    }
	  else
	    {
		gaiaSetPoint (line2->Coords, iv, x, y);
	    }
      }
    g = gaiaToGeos (geo);
    gaiaFreeGeomColl (geo);
    ret = GEOSisRing (g);
    GEOSGeom_destroy (g);
    if (ret == 2)
	return -1;
    return ret;
}

GAIAGEO_DECLARE int
gaiaIsValid (gaiaGeomCollPtr geom)
{
/* checks if this GEOMETRYCOLLECTION is a valid one */
    int ret;
    GEOSGeometry *g;
    gaiaResetGeosMsg ();
    if (!geom)
	return -1;
    if (gaiaIsToxic (geom))
	return 0;
    g = gaiaToGeos (geom);
    ret = GEOSisValid (g);
    GEOSGeom_destroy (g);
    if (ret == 2)
	return -1;
    return ret;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaGeomCollSimplify (gaiaGeomCollPtr geom, double tolerance)
{
/* builds a simplified geometry using the Douglas-Peuker algorihtm */
    gaiaGeomCollPtr geo;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    if (!geom)
	return NULL;
    g1 = gaiaToGeos (geom);
    g2 = GEOSSimplify (g1, tolerance);
    GEOSGeom_destroy (g1);
    if (!g2)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	geo = gaiaFromGeos_XYZ (g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	geo = gaiaFromGeos_XYM (g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	geo = gaiaFromGeos_XYZM (g2);
    else
	geo = gaiaFromGeos_XY (g2);
    GEOSGeom_destroy (g2);
    if (geo == NULL)
	return NULL;
    geo->Srid = geom->Srid;
    return geo;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaGeomCollSimplifyPreserveTopology (gaiaGeomCollPtr geom, double tolerance)
{
/* builds a simplified geometry using the Douglas-Peuker algorihtm [preserving topology] */
    gaiaGeomCollPtr geo;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    if (!geom)
	return NULL;
    g1 = gaiaToGeos (geom);
    g2 = GEOSTopologyPreserveSimplify (g1, tolerance);
    GEOSGeom_destroy (g1);
    if (!g2)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	geo = gaiaFromGeos_XYZ (g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	geo = gaiaFromGeos_XYM (g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	geo = gaiaFromGeos_XYZM (g2);
    else
	geo = gaiaFromGeos_XY (g2);
    GEOSGeom_destroy (g2);
    if (geo == NULL)
	return NULL;
    geo->Srid = geom->Srid;
    return geo;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaConvexHull (gaiaGeomCollPtr geom)
{
/* builds a geometry that is the convex hull of GEOM */
    gaiaGeomCollPtr geo;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    if (!geom)
	return NULL;
    g1 = gaiaToGeos (geom);
    g2 = GEOSConvexHull (g1);
    GEOSGeom_destroy (g1);
    if (!g2)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	geo = gaiaFromGeos_XYZ (g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	geo = gaiaFromGeos_XYM (g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	geo = gaiaFromGeos_XYZM (g2);
    else
	geo = gaiaFromGeos_XY (g2);
    GEOSGeom_destroy (g2);
    if (geo == NULL)
	return NULL;
    geo->Srid = geom->Srid;
    return geo;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaGeomCollBuffer (gaiaGeomCollPtr geom, double radius, int points)
{
/* builds a geometry that is the GIS buffer of GEOM */
    gaiaGeomCollPtr geo;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    if (!geom)
	return NULL;
    g1 = gaiaToGeos (geom);
    g2 = GEOSBuffer (g1, radius, points);
    GEOSGeom_destroy (g1);
    if (!g2)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	geo = gaiaFromGeos_XYZ (g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	geo = gaiaFromGeos_XYM (g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	geo = gaiaFromGeos_XYZM (g2);
    else
	geo = gaiaFromGeos_XY (g2);
    GEOSGeom_destroy (g2);
    if (geo == NULL)
	return NULL;
    geo->Srid = geom->Srid;
    return geo;
}

static int
polygonize_eval_rings (gaiaRingPtr rng1, gaiaRingPtr rng2)
{
/* checking if two rings are identical */
    int iv1;
    int iv2;
    double x1;
    double y1;
    double z1;
    double m;
    double x2;
    double y2;
    double z2;
    int count = 0;
    if (rng1->Points != rng2->Points)
	return 0;
    if (rng1->DimensionModel != rng2->DimensionModel)
	return 0;
    for (iv1 = 0; iv1 < rng1->Points; iv1++)
      {
	  if (rng1->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (rng1->Coords, iv1, &x1, &y1, &z1);
	    }
	  else if (rng1->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (rng1->Coords, iv1, &x1, &y1, &m);
		z1 = 0.0;
	    }
	  else if (rng1->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (rng1->Coords, iv1, &x1, &y1, &z1, &m);
	    }
	  else
	    {
		gaiaGetPoint (rng1->Coords, iv1, &x1, &y1);
		z1 = 0.0;
	    }
	  for (iv2 = 0; iv2 < rng2->Points; iv2++)
	    {
		if (rng2->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (rng2->Coords, iv2, &x2, &y2, &z2);
		  }
		else if (rng2->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (rng2->Coords, iv2, &x2, &y2, &m);
		      z2 = 0.0;
		  }
		else if (rng2->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (rng2->Coords, iv2, &x2, &y2, &z2, &m);
		  }
		else
		  {
		      gaiaGetPoint (rng2->Coords, iv2, &x2, &y2);
		      z2 = 0.0;
		  }
		if (x1 == x2 && y1 == y2 && z1 == z2)
		  {
		      count++;
		      break;
		  }
	    }
      }
    if (count == rng1->Points)
	return 1;
    return 0;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaPolygonize (gaiaGeomCollPtr geom_org, int force_multipolygon)
{
/* trying to promote a (MULTI)LINESTRING to (MULTI)POLYGON */
    int i;
    int n_geoms = 0;
    gaiaGeomCollPtr result;
    gaiaGeomCollPtr geom;
    gaiaLinestringPtr ln1;
    gaiaLinestringPtr ln2;
    gaiaPointPtr pt;
    gaiaPolygonPtr pg;
    gaiaPolygonPtr *polygons;
    char *valids;
    GEOSGeometry **g_array;
    GEOSGeometry *g2;
    double x;
    double y;
    double z;
    double m;
    int npt;
    int nln;
    int npg;
    int ipg;
    if (!geom_org)
	return NULL;
    ln1 = geom_org->FirstLinestring;
    while (ln1)
      {
	  /* counting how many linestrings are there */
	  n_geoms++;
	  ln1 = ln1->Next;
      }
    g_array = malloc (sizeof (GEOSGeometry *) * n_geoms);
    n_geoms = 0;
    ln1 = geom_org->FirstLinestring;
    while (ln1)
      {
	  /* preparing individual LINESTRINGs */
	  if (geom_org->DimensionModel == GAIA_XY_Z)
	      geom = gaiaAllocGeomCollXYZ ();
	  else if (geom_org->DimensionModel == GAIA_XY_M)
	      geom = gaiaAllocGeomCollXYM ();
	  else if (geom_org->DimensionModel == GAIA_XY_Z_M)
	      geom = gaiaAllocGeomCollXYZM ();
	  else
	      geom = gaiaAllocGeomColl ();
	  ln2 = gaiaAddLinestringToGeomColl (geom, ln1->Points);
	  for (i = 0; i < ln1->Points; i++)
	    {
		z = 0.0;
		m = 0.0;
		if (geom_org->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ln1->Coords, i, &x, &y, &z);
		  }
		else if (geom_org->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ln1->Coords, i, &x, &y, &m);
		  }
		else if (geom_org->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ln1->Coords, i, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (ln1->Coords, i, &x, &y);
		  }
		if (geom->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (ln2->Coords, i, x, y, z);
		  }
		else if (geom->DimensionModel == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (ln2->Coords, i, x, y, m);
		  }
		else if (geom->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (ln2->Coords, i, x, y, z, m);
		  }
		else
		  {
		      gaiaSetPoint (ln2->Coords, i, x, y);
		  }
	    }
	  *(g_array + n_geoms) = gaiaToGeos (geom);

	  /* memory cleanup: Kashif Rasul 14 Jan 2010 */
	  gaiaFreeGeomColl (geom);

	  n_geoms++;
	  ln1 = ln1->Next;
      }
    g2 = GEOSPolygonize ((const GEOSGeometry ** const) g_array, n_geoms);
    for (i = 0; i < n_geoms; i++)
	GEOSGeom_destroy (*(g_array + i));
    free (g_array);
    if (!g2)
	return NULL;
    if (geom_org->DimensionModel == GAIA_XY_Z)
	result = gaiaFromGeos_XYZ (g2);
    else if (geom_org->DimensionModel == GAIA_XY_M)
	result = gaiaFromGeos_XYM (g2);
    else if (geom_org->DimensionModel == GAIA_XY_Z_M)
	result = gaiaFromGeos_XYZM (g2);
    else
	result = gaiaFromGeos_XY (g2);
    GEOSGeom_destroy (g2);
    if (!result)
	return NULL;
    npt = 0;
    pt = result->FirstPoint;
    while (pt)
      {
	  npt++;
	  pt = pt->Next;
      }
    nln = 0;
    ln1 = result->FirstLinestring;
    while (ln1)
      {
	  nln++;
	  ln1 = ln1->Next;
      }
    npg = 0;
    pg = result->FirstPolygon;
    while (pg)
      {
	  npg++;
	  pg = pg->Next;
      }
    if (npt || nln)
      {
	  /* invalid geometry; not a (MULTI)POLYGON */
	  gaiaFreeGeomColl (result);
	  return NULL;
      }
    polygons = malloc (sizeof (gaiaPolygonPtr) * npg);
    valids = malloc (sizeof (char) * npg);
    ipg = 0;
    pg = result->FirstPolygon;
    while (pg)
      {
	  /* identifying any INTERIOR RING corresponding to some EXTERIOR RING */
	  gaiaRingPtr ext_rng = pg->Exterior;
	  gaiaPolygonPtr pg2 = result->FirstPolygon;
	  polygons[ipg] = pg;
	  valids[ipg] = 1;
	  while (pg2)
	    {
		if (pg != pg2)
		  {
		      gaiaRingPtr int_rng;
		      int ib;
		      for (ib = 0; ib < pg2->NumInteriors; ib++)
			{
			    int_rng = pg2->Interiors + ib;
			    if (polygonize_eval_rings (int_rng, ext_rng))
			      {
				  /* marking a POLYGON to be deleted */
				  valids[ipg] = 0;
				  break;
			      }
			}
		      if (valids[ipg] == 0)
			  break;
		  }
		pg2 = pg2->Next;
	    }
	  ipg++;
	  pg = pg->Next;
      }
/* rebuilding the POLYGONs list */
    result->FirstPolygon = NULL;
    result->LastPolygon = NULL;
    for (ipg = 0; ipg < npg; ipg++)
      {
	  if (valids[ipg] == 0)
	      gaiaFreePolygon (polygons[ipg]);
	  else
	    {
		pg = polygons[ipg];
		pg->Next = NULL;
		if (result->FirstPolygon == NULL)
		    result->FirstPolygon = pg;
		if (result->LastPolygon != NULL)
		    result->LastPolygon->Next = pg;
		result->LastPolygon = pg;
	    }
      }
    free (polygons);
    free (valids);
    result->Srid = geom_org->Srid;
    if (npg == 1)
      {
	  if (force_multipolygon)
	      result->DeclaredType = GAIA_MULTIPOLYGON;
	  else
	      result->DeclaredType = GAIA_POLYGON;
      }
    else
	result->DeclaredType = GAIA_MULTIPOLYGON;
    return result;
}

#endif /* end including GEOS */
