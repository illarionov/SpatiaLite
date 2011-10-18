/*

 gg_relations.c -- Gaia spatial relations
    
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

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>

#ifndef OMIT_GEOS		/* including GEOS */
#include <geos_c.h>
#endif

#ifdef SPL_AMALGAMATION		/* spatialite-amalgamation */
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
    if (gaiaIsToxic (geom))
	return 0;
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
    if (gaiaIsToxic (geom))
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

static void
test_interior_ring (gaiaDynamicLinePtr dyn1, gaiaDynamicLinePtr dyn2,
		    int *contains, int *within, int *crosses)
{
/* testing if Ring-1 contains Ring-2 */
    gaiaGeomCollPtr geom1;
    gaiaGeomCollPtr geom2;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    int iv;
    int pts;
    gaiaPointPtr pt;

/* creating the Polygon-1 geometry */
    pts = 0;
    pt = dyn1->First;
    while (pt)
      {
	  pts++;
	  pt = pt->Next;
      }
    geom1 = gaiaAllocGeomColl ();
    pg = gaiaAddPolygonToGeomColl (geom1, pts, 0);
    rng = pg->Exterior;
    iv = 0;
    pt = dyn1->First;
    while (pt)
      {
	  /* EXTERIOR RING */
	  gaiaSetPoint (rng->Coords, iv, pt->X, pt->Y);
	  iv++;
	  pt = pt->Next;
      }

/* creating the Polygon-2 geometry */
    pts = 0;
    pt = dyn2->First;
    while (pt)
      {
	  pts++;
	  pt = pt->Next;
      }
    geom2 = gaiaAllocGeomColl ();
    pg = gaiaAddPolygonToGeomColl (geom2, pts, 0);
    rng = pg->Exterior;
    iv = 0;
    pt = dyn2->First;
    while (pt)
      {
	  /* EXTERIOR RING */
	  gaiaSetPoint (rng->Coords, iv, pt->X, pt->Y);
	  iv++;
	  pt = pt->Next;
      }
    *contains = gaiaGeomCollContains (geom1, geom2);
    *within = gaiaGeomCollWithin (geom1, geom2);
    *crosses = gaiaGeomCollCrosses (geom1, geom2);
    gaiaFreeGeomColl (geom1);
    gaiaFreeGeomColl (geom2);
}

static gaiaDynamicLinePtr
build_dyn_ring (gaiaLinestringPtr ln)
{
/* creating a DynamicLine from a Linestring */
    int iv;
    double x;
    double y;
    double m;
    double z;
    gaiaDynamicLinePtr dyn = gaiaAllocDynamicLine ();
    for (iv = 0; iv < ln->Points; iv++)
      {
	  if (ln->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		gaiaAppendPointZMToDynamicLine (dyn, x, y, z, m);
	    }
	  else if (ln->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		gaiaAppendPointZToDynamicLine (dyn, x, y, z);
	    }
	  else if (ln->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		gaiaAppendPointMToDynamicLine (dyn, x, y, m);
	    }
	  else
	    {
		gaiaGetPoint (ln->Coords, iv, &x, &y);
		gaiaAppendPointToDynamicLine (dyn, x, y);
	    }
      }
    return dyn;
}

static int
is_closed_dyn_ring (gaiaDynamicLinePtr dyn)
{
/* checking if a candidate Ring is already closed */
    gaiaPointPtr pt1;
    gaiaPointPtr pt2;
    if (!dyn)
	return 0;
    pt1 = dyn->First;
    pt2 = dyn->Last;
    if (pt1 == NULL || pt2 == NULL)
	return 0;
    if (pt1 == pt2)
	return 0;
    if (pt1->X == pt2->X && pt1->Y == pt2->Y && pt1->Z == pt2->Z)
	return 1;
    return 0;
}

static int
to_be_appended (gaiaDynamicLinePtr dyn, gaiaLinestringPtr ln)
{
/* checks is the Linestring has to be appended to the DynamicLine */
    gaiaPointPtr pt = dyn->Last;
    int iv = 0;
    double x;
    double y;
    double z;
    double m;
    if (ln->DimensionModel == GAIA_XY_Z_M)
      {
	  gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
      }
    else if (ln->DimensionModel == GAIA_XY_Z)
      {
	  gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
      }
    else if (ln->DimensionModel == GAIA_XY_M)
      {
	  gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
      }
    else
      {
	  gaiaGetPoint (ln->Coords, iv, &x, &y);
      }
    if (ln->DimensionModel == GAIA_XY_Z_M || ln->DimensionModel == GAIA_XY_Z)
      {
	  if (pt->X == x && pt->Y == y && pt->Z == z)
	      return 1;
      }
    else
      {
	  if (pt->X == x && pt->Y == y)
	      return 1;
      }
    return 0;
}

static int
to_be_prepended (gaiaDynamicLinePtr dyn, gaiaLinestringPtr ln)
{
/* checks is the Linestring has to be prepended to the DynamicLine */
    gaiaPointPtr pt = dyn->First;
    int iv = ln->Points - 1;
    double x;
    double y;
    double z;
    double m;
    if (ln->DimensionModel == GAIA_XY_Z_M)
      {
	  gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
      }
    else if (ln->DimensionModel == GAIA_XY_Z)
      {
	  gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
      }
    else if (ln->DimensionModel == GAIA_XY_M)
      {
	  gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
      }
    else
      {
	  gaiaGetPoint (ln->Coords, iv, &x, &y);
      }
    if (ln->DimensionModel == GAIA_XY_Z_M || ln->DimensionModel == GAIA_XY_Z)
      {
	  if (pt->X == x && pt->Y == y && pt->Z == z)
	      return 1;
      }
    else
      {
	  if (pt->X == x && pt->Y == y)
	      return 1;
      }
    return 0;
}

static int
to_be_appended_reverse (gaiaDynamicLinePtr dyn, gaiaLinestringPtr ln)
{
/* checks is the Linestring (reversed) has to be appended to the DynamicLine */
    gaiaPointPtr pt = dyn->Last;
    int iv = ln->Points - 1;
    double x;
    double y;
    double z;
    double m;
    if (ln->DimensionModel == GAIA_XY_Z_M)
      {
	  gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
      }
    else if (ln->DimensionModel == GAIA_XY_Z)
      {
	  gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
      }
    else if (ln->DimensionModel == GAIA_XY_M)
      {
	  gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
      }
    else
      {
	  gaiaGetPoint (ln->Coords, iv, &x, &y);
      }
    if (ln->DimensionModel == GAIA_XY_Z_M || ln->DimensionModel == GAIA_XY_Z)
      {
	  if (pt->X == x && pt->Y == y && pt->Z == z)
	      return 1;
      }
    else
      {
	  if (pt->X == x && pt->Y == y)
	      return 1;
      }
    return 0;
}

static int
to_be_prepended_reverse (gaiaDynamicLinePtr dyn, gaiaLinestringPtr ln)
{
/* checks is the Linestring (reversed) has to be prepended to the DynamicLine */
    gaiaPointPtr pt = dyn->First;
    int iv = 0;
    double x;
    double y;
    double z;
    double m;
    if (ln->DimensionModel == GAIA_XY_Z_M)
      {
	  gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
      }
    else if (ln->DimensionModel == GAIA_XY_Z)
      {
	  gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
      }
    else if (ln->DimensionModel == GAIA_XY_M)
      {
	  gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
      }
    else
      {
	  gaiaGetPoint (ln->Coords, iv, &x, &y);
      }
    if (ln->DimensionModel == GAIA_XY_Z_M || ln->DimensionModel == GAIA_XY_Z)
      {
	  if (pt->X == x && pt->Y == y && pt->Z == z)
	      return 1;
      }
    else
      {
	  if (pt->X == x && pt->Y == y)
	      return 1;
      }
    return 0;
}

static void
append_to_ring (gaiaDynamicLinePtr dyn, gaiaLinestringPtr ln, int reversed)
{
/* appending a Linestring to a DynamicRing */
    int iv;
    double x;
    double y;
    double z;
    double m;
    if (reversed)
      {
	  for (iv = ln->Points - 2; iv >= 0; iv--)
	    {
		if (ln->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		      gaiaAppendPointZMToDynamicLine (dyn, x, y, z, m);
		  }
		else if (ln->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		      gaiaAppendPointZToDynamicLine (dyn, x, y, z);
		  }
		else if (ln->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		      gaiaAppendPointMToDynamicLine (dyn, x, y, m);
		  }
		else
		  {
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		      gaiaAppendPointToDynamicLine (dyn, x, y);
		  }
	    }
      }
    else
      {
	  for (iv = 1; iv < ln->Points; iv++)
	    {
		if (ln->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		      gaiaAppendPointZMToDynamicLine (dyn, x, y, z, m);
		  }
		else if (ln->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		      gaiaAppendPointZToDynamicLine (dyn, x, y, z);
		  }
		else if (ln->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		      gaiaAppendPointMToDynamicLine (dyn, x, y, m);
		  }
		else
		  {
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		      gaiaAppendPointToDynamicLine (dyn, x, y);
		  }
	    }
      }
}

static void
prepend_to_ring (gaiaDynamicLinePtr dyn, gaiaLinestringPtr ln, int reversed)
{
/* appending a Linestring to a DynamicRing */
    int iv;
    double x;
    double y;
    double z;
    double m;
    if (reversed)
      {
	  for (iv = 1; iv < ln->Points; iv++)
	    {
		if (ln->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		      gaiaPrependPointZMToDynamicLine (dyn, x, y, z, m);
		  }
		else if (ln->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		      gaiaPrependPointZToDynamicLine (dyn, x, y, z);
		  }
		else if (ln->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		      gaiaPrependPointMToDynamicLine (dyn, x, y, m);
		  }
		else
		  {
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		      gaiaPrependPointToDynamicLine (dyn, x, y);
		  }
	    }
      }
    else
      {
	  for (iv = ln->Points - 2; iv >= 0; iv--)
	    {
		if (ln->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		      gaiaPrependPointZMToDynamicLine (dyn, x, y, z, m);
		  }
		else if (ln->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		      gaiaPrependPointZToDynamicLine (dyn, x, y, z);
		  }
		else if (ln->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		      gaiaPrependPointMToDynamicLine (dyn, x, y, m);
		  }
		else
		  {
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		      gaiaPrependPointToDynamicLine (dyn, x, y);
		  }
	    }
      }
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaPolygonize (gaiaGeomCollPtr geom, int force_multi)
{
/* attempts to rearrange a generic Geometry into a (multi)polygon */
    int pts = 0;
    int lns = 0;
    int pgs = 0;
    gaiaGeomCollPtr result;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    int dummy;
    int ok;
    int ok2;
    int i;
    int i2;
    int iv;
    int ib;
    double x;
    double y;
    double m;
    double z;
    double x0;
    double y0;
    double z0;
    int contains;
    int within;
    int crosses;
    int num_interiors;
    gaiaLinestringPtr *ln_array = NULL;
    gaiaDynamicLinePtr *dyn_array = NULL;
    gaiaDynamicLinePtr *ext_array = NULL;
    gaiaDynamicLinePtr dyn;
    gaiaDynamicLinePtr dyn2;

    if (!geom)
	return NULL;
    pt = geom->FirstPoint;
    while (pt)
      {
	  pts++;
	  pt = pt->Next;
      }
    pg = geom->FirstPolygon;
    while (pg)
      {
	  pgs++;
	  pg = pg->Next;
      }
    if (pts || pgs)
	return NULL;
    ln = geom->FirstLinestring;
    while (ln)
      {
	  lns++;
	  ln = ln->Next;
      }
    if (!lns)
	return NULL;
/* allocating and initializing aux-arrays */
    ln_array = malloc (sizeof (gaiaLinestringPtr) * lns);
    dyn_array = malloc (sizeof (gaiaDynamicLinePtr) * lns);
    ext_array = malloc (sizeof (gaiaDynamicLinePtr) * lns);
    i = 0;
    ln = geom->FirstLinestring;
    while (ln)
      {
	  ln_array[i] = ln;
	  dyn_array[i] = NULL;
	  ext_array[i] = NULL;
	  i++;
	  ln = ln->Next;
      }

    for (i = 0; i < lns; i++)
      {
	  /* processing closed rings */
	  ln = ln_array[i];
	  iv = ln->Points - 1;
	  if (ln->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (ln->Coords, 0, &x0, &y0, &z0, &m);
		gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		if (x0 == x && y0 == y && z0 == z)
		  {
		      dyn_array[i] = build_dyn_ring (ln);
		      ln_array[i] = NULL;
		  }
	    }
	  else if (ln->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (ln->Coords, 0, &x0, &y0, &z0);
		gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		if (x0 == x && y0 == y && z0 == z)
		  {
		      dyn_array[i] = build_dyn_ring (ln);
		      ln_array[i] = NULL;
		  }
	    }
	  else if (ln->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (ln->Coords, 0, &x0, &y0, &m);
		gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		if (x0 == x && y0 == y)
		  {
		      dyn_array[i] = build_dyn_ring (ln);
		      ln_array[i] = NULL;
		  }
	    }
	  else
	    {
		gaiaGetPoint (ln->Coords, 0, &x0, &y0);
		gaiaGetPoint (ln->Coords, iv, &x, &y);
		if (x0 == x && y0 == y)
		  {
		      dyn_array[i] = build_dyn_ring (ln);
		      ln_array[i] = NULL;
		  }
	    }
      }

    ok = 1;
    while (ok)
      {
	  if (dummy == 0)
	      ok = dummy;	/* simply suppressing stupid compiler warnings */
	  ok = 0;
	  for (i = 0; i < lns; i++)
	    {
		/* attempting to create rings */
		ln = ln_array[i];
		if (ln == NULL)
		    continue;
		ok = 1;
		dyn_array[i] = build_dyn_ring (ln);
		ln_array[i] = NULL;
		dyn = dyn_array[i];
		ok2 = 1;
		while (ok2)
		  {
		      ok2 = 0;
		      for (i2 = 0; i2 < lns; i2++)
			{
			    if (is_closed_dyn_ring (dyn) == 1)
				goto ring_done;
			    ln = ln_array[i2];
			    if (ln == NULL)
				continue;
			    if (to_be_appended (dyn, ln) == 1)
			      {
				  append_to_ring (dyn, ln, 0);
				  ln_array[i2] = NULL;
				  ok2 = 1;
				  break;
			      }
			    if (to_be_prepended (dyn, ln) == 1)
			      {
				  prepend_to_ring (dyn, ln, 0);
				  ln_array[i2] = NULL;
				  ok2 = 1;
				  break;
			      }
			    if (to_be_appended_reverse (dyn, ln) == 1)
			      {
				  append_to_ring (dyn, ln, 1);
				  ln_array[i2] = NULL;
				  ok2 = 1;
				  break;
			      }
			    if (to_be_prepended_reverse (dyn, ln) == 1)
			      {
				  prepend_to_ring (dyn, ln, 1);
				  ln_array[i2] = NULL;
				  ok2 = 1;
				  break;
			      }
			}
		  }
	    }
	ring_done:
	  dummy = 0;
      }

    ok = 1;
    for (i = 0; i < lns; i++)
      {
	  /* checking if any ring is closed */
	  dyn = dyn_array[i];
	  if (dyn == NULL)
	      continue;
	  if (is_closed_dyn_ring (dyn) == 0)
	      ok = 0;
      }
    if (ok == 0)
      {
	  /* invalid: quitting */
	  for (i = 0; i < lns; i++)
	    {
		dyn = dyn_array[i];
		if (dyn == NULL)
		    continue;
		gaiaFreeDynamicLine (dyn);
	    }
	  free (dyn_array);
	  free (ext_array);
	  free (ln_array);
	  return NULL;
      }

    ok = 1;
    for (i = 0; i < lns; i++)
      {
	  /* testing interior/exterior relationships */
	  dyn = dyn_array[i];
	  if (dyn == NULL)
	      continue;
	  for (i2 = i + 1; i2 < lns; i2++)
	    {
		/* testing interior/exterior relationships */
		dyn2 = dyn_array[i2];
		if (dyn2 == NULL)
		    continue;
		test_interior_ring (dyn, dyn2, &contains, &within, &crosses);
		if (contains)
		    ext_array[i2] = dyn;
		if (within)
		    ext_array[i] = dyn2;
		if (crosses)
		    ok = 0;
	    }
      }
    if (ok == 0)
      {
	  /* invalid: quitting */
	  for (i = 0; i < lns; i++)
	    {
		dyn = dyn_array[i];
		if (dyn == NULL)
		    continue;
		gaiaFreeDynamicLine (dyn);
	    }
	  free (dyn_array);
	  free (ext_array);
	  free (ln_array);
	  return NULL;
      }

    if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaAllocGeomCollXYZM ();
    else if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaAllocGeomCollXYZ ();
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaAllocGeomCollXYM ();
    else
	result = gaiaAllocGeomColl ();
    result->Srid = geom->Srid;
    if (force_multi)
	result->DeclaredType = GAIA_MULTIPOLYGON;

    for (i = 0; i < lns; i++)
      {
	  /* creating Polygons */
	  dyn = dyn_array[i];
	  if (dyn == NULL)
	      continue;
	  if (ext_array[i] != NULL)
	    {
		/* skipping any INTERIOR RING */
		continue;
	    }
	  pts = 0;
	  pt = dyn->First;
	  while (pt)
	    {
		/* counting how many points are there */
		pts++;
		pt = pt->Next;
	    }
	  num_interiors = 0;
	  for (i2 = 0; i2 < lns; i2++)
	    {
		if (ext_array[i2] == dyn)
		    num_interiors++;
	    }
	  pg = gaiaAddPolygonToGeomColl (result, pts, num_interiors);
	  rng = pg->Exterior;
	  iv = 0;
	  pt = dyn->First;
	  while (pt)
	    {
		/* EXTERIOR RING */
		if (result->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (rng->Coords, iv, pt->X, pt->Y, pt->Z,
					pt->M);
		  }
		else if (result->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (rng->Coords, iv, pt->X, pt->Y, pt->Z);
		  }
		else if (result->DimensionModel == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (rng->Coords, iv, pt->X, pt->Y, pt->M);
		  }
		else
		  {
		      gaiaSetPoint (rng->Coords, iv, pt->X, pt->Y);
		  }
		iv++;
		pt = pt->Next;
	    }
	  ib = 0;
	  for (i2 = 0; i2 < lns; i2++)
	    {
		/* inserting any INTERIOR RING */
		if (ext_array[i2] == dyn)
		  {
		      dyn2 = dyn_array[i2];
		      ok = 1;
		      pts = 0;
		      pt = dyn2->First;
		      while (pt)
			{
			    /* counting how many points are there */
			    pts++;
			    pt = pt->Next;
			}
		      rng = gaiaAddInteriorRing (pg, ib, pts);
		      ib++;
		      iv = 0;
		      pt = dyn2->First;
		      while (pt)
			{
			    /* INTERIOR RING */
			    if (result->DimensionModel == GAIA_XY_Z_M)
			      {
				  gaiaSetPointXYZM (rng->Coords, iv, pt->X,
						    pt->Y, pt->Z, pt->M);
			      }
			    else if (result->DimensionModel == GAIA_XY_Z)
			      {
				  gaiaSetPointXYZ (rng->Coords, iv, pt->X,
						   pt->Y, pt->Z);
			      }
			    else if (result->DimensionModel == GAIA_XY_M)
			      {
				  gaiaSetPointXYM (rng->Coords, iv, pt->X,
						   pt->Y, pt->M);
			      }
			    else
			      {
				  gaiaSetPoint (rng->Coords, iv, pt->X, pt->Y);
			      }
			    iv++;
			    pt = pt->Next;
			}
		  }
	    }
      }

/* memory cleanup */
    for (i = 0; i < lns; i++)
      {
	  dyn = dyn_array[i];
	  if (dyn == NULL)
	      continue;
	  gaiaFreeDynamicLine (dyn);
      }
    free (dyn_array);
    free (ext_array);
    free (ln_array);
    if (result->FirstPolygon == NULL)
      {
	  gaiaFreeGeomColl (result);
	  return NULL;
      }
    return result;
}

#ifdef GEOS_ADVANCED		/* GEOS advanced and experimental features */

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaOffsetCurve (gaiaGeomCollPtr geom, double radius, int points,
		 int left_right)
{
/*
// builds a geometry that is the OffsetCurve of GEOM 
// (which is expected to be of the LINESTRING type)
//
*/
    gaiaGeomCollPtr geo;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    int pts = 0;
    int lns = 0;
    int pgs = 0;
    int closed = 0;
    if (!geom)
	return NULL;

/* checking the input geometry for validity */
    pt = geom->FirstPoint;
    while (pt)
      {
	  /* counting how many POINTs are there */
	  pts++;
	  pt = pt->Next;
      }
    ln = geom->FirstLinestring;
    while (ln)
      {
	  /* counting how many LINESTRINGs are there */
	  lns++;
	  if (gaiaIsClosed (ln))
	      closed++;
	  ln = ln->Next;
      }
    pg = geom->FirstPolygon;
    while (pg)
      {
	  /* counting how many POLYGON are there */
	  pgs++;
	  pg = pg->Next;
      }
    if (pts > 0 || pgs > 0 || lns > 1 || closed > 0)
	return NULL;

/* all right: this one simply is a LINESTRING */
    geom->DeclaredType = GAIA_LINESTRING;

    g1 = gaiaToGeos (geom);
    g2 = GEOSSingleSidedBuffer (g1, radius, points, GEOSBUF_JOIN_ROUND, 5.0,
				left_right);
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
gaiaSingleSidedBuffer (gaiaGeomCollPtr geom, double radius, int points,
		       int left_right)
{
/*
// builds a geometry that is the SingleSided BUFFER of GEOM 
// (which is expected to be of the LINESTRING type)
//
*/
    gaiaGeomCollPtr geo;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    GEOSBufferParams *params = NULL;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    int pts = 0;
    int lns = 0;
    int pgs = 0;
    int closed = 0;
    if (!geom)
	return NULL;

/* checking the input geometry for validity */
    pt = geom->FirstPoint;
    while (pt)
      {
	  /* counting how many POINTs are there */
	  pts++;
	  pt = pt->Next;
      }
    ln = geom->FirstLinestring;
    while (ln)
      {
	  /* counting how many LINESTRINGs are there */
	  lns++;
	  if (gaiaIsClosed (ln))
	      closed++;
	  ln = ln->Next;
      }
    pg = geom->FirstPolygon;
    while (pg)
      {
	  /* counting how many POLYGON are there */
	  pgs++;
	  pg = pg->Next;
      }
    if (pts > 0 || pgs > 0 || lns > 1 || closed > 0)
	return NULL;

/* all right: this one simply is a LINESTRING */
    geom->DeclaredType = GAIA_LINESTRING;

    g1 = gaiaToGeos (geom);
/* setting up Buffer params */
    params = GEOSBufferParams_create ();
    GEOSBufferParams_setJoinStyle (params, GEOSBUF_JOIN_ROUND);
    GEOSBufferParams_setMitreLimit (params, 5.0);
    GEOSBufferParams_setQuadrantSegments (params, points);
    GEOSBufferParams_setSingleSided (params, 1);

/* creating the SingleSided Buffer */
    if (left_right == 0)
      {
	  /* right-sided requires NEGATIVE radius */
	  radius *= -1.0;
      }
    g2 = GEOSBufferWithParams (g1, params, radius);
    GEOSGeom_destroy (g1);
    GEOSBufferParams_destroy (params);
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
gaiaHausdorffDistance (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2,
		       double *xdist)
{
/* 
/ computes the (discrete) Hausdorff distance intercurring 
/ between GEOM-1 and GEOM-2 
*/
    double dist;
    int ret;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    if (!geom1 || !geom2)
	return 0;
    g1 = gaiaToGeos (geom1);
    g2 = gaiaToGeos (geom2);
    ret = GEOSHausdorffDistance (g1, g2, &dist);
    GEOSGeom_destroy (g1);
    GEOSGeom_destroy (g2);
    if (ret)
	*xdist = dist;
    return ret;
}

static gaiaGeomCollPtr
geom_as_lines (gaiaGeomCollPtr geom)
{
/* transforms a Geometry into a LINESTRING/MULTILINESTRING (if possible) */
    gaiaGeomCollPtr result;
    gaiaLinestringPtr ln;
    gaiaLinestringPtr new_ln;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    int iv;
    int ib;
    double x;
    double y;
    double z;
    double m;

    if (!geom)
	return NULL;
    if (geom->FirstPoint != NULL)
      {
	  /* invalid: GEOM contains at least one POINT */
	  return NULL;
      }

    switch (geom->DimensionModel)
      {
      case GAIA_XY_Z_M:
	  result = gaiaAllocGeomCollXYZM ();
	  break;
      case GAIA_XY_Z:
	  result = gaiaAllocGeomCollXYZ ();
	  break;
      case GAIA_XY_M:
	  result = gaiaAllocGeomCollXYM ();
	  break;
      default:
	  result = gaiaAllocGeomColl ();
	  break;
      };
    result->Srid = geom->Srid;
    ln = geom->FirstLinestring;
    while (ln)
      {
	  /* copying any Linestring */
	  new_ln = gaiaAddLinestringToGeomColl (result, ln->Points);
	  for (iv = 0; iv < ln->Points; iv++)
	    {
		if (ln->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		      gaiaSetPointXYZ (new_ln->Coords, iv, x, y, z);
		  }
		else if (ln->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		      gaiaSetPointXYM (new_ln->Coords, iv, x, y, m);
		  }
		else if (ln->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		      gaiaSetPointXYZM (new_ln->Coords, iv, x, y, z, m);
		  }
		else
		  {
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		      gaiaSetPoint (new_ln->Coords, iv, x, y);
		  }
	    }
	  ln = ln->Next;
      }
    pg = geom->FirstPolygon;
    while (pg)
      {
	  /* copying any Polygon Ring (as Linestring) */
	  rng = pg->Exterior;
	  new_ln = gaiaAddLinestringToGeomColl (result, rng->Points);
	  for (iv = 0; iv < rng->Points; iv++)
	    {
		/* exterior Ring */
		if (rng->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
		      gaiaSetPointXYZ (new_ln->Coords, iv, x, y, z);
		  }
		else if (rng->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
		      gaiaSetPointXYM (new_ln->Coords, iv, x, y, m);
		  }
		else if (rng->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
		      gaiaSetPointXYZM (new_ln->Coords, iv, x, y, z, m);
		  }
		else
		  {
		      gaiaGetPoint (rng->Coords, iv, &x, &y);
		      gaiaSetPoint (new_ln->Coords, iv, x, y);
		  }
	    }
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		rng = pg->Interiors + ib;
		new_ln = gaiaAddLinestringToGeomColl (result, rng->Points);
		for (iv = 0; iv < rng->Points; iv++)
		  {
		      /* any interior Ring */
		      if (rng->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
			    gaiaSetPointXYZ (new_ln->Coords, iv, x, y, z);
			}
		      else if (rng->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
			    gaiaSetPointXYM (new_ln->Coords, iv, x, y, m);
			}
		      else if (rng->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
			    gaiaSetPointXYZM (new_ln->Coords, iv, x, y, z, m);
			}
		      else
			{
			    gaiaGetPoint (rng->Coords, iv, &x, &y);
			    gaiaSetPoint (new_ln->Coords, iv, x, y);
			}
		  }
	    }
	  pg = pg->Next;
      }
    return result;
}

static void
add_shared_linestring (gaiaGeomCollPtr geom, gaiaDynamicLinePtr dyn)
{
/* adding a LINESTRING from Dynamic Line */
    int count = 0;
    gaiaLinestringPtr ln;
    gaiaPointPtr pt;
    int iv;

    if (!geom)
	return;
    if (!dyn)
	return;
    pt = dyn->First;
    while (pt)
      {
	  /* counting how many Points are there */
	  count++;
	  pt = pt->Next;
      }
    if (count == 0)
	return;
    ln = gaiaAddLinestringToGeomColl (geom, count);
    iv = 0;
    pt = dyn->First;
    while (pt)
      {
	  /* copying points into the LINESTRING */
	  if (ln->DimensionModel == GAIA_XY_Z)
	    {
		gaiaSetPointXYZ (ln->Coords, iv, pt->X, pt->Y, pt->Z);
	    }
	  else if (ln->DimensionModel == GAIA_XY_M)
	    {
		gaiaSetPointXYM (ln->Coords, iv, pt->X, pt->Y, pt->M);
	    }
	  else if (ln->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaSetPointXYZM (ln->Coords, iv, pt->X, pt->Y, pt->Z, pt->M);
	    }
	  else
	    {
		gaiaSetPoint (ln->Coords, iv, pt->X, pt->Y);
	    }
	  iv++;
	  pt = pt->Next;
      }
}

static void
append_shared_path (gaiaDynamicLinePtr dyn, gaiaLinestringPtr ln, int order)
{
/* appends a Shared Path item to Dynamic Line */
    int iv;
    double x;
    double y;
    double z;
    double m;
    if (order)
      {
	  /* reversed order */
	  for (iv = ln->Points - 1; iv >= 0; iv--)
	    {
		if (ln->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		      if (x == dyn->Last->X && y == dyn->Last->Y
			  && z == dyn->Last->Z)
			  ;
		      else
			  gaiaAppendPointZToDynamicLine (dyn, x, y, z);
		  }
		else if (ln->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		      if (x == dyn->Last->X && y == dyn->Last->Y
			  && m == dyn->Last->M)
			  ;
		      else
			  gaiaAppendPointMToDynamicLine (dyn, x, y, m);
		  }
		else if (ln->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		      if (x == dyn->Last->X && y == dyn->Last->Y
			  && z == dyn->Last->Z && m == dyn->Last->M)
			  ;
		      else
			  gaiaAppendPointZMToDynamicLine (dyn, x, y, z, m);
		  }
		else
		  {
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		      if (x == dyn->Last->X && y == dyn->Last->Y)
			  ;
		      else
			  gaiaAppendPointToDynamicLine (dyn, x, y);
		  }
	    }
      }
    else
      {
	  /* conformant order */
	  for (iv = 0; iv < ln->Points; iv++)
	    {
		if (ln->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		      if (x == dyn->Last->X && y == dyn->Last->Y
			  && z == dyn->Last->Z)
			  ;
		      else
			  gaiaAppendPointZToDynamicLine (dyn, x, y, z);
		  }
		else if (ln->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		      if (x == dyn->Last->X && y == dyn->Last->Y
			  && m == dyn->Last->M)
			  ;
		      else
			  gaiaAppendPointMToDynamicLine (dyn, x, y, m);
		  }
		else if (ln->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		      if (x == dyn->Last->X && y == dyn->Last->Y
			  && z == dyn->Last->Z && m == dyn->Last->M)
			  ;
		      else
			  gaiaAppendPointZMToDynamicLine (dyn, x, y, z, m);
		  }
		else
		  {
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		      if (x == dyn->Last->X && y == dyn->Last->Y)
			  ;
		      else
			  gaiaAppendPointToDynamicLine (dyn, x, y);
		  }
	    }
      }
}

static void
prepend_shared_path (gaiaDynamicLinePtr dyn, gaiaLinestringPtr ln, int order)
{
/* prepends a Shared Path item to Dynamic Line */
    int iv;
    double x;
    double y;
    double z;
    double m;
    if (order)
      {
	  /* reversed order */
	  for (iv = 0; iv < ln->Points; iv++)
	    {
		if (ln->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		      if (x == dyn->First->X && y == dyn->First->Y
			  && z == dyn->First->Z)
			  ;
		      else
			  gaiaPrependPointZToDynamicLine (dyn, x, y, z);
		  }
		else if (ln->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		      if (x == dyn->First->X && y == dyn->First->Y
			  && m == dyn->First->M)
			  ;
		      else
			  gaiaPrependPointMToDynamicLine (dyn, x, y, m);
		  }
		else if (ln->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		      if (x == dyn->First->X && y == dyn->First->Y
			  && z == dyn->First->Z && m == dyn->First->M)
			  ;
		      else
			  gaiaPrependPointZMToDynamicLine (dyn, x, y, z, m);
		  }
		else
		  {
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		      if (x == dyn->First->X && y == dyn->First->Y)
			  ;
		      else
			  gaiaPrependPointToDynamicLine (dyn, x, y);
		  }
	    }
      }
    else
      {
	  /* conformant order */
	  for (iv = ln->Points - 1; iv >= 0; iv--)
	    {
		if (ln->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		      if (x == dyn->First->X && y == dyn->First->Y
			  && z == dyn->First->Z)
			  ;
		      else
			  gaiaPrependPointZToDynamicLine (dyn, x, y, z);
		  }
		else if (ln->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		      if (x == dyn->First->X && y == dyn->First->Y
			  && m == dyn->First->M)
			  ;
		      else
			  gaiaPrependPointMToDynamicLine (dyn, x, y, m);
		  }
		else if (ln->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		      if (x == dyn->First->X && y == dyn->First->Y
			  && z == dyn->First->Z && m == dyn->First->M)
			  ;
		      else
			  gaiaPrependPointZMToDynamicLine (dyn, x, y, z, m);
		  }
		else
		  {
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		      if (x == dyn->First->X && y == dyn->First->Y)
			  ;
		      else
			  gaiaPrependPointToDynamicLine (dyn, x, y);
		  }
	    }
      }
}

static gaiaGeomCollPtr
arrange_shared_paths (gaiaGeomCollPtr geom)
{
/* final aggregation step for shared paths */
    gaiaLinestringPtr ln;
    gaiaLinestringPtr *ln_array;
    gaiaGeomCollPtr result;
    gaiaDynamicLinePtr dyn;
    int count;
    int i;
    int i2;
    int iv;
    double x;
    double y;
    double z;
    double m;
    int ok;
    int ok2;

    if (!geom)
	return NULL;
    count = 0;
    ln = geom->FirstLinestring;
    while (ln)
      {
	  /* counting how many Linestrings are there */
	  count++;
	  ln = ln->Next;
      }
    if (count == 0)
	return NULL;

    ln_array = malloc (sizeof (gaiaLinestringPtr) * count);
    i = 0;
    ln = geom->FirstLinestring;
    while (ln)
      {
	  /* populating the Linestring references array */
	  ln_array[i++] = ln;
	  ln = ln->Next;
      }

/* allocating a new Geometry [MULTILINESTRING] */
    switch (geom->DimensionModel)
      {
      case GAIA_XY_Z_M:
	  result = gaiaAllocGeomCollXYZM ();
	  break;
      case GAIA_XY_Z:
	  result = gaiaAllocGeomCollXYZ ();
	  break;
      case GAIA_XY_M:
	  result = gaiaAllocGeomCollXYM ();
	  break;
      default:
	  result = gaiaAllocGeomColl ();
	  break;
      };
    result->Srid = geom->Srid;
    result->DeclaredType = GAIA_MULTILINESTRING;

    ok = 1;
    while (ok)
      {
	  /* looping until we have processed any input item */
	  ok = 0;
	  for (i = 0; i < count; i++)
	    {
		if (ln_array[i] != NULL)
		  {
		      /* starting a new LINESTRING */
		      dyn = gaiaAllocDynamicLine ();
		      ln = ln_array[i];
		      ln_array[i] = NULL;
		      for (iv = 0; iv < ln->Points; iv++)
			{
			    /* inserting the 'seed' path */
			    if (ln->DimensionModel == GAIA_XY_Z)
			      {
				  gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
				  gaiaAppendPointZToDynamicLine (dyn, x, y, z);
			      }
			    else if (ln->DimensionModel == GAIA_XY_M)
			      {
				  gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
				  gaiaAppendPointMToDynamicLine (dyn, x, y, m);
			      }
			    else if (ln->DimensionModel == GAIA_XY_Z_M)
			      {
				  gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z,
						    &m);
				  gaiaAppendPointZMToDynamicLine (dyn, x, y, z,
								  m);
			      }
			    else
			      {
				  gaiaGetPoint (ln->Coords, iv, &x, &y);
				  gaiaAppendPointToDynamicLine (dyn, x, y);
			      }
			}
		      ok2 = 1;
		      while (ok2)
			{
			    /* looping until we have checked any other item */
			    ok2 = 0;
			    for (i2 = 0; i2 < count; i2++)
			      {
				  /* expanding the 'seed' path */
				  if (ln_array[i2] == NULL)
				      continue;
				  ln = ln_array[i2];
				  /* checking the first vertex */
				  iv = 0;
				  if (ln->DimensionModel == GAIA_XY_Z)
				    {
					gaiaGetPointXYZ (ln->Coords, iv, &x, &y,
							 &z);
				    }
				  else if (ln->DimensionModel == GAIA_XY_M)
				    {
					gaiaGetPointXYM (ln->Coords, iv, &x, &y,
							 &m);
				    }
				  else if (ln->DimensionModel == GAIA_XY_Z_M)
				    {
					gaiaGetPointXYZM (ln->Coords, iv, &x,
							  &y, &z, &m);
				    }
				  else
				    {
					gaiaGetPoint (ln->Coords, iv, &x, &y);
				    }
				  if (x == dyn->Last->X && y == dyn->Last->Y)
				    {
					/* appending this item to the 'seed' (conformant order) */
					append_shared_path (dyn, ln, 0);
					ln_array[i2] = NULL;
					ok2 = 1;
					continue;
				    }
				  if (x == dyn->First->X && y == dyn->First->Y)
				    {
					/* prepending this item to the 'seed' (reversed order) */
					prepend_shared_path (dyn, ln, 1);
					ln_array[i2] = NULL;
					ok2 = 1;
					continue;
				    }
				  /* checking the last vertex */
				  iv = ln->Points - 1;
				  if (ln->DimensionModel == GAIA_XY_Z)
				    {
					gaiaGetPointXYZ (ln->Coords, iv, &x, &y,
							 &z);
				    }
				  else if (ln->DimensionModel == GAIA_XY_M)
				    {
					gaiaGetPointXYM (ln->Coords, iv, &x, &y,
							 &m);
				    }
				  else if (ln->DimensionModel == GAIA_XY_Z_M)
				    {
					gaiaGetPointXYZM (ln->Coords, iv, &x,
							  &y, &z, &m);
				    }
				  else
				    {
					gaiaGetPoint (ln->Coords, iv, &x, &y);
				    }
				  if (x == dyn->Last->X && y == dyn->Last->Y)
				    {
					/* appending this item to the 'seed' (reversed order) */
					append_shared_path (dyn, ln, 1);
					ln_array[i2] = NULL;
					ok2 = 1;
					continue;
				    }
				  if (x == dyn->First->X && y == dyn->First->Y)
				    {
					/* prepending this item to the 'seed' (conformant order) */
					prepend_shared_path (dyn, ln, 0);
					ln_array[i2] = NULL;
					ok2 = 1;
					continue;
				    }
			      }
			}
		      add_shared_linestring (result, dyn);
		      gaiaFreeDynamicLine (dyn);
		      ok = 1;
		      break;
		  }
	    }
      }
    free (ln_array);
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaSharedPaths (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2)
{
/*
// builds a geometry containing Shared Paths commons to GEOM1 & GEOM2 
// (which are expected to be of the LINESTRING/MULTILINESTRING type)
//
*/
    gaiaGeomCollPtr geo;
    gaiaGeomCollPtr result;
    gaiaGeomCollPtr line1;
    gaiaGeomCollPtr line2;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    GEOSGeometry *g3;
    if (!geom1)
	return NULL;
    if (!geom2)
	return NULL;
/* transforming input geoms as Lines */
    line1 = geom_as_lines (geom1);
    line2 = geom_as_lines (geom2);
    if (line1 == NULL || line2 == NULL)
      {
	  if (line1)
	      gaiaFreeGeomColl (line1);
	  if (line2)
	      gaiaFreeGeomColl (line2);
	  return NULL;
      }

    g1 = gaiaToGeos (line1);
    g2 = gaiaToGeos (line2);
    gaiaFreeGeomColl (line1);
    gaiaFreeGeomColl (line2);
    g3 = GEOSSharedPaths (g1, g2);
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
    result = arrange_shared_paths (geo);
    gaiaFreeGeomColl (geo);
    return result;
}

GAIAGEO_DECLARE int
gaiaGeomCollCovers (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2)
{
/* checks if geom1 "spatially covers" geom2 */
    int ret;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    if (!geom1 || !geom2)
	return -1;
    g1 = gaiaToGeos (geom1);
    g2 = gaiaToGeos (geom2);
    ret = GEOSCovers (g1, g2);
    GEOSGeom_destroy (g1);
    GEOSGeom_destroy (g2);
    return ret;
}

GAIAGEO_DECLARE int
gaiaGeomCollCoveredBy (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2)
{
/* checks if geom1 is "spatially covered by" geom2 */
    int ret;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    if (!geom1 || !geom2)
	return -1;
    g1 = gaiaToGeos (geom1);
    g2 = gaiaToGeos (geom2);
    ret = GEOSCoveredBy (g1, g2);
    GEOSGeom_destroy (g1);
    GEOSGeom_destroy (g2);
    return ret;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaLineInterpolatePoint (gaiaGeomCollPtr geom, double fraction)
{
/*
 * attempts to intepolate a point on line at dist "fraction" 
 *
 * the fraction is expressed into the range from 0.0 to 1.0
 */
    int pts = 0;
    int lns = 0;
    int pgs = 0;
    gaiaGeomCollPtr result;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    GEOSGeometry *g;
    GEOSGeometry *g_pt;
    double length;
    double projection;
    if (!geom)
	return NULL;

/* checking if a single Linestring has been passed */
    pt = geom->FirstPoint;
    while (pt)
      {
	  pts++;
	  pt = pt->Next;
      }
    ln = geom->FirstLinestring;
    while (ln)
      {
	  lns++;
	  ln = ln->Next;
      }
    pg = geom->FirstPolygon;
    while (pg)
      {
	  pgs++;
	  pg = pg->Next;
      }
    if (pts == 0 && lns == 1 && pgs == 0)
	;
    else
	return NULL;

    g = gaiaToGeos (geom);
    if (GEOSLength (g, &length))
      {
	  /* transforming fraction to length */
	  if (fraction < 0.0)
	      fraction = 0.0;
	  if (fraction > 1.0)
	      fraction = 1.0;
	  projection = length * fraction;
      }
    else
      {
	  GEOSGeom_destroy (g);
	  return NULL;
      }
    g_pt = GEOSInterpolate (g, projection);
    GEOSGeom_destroy (g);
    if (!g_pt)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaFromGeos_XYZ (g_pt);
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaFromGeos_XYM (g_pt);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaFromGeos_XYZM (g_pt);
    else
	result = gaiaFromGeos_XY (g_pt);
    GEOSGeom_destroy (g_pt);
    if (result == NULL)
	return NULL;
    result->Srid = geom->Srid;
    return result;
}

GAIAGEO_DECLARE double
gaiaLineLocatePoint (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2)
{
/* 
 * attempts to compute the location of the closest point on LineString 
 * to the given Point, as a fraction of total 2d line length 
 *
 * the fraction is expressed into the range from 0.0 to 1.0
 */
    int pts1 = 0;
    int lns1 = 0;
    int pgs1 = 0;
    int pts2 = 0;
    int lns2 = 0;
    int pgs2 = 0;
    double length;
    double projection;
    double result;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    if (!geom1 || !geom2)
	return -1.0;

/* checking if a single Linestring has been passed */
    pt = geom1->FirstPoint;
    while (pt)
      {
	  pts1++;
	  pt = pt->Next;
      }
    ln = geom1->FirstLinestring;
    while (ln)
      {
	  lns1++;
	  ln = ln->Next;
      }
    pg = geom1->FirstPolygon;
    while (pg)
      {
	  pgs1++;
	  pg = pg->Next;
      }
    if (pts1 == 0 && lns1 >= 1 && pgs1 == 0)
	;
    else
	return -1.0;

/* checking if a single Point has been passed */
    pt = geom2->FirstPoint;
    while (pt)
      {
	  pts2++;
	  pt = pt->Next;
      }
    ln = geom2->FirstLinestring;
    while (ln)
      {
	  lns2++;
	  ln = ln->Next;
      }
    pg = geom2->FirstPolygon;
    while (pg)
      {
	  pgs2++;
	  pg = pg->Next;
      }
    if (pts2 == 1 && lns2 == 0 && pgs2 == 0)
	;
    else
	return -1.0;

    g1 = gaiaToGeos (geom1);
    g2 = gaiaToGeos (geom2);
    projection = GEOSProject (g1, g2);
    if (GEOSLength (g1, &length))
      {
	  /* normalizing as a fraction between 0.0 and 1.0 */
	  result = projection / length;
      }
    else
	result = -1.0;
    GEOSGeom_destroy (g1);
    GEOSGeom_destroy (g2);
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaLineSubstring (gaiaGeomCollPtr geom, double start_fraction,
		   double end_fraction)
{
/* 
 * attempts to build a new Linestring being a substring of the input one starting 
 * and ending at the given fractions of total 2d length 
 */
    int pts = 0;
    int lns = 0;
    int pgs = 0;
    gaiaGeomCollPtr result;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaLinestringPtr out;
    gaiaPolygonPtr pg;
    GEOSGeometry *g;
    GEOSGeometry *g_start;
    GEOSGeometry *g_end;
    GEOSCoordSequence *cs;
    const GEOSCoordSequence *in_cs;
    GEOSGeometry *segm;
    double length;
    double total = 0.0;
    double start;
    double end;
    int iv;
    int i_start = -1;
    int i_end = -1;
    int points;
    double x;
    double y;
    double z;
    double m;
    unsigned int dims;
    if (!geom)
	return NULL;

/* checking if a single Linestring has been passed */
    pt = geom->FirstPoint;
    while (pt)
      {
	  pts++;
	  pt = pt->Next;
      }
    ln = geom->FirstLinestring;
    while (ln)
      {
	  lns++;
	  ln = ln->Next;
      }
    pg = geom->FirstPolygon;
    while (pg)
      {
	  pgs++;
	  pg = pg->Next;
      }
    if (pts == 0 && lns == 1 && pgs == 0)
	;
    else
	return NULL;

    if (start_fraction < 0.0)
	start_fraction = 0.0;
    if (start_fraction > 1.0)
	start_fraction = 1.0;
    if (end_fraction < 0.0)
	end_fraction = 0.0;
    if (end_fraction > 1.0)
	end_fraction = 1.0;
    if (start_fraction >= end_fraction)
	return NULL;
    g = gaiaToGeos (geom);
    if (GEOSLength (g, &length))
      {
	  start = length * start_fraction;
	  end = length * end_fraction;
      }
    else
      {
	  GEOSGeom_destroy (g);
	  return NULL;
      }
    g_start = GEOSInterpolate (g, start);
    g_end = GEOSInterpolate (g, end);
    GEOSGeom_destroy (g);
    if (!g_start || !g_end)
	return NULL;

/* identifying first and last valid vertex */
    ln = geom->FirstLinestring;
    for (iv = 0; iv < ln->Points; iv++)
      {

	  double x0;
	  double y0;
	  switch (ln->DimensionModel)
	    {
	    case GAIA_XY_Z:
		gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		break;
	    case GAIA_XY_M:
		gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		break;
	    case GAIA_XY_Z_M:
		gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		break;
	    default:
		gaiaGetPoint (ln->Coords, iv, &x, &y);
		break;
	    };

	  if (iv > 0)
	    {
		cs = GEOSCoordSeq_create (2, 2);
		GEOSCoordSeq_setX (cs, 0, x0);
		GEOSCoordSeq_setY (cs, 0, y0);
		GEOSCoordSeq_setX (cs, 1, x);
		GEOSCoordSeq_setY (cs, 1, y);
		segm = GEOSGeom_createLineString (cs);
		GEOSLength (segm, &length);
		total += length;
		GEOSGeom_destroy (segm);
		if (total > start && i_start < 0)
		    i_start = iv;
		if (total < end)
		    i_end = iv;
	    }
	  x0 = x;
	  y0 = y;
      }
    if (i_start < 0 || i_end < 0)
      {
	  i_start = -1;
	  i_end = -1;
	  points = 2;
      }
    else
	points = i_end - i_start + 3;

/* creating the output geometry */
    switch (ln->DimensionModel)
      {
      case GAIA_XY_Z:
	  result = gaiaAllocGeomCollXYZ ();
	  break;
      case GAIA_XY_M:
	  result = gaiaAllocGeomCollXYM ();
	  break;
      case GAIA_XY_Z_M:
	  result = gaiaAllocGeomCollXYZM ();
	  break;
      default:
	  result = gaiaAllocGeomColl ();
	  break;
      };
    result->Srid = geom->Srid;
    out = gaiaAddLinestringToGeomColl (result, points);

/* start vertex */
    points = 0;
    in_cs = GEOSGeom_getCoordSeq (g_start);
    GEOSCoordSeq_getDimensions (in_cs, &dims);
    if (dims == 3)
      {
	  GEOSCoordSeq_getX (in_cs, 0, &x);
	  GEOSCoordSeq_getY (in_cs, 0, &y);
	  GEOSCoordSeq_getZ (in_cs, 0, &z);
	  m = 0.0;
      }
    else
      {
	  GEOSCoordSeq_getX (in_cs, 0, &x);
	  GEOSCoordSeq_getY (in_cs, 0, &y);
	  z = 0.0;
	  m = 0.0;
      }
    GEOSGeom_destroy (g_start);
    switch (out->DimensionModel)
      {
      case GAIA_XY_Z:
	  gaiaSetPointXYZ (out->Coords, points, x, y, z);
	  break;
      case GAIA_XY_M:
	  gaiaSetPointXYM (out->Coords, points, x, y, 0.0);
	  break;
      case GAIA_XY_Z_M:
	  gaiaSetPointXYZM (out->Coords, points, x, y, z, 0.0);
	  break;
      default:
	  gaiaSetPoint (out->Coords, points, x, y);
	  break;
      };
    points++;

    if (i_start < 0 || i_end < 0)
	;
    else
      {
	  for (iv = i_start; iv <= i_end; iv++)
	    {
		z = 0.0;
		m = 0.0;
		switch (ln->DimensionModel)
		  {
		  case GAIA_XY_Z:
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		      break;
		  case GAIA_XY_M:
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		      break;
		  case GAIA_XY_Z_M:
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		      break;
		  default:
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		      break;
		  };
		switch (out->DimensionModel)
		  {
		  case GAIA_XY_Z:
		      gaiaSetPointXYZ (out->Coords, points, x, y, z);
		      break;
		  case GAIA_XY_M:
		      gaiaSetPointXYM (out->Coords, points, x, y, 0.0);
		      break;
		  case GAIA_XY_Z_M:
		      gaiaSetPointXYZM (out->Coords, points, x, y, z, 0.0);
		      break;
		  default:
		      gaiaSetPoint (out->Coords, points, x, y);
		      break;
		  };
		points++;
	    }
      }

/* end vertex */
    in_cs = GEOSGeom_getCoordSeq (g_end);
    GEOSCoordSeq_getDimensions (in_cs, &dims);
    if (dims == 3)
      {
	  GEOSCoordSeq_getX (in_cs, 0, &x);
	  GEOSCoordSeq_getY (in_cs, 0, &y);
	  GEOSCoordSeq_getZ (in_cs, 0, &z);
	  m = 0.0;
      }
    else
      {
	  GEOSCoordSeq_getX (in_cs, 0, &x);
	  GEOSCoordSeq_getY (in_cs, 0, &y);
	  z = 0.0;
	  m = 0.0;
      }
    GEOSGeom_destroy (g_end);
    switch (out->DimensionModel)
      {
      case GAIA_XY_Z:
	  gaiaSetPointXYZ (out->Coords, points, x, y, z);
	  break;
      case GAIA_XY_M:
	  gaiaSetPointXYM (out->Coords, points, x, y, 0.0);
	  break;
      case GAIA_XY_Z_M:
	  gaiaSetPointXYZM (out->Coords, points, x, y, z, 0.0);
	  break;
      default:
	  gaiaSetPoint (out->Coords, points, x, y);
	  break;
      };
    return result;
}

static GEOSGeometry *
buildGeosPoints (const gaiaGeomCollPtr gaia)
{
/* converting a GAIA Geometry into a GEOS Geometry of POINTS */
    int pts = 0;
    unsigned int dims;
    int iv;
    int ib;
    int nItem;
    double x;
    double y;
    double z;
    double m;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    GEOSGeometry *geos;
    GEOSGeometry *geos_item;
    GEOSGeometry **geos_coll;
    GEOSCoordSequence *cs;
    if (!gaia)
	return NULL;
    pt = gaia->FirstPoint;
    while (pt)
      {
	  /* counting how many POINTs are there */
	  pts++;
	  pt = pt->Next;
      }
    ln = gaia->FirstLinestring;
    while (ln)
      {
	  /* counting how many POINTs are there */
	  pts += ln->Points;
	  ln = ln->Next;
      }
    pg = gaia->FirstPolygon;
    while (pg)
      {
	  /* counting how many POINTs are there */
	  rng = pg->Exterior;
	  pts += rng->Points - 1;	/* exterior ring */
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		/* interior ring */
		rng = pg->Interiors + ib;
		pts += rng->Points - 1;
	    }
	  pg = pg->Next;
      }
    if (pts == 0)
	return NULL;
    switch (gaia->DimensionModel)
      {
      case GAIA_XY_Z:
      case GAIA_XY_Z_M:
	  dims = 3;
	  break;
      default:
	  dims = 2;
	  break;
      };
    nItem = 0;
    geos_coll = malloc (sizeof (GEOSGeometry *) * (pts));
    pt = gaia->FirstPoint;
    while (pt)
      {
	  cs = GEOSCoordSeq_create (1, dims);
	  switch (pt->DimensionModel)
	    {
	    case GAIA_XY_Z:
	    case GAIA_XY_Z_M:
		GEOSCoordSeq_setX (cs, 0, pt->X);
		GEOSCoordSeq_setY (cs, 0, pt->Y);
		GEOSCoordSeq_setZ (cs, 0, pt->Z);
		break;
	    default:
		GEOSCoordSeq_setX (cs, 0, pt->X);
		GEOSCoordSeq_setY (cs, 0, pt->Y);
		break;
	    };
	  geos_item = GEOSGeom_createPoint (cs);
	  *(geos_coll + nItem++) = geos_item;
	  pt = pt->Next;
      }
    ln = gaia->FirstLinestring;
    while (ln)
      {
	  for (iv = 0; iv < ln->Points; iv++)
	    {
		z = 0.0;
		m = 0.0;
		switch (ln->DimensionModel)
		  {
		  case GAIA_XY_Z:
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		      break;
		  case GAIA_XY_M:
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		      break;
		  case GAIA_XY_Z_M:
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		      break;
		  default:
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		      break;
		  };
		cs = GEOSCoordSeq_create (1, dims);
		if (dims == 3)
		  {
		      GEOSCoordSeq_setX (cs, 0, x);
		      GEOSCoordSeq_setY (cs, 0, y);
		      GEOSCoordSeq_setZ (cs, 0, z);
		  }
		else
		  {
		      GEOSCoordSeq_setX (cs, 0, x);
		      GEOSCoordSeq_setY (cs, 0, y);
		  }
		geos_item = GEOSGeom_createPoint (cs);
		*(geos_coll + nItem++) = geos_item;
	    }
	  ln = ln->Next;
      }
    pg = gaia->FirstPolygon;
    while (pg)
      {
	  rng = pg->Exterior;
	  for (iv = 1; iv < rng->Points; iv++)
	    {
		/* exterior ring */
		z = 0.0;
		m = 0.0;
		switch (rng->DimensionModel)
		  {
		  case GAIA_XY_Z:
		      gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
		      break;
		  case GAIA_XY_M:
		      gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
		      break;
		  case GAIA_XY_Z_M:
		      gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
		      break;
		  default:
		      gaiaGetPoint (rng->Coords, iv, &x, &y);
		      break;
		  };
		cs = GEOSCoordSeq_create (1, dims);
		if (dims == 3)
		  {
		      GEOSCoordSeq_setX (cs, 0, x);
		      GEOSCoordSeq_setY (cs, 0, y);
		      GEOSCoordSeq_setZ (cs, 0, z);
		  }
		else
		  {
		      GEOSCoordSeq_setX (cs, 0, x);
		      GEOSCoordSeq_setY (cs, 0, y);
		  }
		geos_item = GEOSGeom_createPoint (cs);
		*(geos_coll + nItem++) = geos_item;
	    }
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		/* interior ring */
		rng = pg->Interiors + ib;
		for (iv = 1; iv < rng->Points; iv++)
		  {
		      /* exterior ring */
		      z = 0.0;
		      m = 0.0;
		      switch (rng->DimensionModel)
			{
			case GAIA_XY_Z:
			    gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
			    break;
			case GAIA_XY_M:
			    gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
			    break;
			case GAIA_XY_Z_M:
			    gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
			    break;
			default:
			    gaiaGetPoint (rng->Coords, iv, &x, &y);
			    break;
			};
		      cs = GEOSCoordSeq_create (1, dims);
		      if (dims == 3)
			{
			    GEOSCoordSeq_setX (cs, 0, x);
			    GEOSCoordSeq_setY (cs, 0, y);
			    GEOSCoordSeq_setZ (cs, 0, z);
			}
		      else
			{
			    GEOSCoordSeq_setX (cs, 0, x);
			    GEOSCoordSeq_setY (cs, 0, y);
			}
		      geos_item = GEOSGeom_createPoint (cs);
		      *(geos_coll + nItem++) = geos_item;
		  }
	    }
	  pg = pg->Next;
      }
    geos = GEOSGeom_createCollection (GEOS_MULTIPOINT, geos_coll, pts);
    free (geos_coll);
    GEOSSetSRID (geos, gaia->Srid);
    return geos;
}

static GEOSGeometry *
buildGeosSegments (const gaiaGeomCollPtr gaia)
{
/* converting a GAIA Geometry into a GEOS Geometry of SEGMENTS */
    int segms = 0;
    unsigned int dims;
    int iv;
    int ib;
    int nItem;
    double x;
    double y;
    double z;
    double m;
    double x0;
    double y0;
    double z0;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    GEOSGeometry *geos;
    GEOSGeometry *geos_item;
    GEOSGeometry **geos_coll;
    GEOSCoordSequence *cs;
    if (!gaia)
	return NULL;
    ln = gaia->FirstLinestring;
    while (ln)
      {
	  /* counting how many SEGMENTs are there */
	  segms += ln->Points - 1;
	  ln = ln->Next;
      }
    pg = gaia->FirstPolygon;
    while (pg)
      {
	  /* counting how many SEGMENTs are there */
	  rng = pg->Exterior;
	  segms += rng->Points - 1;	/* exterior ring */
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		/* interior ring */
		rng = pg->Interiors + ib;
		segms += rng->Points - 1;
	    }
	  pg = pg->Next;
      }
    if (segms == 0)
	return NULL;
    switch (gaia->DimensionModel)
      {
      case GAIA_XY_Z:
      case GAIA_XY_Z_M:
	  dims = 3;
	  break;
      default:
	  dims = 2;
	  break;
      };
    nItem = 0;
    geos_coll = malloc (sizeof (GEOSGeometry *) * (segms));
    ln = gaia->FirstLinestring;
    while (ln)
      {
	  for (iv = 0; iv < ln->Points; iv++)
	    {
		z = 0.0;
		m = 0.0;
		switch (ln->DimensionModel)
		  {
		  case GAIA_XY_Z:
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		      break;
		  case GAIA_XY_M:
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		      break;
		  case GAIA_XY_Z_M:
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		      break;
		  default:
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		      break;
		  };
		if (iv > 0)
		  {
		      cs = GEOSCoordSeq_create (2, dims);
		      if (dims == 3)
			{
			    GEOSCoordSeq_setX (cs, 0, x0);
			    GEOSCoordSeq_setY (cs, 0, y0);
			    GEOSCoordSeq_setZ (cs, 0, z0);
			    GEOSCoordSeq_setX (cs, 1, x);
			    GEOSCoordSeq_setY (cs, 1, y);
			    GEOSCoordSeq_setZ (cs, 1, z);
			}
		      else
			{
			    GEOSCoordSeq_setX (cs, 0, x0);
			    GEOSCoordSeq_setY (cs, 0, y0);
			    GEOSCoordSeq_setX (cs, 1, x);
			    GEOSCoordSeq_setY (cs, 1, y);
			}
		      geos_item = GEOSGeom_createLineString (cs);
		      *(geos_coll + nItem++) = geos_item;
		  }
		x0 = x;
		y0 = y;
		z0 = z;
	    }
	  ln = ln->Next;
      }
    pg = gaia->FirstPolygon;
    while (pg)
      {
	  rng = pg->Exterior;
	  for (iv = 0; iv < rng->Points; iv++)
	    {
		/* exterior ring */
		z = 0.0;
		m = 0.0;
		switch (rng->DimensionModel)
		  {
		  case GAIA_XY_Z:
		      gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
		      break;
		  case GAIA_XY_M:
		      gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
		      break;
		  case GAIA_XY_Z_M:
		      gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
		      break;
		  default:
		      gaiaGetPoint (rng->Coords, iv, &x, &y);
		      break;
		  };
		if (iv > 0)
		  {
		      cs = GEOSCoordSeq_create (2, dims);
		      if (dims == 3)
			{
			    GEOSCoordSeq_setX (cs, 0, x0);
			    GEOSCoordSeq_setY (cs, 0, y0);
			    GEOSCoordSeq_setZ (cs, 0, z0);
			    GEOSCoordSeq_setX (cs, 1, x);
			    GEOSCoordSeq_setY (cs, 1, y);
			    GEOSCoordSeq_setZ (cs, 1, z);
			}
		      else
			{
			    GEOSCoordSeq_setX (cs, 0, x0);
			    GEOSCoordSeq_setY (cs, 0, y0);
			    GEOSCoordSeq_setX (cs, 1, x);
			    GEOSCoordSeq_setY (cs, 1, y);
			}
		      geos_item = GEOSGeom_createLineString (cs);
		      *(geos_coll + nItem++) = geos_item;
		  }
		x0 = x;
		y0 = y;
		z0 = z;
	    }
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		/* interior ring */
		rng = pg->Interiors + ib;
		for (iv = 0; iv < rng->Points; iv++)
		  {
		      /* exterior ring */
		      z = 0.0;
		      m = 0.0;
		      switch (rng->DimensionModel)
			{
			case GAIA_XY_Z:
			    gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
			    break;
			case GAIA_XY_M:
			    gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
			    break;
			case GAIA_XY_Z_M:
			    gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
			    break;
			default:
			    gaiaGetPoint (rng->Coords, iv, &x, &y);
			    break;
			};
		      if (iv > 0)
			{
			    cs = GEOSCoordSeq_create (2, dims);
			    if (dims == 3)
			      {
				  GEOSCoordSeq_setX (cs, 0, x0);
				  GEOSCoordSeq_setY (cs, 0, y0);
				  GEOSCoordSeq_setZ (cs, 0, z0);
				  GEOSCoordSeq_setX (cs, 1, x);
				  GEOSCoordSeq_setY (cs, 1, y);
				  GEOSCoordSeq_setZ (cs, 1, z);
			      }
			    else
			      {
				  GEOSCoordSeq_setX (cs, 0, x0);
				  GEOSCoordSeq_setY (cs, 0, y0);
				  GEOSCoordSeq_setX (cs, 1, x);
				  GEOSCoordSeq_setY (cs, 1, y);
			      }
			    geos_item = GEOSGeom_createLineString (cs);
			    *(geos_coll + nItem++) = geos_item;
			}
		      x0 = x;
		      y0 = y;
		      z0 = z;
		  }
	    }
	  pg = pg->Next;
      }
    geos = GEOSGeom_createCollection (GEOS_MULTILINESTRING, geos_coll, segms);
    free (geos_coll);
    GEOSSetSRID (geos, gaia->Srid);
    return geos;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaShortestLine (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2)
{
/* attempts to compute the the shortest line between two geometries */
    GEOSGeometry *g1_points;
    GEOSGeometry *g1_segments;
    const GEOSGeometry *g1_item;
    GEOSGeometry *g2_points;
    GEOSGeometry *g2_segments;
    const GEOSGeometry *g2_item;
    const GEOSCoordSequence *cs;
    GEOSGeometry *g_pt;
    gaiaGeomCollPtr result;
    gaiaLinestringPtr ln;
    int nItems1;
    int nItems2;
    int it1;
    int it2;
    unsigned int dims;
    double x_ini;
    double y_ini;
    double z_ini;
    double x_fin;
    double y_fin;
    double z_fin;
    double dist;
    double min_dist = DBL_MAX;
    double projection;
    if (!geom1 || !geom2)
	return NULL;

    g1_points = buildGeosPoints (geom1);
    g1_segments = buildGeosSegments (geom1);
    g2_points = buildGeosPoints (geom2);
    g2_segments = buildGeosSegments (geom2);

    if (g1_points && g2_points)
      {
	  /* computing distances between POINTs */
	  nItems1 = GEOSGetNumGeometries (g1_points);
	  nItems2 = GEOSGetNumGeometries (g2_points);
	  for (it1 = 0; it1 < nItems1; it1++)
	    {
		g1_item = GEOSGetGeometryN (g1_points, it1);
		for (it2 = 0; it2 < nItems2; it2++)
		  {
		      g2_item = GEOSGetGeometryN (g2_points, it2);
		      if (GEOSDistance (g1_item, g2_item, &dist))
			{
			    if (dist < min_dist)
			      {
				  /* saving min-dist points */
				  min_dist = dist;
				  cs = GEOSGeom_getCoordSeq (g1_item);
				  GEOSCoordSeq_getDimensions (cs, &dims);
				  if (dims == 3)
				    {
					GEOSCoordSeq_getX (cs, 0, &x_ini);
					GEOSCoordSeq_getY (cs, 0, &y_ini);
					GEOSCoordSeq_getZ (cs, 0, &z_ini);
				    }
				  else
				    {
					GEOSCoordSeq_getX (cs, 0, &x_ini);
					GEOSCoordSeq_getY (cs, 0, &y_ini);
					z_ini = 0.0;
				    }
				  cs = GEOSGeom_getCoordSeq (g2_item);
				  GEOSCoordSeq_getDimensions (cs, &dims);
				  if (dims == 3)
				    {
					GEOSCoordSeq_getX (cs, 0, &x_fin);
					GEOSCoordSeq_getY (cs, 0, &y_fin);
					GEOSCoordSeq_getZ (cs, 0, &z_fin);
				    }
				  else
				    {
					GEOSCoordSeq_getX (cs, 0, &x_fin);
					GEOSCoordSeq_getY (cs, 0, &y_fin);
					z_fin = 0.0;
				    }
			      }
			}
		  }
	    }
      }

    if (g1_points && g2_segments)
      {
	  /* computing distances between POINTs (g1) and SEGMENTs (g2) */
	  nItems1 = GEOSGetNumGeometries (g1_points);
	  nItems2 = GEOSGetNumGeometries (g2_segments);
	  for (it1 = 0; it1 < nItems1; it1++)
	    {
		g1_item = GEOSGetGeometryN (g1_points, it1);
		for (it2 = 0; it2 < nItems2; it2++)
		  {
		      g2_item = GEOSGetGeometryN (g2_segments, it2);
		      if (GEOSDistance (g1_item, g2_item, &dist))
			{
			    if (dist < min_dist)
			      {
				  /* saving min-dist points */
				  projection = GEOSProject (g2_item, g1_item);
				  g_pt = GEOSInterpolate (g2_item, projection);
				  if (g_pt)
				    {
					min_dist = dist;
					cs = GEOSGeom_getCoordSeq (g1_item);
					GEOSCoordSeq_getDimensions (cs, &dims);
					if (dims == 3)
					  {
					      GEOSCoordSeq_getX (cs, 0, &x_ini);
					      GEOSCoordSeq_getY (cs, 0, &y_ini);
					      GEOSCoordSeq_getZ (cs, 0, &z_ini);
					  }
					else
					  {
					      GEOSCoordSeq_getX (cs, 0, &x_ini);
					      GEOSCoordSeq_getY (cs, 0, &y_ini);
					      z_ini = 0.0;
					  }
					cs = GEOSGeom_getCoordSeq (g_pt);
					GEOSCoordSeq_getDimensions (cs, &dims);
					if (dims == 3)
					  {
					      GEOSCoordSeq_getX (cs, 0, &x_fin);
					      GEOSCoordSeq_getY (cs, 0, &y_fin);
					      GEOSCoordSeq_getZ (cs, 0, &z_fin);
					  }
					else
					  {
					      GEOSCoordSeq_getX (cs, 0, &x_fin);
					      GEOSCoordSeq_getY (cs, 0, &y_fin);
					      z_fin = 0.0;
					  }
					GEOSGeom_destroy (g_pt);
				    }
			      }
			}
		  }
	    }
      }

    if (g1_segments && g2_points)
      {
	  /* computing distances between SEGMENTs (g1) and POINTs (g2) */
	  nItems1 = GEOSGetNumGeometries (g1_segments);
	  nItems2 = GEOSGetNumGeometries (g2_points);
	  for (it1 = 0; it1 < nItems1; it1++)
	    {
		g1_item = GEOSGetGeometryN (g1_segments, it1);
		for (it2 = 0; it2 < nItems2; it2++)
		  {
		      g2_item = GEOSGetGeometryN (g2_points, it2);
		      if (GEOSDistance (g1_item, g2_item, &dist))
			{
			    if (dist < min_dist)
			      {
				  /* saving min-dist points */
				  projection = GEOSProject (g1_item, g2_item);
				  g_pt = GEOSInterpolate (g1_item, projection);
				  if (g_pt)
				    {
					min_dist = dist;
					cs = GEOSGeom_getCoordSeq (g_pt);
					GEOSCoordSeq_getDimensions (cs, &dims);
					if (dims == 3)
					  {
					      GEOSCoordSeq_getX (cs, 0, &x_ini);
					      GEOSCoordSeq_getY (cs, 0, &y_ini);
					      GEOSCoordSeq_getZ (cs, 0, &z_ini);
					  }
					else
					  {
					      GEOSCoordSeq_getX (cs, 0, &x_ini);
					      GEOSCoordSeq_getY (cs, 0, &y_ini);
					      z_ini = 0.0;
					  }
					cs = GEOSGeom_getCoordSeq (g2_item);
					GEOSCoordSeq_getDimensions (cs, &dims);
					if (dims == 3)
					  {
					      GEOSCoordSeq_getX (cs, 0, &x_fin);
					      GEOSCoordSeq_getY (cs, 0, &y_fin);
					      GEOSCoordSeq_getZ (cs, 0, &z_fin);
					  }
					else
					  {
					      GEOSCoordSeq_getX (cs, 0, &x_fin);
					      GEOSCoordSeq_getY (cs, 0, &y_fin);
					      z_fin = 0.0;
					  }
					GEOSGeom_destroy (g_pt);
				    }
			      }
			}
		  }
	    }
      }
    if (g1_points)
	GEOSGeom_destroy (g1_points);
    if (g1_segments)
	GEOSGeom_destroy (g1_segments);
    if (g2_points)
	GEOSGeom_destroy (g2_points);
    if (g2_segments)
	GEOSGeom_destroy (g2_segments);
    if (min_dist == DBL_MAX || min_dist <= 0.0)
	return NULL;

/* building the shortest line */
    switch (geom1->DimensionModel)
      {
      case GAIA_XY_Z:
	  result = gaiaAllocGeomCollXYZ ();
	  break;
      case GAIA_XY_M:
	  result = gaiaAllocGeomCollXYM ();
	  break;
      case GAIA_XY_Z_M:
	  result = gaiaAllocGeomCollXYZM ();
	  break;
      default:
	  result = gaiaAllocGeomColl ();
	  break;
      };
    result->Srid = geom1->Srid;
    ln = gaiaAddLinestringToGeomColl (result, 2);
    switch (ln->DimensionModel)
      {
      case GAIA_XY_Z:
	  gaiaSetPointXYZ (ln->Coords, 0, x_ini, y_ini, z_ini);
	  gaiaSetPointXYZ (ln->Coords, 1, x_fin, y_fin, z_fin);
	  break;
      case GAIA_XY_M:
	  gaiaSetPointXYM (ln->Coords, 0, x_ini, y_ini, 0.0);
	  gaiaSetPointXYM (ln->Coords, 1, x_fin, y_fin, 0.0);
	  break;
      case GAIA_XY_Z_M:
	  gaiaSetPointXYZM (ln->Coords, 0, x_ini, y_ini, z_ini, 0.0);
	  gaiaSetPointXYZM (ln->Coords, 1, x_fin, y_fin, z_fin, 0.0);
	  break;
      default:
	  gaiaSetPoint (ln->Coords, 0, x_ini, y_ini);
	  gaiaSetPoint (ln->Coords, 1, x_fin, y_fin);
	  break;
      };
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaSnap (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2, double tolerance)
{
/* attempts to "snap" geom1 on geom2 using the given tolerance */
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    GEOSGeometry *g3;
    gaiaGeomCollPtr result;
    if (!geom1 || !geom2)
	return NULL;

    g1 = gaiaToGeos (geom1);
    g2 = gaiaToGeos (geom2);
    g3 = GEOSSnap (g1, g2, tolerance);
    GEOSGeom_destroy (g1);
    GEOSGeom_destroy (g2);
    if (!g3)
	return NULL;
    if (geom1->DimensionModel == GAIA_XY_Z)
	result = gaiaFromGeos_XYZ (g3);
    else if (geom1->DimensionModel == GAIA_XY_M)
	result = gaiaFromGeos_XYM (g3);
    else if (geom1->DimensionModel == GAIA_XY_Z_M)
	result = gaiaFromGeos_XYZM (g3);
    else
	result = gaiaFromGeos_XY (g3);
    GEOSGeom_destroy (g3);
    if (result == NULL)
	return NULL;
    result->Srid = geom1->Srid;
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaLineMerge (gaiaGeomCollPtr geom)
{
/* attempts to reassemble lines from a collection of sparse fragments */
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    gaiaGeomCollPtr result;
    if (!geom)
	return NULL;
    if (gaiaIsToxic (geom))
	return NULL;

    g1 = gaiaToGeos (geom);
    g2 = GEOSLineMerge (g1);
    GEOSGeom_destroy (g1);
    if (!g2)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaFromGeos_XYZ (g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaFromGeos_XYM (g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaFromGeos_XYZM (g2);
    else
	result = gaiaFromGeos_XY (g2);
    GEOSGeom_destroy (g2);
    if (result == NULL)
	return NULL;
    result->Srid = geom->Srid;
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaUnaryUnion (gaiaGeomCollPtr geom)
{
/* Unary Union (single Collection) */
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    gaiaGeomCollPtr result;
    if (!geom)
	return NULL;
    if (gaiaIsToxic (geom))
	return NULL;
    g1 = gaiaToGeos (geom);
    g2 = GEOSUnaryUnion (g1);
    GEOSGeom_destroy (g1);
    if (!g2)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaFromGeos_XYZ (g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaFromGeos_XYM (g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaFromGeos_XYZM (g2);
    else
	result = gaiaFromGeos_XY (g2);
    GEOSGeom_destroy (g2);
    if (result == NULL)
	return NULL;
    result->Srid = geom->Srid;
    return result;
}

#endif /* end GEOS advanced and experimental features */

#endif /* end including GEOS */
