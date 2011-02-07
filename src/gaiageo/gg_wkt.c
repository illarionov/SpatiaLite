/*

 gg_wkt.c -- Gaia common support for WKT encoded geometries
  
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
Klaus Foerster klaus.foerster@svg.cc

The Vanuatu Team - University of Toronto - Supervisor:
Greg Wilson	gvwilson@cs.toronto.ca

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

#ifdef SPL_AMALGAMATION	/* spatialite-amalgamation */
#include <spatialite/sqlite3ext.h>
#else
#include <sqlite3ext.h>
#endif

#include <spatialite/gaiageo.h>

static int
checkValidity (gaiaGeomCollPtr geom)
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

static void
gaiaOutClean (char *buffer)
{
/* cleans unneeded trailing zeros */
    int i;
    for (i = strlen (buffer) - 1; i > 0; i--)
      {
	  if (buffer[i] == '0')
	      buffer[i] = '\0';
	  else
	      break;
      }
    if (buffer[i] == '.')
	buffer[i] = '\0';
}

GAIAGEO_DECLARE void
gaiaOutBufferInitialize (gaiaOutBufferPtr buf)
{
/* initializing a dynamically growing output buffer */
    buf->Buffer = NULL;
    buf->WriteOffset = 0;
    buf->BufferSize = 0;
    buf->Error = 0;
}

GAIAGEO_DECLARE void
gaiaOutBufferReset (gaiaOutBufferPtr buf)
{
/* cleaning a dynamically growing output buffer */
    if (buf->Buffer)
	free (buf->Buffer);
    buf->Buffer = NULL;
    buf->WriteOffset = 0;
    buf->BufferSize = 0;
    buf->Error = 0;
}

GAIAGEO_DECLARE void
gaiaAppendToOutBuffer (gaiaOutBufferPtr buf, const char *text)
{
/* appending a text string */
    int len = strlen (text);
    int free_size = buf->BufferSize - buf->WriteOffset;
    if ((len + 1) > free_size)
      {
	  /* we must allocate a bigger buffer */
	  int new_size;
	  char *new_buf;
	  if (buf->BufferSize == 0)
	      new_size = 1024;
	  else if (buf->BufferSize <= 4196)
	      new_size = buf->BufferSize + (len + 1) + 4196;
	  else if (buf->BufferSize <= 65536)
	      new_size = buf->BufferSize + (len + 1) + 65536;
	  else
	      new_size = buf->BufferSize + (len + 1) + (1024 * 1024);
	  new_buf = malloc (new_size);
	  if (!new_buf)
	    {
		buf->Error = 1;
		return;
	    }
	  memcpy (new_buf, buf->Buffer, buf->WriteOffset);
	  if (buf->Buffer)
	      free (buf->Buffer);
	  buf->Buffer = new_buf;
	  buf->BufferSize = new_size;
      }
    strcpy (buf->Buffer + buf->WriteOffset, text);
    buf->WriteOffset += len;
}

static void
gaiaOutPoint (gaiaOutBufferPtr out_buf, gaiaPointPtr point)
{
/* formats a WKT POINT */
    char buf_x[128];
    char buf_y[128];
    char buf[256];
    sprintf (buf_x, "%1.6f", point->X);
    gaiaOutClean (buf_x);
    sprintf (buf_y, "%1.6f", point->Y);
    gaiaOutClean (buf_y);
    sprintf (buf, "%s %s", buf_x, buf_y);
    gaiaAppendToOutBuffer (out_buf, buf);
}

static void
gaiaOutPointZ (gaiaOutBufferPtr out_buf, gaiaPointPtr point)
{
/* formats a WKT POINTZ */
    char buf_x[128];
    char buf_y[128];
    char buf_z[128];
    char buf[512];
    sprintf (buf_x, "%1.6f", point->X);
    gaiaOutClean (buf_x);
    sprintf (buf_y, "%1.6f", point->Y);
    gaiaOutClean (buf_y);
    sprintf (buf_z, "%1.6f", point->Z);
    gaiaOutClean (buf_z);
    sprintf (buf, "%s %s %s", buf_x, buf_y, buf_z);
    gaiaAppendToOutBuffer (out_buf, buf);
}

static void
gaiaOutPointM (gaiaOutBufferPtr out_buf, gaiaPointPtr point)
{
/* formats a WKT POINTM */
    char buf_x[128];
    char buf_y[128];
    char buf_m[128];
    char buf[512];
    sprintf (buf_x, "%1.6f", point->X);
    gaiaOutClean (buf_x);
    sprintf (buf_y, "%1.6f", point->Y);
    gaiaOutClean (buf_y);
    sprintf (buf_m, "%1.6f", point->M);
    gaiaOutClean (buf_m);
    sprintf (buf, "%s %s %s", buf_x, buf_y, buf_m);
    gaiaAppendToOutBuffer (out_buf, buf);
}

static void
gaiaOutPointZM (gaiaOutBufferPtr out_buf, gaiaPointPtr point)
{
/* formats a WKT POINTZM */
    char buf_x[128];
    char buf_y[128];
    char buf_z[128];
    char buf_m[128];
    char buf[1024];
    sprintf (buf_x, "%1.6f", point->X);
    gaiaOutClean (buf_x);
    sprintf (buf_y, "%1.6f", point->Y);
    gaiaOutClean (buf_y);
    sprintf (buf_z, "%1.6f", point->Z);
    gaiaOutClean (buf_z);
    sprintf (buf_m, "%1.6f", point->M);
    gaiaOutClean (buf_m);
    sprintf (buf, "%s %s %s %s", buf_x, buf_y, buf_z, buf_m);
    gaiaAppendToOutBuffer (out_buf, buf);
}

static void
gaiaOutLinestring (gaiaOutBufferPtr out_buf, gaiaLinestringPtr line)
{
/* formats a WKT LINESTRING */
    char buf_x[128];
    char buf_y[128];
    char buf[256];
    double x;
    double y;
    int iv;
    for (iv = 0; iv < line->Points; iv++)
      {
	  gaiaGetPoint (line->Coords, iv, &x, &y);
	  sprintf (buf_x, "%1.6f", x);
	  gaiaOutClean (buf_x);
	  sprintf (buf_y, "%1.6f", y);
	  gaiaOutClean (buf_y);
	  if (iv > 0)
	      sprintf (buf, ", %s %s", buf_x, buf_y);
	  else
	      sprintf (buf, "%s %s", buf_x, buf_y);
	  gaiaAppendToOutBuffer (out_buf, buf);
      }
}

static void
gaiaOutLinestringZ (gaiaOutBufferPtr out_buf, gaiaLinestringPtr line)
{
/* formats a WKT LINESTRINGZ */
    char buf_x[128];
    char buf_y[128];
    char buf_z[128];
    char buf[512];
    double x;
    double y;
    double z;
    int iv;
    for (iv = 0; iv < line->Points; iv++)
      {
	  gaiaGetPointXYZ (line->Coords, iv, &x, &y, &z);
	  sprintf (buf_x, "%1.6f", x);
	  gaiaOutClean (buf_x);
	  sprintf (buf_y, "%1.6f", y);
	  gaiaOutClean (buf_y);
	  sprintf (buf_z, "%1.6f", z);
	  gaiaOutClean (buf_z);
	  if (iv > 0)
	      sprintf (buf, ", %s %s %s", buf_x, buf_y, buf_z);
	  else
	      sprintf (buf, "%s %s %s", buf_x, buf_y, buf_z);
	  gaiaAppendToOutBuffer (out_buf, buf);
      }
}

static void
gaiaOutLinestringM (gaiaOutBufferPtr out_buf, gaiaLinestringPtr line)
{
/* formats a WKT LINESTRINGM */
    char buf_x[128];
    char buf_y[128];
    char buf_m[128];
    char buf[512];
    double x;
    double y;
    double m;
    int iv;
    for (iv = 0; iv < line->Points; iv++)
      {
	  gaiaGetPointXYM (line->Coords, iv, &x, &y, &m);
	  sprintf (buf_x, "%1.6f", x);
	  gaiaOutClean (buf_x);
	  sprintf (buf_y, "%1.6f", y);
	  gaiaOutClean (buf_y);
	  sprintf (buf_m, "%1.6f", m);
	  gaiaOutClean (buf_m);
	  if (iv > 0)
	      sprintf (buf, ", %s %s %s", buf_x, buf_y, buf_m);
	  else
	      sprintf (buf, "%s %s %s", buf_x, buf_y, buf_m);
	  gaiaAppendToOutBuffer (out_buf, buf);
      }
}

static void
gaiaOutLinestringZM (gaiaOutBufferPtr out_buf, gaiaLinestringPtr line)
{
/* formats a WKT LINESTRINGZM */
    char buf_x[128];
    char buf_y[128];
    char buf_z[128];
    char buf_m[128];
    char buf[1024];
    double x;
    double y;
    double z;
    double m;
    int iv;
    for (iv = 0; iv < line->Points; iv++)
      {
	  gaiaGetPointXYZM (line->Coords, iv, &x, &y, &z, &m);
	  sprintf (buf_x, "%1.6f", x);
	  gaiaOutClean (buf_x);
	  sprintf (buf_y, "%1.6f", y);
	  gaiaOutClean (buf_y);
	  sprintf (buf_z, "%1.6f", z);
	  gaiaOutClean (buf_z);
	  sprintf (buf_m, "%1.6f", m);
	  gaiaOutClean (buf_m);
	  if (iv > 0)
	      sprintf (buf, ", %s %s %s %s", buf_x, buf_y, buf_z, buf_m);
	  else
	      sprintf (buf, "%s %s %s %s", buf_x, buf_y, buf_z, buf_m);
	  gaiaAppendToOutBuffer (out_buf, buf);
      }
}

static void
gaiaOutPolygon (gaiaOutBufferPtr out_buf, gaiaPolygonPtr polyg)
{
/* formats a WKT POLYGON */
    char buf_x[128];
    char buf_y[128];
    char buf[256];
    int ib;
    int iv;
    double x;
    double y;
    gaiaRingPtr ring = polyg->Exterior;
    for (iv = 0; iv < ring->Points; iv++)
      {
	  gaiaGetPoint (ring->Coords, iv, &x, &y);
	  sprintf (buf_x, "%1.6f", x);
	  gaiaOutClean (buf_x);
	  sprintf (buf_y, "%1.6f", y);
	  gaiaOutClean (buf_y);
	  if (iv == 0)
	      sprintf (buf, "(%s %s", buf_x, buf_y);
	  else if (iv == (ring->Points - 1))
	      sprintf (buf, ", %s %s)", buf_x, buf_y);
	  else
	      sprintf (buf, ", %s %s", buf_x, buf_y);
	  gaiaAppendToOutBuffer (out_buf, buf);
      }
    for (ib = 0; ib < polyg->NumInteriors; ib++)
      {
	  ring = polyg->Interiors + ib;
	  for (iv = 0; iv < ring->Points; iv++)
	    {
		gaiaGetPoint (ring->Coords, iv, &x, &y);
		sprintf (buf_x, "%1.6f", x);
		gaiaOutClean (buf_x);
		sprintf (buf_y, "%1.6f", y);
		gaiaOutClean (buf_y);
		if (iv == 0)
		    sprintf (buf, ", (%s %s", buf_x, buf_y);
		else if (iv == (ring->Points - 1))
		    sprintf (buf, ", %s %s)", buf_x, buf_y);
		else
		    sprintf (buf, ", %s %s", buf_x, buf_y);
		gaiaAppendToOutBuffer (out_buf, buf);
	    }
      }
}

static void
gaiaOutPolygonZ (gaiaOutBufferPtr out_buf, gaiaPolygonPtr polyg)
{
/* formats a WKT POLYGONZ */
    char buf_x[128];
    char buf_y[128];
    char buf_z[128];
    char buf[512];
    int ib;
    int iv;
    double x;
    double y;
    double z;
    gaiaRingPtr ring = polyg->Exterior;
    for (iv = 0; iv < ring->Points; iv++)
      {
	  gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
	  sprintf (buf_x, "%1.6f", x);
	  gaiaOutClean (buf_x);
	  sprintf (buf_y, "%1.6f", y);
	  gaiaOutClean (buf_y);
	  sprintf (buf_z, "%1.6f", z);
	  gaiaOutClean (buf_z);
	  if (iv == 0)
	      sprintf (buf, "(%s %s %s", buf_x, buf_y, buf_z);
	  else if (iv == (ring->Points - 1))
	      sprintf (buf, ", %s %s %s)", buf_x, buf_y, buf_z);
	  else
	      sprintf (buf, ", %s %s %s", buf_x, buf_y, buf_z);
	  gaiaAppendToOutBuffer (out_buf, buf);
      }
    for (ib = 0; ib < polyg->NumInteriors; ib++)
      {
	  ring = polyg->Interiors + ib;
	  for (iv = 0; iv < ring->Points; iv++)
	    {
		gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
		sprintf (buf_x, "%1.6f", x);
		gaiaOutClean (buf_x);
		sprintf (buf_y, "%1.6f", y);
		gaiaOutClean (buf_y);
		sprintf (buf_z, "%1.6f", z);
		gaiaOutClean (buf_z);
		if (iv == 0)
		    sprintf (buf, ", (%s %s %s", buf_x, buf_y, buf_z);
		else if (iv == (ring->Points - 1))
		    sprintf (buf, ", %s %s %s)", buf_x, buf_y, buf_z);
		else
		    sprintf (buf, ", %s %s %s", buf_x, buf_y, buf_z);
		gaiaAppendToOutBuffer (out_buf, buf);
	    }
      }
}

static void
gaiaOutPolygonM (gaiaOutBufferPtr out_buf, gaiaPolygonPtr polyg)
{
/* formats a WKT POLYGONM */
    char buf_x[128];
    char buf_y[128];
    char buf_m[128];
    char buf[512];
    int ib;
    int iv;
    double x;
    double y;
    double m;
    gaiaRingPtr ring = polyg->Exterior;
    for (iv = 0; iv < ring->Points; iv++)
      {
	  gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
	  sprintf (buf_x, "%1.6f", x);
	  gaiaOutClean (buf_x);
	  sprintf (buf_y, "%1.6f", y);
	  gaiaOutClean (buf_y);
	  sprintf (buf_m, "%1.6f", m);
	  gaiaOutClean (buf_m);
	  if (iv == 0)
	      sprintf (buf, "(%s %s %s", buf_x, buf_y, buf_m);
	  else if (iv == (ring->Points - 1))
	      sprintf (buf, ", %s %s %s)", buf_x, buf_y, buf_m);
	  else
	      sprintf (buf, ", %s %s %s", buf_x, buf_y, buf_m);
	  gaiaAppendToOutBuffer (out_buf, buf);
      }
    for (ib = 0; ib < polyg->NumInteriors; ib++)
      {
	  ring = polyg->Interiors + ib;
	  for (iv = 0; iv < ring->Points; iv++)
	    {
		gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
		sprintf (buf_x, "%1.6f", x);
		gaiaOutClean (buf_x);
		sprintf (buf_y, "%1.6f", y);
		gaiaOutClean (buf_y);
		sprintf (buf_m, "%1.6f", m);
		gaiaOutClean (buf_m);
		if (iv == 0)
		    sprintf (buf, ", (%s %s %s", buf_x, buf_y, buf_m);
		else if (iv == (ring->Points - 1))
		    sprintf (buf, ", %s %s %s)", buf_x, buf_y, buf_m);
		else
		    sprintf (buf, ", %s %s %s", buf_x, buf_y, buf_m);
		gaiaAppendToOutBuffer (out_buf, buf);
	    }
      }
}

static void
gaiaOutPolygonZM (gaiaOutBufferPtr out_buf, gaiaPolygonPtr polyg)
{
/* formats a WKT POLYGONZM */
    char buf_x[128];
    char buf_y[128];
    char buf_z[128];
    char buf_m[128];
    char buf[1024];
    int ib;
    int iv;
    double x;
    double y;
    double z;
    double m;
    gaiaRingPtr ring = polyg->Exterior;
    for (iv = 0; iv < ring->Points; iv++)
      {
	  gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
	  sprintf (buf_x, "%1.6f", x);
	  gaiaOutClean (buf_x);
	  sprintf (buf_y, "%1.6f", y);
	  gaiaOutClean (buf_y);
	  sprintf (buf_z, "%1.6f", z);
	  gaiaOutClean (buf_z);
	  sprintf (buf_m, "%1.6f", m);
	  gaiaOutClean (buf_m);
	  if (iv == 0)
	      sprintf (buf, "(%s %s %s %s", buf_x, buf_y, buf_z, buf_m);
	  else if (iv == (ring->Points - 1))
	      sprintf (buf, ", %s %s %s %s)", buf_x, buf_y, buf_z, buf_m);
	  else
	      sprintf (buf, ", %s %s %s %s", buf_x, buf_y, buf_z, buf_m);
	  gaiaAppendToOutBuffer (out_buf, buf);
      }
    for (ib = 0; ib < polyg->NumInteriors; ib++)
      {
	  ring = polyg->Interiors + ib;
	  for (iv = 0; iv < ring->Points; iv++)
	    {
		gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
		sprintf (buf_x, "%1.6f", x);
		gaiaOutClean (buf_x);
		sprintf (buf_y, "%1.6f", y);
		gaiaOutClean (buf_y);
		sprintf (buf_z, "%1.6f", z);
		gaiaOutClean (buf_z);
		sprintf (buf_m, "%1.6f", m);
		gaiaOutClean (buf_m);
		if (iv == 0)
		    sprintf (buf, ", (%s %s %s %s", buf_x, buf_y, buf_z, buf_m);
		else if (iv == (ring->Points - 1))
		    sprintf (buf, ", %s %s %s %s)", buf_x, buf_y, buf_z, buf_m);
		else
		    sprintf (buf, ", %s %s %s %s", buf_x, buf_y, buf_z, buf_m);
		gaiaAppendToOutBuffer (out_buf, buf);
	    }
      }
}

GAIAGEO_DECLARE void
gaiaOutWkt (gaiaOutBufferPtr out_buf, gaiaGeomCollPtr geom)
{
/* prints the WKT representation of current geometry */
    int pts = 0;
    int lns = 0;
    int pgs = 0;
    gaiaPointPtr point;
    gaiaLinestringPtr line;
    gaiaPolygonPtr polyg;
    if (!geom)
	return;
    point = geom->FirstPoint;
    while (point)
      {
	  /* counting how many POINTs are there */
	  pts++;
	  point = point->Next;
      }
    line = geom->FirstLinestring;
    while (line)
      {
	  /* counting how many LINESTRINGs are there */
	  lns++;
	  line = line->Next;
      }
    polyg = geom->FirstPolygon;
    while (polyg)
      {
	  /* counting how many POLYGONs are there */
	  pgs++;
	  polyg = polyg->Next;
      }
    if ((pts + lns + pgs) == 1
	&& (geom->DeclaredType == GAIA_POINT
	    || geom->DeclaredType == GAIA_LINESTRING
	    || geom->DeclaredType == GAIA_POLYGON))
      {
	  /* we have only one elementary geometry */
	  point = geom->FirstPoint;
	  while (point)
	    {
		if (point->DimensionModel == GAIA_XY_Z)
		  {
		      /* processing POINTZ */
		      gaiaAppendToOutBuffer (out_buf, "POINT Z(");
		      gaiaOutPointZ (out_buf, point);
		  }
		else if (point->DimensionModel == GAIA_XY_M)
		  {
		      /* processing POINTM */
		      gaiaAppendToOutBuffer (out_buf, "POINT M(");
		      gaiaOutPointM (out_buf, point);
		  }
		else if (point->DimensionModel == GAIA_XY_Z_M)
		  {
		      /* processing POINTZM */
		      gaiaAppendToOutBuffer (out_buf, "POINT ZM(");
		      gaiaOutPointZM (out_buf, point);
		  }
		else
		  {
		      /* processing POINT */
		      gaiaAppendToOutBuffer (out_buf, "POINT(");
		      gaiaOutPoint (out_buf, point);
		  }
		gaiaAppendToOutBuffer (out_buf, ")");
		point = point->Next;
	    }
	  line = geom->FirstLinestring;
	  while (line)
	    {
		if (line->DimensionModel == GAIA_XY_Z)
		  {
		      /* processing LINESTRINGZ */
		      gaiaAppendToOutBuffer (out_buf, "LINESTRING Z(");
		      gaiaOutLinestringZ (out_buf, line);
		  }
		else if (line->DimensionModel == GAIA_XY_M)
		  {
		      /* processing LINESTRINGM */
		      gaiaAppendToOutBuffer (out_buf, "LINESTRING M(");
		      gaiaOutLinestringM (out_buf, line);
		  }
		else if (line->DimensionModel == GAIA_XY_Z_M)
		  {
		      /* processing LINESTRINGZM */
		      gaiaAppendToOutBuffer (out_buf, "LINESTRING ZM(");
		      gaiaOutLinestringZM (out_buf, line);
		  }
		else
		  {
		      /* processing LINESTRING */
		      gaiaAppendToOutBuffer (out_buf, "LINESTRING(");
		      gaiaOutLinestring (out_buf, line);
		  }
		gaiaAppendToOutBuffer (out_buf, ")");
		line = line->Next;
	    }
	  polyg = geom->FirstPolygon;
	  while (polyg)
	    {
		if (polyg->DimensionModel == GAIA_XY_Z)
		  {
		      /* processing POLYGONZ */
		      gaiaAppendToOutBuffer (out_buf, "POLYGON Z(");
		      gaiaOutPolygonZ (out_buf, polyg);
		  }
		else if (polyg->DimensionModel == GAIA_XY_M)
		  {
		      /* processing POLYGONM */
		      gaiaAppendToOutBuffer (out_buf, "POLYGON M(");
		      gaiaOutPolygonM (out_buf, polyg);
		  }
		else if (polyg->DimensionModel == GAIA_XY_Z_M)
		  {
		      /* processing POLYGONZM */
		      gaiaAppendToOutBuffer (out_buf, "POLYGON ZM(");
		      gaiaOutPolygonZM (out_buf, polyg);
		  }
		else
		  {
		      /* processing POLYGON */
		      gaiaAppendToOutBuffer (out_buf, "POLYGON(");
		      gaiaOutPolygon (out_buf, polyg);
		  }
		gaiaAppendToOutBuffer (out_buf, ")");
		polyg = polyg->Next;
	    }
      }
    else
      {
	  /* we have some kind of complex geometry */
	  if (pts > 0 && lns == 0 && pgs == 0
	      && geom->DeclaredType == GAIA_MULTIPOINT)
	    {
		/* some kind of MULTIPOINT */
		if (geom->DimensionModel == GAIA_XY_Z)
		    gaiaAppendToOutBuffer (out_buf, "MULTIPOINT Z(");
		else if (geom->DimensionModel == GAIA_XY_M)
		    gaiaAppendToOutBuffer (out_buf, "MULTIPOINT M(");
		else if (geom->DimensionModel == GAIA_XY_Z_M)
		    gaiaAppendToOutBuffer (out_buf, "MULTIPOINT ZM(");
		else
		    gaiaAppendToOutBuffer (out_buf, "MULTIPOINT(");
		point = geom->FirstPoint;
		while (point)
		  {
		      if (point->DimensionModel == GAIA_XY_Z)
			{
			    if (point != geom->FirstPoint)
				gaiaAppendToOutBuffer (out_buf, ", ");
			    gaiaOutPointZ (out_buf, point);
			}
		      else if (point->DimensionModel == GAIA_XY_M)
			{
			    if (point != geom->FirstPoint)
				gaiaAppendToOutBuffer (out_buf, ", ");
			    gaiaOutPointM (out_buf, point);
			}
		      else if (point->DimensionModel == GAIA_XY_Z_M)
			{
			    if (point != geom->FirstPoint)
				gaiaAppendToOutBuffer (out_buf, ", ");
			    gaiaOutPointZM (out_buf, point);
			}
		      else
			{
			    if (point != geom->FirstPoint)
				gaiaAppendToOutBuffer (out_buf, ", ");
			    gaiaOutPoint (out_buf, point);
			}
		      point = point->Next;
		  }
		gaiaAppendToOutBuffer (out_buf, ")");
	    }
	  else if (pts == 0 && lns > 0 && pgs == 0
		   && geom->DeclaredType == GAIA_MULTILINESTRING)
	    {
		/* some kind of MULTILINESTRING */
		if (geom->DimensionModel == GAIA_XY_Z)
		    gaiaAppendToOutBuffer (out_buf, "MULTILINESTRING Z(");
		else if (geom->DimensionModel == GAIA_XY_M)
		    gaiaAppendToOutBuffer (out_buf, "MULTILINESTRING M(");
		else if (geom->DimensionModel == GAIA_XY_Z_M)
		    gaiaAppendToOutBuffer (out_buf, "MULTILINESTRING ZM(");
		else
		    gaiaAppendToOutBuffer (out_buf, "MULTILINESTRING(");
		line = geom->FirstLinestring;
		while (line)
		  {
		      if (line != geom->FirstLinestring)
			  gaiaAppendToOutBuffer (out_buf, ", (");
		      else
			  gaiaAppendToOutBuffer (out_buf, "(");
		      if (line->DimensionModel == GAIA_XY_Z)
			{
			    gaiaOutLinestringZ (out_buf, line);
			    gaiaAppendToOutBuffer (out_buf, ")");
			}
		      else if (line->DimensionModel == GAIA_XY_M)
			{
			    gaiaOutLinestringM (out_buf, line);
			    gaiaAppendToOutBuffer (out_buf, ")");
			}
		      else if (line->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaOutLinestringZM (out_buf, line);
			    gaiaAppendToOutBuffer (out_buf, ")");
			}
		      else
			{
			    gaiaOutLinestring (out_buf, line);
			    gaiaAppendToOutBuffer (out_buf, ")");
			}
		      line = line->Next;
		  }
		gaiaAppendToOutBuffer (out_buf, ")");
	    }
	  else if (pts == 0 && lns == 0 && pgs > 0
		   && geom->DeclaredType == GAIA_MULTIPOLYGON)
	    {
		/* some kind of MULTIPOLYGON */
		if (geom->DimensionModel == GAIA_XY_Z)
		    gaiaAppendToOutBuffer (out_buf, "MULTIPOLYGON Z(");
		else if (geom->DimensionModel == GAIA_XY_M)
		    gaiaAppendToOutBuffer (out_buf, "MULTIPOLYGON M(");
		else if (geom->DimensionModel == GAIA_XY_Z_M)
		    gaiaAppendToOutBuffer (out_buf, "MULTIPOLYGON ZM(");
		else
		    gaiaAppendToOutBuffer (out_buf, "MULTIPOLYGON(");
		polyg = geom->FirstPolygon;
		while (polyg)
		  {
		      if (polyg != geom->FirstPolygon)
			  gaiaAppendToOutBuffer (out_buf, ", (");
		      else
			  gaiaAppendToOutBuffer (out_buf, "(");
		      if (polyg->DimensionModel == GAIA_XY_Z)
			{
			    gaiaOutPolygonZ (out_buf, polyg);
			    gaiaAppendToOutBuffer (out_buf, ")");
			}
		      else if (polyg->DimensionModel == GAIA_XY_M)
			{
			    gaiaOutPolygonM (out_buf, polyg);
			    gaiaAppendToOutBuffer (out_buf, ")");
			}
		      else if (polyg->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaOutPolygonZM (out_buf, polyg);
			    gaiaAppendToOutBuffer (out_buf, ")");
			}
		      else
			{
			    gaiaOutPolygon (out_buf, polyg);
			    gaiaAppendToOutBuffer (out_buf, ")");
			}
		      polyg = polyg->Next;
		  }
		gaiaAppendToOutBuffer (out_buf, ")");
	    }
	  else
	    {
		/* some kind of GEOMETRYCOLLECTION */
		int ie = 0;
		if (geom->DimensionModel == GAIA_XY_Z)
		    gaiaAppendToOutBuffer (out_buf, "GEOMETRYCOLLECTION Z(");
		else if (geom->DimensionModel == GAIA_XY_M)
		    gaiaAppendToOutBuffer (out_buf, "GEOMETRYCOLLECTION M(");
		else if (geom->DimensionModel == GAIA_XY_Z_M)
		    gaiaAppendToOutBuffer (out_buf, "GEOMETRYCOLLECTION ZM(");
		else
		    gaiaAppendToOutBuffer (out_buf, "GEOMETRYCOLLECTION(");
		point = geom->FirstPoint;
		while (point)
		  {
		      /* processing POINTs */
		      if (ie > 0)
			  gaiaAppendToOutBuffer (out_buf, ", ");
		      ie++;
		      if (point->DimensionModel == GAIA_XY_Z)
			{
			    gaiaAppendToOutBuffer (out_buf, "POINT Z(");
			    gaiaOutPointZ (out_buf, point);
			}
		      else if (point->DimensionModel == GAIA_XY_M)
			{
			    gaiaAppendToOutBuffer (out_buf, "POINT M(");
			    gaiaOutPointM (out_buf, point);
			}
		      else if (point->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaAppendToOutBuffer (out_buf, "POINT ZM(");
			    gaiaOutPointZM (out_buf, point);
			}
		      else
			{
			    gaiaAppendToOutBuffer (out_buf, "POINT(");
			    gaiaOutPoint (out_buf, point);
			}
		      gaiaAppendToOutBuffer (out_buf, ")");
		      point = point->Next;
		  }
		line = geom->FirstLinestring;
		while (line)
		  {
		      /* processing LINESTRINGs */
		      if (ie > 0)
			  gaiaAppendToOutBuffer (out_buf, ", ");
		      ie++;
		      if (line->DimensionModel == GAIA_XY_Z)
			{
			    gaiaAppendToOutBuffer (out_buf, "LINESTRING Z(");
			    gaiaOutLinestringZ (out_buf, line);
			}
		      else if (line->DimensionModel == GAIA_XY_M)
			{
			    gaiaAppendToOutBuffer (out_buf, "LINESTRING M(");
			    gaiaOutLinestringM (out_buf, line);
			}
		      else if (line->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaAppendToOutBuffer (out_buf, "LINESTRING ZM(");
			    gaiaOutLinestringZM (out_buf, line);
			}
		      else
			{
			    gaiaAppendToOutBuffer (out_buf, "LINESTRING(");
			    gaiaOutLinestring (out_buf, line);
			}
		      gaiaAppendToOutBuffer (out_buf, ")");
		      line = line->Next;
		  }
		polyg = geom->FirstPolygon;
		while (polyg)
		  {
		      /* processing POLYGONs */
		      if (ie > 0)
			  gaiaAppendToOutBuffer (out_buf, ", ");
		      ie++;
		      if (polyg->DimensionModel == GAIA_XY_Z)
			{
			    gaiaAppendToOutBuffer (out_buf, "POLYGON Z(");
			    gaiaOutPolygonZ (out_buf, polyg);
			}
		      else if (polyg->DimensionModel == GAIA_XY_M)
			{
			    gaiaAppendToOutBuffer (out_buf, "POLYGON M(");
			    gaiaOutPolygonM (out_buf, polyg);
			}
		      else if (polyg->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaAppendToOutBuffer (out_buf, "POLYGON ZM(");
			    gaiaOutPolygonZM (out_buf, polyg);
			}
		      else
			{
			    gaiaAppendToOutBuffer (out_buf, "POLYGON(");
			    gaiaOutPolygon (out_buf, polyg);
			}
		      gaiaAppendToOutBuffer (out_buf, ")");
		      polyg = polyg->Next;
		  }
		gaiaAppendToOutBuffer (out_buf, ")");
	    }
      }
}

/*
/
/  Gaia common support for SVG encoded geometries
/
////////////////////////////////////////////////////////////
/
/ Author: Klaus Foerster klaus.foerster@svg.cc
/ version 0.9. 2008 September 21
 /
 */

static void
SvgCoords (gaiaOutBufferPtr out_buf, gaiaPointPtr point, int precision)
{
/* formats POINT as SVG-attributes x,y */
    char buf_x[128];
    char buf_y[128];
    char buf[256];
    sprintf (buf_x, "%.*f", precision, point->X);
    gaiaOutClean (buf_x);
    sprintf (buf_y, "%.*f", precision, point->Y * -1);
    gaiaOutClean (buf_y);
    sprintf (buf, "x=\"%s\" y=\"%s\"", buf_x, buf_y);
    gaiaAppendToOutBuffer (out_buf, buf);
}

static void
SvgCircle (gaiaOutBufferPtr out_buf, gaiaPointPtr point, int precision)
{
/* formats POINT as SVG-attributes cx,cy */
    char buf_x[128];
    char buf_y[128];
    char buf[256];
    sprintf (buf_x, "%.*f", precision, point->X);
    gaiaOutClean (buf_x);
    sprintf (buf_y, "%.*f", precision, point->Y * -1);
    gaiaOutClean (buf_y);
    sprintf (buf, "cx=\"%s\" cy=\"%s\"", buf_x, buf_y);
    gaiaAppendToOutBuffer (out_buf, buf);
}

static void
SvgPathRelative (gaiaOutBufferPtr out_buf, int dims, int points, double *coords,
		 int precision, int closePath)
{
/* formats LINESTRING as SVG-path d-attribute with relative coordinate moves */
    char buf_x[128];
    char buf_y[128];
    char buf[256];
    double x;
    double y;
    double z;
    double m;
    double lastX = 0.0;
    double lastY = 0.0;
    int iv;
    for (iv = 0; iv < points; iv++)
      {
	  if (dims == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (coords, iv, &x, &y, &z);
	    }
	  else if (dims == GAIA_XY_M)
	    {
		gaiaGetPointXYM (coords, iv, &x, &y, &m);
	    }
	  else if (dims == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (coords, iv, &x, &y);
	    }
	  sprintf (buf_x, "%.*f", precision, x - lastX);
	  gaiaOutClean (buf_x);
	  sprintf (buf_y, "%.*f", precision, (y - lastY) * -1);
	  gaiaOutClean (buf_y);
	  if (iv == 0)
	      sprintf (buf, "M %s %s l ", buf_x, buf_y);
	  else
	      sprintf (buf, "%s %s ", buf_x, buf_y);
	  lastX = x;
	  lastY = y;
	  if (iv == points - 1 && closePath == 1)
	      sprintf (buf, "z ");
	  gaiaAppendToOutBuffer (out_buf, buf);
      }
}

static void
SvgPathAbsolute (gaiaOutBufferPtr out_buf, int dims, int points, double *coords,
		 int precision, int closePath)
{
/* formats LINESTRING as SVG-path d-attribute with relative coordinate moves */
    char buf_x[128];
    char buf_y[128];
    char buf[256];
    double x;
    double y;
    double z;
    double m;
    int iv;
    for (iv = 0; iv < points; iv++)
      {
	  if (dims == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (coords, iv, &x, &y, &z);
	    }
	  else if (dims == GAIA_XY_M)
	    {
		gaiaGetPointXYM (coords, iv, &x, &y, &m);
	    }
	  else if (dims == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (coords, iv, &x, &y);
	    }
	  sprintf (buf_x, "%.*f", precision, x);
	  gaiaOutClean (buf_x);
	  sprintf (buf_y, "%.*f", precision, y * -1);
	  gaiaOutClean (buf_y);
	  if (iv == 0)
	      sprintf (buf, "M %s %s L ", buf_x, buf_y);
	  else
	      sprintf (buf, "%s %s ", buf_x, buf_y);
	  if (iv == points - 1 && closePath == 1)
	      sprintf (buf, "z ");
	  gaiaAppendToOutBuffer (out_buf, buf);
      }
}

GAIAGEO_DECLARE void
gaiaOutSvg (gaiaOutBufferPtr out_buf, gaiaGeomCollPtr geom, int relative,
	    int precision)
{
/* prints the SVG representation of current geometry */
    int pts = 0;
    int lns = 0;
    int pgs = 0;
    int ib;
    gaiaPointPtr point;
    gaiaLinestringPtr line;
    gaiaPolygonPtr polyg;
    gaiaRingPtr ring;
    if (!geom)
	return;
    point = geom->FirstPoint;
    while (point)
      {
	  /* counting how many POINTs are there */
	  pts++;
	  point = point->Next;
      }
    line = geom->FirstLinestring;
    while (line)
      {
	  /* counting how many LINESTRINGs are there */
	  lns++;
	  line = line->Next;
      }
    polyg = geom->FirstPolygon;
    while (polyg)
      {
	  /* counting how many POLYGONs are there */
	  pgs++;
	  polyg = polyg->Next;
      }

    if ((pts + lns + pgs) == 1)
      {
	  /* we have only one elementary geometry */
	  point = geom->FirstPoint;
	  while (point)
	    {
		/* processing POINT */
		if (relative == 1)
		    SvgCoords (out_buf, point, precision);
		else
		    SvgCircle (out_buf, point, precision);
		point = point->Next;
	    }
	  line = geom->FirstLinestring;
	  while (line)
	    {
		/* processing LINESTRING */
		if (relative == 1)
		    SvgPathRelative (out_buf, line->DimensionModel,
				     line->Points, line->Coords, precision, 0);
		else
		    SvgPathAbsolute (out_buf, line->DimensionModel,
				     line->Points, line->Coords, precision, 0);
		line = line->Next;
	    }
	  polyg = geom->FirstPolygon;
	  while (polyg)
	    {
		/* process exterior and interior rings */
		ring = polyg->Exterior;
		if (relative == 1)
		  {
		      SvgPathRelative (out_buf, ring->DimensionModel,
				       ring->Points, ring->Coords, precision,
				       1);
		      for (ib = 0; ib < polyg->NumInteriors; ib++)
			{
			    ring = polyg->Interiors + ib;
			    SvgPathRelative (out_buf, ring->DimensionModel,
					     ring->Points, ring->Coords,
					     precision, 1);
			}
		  }
		else
		  {
		      SvgPathAbsolute (out_buf, ring->DimensionModel,
				       ring->Points, ring->Coords, precision,
				       1);
		      for (ib = 0; ib < polyg->NumInteriors; ib++)
			{
			    ring = polyg->Interiors + ib;
			    SvgPathAbsolute (out_buf, ring->DimensionModel,
					     ring->Points, ring->Coords,
					     precision, 1);
			}
		  }
		polyg = polyg->Next;
	    }
      }
    else
      {
	  /* we have some kind of complex geometry */
	  if (pts > 0 && lns == 0 && pgs == 0)
	    {
		/* this one is a MULTIPOINT */
		point = geom->FirstPoint;
		while (point)
		  {
		      /* processing POINTs */
		      if (point != geom->FirstPoint)
			  gaiaAppendToOutBuffer (out_buf, ",");
		      if (relative == 1)
			  SvgCoords (out_buf, point, precision);
		      else
			  SvgCircle (out_buf, point, precision);
		      point = point->Next;
		  }
	    }
	  else if (pts == 0 && lns > 0 && pgs == 0)
	    {
		/* this one is a MULTILINESTRING */
		line = geom->FirstLinestring;
		while (line)
		  {
		      /* processing LINESTRINGs */
		      if (relative == 1)
			  SvgPathRelative (out_buf, line->DimensionModel,
					   line->Points, line->Coords,
					   precision, 0);
		      else
			  SvgPathAbsolute (out_buf, line->DimensionModel,
					   line->Points, line->Coords,
					   precision, 0);
		      line = line->Next;
		  }
	    }
	  else if (pts == 0 && lns == 0 && pgs > 0)
	    {
		/* this one is a MULTIPOLYGON */
		polyg = geom->FirstPolygon;
		while (polyg)
		  {
		      /* processing POLYGONs */
		      ring = polyg->Exterior;
		      if (relative == 1)
			{
			    SvgPathRelative (out_buf, ring->DimensionModel,
					     ring->Points, ring->Coords,
					     precision, 1);
			    for (ib = 0; ib < polyg->NumInteriors; ib++)
			      {
				  ring = polyg->Interiors + ib;
				  SvgPathRelative (out_buf,
						   ring->DimensionModel,
						   ring->Points, ring->Coords,
						   precision, 1);
			      }
			}
		      else
			{
			    SvgPathAbsolute (out_buf, ring->DimensionModel,
					     ring->Points, ring->Coords,
					     precision, 1);
			    for (ib = 0; ib < polyg->NumInteriors; ib++)
			      {
				  ring = polyg->Interiors + ib;
				  SvgPathAbsolute (out_buf,
						   ring->DimensionModel,
						   ring->Points, ring->Coords,
						   precision, 1);
			      }
			}
		      polyg = polyg->Next;
		  }
	    }
	  else
	    {
		/* this one is a GEOMETRYCOLLECTION */
		int ie = 0;
		point = geom->FirstPoint;
		while (point)
		  {
		      /* processing POINTs */
		      if (ie > 0)
			{
			    gaiaAppendToOutBuffer (out_buf, ";");
			}
		      ie++;
		      if (relative == 1)
			  SvgCoords (out_buf, point, precision);
		      else
			  SvgCircle (out_buf, point, precision);
		      point = point->Next;
		  }
		line = geom->FirstLinestring;
		while (line)
		  {
		      /* processing LINESTRINGs */
		      if (ie > 0)
			  gaiaAppendToOutBuffer (out_buf, ";");
		      ie++;
		      if (relative == 1)
			  SvgPathRelative (out_buf, line->DimensionModel,
					   line->Points, line->Coords,
					   precision, 0);
		      else
			  SvgPathAbsolute (out_buf, line->DimensionModel,
					   line->Points, line->Coords,
					   precision, 0);
		      line = line->Next;
		  }
		polyg = geom->FirstPolygon;
		while (polyg)
		  {
		      /* processing POLYGONs */
		      ie++;
		      /* process exterior and interior rings */
		      ring = polyg->Exterior;
		      if (relative == 1)
			{
			    SvgPathRelative (out_buf, ring->DimensionModel,
					     ring->Points, ring->Coords,
					     precision, 1);
			    for (ib = 0; ib < polyg->NumInteriors; ib++)
			      {
				  ring = polyg->Interiors + ib;
				  SvgPathRelative (out_buf,
						   ring->DimensionModel,
						   ring->Points, ring->Coords,
						   precision, 1);
			      }
			}
		      else
			{
			    SvgPathAbsolute (out_buf, ring->DimensionModel,
					     ring->Points, ring->Coords,
					     precision, 1);
			    for (ib = 0; ib < polyg->NumInteriors; ib++)
			      {
				  ring = polyg->Interiors + ib;
				  SvgPathAbsolute (out_buf,
						   ring->DimensionModel,
						   ring->Points, ring->Coords,
						   precision, 1);
			      }
			}
		      polyg = polyg->Next;
		  }
	    }
      }
}

/* END of Klaus Foerster SVG implementation */


static char *
XmlClean (const char *string)
{
/* well formatting a text string for XML */
    int ind;
    char *clean;
    char *p_out;
    int len = strlen (string);
    clean = malloc (len * 3);
    if (!clean)
	return NULL;
    p_out = clean;
    for (ind = 0; ind < len; ind++)
      {
	  /* masking XML special chars */
	  switch (string[ind])
	    {
	    case '&':
		*p_out++ = '&';
		*p_out++ = 'a';
		*p_out++ = 'm';
		*p_out++ = 'p';
		*p_out++ = ';';
		break;
	    case '<':
		*p_out++ = '&';
		*p_out++ = 'l';
		*p_out++ = 't';
		*p_out++ = ';';
		break;
	    case '>':
		*p_out++ = '&';
		*p_out++ = 'g';
		*p_out++ = 't';
		*p_out++ = ';';
		break;
	    case '"':
		*p_out++ = '&';
		*p_out++ = 'q';
		*p_out++ = 'u';
		*p_out++ = 'o';
		*p_out++ = 't';
		*p_out++ = ';';
		break;
	    default:
		*p_out++ = string[ind];
		break;
	    };
      }
    *p_out = '\0';
    return clean;
}

static void
out_bare_kml_point (gaiaOutBufferPtr out_buf, gaiaPointPtr point, int precision)
{
/* formats POINT as 'bare' KML [x,y] */
    char buf_x[128];
    char buf_y[128];
    char buf[256];
    sprintf (buf_x, "%.*f", precision, point->X);
    gaiaOutClean (buf_x);
    sprintf (buf_y, "%.*f", precision, point->Y);
    gaiaOutClean (buf_y);
    gaiaAppendToOutBuffer (out_buf, "<Point><coordinates>");
    sprintf (buf, "%s,%s", buf_x, buf_y);
    gaiaAppendToOutBuffer (out_buf, buf);
    gaiaAppendToOutBuffer (out_buf, "</coordinates></Point>");
}

static void
out_full_kml_point (gaiaOutBufferPtr out_buf, const char *name,
		    const char *desc, gaiaPointPtr point, int precision)
{
/* formats POINT as 'full' KML [x,y] */
    char buf_x[128];
    char buf_y[128];
    char buf[256];
    char *xml_clean;
    sprintf (buf_x, "%.*f", precision, point->X);
    gaiaOutClean (buf_x);
    sprintf (buf_y, "%.*f", precision, point->Y);
    gaiaOutClean (buf_y);
    gaiaAppendToOutBuffer (out_buf, "<Placemark><name>");
    xml_clean = XmlClean (name);
    if (xml_clean)
      {
	  gaiaAppendToOutBuffer (out_buf, xml_clean);
	  free (xml_clean);
      }
    else
	gaiaAppendToOutBuffer (out_buf, " ");
    gaiaAppendToOutBuffer (out_buf, "</name><description>");
    xml_clean = XmlClean (desc);
    if (xml_clean)
      {
	  gaiaAppendToOutBuffer (out_buf, xml_clean);
	  free (xml_clean);
      }
    else
	gaiaAppendToOutBuffer (out_buf, " ");
    gaiaAppendToOutBuffer (out_buf, "</description><Point><coordinates>");
    sprintf (buf, "%s,%s", buf_x, buf_y);
    gaiaAppendToOutBuffer (out_buf, buf);
    gaiaAppendToOutBuffer (out_buf, "</coordinates></Point></Placemark>");
}

static void
out_bare_kml_linestring (gaiaOutBuffer * out_buf, int dims, int points,
			 double *coords, int precision)
{
/* formats LINESTRING as 'bare' KML [x,y] */
    char buf_x[128];
    char buf_y[128];
    char buf[256];
    int iv;
    double x;
    double y;
    double z;
    double m;
    gaiaAppendToOutBuffer (out_buf, "<LineString><coordinates>");
    for (iv = 0; iv < points; iv++)
      {
	  /* exporting vertices */
	  if (dims == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (coords, iv, &x, &y, &z);
	    }
	  else if (dims == GAIA_XY_M)
	    {
		gaiaGetPointXYM (coords, iv, &x, &y, &m);
	    }
	  else if (dims == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (coords, iv, &x, &y);
	    }
	  sprintf (buf_x, "%.*f", precision, x);
	  gaiaOutClean (buf_x);
	  sprintf (buf_y, "%.*f", precision, y);
	  gaiaOutClean (buf_y);
	  if (iv == 0)
	      sprintf (buf, "%s,%s", buf_x, buf_y);
	  else
	      sprintf (buf, " %s,%s", buf_x, buf_y);
	  gaiaAppendToOutBuffer (out_buf, buf);
      }
    gaiaAppendToOutBuffer (out_buf, "</coordinates></LineString>");
}

static void
out_full_kml_linestring (gaiaOutBufferPtr out_buf, const char *name,
			 const char *desc, int dims, int points, double *coords,
			 int precision)
{
/* formats LINESTRING as 'full' KML [x,y] */
    char buf_x[128];
    char buf_y[128];
    char buf[256];
    char *xml_clean;
    int iv;
    double x;
    double y;
    double z;
    double m;
    gaiaAppendToOutBuffer (out_buf, "<Placemark><name>");
    xml_clean = XmlClean (name);
    if (xml_clean)
      {
	  gaiaAppendToOutBuffer (out_buf, xml_clean);
	  free (xml_clean);
      }
    else
	gaiaAppendToOutBuffer (out_buf, " ");
    gaiaAppendToOutBuffer (out_buf, "</name><description>");
    xml_clean = XmlClean (desc);
    if (xml_clean)
      {
	  gaiaAppendToOutBuffer (out_buf, xml_clean);
	  free (xml_clean);
      }
    else
	gaiaAppendToOutBuffer (out_buf, " ");
    gaiaAppendToOutBuffer (out_buf, "</description><LineString><coordinates>");
    for (iv = 0; iv < points; iv++)
      {
	  /* exporting vertices */
	  if (dims == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (coords, iv, &x, &y, &z);
	    }
	  else if (dims == GAIA_XY_M)
	    {
		gaiaGetPointXYM (coords, iv, &x, &y, &m);
	    }
	  else if (dims == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (coords, iv, &x, &y);
	    }
	  sprintf (buf_x, "%.*f", precision, x);
	  gaiaOutClean (buf_x);
	  sprintf (buf_y, "%.*f", precision, y);
	  gaiaOutClean (buf_y);
	  if (iv == 0)
	      sprintf (buf, "%s,%s", buf_x, buf_y);
	  else
	      sprintf (buf, " %s,%s", buf_x, buf_y);
	  gaiaAppendToOutBuffer (out_buf, buf);
      }
    gaiaAppendToOutBuffer (out_buf, "</coordinates></LineString></Placemark>");
}

static void
out_bare_kml_polygon (gaiaOutBufferPtr out_buf, gaiaPolygonPtr polygon,
		      int precision)
{
/* formats POLYGON as 'bare' KML [x,y] */
    char buf_x[128];
    char buf_y[128];
    char buf[256];
    gaiaRingPtr ring;
    int iv;
    int ib;
    double x;
    double y;
    double z;
    double m;
    gaiaAppendToOutBuffer (out_buf, "<Polygon>");
    gaiaAppendToOutBuffer (out_buf,
			   "<outerBoundaryIs><LinearRing><coordinates>");
    ring = polygon->Exterior;
    for (iv = 0; iv < ring->Points; iv++)
      {
	  /* exporting vertices [Exterior Ring] */
	  if (ring->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
	    }
	  else if (ring->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
	    }
	  else if (ring->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (ring->Coords, iv, &x, &y);
	    }
	  sprintf (buf_x, "%.*f", precision, x);
	  gaiaOutClean (buf_x);
	  sprintf (buf_y, "%.*f", precision, y);
	  gaiaOutClean (buf_y);
	  if (iv == 0)
	      sprintf (buf, "%s,%s", buf_x, buf_y);
	  else
	      sprintf (buf, " %s,%s", buf_x, buf_y);
	  gaiaAppendToOutBuffer (out_buf, buf);
      }
    gaiaAppendToOutBuffer (out_buf,
			   "</coordinates></LinearRing></outerBoundaryIs>");
    for (ib = 0; ib < polygon->NumInteriors; ib++)
      {
	  /* interior rings */
	  ring = polygon->Interiors + ib;
	  gaiaAppendToOutBuffer (out_buf,
				 "<innerBoundaryIs><LinearRing><coordinates>");
	  for (iv = 0; iv < ring->Points; iv++)
	    {
		/* exporting vertices [Interior Ring] */
		if (ring->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
		  }
		else if (ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
		  }
		else if (ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (ring->Coords, iv, &x, &y);
		  }
		sprintf (buf_x, "%.*f", precision, x);
		gaiaOutClean (buf_x);
		sprintf (buf_y, "%.*f", precision, y);
		gaiaOutClean (buf_y);
		if (iv == 0)
		    sprintf (buf, "%s,%s", buf_x, buf_y);
		else
		    sprintf (buf, " %s,%s", buf_x, buf_y);
		gaiaAppendToOutBuffer (out_buf, buf);
	    }
	  gaiaAppendToOutBuffer (out_buf,
				 "</coordinates></LinearRing></innerBoundaryIs>");
      }
    strcpy (buf, "</Polygon>");
    gaiaAppendToOutBuffer (out_buf, buf);
}

static void
out_full_kml_polygon (gaiaOutBufferPtr out_buf, const char *name,
		      const char *desc, gaiaPolygonPtr polygon, int precision)
{
/* formats POLYGON as 'full' KML [x,y] */
    char buf_x[128];
    char buf_y[128];
    char buf[256];
    char *xml_clean;
    gaiaRingPtr ring;
    int iv;
    int ib;
    double x;
    double y;
    double z;
    double m;
    gaiaAppendToOutBuffer (out_buf, "<Placemark><name>");
    xml_clean = XmlClean (name);
    if (xml_clean)
      {
	  gaiaAppendToOutBuffer (out_buf, xml_clean);
	  free (xml_clean);
      }
    else
	gaiaAppendToOutBuffer (out_buf, " ");
    gaiaAppendToOutBuffer (out_buf, "</name><description>");
    xml_clean = XmlClean (desc);
    if (xml_clean)
      {
	  gaiaAppendToOutBuffer (out_buf, xml_clean);
	  free (xml_clean);
      }
    else
	gaiaAppendToOutBuffer (out_buf, " ");
    gaiaAppendToOutBuffer (out_buf, "</description><Polygon>");
    gaiaAppendToOutBuffer (out_buf,
			   "<outerBoundaryIs><LinearRing><coordinates>");
    ring = polygon->Exterior;
    for (iv = 0; iv < ring->Points; iv++)
      {
	  /* exporting vertices [Exterior Ring] */
	  if (ring->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
	    }
	  else if (ring->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
	    }
	  else if (ring->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (ring->Coords, iv, &x, &y);
	    }
	  sprintf (buf_x, "%.*f", precision, x);
	  gaiaOutClean (buf_x);
	  sprintf (buf_y, "%.*f", precision, y);
	  gaiaOutClean (buf_y);
	  if (iv == 0)
	      sprintf (buf, "%s,%s", buf_x, buf_y);
	  else
	      sprintf (buf, " %s,%s", buf_x, buf_y);
	  gaiaAppendToOutBuffer (out_buf, buf);
      }
    gaiaAppendToOutBuffer (out_buf,
			   "</coordinates></LinearRing></outerBoundaryIs>");
    for (ib = 0; ib < polygon->NumInteriors; ib++)
      {
	  /* interior rings */
	  ring = polygon->Interiors + ib;
	  gaiaAppendToOutBuffer (out_buf,
				 "<innerBoundaryIs><LinearRing><coordinates>");
	  for (iv = 0; iv < ring->Points; iv++)
	    {
		/* exporting vertices [Interior Ring] */
		if (ring->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
		  }
		else if (ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
		  }
		else if (ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (ring->Coords, iv, &x, &y);
		  }
		sprintf (buf_x, "%.*f", precision, x);
		gaiaOutClean (buf_x);
		sprintf (buf_y, "%.*f", precision, y);
		gaiaOutClean (buf_y);
		if (iv == 0)
		    sprintf (buf, "%s,%s", buf_x, buf_y);
		else
		    sprintf (buf, " %s,%s", buf_x, buf_y);
		gaiaAppendToOutBuffer (out_buf, buf);
	    }
	  gaiaAppendToOutBuffer (out_buf,
				 "</coordinates></LinearRing></innerBoundaryIs>");
      }
    gaiaAppendToOutBuffer (out_buf, "</Polygon></Placemark>");
}

GAIAGEO_DECLARE void
gaiaOutFullKml (gaiaOutBufferPtr out_buf, const char *name, const char *desc,
		gaiaGeomCollPtr geom, int precision)
{
/* prints the 'full' KML representation of current geometry */
    gaiaPointPtr point;
    gaiaLinestringPtr line;
    gaiaPolygonPtr polyg;
    if (!geom)
	return;
    if (precision > 18)
	precision = 18;
    point = geom->FirstPoint;
    while (point)
      {
	  /* processing POINT */
	  out_full_kml_point (out_buf, name, desc, point, precision);
	  point = point->Next;
      }
    line = geom->FirstLinestring;
    while (line)
      {
	  /* processing LINESTRING */
	  out_full_kml_linestring (out_buf, name, desc, line->DimensionModel,
				   line->Points, line->Coords, precision);
	  line = line->Next;
      }
    polyg = geom->FirstPolygon;
    while (polyg)
      {
	  /* processing POLYGON */
	  out_full_kml_polygon (out_buf, name, desc, polyg, precision);
	  polyg = polyg->Next;
      }
}

GAIAGEO_DECLARE void
gaiaOutBareKml (gaiaOutBufferPtr out_buf, gaiaGeomCollPtr geom, int precision)
{
/* prints the 'bare' KML representation of current geometry */
    gaiaPointPtr point;
    gaiaLinestringPtr line;
    gaiaPolygonPtr polyg;
    if (!geom)
	return;
    if (precision > 18)
	precision = 18;
    point = geom->FirstPoint;
    while (point)
      {
	  /* processing POINT */
	  out_bare_kml_point (out_buf, point, precision);
	  point = point->Next;
      }
    line = geom->FirstLinestring;
    while (line)
      {
	  /* processing LINESTRING */
	  out_bare_kml_linestring (out_buf, line->DimensionModel, line->Points,
				   line->Coords, precision);
	  line = line->Next;
      }
    polyg = geom->FirstPolygon;
    while (polyg)
      {
	  /* processing POLYGON */
	  out_bare_kml_polygon (out_buf, polyg, precision);
	  polyg = polyg->Next;
      }
}

GAIAGEO_DECLARE void
gaiaOutGml (gaiaOutBufferPtr out_buf, int version, int precision,
	    gaiaGeomCollPtr geom)
{
/*
/ prints the GML representation of current geometry
/ *result* returns the encoded GML or NULL if any error is encountered
*/
    gaiaPointPtr point;
    gaiaLinestringPtr line;
    gaiaPolygonPtr polyg;
    gaiaRingPtr ring;
    int iv;
    int ib;
    double x;
    double y;
    double z;
    double m;
    int has_z;
    int is_multi = 1;
    char buf[1024];
    char buf_x[128];
    char buf_y[128];
    char buf_z[128];
    if (!geom)
	return;
    if (precision > 18)
	precision = 18;

    switch (geom->DeclaredType)
      {
      case GAIA_POINT:
      case GAIA_LINESTRING:
      case GAIA_POLYGON:
	  *buf = '\0';
	  is_multi = 0;
	  break;
      case GAIA_MULTIPOINT:
	  sprintf (buf, "<gml:MultiPoint SrsName=\"EPSG::%d\">", geom->Srid);
	  break;
      case GAIA_MULTILINESTRING:
	  if (version == 3)
	      sprintf (buf, "<gml:MultiCurve SrsName=\"EPSG::%d\">",
		       geom->Srid);
	  else
	      sprintf (buf,
		       "<gml:MultiLineString SrsName=\"EPSG::%d\">",
		       geom->Srid);
	  break;
      case GAIA_MULTIPOLYGON:
	  if (version == 3)
	      sprintf (buf, "<gml:MultiSurface SrsName=\"EPSG::%d\">",
		       geom->Srid);
	  else
	      sprintf (buf, "<gml:MultiPolygon SrsName=\"EPSG::%d\">",
		       geom->Srid);
	  break;
      default:
	  sprintf (buf, "<gml:MultiGeometry SrsName=\"EPSG::%d\">", geom->Srid);
	  break;
      };
    gaiaAppendToOutBuffer (out_buf, buf);
    point = geom->FirstPoint;
    while (point)
      {
	  /* processing POINT */
	  if (is_multi)
	    {
		strcpy (buf, "<gml:pointMember>");
		strcat (buf, "<gml:Point>");
	    }
	  else
	      sprintf (buf, "<gml:Point SrsName=\"EPSG::%d\">", geom->Srid);
	  if (version == 3)
	      strcat (buf, "<gml:pos>");
	  else
	      strcat (buf, "<gml:coordinates decimal=\".\" cs=\",\" ts=\" \">");
	  sprintf (buf_x, "%.*f", precision, point->X);
	  gaiaOutClean (buf_x);
	  sprintf (buf_y, "%.*f", precision, point->Y);
	  gaiaOutClean (buf_y);
	  if (point->DimensionModel == GAIA_XY_Z
	      || point->DimensionModel == GAIA_XY_Z_M)
	    {
		sprintf (buf_z, "%.*f", precision, point->Z);
		gaiaOutClean (buf_z);
		if (version == 3)
		  {
		      strcat (buf, buf_x);
		      strcat (buf, " ");
		      strcat (buf, buf_y);
		      strcat (buf, " ");
		      strcat (buf, buf_z);
		  }
		else
		  {
		      strcat (buf, buf_x);
		      strcat (buf, ",");
		      strcat (buf, buf_y);
		      strcat (buf, ",");
		      strcat (buf, buf_z);
		  }
	    }
	  else
	    {
		if (version == 3)
		  {
		      strcat (buf, buf_x);
		      strcat (buf, " ");
		      strcat (buf, buf_y);
		  }
		else
		  {
		      strcat (buf, buf_x);
		      strcat (buf, ",");
		      strcat (buf, buf_y);
		  }
	    }
	  if (version == 3)
	      strcat (buf, "</gml:pos>");
	  else
	      strcat (buf, "</gml:coordinates>");
	  if (is_multi)
	    {
		strcat (buf, "</gml:Point>");
		strcat (buf, "</gml:pointMember>");
	    }
	  else
	      strcat (buf, "</gml:Point>");
	  gaiaAppendToOutBuffer (out_buf, buf);
	  point = point->Next;
      }
    line = geom->FirstLinestring;
    while (line)
      {
	  /* processing LINESTRING */
	  if (is_multi)
	    {
		if (version == 3)
		  {
		      strcpy (buf, "<gml:curveMember>");
		      strcat (buf, "<gml:LineString>");
		      strcat (buf, "<gml:posList>");
		  }
		else
		  {
		      strcpy (buf, "<gml:lineStringMember>");
		      strcat (buf, "<gml:LineString>");
		      strcat (buf,
			      "<gml:coordinates decimal=\".\" cs=\",\" ts=\" \">");
		  }
	    }
	  else
	    {
		sprintf (buf, "<gml:LineString SrsName=\"EPSG::%d\">",
			 geom->Srid);
		if (version == 3)
		    strcat (buf, "<gml:posList>");
		else
		    strcat (buf,
			    "<gml:coordinates decimal=\".\" cs=\",\" ts=\" \">");
	    }
	  gaiaAppendToOutBuffer (out_buf, buf);
	  for (iv = 0; iv < line->Points; iv++)
	    {
		/* exporting vertices */
		has_z = 0;
		if (line->DimensionModel == GAIA_XY_Z)
		  {
		      has_z = 1;
		      gaiaGetPointXYZ (line->Coords, iv, &x, &y, &z);
		  }
		else if (line->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (line->Coords, iv, &x, &y, &m);
		  }
		else if (line->DimensionModel == GAIA_XY_Z_M)
		  {
		      has_z = 1;
		      gaiaGetPointXYZM (line->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (line->Coords, iv, &x, &y);
		  }
		if (iv == 0)
		    *buf = '\0';
		else
		    strcpy (buf, " ");
		if (has_z)
		  {
		      sprintf (buf_x, "%.*f", precision, x);
		      gaiaOutClean (buf_x);
		      sprintf (buf_y, "%.*f", precision, y);
		      gaiaOutClean (buf_y);
		      sprintf (buf_z, "%.*f", precision, z);
		      gaiaOutClean (buf_z);
		      if (version == 3)
			{
			    strcat (buf, buf_x);
			    strcat (buf, " ");
			    strcat (buf, buf_y);
			    strcat (buf, " ");
			    strcat (buf, buf_z);
			}
		      else
			{
			    strcat (buf, buf_x);
			    strcat (buf, ",");
			    strcat (buf, buf_y);
			    strcat (buf, ",");
			    strcat (buf, buf_z);
			}
		  }
		else
		  {
		      sprintf (buf_x, "%.*f", precision, x);
		      gaiaOutClean (buf_x);
		      sprintf (buf_y, "%.*f", precision, y);
		      gaiaOutClean (buf_y);
		      if (version == 3)
			{
			    strcat (buf, buf_x);
			    strcat (buf, " ");
			    strcat (buf, buf_y);
			}
		      else
			{
			    strcat (buf, buf_x);
			    strcat (buf, ",");
			    strcat (buf, buf_y);
			}
		  }
		gaiaAppendToOutBuffer (out_buf, buf);
	    }
	  if (is_multi)
	    {
		if (version == 3)
		  {
		      strcpy (buf, "</gml:posList>");
		      strcat (buf, "</gml:LineString>");
		      strcat (buf, "</gml:curveMember>");
		  }
		else
		  {
		      strcpy (buf, "</gml:coordinates>");
		      strcat (buf, "</gml:LineString>");
		      strcat (buf, "</gml:lineStringMember>");
		  }
	    }
	  else
	    {
		if (version == 3)
		    strcpy (buf, "</gml:posList>");
		else
		    strcpy (buf, "</gml:coordinates>");
		strcat (buf, "</gml:LineString>");
	    }
	  gaiaAppendToOutBuffer (out_buf, buf);
	  line = line->Next;
      }
    polyg = geom->FirstPolygon;
    while (polyg)
      {
	  /* processing POLYGON */
	  if (is_multi)
	    {
		if (version == 3)
		  {
		      strcpy (buf, "<gml:surfaceMember>");
		      strcat (buf, "<gml:Polygon>");
		      strcat (buf, "<gml:exterior>");
		      strcat (buf, "<gml:LinearRing>");
		      strcat (buf, "<gml:posList>");
		  }
		else
		  {
		      strcpy (buf, "<gml:polygonMember>");
		      strcat (buf, "<gml:Polygon>");
		      strcat (buf, "<gml:outerBoundaryIs>");
		      strcat (buf, "<gml:LinearRing>");
		      strcat (buf,
			      "<gml:coordinates decimal=\".\" cs=\",\" ts=\" \">");
		  }
	    }
	  else
	    {
		sprintf (buf, "<gml:Polygon SrsName=\"EPSG::%d\">", geom->Srid);
		if (version == 3)
		  {
		      strcat (buf, "<gml:exterior>");
		      strcat (buf, "<gml:LinearRing>");
		      strcat (buf, "<gml:posList>");
		  }
		else
		  {
		      strcat (buf, "<gml:outerBoundaryIs>");
		      strcat (buf, "<gml:LinearRing>");
		      strcat (buf,
			      "<gml:coordinates decimal=\".\" cs=\",\" ts=\" \">");
		  }
	    }
	  gaiaAppendToOutBuffer (out_buf, buf);
	  ring = polyg->Exterior;
	  for (iv = 0; iv < ring->Points; iv++)
	    {
		/* exporting vertices [Interior Ring] */
		has_z = 0;
		if (ring->DimensionModel == GAIA_XY_Z)
		  {
		      has_z = 1;
		      gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
		  }
		else if (ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
		  }
		else if (ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      has_z = 1;
		      gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (ring->Coords, iv, &x, &y);
		  }
		if (iv == 0)
		    *buf = '\0';
		else
		    strcpy (buf, " ");
		if (has_z)
		  {
		      sprintf (buf_x, "%.*f", precision, x);
		      gaiaOutClean (buf_x);
		      sprintf (buf_y, "%.*f", precision, y);
		      gaiaOutClean (buf_y);
		      sprintf (buf_z, "%.*f", precision, z);
		      gaiaOutClean (buf_z);
		      if (version == 3)
			{
			    strcat (buf, buf_x);
			    strcat (buf, " ");
			    strcat (buf, buf_y);
			    strcat (buf, " ");
			    strcat (buf, buf_z);
			}
		      else
			{
			    strcat (buf, buf_x);
			    strcat (buf, ",");
			    strcat (buf, buf_y);
			    strcat (buf, ",");
			    strcat (buf, buf_z);
			}
		  }
		else
		  {
		      sprintf (buf_x, "%.*f", precision, x);
		      gaiaOutClean (buf_x);
		      sprintf (buf_y, "%.*f", precision, y);
		      gaiaOutClean (buf_y);
		      if (version == 3)
			{
			    strcat (buf, buf_x);
			    strcat (buf, " ");
			    strcat (buf, buf_y);
			}
		      else
			{
			    strcat (buf, buf_x);
			    strcat (buf, ",");
			    strcat (buf, buf_y);
			}
		  }
		gaiaAppendToOutBuffer (out_buf, buf);
	    }
	  /* closing the Exterior Ring */
	  if (is_multi)
	    {
		if (version == 3)
		  {
		      strcpy (buf, "</gml:posList>");
		      strcat (buf, "</gml:LinearRing>");
		      strcat (buf, "</gml:exterior>");
		  }
		else
		  {
		      strcpy (buf, "</gml:coordinates>");
		      strcat (buf, "</gml:LinearRing>");
		      strcat (buf, "</gml:outerBoundaryIs>");
		  }
	    }
	  else
	    {
		if (version == 3)
		  {
		      strcpy (buf, "</gml:posList>");
		      strcat (buf, "</gml:LinearRing>");
		      strcat (buf, "</gml:exterior>");
		  }
		else
		  {
		      strcpy (buf, "</gml:coordinates>");
		      strcat (buf, "</gml:LinearRing>");
		      strcat (buf, "</gml:outerBoundaryIs>");
		  }
	    }
	  gaiaAppendToOutBuffer (out_buf, buf);
	  for (ib = 0; ib < polyg->NumInteriors; ib++)
	    {
		/* interior rings */
		ring = polyg->Interiors + ib;
		if (is_multi)
		  {
		      if (version == 3)
			{
			    strcpy (buf, "<gml:interior>");
			    strcat (buf, "<gml:LinearRing>");
			    strcat (buf, "<gml:posList>");
			}
		      else
			{
			    strcpy (buf, "<gml:innerBoundaryIs>");
			    strcat (buf, "<gml:LinearRing>");
			    strcat (buf,
				    "<gml:coordinates decimal=\".\" cs=\",\" ts=\" \">");
			}
		  }
		else
		  {
		      if (version == 3)
			{
			    strcpy (buf, "<gml:interior>");
			    strcat (buf, "<gml:LinearRing>");
			    strcat (buf, "<gml:posList>");
			}
		      else
			{
			    strcpy (buf, "<gml:innerBoundaryIs>");
			    strcat (buf, "<gml:LinearRing>");
			    strcat (buf,
				    "<gml:coordinates decimal=\".\" cs=\",\" ts=\" \">");
			}
		  }
		gaiaAppendToOutBuffer (out_buf, buf);
		for (iv = 0; iv < ring->Points; iv++)
		  {
		      /* exporting vertices [Interior Ring] */
		      has_z = 0;
		      if (ring->DimensionModel == GAIA_XY_Z)
			{
			    has_z = 1;
			    gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
			}
		      else if (ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
			}
		      else if (ring->DimensionModel == GAIA_XY_Z_M)
			{
			    has_z = 1;
			    gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
			}
		      else
			{
			    gaiaGetPoint (ring->Coords, iv, &x, &y);
			}
		      if (iv == 0)
			  *buf = '\0';
		      else
			  strcpy (buf, " ");
		      if (has_z)
			{
			    sprintf (buf_x, "%.*f", precision, x);
			    gaiaOutClean (buf_x);
			    sprintf (buf_y, "%.*f", precision, y);
			    gaiaOutClean (buf_y);
			    sprintf (buf_z, "%.*f", precision, z);
			    gaiaOutClean (buf_z);
			    if (version == 3)
			      {
				  strcat (buf, buf_x);
				  strcat (buf, " ");
				  strcat (buf, buf_y);
				  strcat (buf, " ");
				  strcat (buf, buf_z);
			      }
			    else
			      {
				  strcat (buf, buf_x);
				  strcat (buf, ",");
				  strcat (buf, buf_y);
				  strcat (buf, ",");
				  strcat (buf, buf_z);
			      }
			}
		      else
			{
			    sprintf (buf_x, "%.*f", precision, x);
			    gaiaOutClean (buf_x);
			    sprintf (buf_y, "%.*f", precision, y);
			    gaiaOutClean (buf_y);
			    if (version == 3)
			      {
				  strcat (buf, buf_x);
				  strcat (buf, " ");
				  strcat (buf, buf_y);
			      }
			    else
			      {
				  strcat (buf, buf_x);
				  strcat (buf, ",");
				  strcat (buf, buf_y);
			      }
			}
		      gaiaAppendToOutBuffer (out_buf, buf);
		  }
		/* closing the Interior Ring */
		if (is_multi)
		  {
		      if (version == 3)
			{
			    strcpy (buf, "</gml:posList>");
			    strcat (buf, "</gml:LinearRing>");
			    strcat (buf, "</gml:interior>");
			}
		      else
			{
			    strcpy (buf, "</gml:coordinates>");
			    strcat (buf, "</gml:LinearRing>");
			    strcat (buf, "</gml:innerBoundaryIs>");
			}
		  }
		else
		  {
		      if (version == 3)
			{
			    strcpy (buf, "</gml:posList>");
			    strcat (buf, "</gml:LinearRing>");
			    strcat (buf, "</gml:interior>");
			}
		      else
			{
			    strcpy (buf, "</gml:coordinates>");
			    strcat (buf, "</gml:LinearRing>");
			    strcat (buf, "</gml:innerBoundaryIs>");
			}
		  }
		gaiaAppendToOutBuffer (out_buf, buf);
	    }
	  /* closing the Polygon */
	  if (is_multi)
	    {
		if (version == 3)
		  {
		      strcpy (buf, "</gml:Polygon>");
		      strcat (buf, "</gml:curveMember>");
		  }
		else
		  {
		      strcpy (buf, "</gml:Polygon>");
		      strcat (buf, "</gml:polygonMember>");
		  }
	    }
	  else
	      strcpy (buf, "</gml:Polygon>");
	  gaiaAppendToOutBuffer (out_buf, buf);
	  polyg = polyg->Next;
      }
    switch (geom->DeclaredType)
      {
      case GAIA_POINT:
      case GAIA_LINESTRING:
      case GAIA_POLYGON:
	  *buf = '\0';
	  break;
      case GAIA_MULTIPOINT:
	  sprintf (buf, "</gml:MultiPoint>");
	  break;
      case GAIA_MULTILINESTRING:
	  if (version == 3)
	      sprintf (buf, "</gml:MultiCurve>");
	  else
	      sprintf (buf, "</gml:MultiLineString>");
	  break;
      case GAIA_MULTIPOLYGON:
	  if (version == 3)
	      sprintf (buf, "</gml:MultiSurface>");
	  else
	      sprintf (buf, "</gml:MultiPolygon>");
	  break;
      default:
	  sprintf (buf, "</gml:MultiGeometry>");
	  break;
      };
    gaiaAppendToOutBuffer (out_buf, buf);
}



int vanuatu_parse_error;

static gaiaGeomCollPtr
gaiaGeometryFromPoint (gaiaPointPtr point)
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
gaiaGeometryFromPointZ (gaiaPointPtr point)
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
gaiaGeometryFromPointM (gaiaPointPtr point)
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
gaiaGeometryFromPointZM (gaiaPointPtr point)
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
gaiaGeometryFromLinestring (gaiaLinestringPtr line)
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
gaiaGeometryFromLinestringZ (gaiaLinestringPtr line)
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
gaiaGeometryFromLinestringM (gaiaLinestringPtr line)
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
gaiaGeometryFromLinestringZM (gaiaLinestringPtr line)
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

/******************************************************************************
** The following code was created by Team Vanuatu of The University of Toronto.
** It is responsible for handling the parsing of wkt expressions.  The parser
** is built using LEMON and the cooresponding methods were written by the 
** students.

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

/* 
 * Creates a 2D (xy) point in SpatiaLite
 * x and y are pointers to doubles which represent the x and y coordinates of the point to be created.
 * Returns a gaiaPointPtr representing the created point.
 *
 * Creates a 2D (xy) point. This is a parser helper function which is called when 2D coordinates are encountered.
 * Parameters x and y are pointers to doubles which represent the x and y coordinates of the point to be created.
 * Returns a gaiaPointPtr pointing to the created 2D point.
 */
static gaiaPointPtr
vanuatu_point_xy (double *x, double *y)
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
vanuatu_point_xyz (double *x, double *y, double *z)
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
vanuatu_point_xym (double *x, double *y, double *m)
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
vanuatu_point_xyzm (double *x, double *y, double *z, double *m)
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
vanuatu_buildGeomFromPoint (gaiaPointPtr point)
{
    switch (point->DimensionModel)
      {
      case GAIA_XY:
	  return gaiaGeometryFromPoint (point);
	  break;
      case GAIA_XY_Z:
	  return gaiaGeometryFromPointZ (point);
	  break;
      case GAIA_XY_M:
	  return gaiaGeometryFromPointM (point);
	  break;
      case GAIA_XY_Z_M:
	  return gaiaGeometryFromPointZM (point);
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
vanuatu_linestring_xy (gaiaPointPtr first)
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
vanuatu_linestring_xyz (gaiaPointPtr first)
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
vanuatu_linestring_xym (gaiaPointPtr first)
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
vanuatu_linestring_xyzm (gaiaPointPtr first)
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
vanuatu_buildGeomFromLinestring (gaiaLinestringPtr line)
{
    switch (line->DimensionModel)
      {
      case GAIA_XY:
	  return gaiaGeometryFromLinestring (line);
	  break;
      case GAIA_XY_Z:
	  return gaiaGeometryFromLinestringZ (line);
	  break;
      case GAIA_XY_M:
	  return gaiaGeometryFromLinestringM (line);
	  break;
      case GAIA_XY_Z_M:
	  return gaiaGeometryFromLinestringZM (line);
	  break;
      }
    return NULL;
}

/*
 * Helper function that determines the number of points in the linked list.
 */
static int
count_points (gaiaPointPtr first)
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
vanuatu_ring_xy (gaiaPointPtr first)
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
    numpoints = count_points (first);
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
vanuatu_ring_xyz (gaiaPointPtr first)
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
    numpoints = count_points (first);
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
vanuatu_ring_xym (gaiaPointPtr first)
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
    numpoints = count_points (first);
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
vanuatu_ring_xyzm (gaiaPointPtr first)
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
    numpoints = count_points (first);
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
polygon_any_type (gaiaRingPtr first)
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
vanuatu_polygon_xy (gaiaRingPtr first)
{
    return polygon_any_type (first);
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
vanuatu_polygon_xyz (gaiaRingPtr first)
{
    return polygon_any_type (first);
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
vanuatu_polygon_xym (gaiaRingPtr first)
{
    return polygon_any_type (first);
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
vanuatu_polygon_xyzm (gaiaRingPtr first)
{
    return polygon_any_type (first);
}

/*
 * Builds a geometry collection from a polygon.
 * NOTE: This function may already be implemented in the SpatiaLite code base. If it is, make sure that we
 *              can use it (ie. it doesn't use any other variables or anything else set by Sandro's parser). If you find
 *              that we can use an existing function then ignore this one.
 */
static gaiaGeomCollPtr
buildGeomFromPolygon (gaiaPolygonPtr polygon)
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
vanuatu_multipoint_xy (gaiaPointPtr first)
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
vanuatu_multipoint_xyz (gaiaPointPtr first)
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
vanuatu_multipoint_xym (gaiaPointPtr first)
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
vanuatu_multipoint_xyzm (gaiaPointPtr first)
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
vanuatu_multilinestring_xy (gaiaLinestringPtr first)
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
vanuatu_multilinestring_xyz (gaiaLinestringPtr first)
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
vanuatu_multilinestring_xym (gaiaLinestringPtr first)
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
vanuatu_multilinestring_xyzm (gaiaLinestringPtr first)
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
vanuatu_multipolygon_xy (gaiaPolygonPtr first)
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
vanuatu_multipolygon_xyz (gaiaPolygonPtr first)
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
vanuatu_multipolygon_xym (gaiaPolygonPtr first)
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
vanuatu_multipolygon_xyzm (gaiaPolygonPtr first)
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
vanuatu_geomColl_common (gaiaGeomCollPtr org, gaiaGeomCollPtr dst)
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
vanuatu_geomColl_xy (gaiaGeomCollPtr first)
{
    gaiaGeomCollPtr geom = gaiaAllocGeomColl ();
    if (geom == NULL)
	return NULL;
    geom->DeclaredType = GAIA_GEOMETRYCOLLECTION;
    geom->DimensionModel = GAIA_XY;
    vanuatu_geomColl_common (first, geom);
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
vanuatu_geomColl_xyz (gaiaGeomCollPtr first)
{
    gaiaGeomCollPtr geom = gaiaAllocGeomColl ();
    if (geom == NULL)
	return NULL;
    geom->DeclaredType = GAIA_GEOMETRYCOLLECTION;
    geom->DimensionModel = GAIA_XY_Z;
    vanuatu_geomColl_common (first, geom);
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
vanuatu_geomColl_xym (gaiaGeomCollPtr first)
{
    gaiaGeomCollPtr geom = gaiaAllocGeomColl ();
    if (geom == NULL)
	return NULL;
    geom->DeclaredType = GAIA_GEOMETRYCOLLECTION;
    geom->DimensionModel = GAIA_XY_M;
    vanuatu_geomColl_common (first, geom);
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
vanuatu_geomColl_xyzm (gaiaGeomCollPtr first)
{
    gaiaGeomCollPtr geom = gaiaAllocGeomColl ();
    if (geom == NULL)
	return NULL;
    geom->DeclaredType = GAIA_GEOMETRYCOLLECTION;
    geom->DimensionModel = GAIA_XY_Z_M;
    vanuatu_geomColl_common (first, geom);
    return geom;
}








/*
 VANUATU_LEMON_H_START - LEMON generated header starts here 
*/

#define VANUATU_NEWLINE                 1
#define VANUATU_POINT                   2
#define VANUATU_OPEN_BRACKET            3
#define VANUATU_CLOSE_BRACKET           4
#define VANUATU_POINT_M                 5
#define VANUATU_POINT_Z                 6
#define VANUATU_POINT_ZM                7
#define VANUATU_NUM                     8
#define VANUATU_COMMA                   9
#define VANUATU_LINESTRING             10
#define VANUATU_LINESTRING_M           11
#define VANUATU_LINESTRING_Z           12
#define VANUATU_LINESTRING_ZM          13
#define VANUATU_POLYGON                14
#define VANUATU_POLYGON_M              15
#define VANUATU_POLYGON_Z              16
#define VANUATU_POLYGON_ZM             17
#define VANUATU_MULTIPOINT             18
#define VANUATU_MULTIPOINT_M           19
#define VANUATU_MULTIPOINT_Z           20
#define VANUATU_MULTIPOINT_ZM          21
#define VANUATU_MULTILINESTRING        22
#define VANUATU_MULTILINESTRING_M      23
#define VANUATU_MULTILINESTRING_Z      24
#define VANUATU_MULTILINESTRING_ZM     25
#define VANUATU_MULTIPOLYGON           26
#define VANUATU_MULTIPOLYGON_M         27
#define VANUATU_MULTIPOLYGON_Z         28
#define VANUATU_MULTIPOLYGON_ZM        29
#define VANUATU_GEOMETRYCOLLECTION     30
#define VANUATU_GEOMETRYCOLLECTION_M   31
#define VANUATU_GEOMETRYCOLLECTION_Z   32
#define VANUATU_GEOMETRYCOLLECTION_ZM  33

/*
 VANUATU_LEMON_H_END - LEMON generated header ends here 
*/









#ifndef YYSTYPE
typedef union
{
    double dval;
    struct symtab *symp;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif


/* extern YYSTYPE yylval; */
YYSTYPE VanuatuWktlval;

/*
** CAVEAT: there is an incompatibility between LEMON and FLEX
** this macro resolves the issue
*/
#define yy_accept	yy_lemon_accept










/*
 VANUATU_LEMON_START - LEMON generated header starts here 
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
#define YYNOCODE 125
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE void *
typedef union
{
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
#define YYNSTATE 358
#define YYNRULE 153
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
    /*     0 */ 166, 228, 229, 230, 231, 232, 233, 234, 235, 236,
    /*    10 */ 237, 238, 239, 240, 241, 242, 243, 244, 245, 246,
    /*    20 */ 247, 248, 249, 250, 251, 252, 253, 254, 255, 256,
    /*    30 */ 257, 258, 259, 260, 358, 263, 167, 512, 1, 169,
    /*    40 */ 171, 173, 174, 51, 54, 57, 60, 63, 66, 72,
    /*    50 */ 78, 84, 90, 92, 94, 96, 98, 103, 108, 113,
    /*    60 */ 118, 123, 128, 133, 138, 145, 152, 159, 167, 139,
    /*    70 */ 143, 144, 140, 141, 142, 169, 54, 146, 150, 151,
    /*    80 */ 66, 57, 147, 148, 149, 72, 171, 153, 157, 158,
    /*    90 */ 170, 262, 60, 47, 173, 270, 78, 154, 155, 156,
    /*   100 */ 63, 172, 273, 49, 84, 160, 164, 165, 48, 161,
    /*   110 */ 162, 163, 14, 168, 175, 55, 176, 46, 46, 46,
    /*   120 */ 56, 265, 177, 58, 46, 47, 47, 59, 50, 179,
    /*   130 */ 47, 49, 61, 62, 49, 49, 181, 51, 64, 51,
    /*   140 */ 184, 65, 51, 185, 46, 186, 70, 46, 189, 46,
    /*   150 */ 46, 47, 267, 190, 191, 76, 47, 47, 47, 194,
    /*   160 */ 52, 49, 195, 196, 49, 49, 82, 53, 49, 199,
    /*   170 */ 51, 200, 51, 201, 51, 88, 51, 91, 95, 93,
    /*   180 */ 49, 46, 47, 269, 97, 51, 16, 271, 17, 19,
    /*   190 */ 178, 274, 20, 22, 276, 180, 277, 23, 279, 25,
    /*   200 */ 182, 280, 67, 282, 26, 69, 187, 73, 68, 286,
    /*   210 */ 30, 75, 183, 71, 192, 285, 79, 74, 290, 188,
    /*   220 */ 289, 77, 34, 81, 197, 80, 85, 193, 83, 294,
    /*   230 */ 38, 293, 202, 86, 87, 198, 297, 89, 42, 203,
    /*   240 */ 298, 43, 300, 204, 44, 302, 205, 45, 304, 206,
    /*   250 */ 99, 306, 100, 102, 101, 104, 207, 309, 308, 105,
    /*   260 */ 106, 109, 208, 107, 311, 110, 312, 209, 111, 112,
    /*   270 */ 114, 314, 315, 116, 115, 117, 119, 120, 210, 121,
    /*   280 */ 124, 317, 122, 318, 126, 125, 129, 131, 211, 130,
    /*   290 */ 321, 320, 132, 127, 134, 136, 2, 135, 3, 212,
    /*   300 */ 4, 323, 137, 5, 6, 324, 8, 7, 9, 227,
    /*   310 */ 10, 213, 513, 261, 11, 15, 264, 12, 13, 326,
    /*   320 */ 327, 266, 268, 272, 275, 18, 214, 329, 278, 281,
    /*   330 */ 21, 24, 330, 283, 513, 27, 28, 215, 332, 29,
    /*   340 */ 333, 334, 216, 337, 217, 284, 287, 31, 32, 218,
    /*   350 */ 339, 340, 341, 219, 220, 344, 221, 33, 346, 288,
    /*   360 */ 291, 347, 348, 35, 222, 223, 36, 351, 37, 224,
    /*   370 */ 292, 295, 353, 354, 39, 513, 355, 225, 226, 40,
    /*   380 */ 41, 296, 299, 301, 303, 305, 307, 310, 313, 316,
    /*   390 */ 319, 322, 325, 328, 331, 335, 336, 338, 342, 343,
    /*   400 */ 345, 349, 350, 352, 356, 357,
};

static const YYCODETYPE yy_lookahead[] = {
    /*     0 */ 37, 38, 39, 40, 41, 42, 43, 44, 45, 46,
    /*    10 */ 47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
    /*    20 */ 57, 58, 59, 60, 61, 62, 63, 64, 65, 66,
    /*    30 */ 67, 68, 69, 70, 0, 8, 2, 35, 36, 5,
    /*    40 */ 6, 7, 74, 75, 10, 11, 12, 13, 14, 15,
    /*    50 */ 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
    /*    60 */ 26, 27, 28, 29, 30, 31, 32, 33, 2, 43,
    /*    70 */ 44, 45, 43, 44, 45, 5, 10, 57, 58, 59,
    /*    80 */ 14, 11, 57, 58, 59, 15, 6, 50, 51, 52,
    /*    90 */ 72, 75, 12, 75, 7, 80, 16, 50, 51, 52,
    /*   100 */ 13, 73, 81, 75, 17, 64, 65, 66, 75, 64,
    /*   110 */ 65, 66, 3, 71, 71, 71, 76, 75, 75, 75,
    /*   120 */ 71, 75, 72, 72, 75, 75, 75, 72, 75, 73,
    /*   130 */ 75, 75, 73, 73, 75, 75, 74, 75, 74, 75,
    /*   140 */ 71, 74, 75, 71, 75, 71, 71, 75, 72, 75,
    /*   150 */ 75, 75, 75, 72, 72, 72, 75, 75, 75, 73,
    /*   160 */ 75, 75, 73, 73, 75, 75, 73, 75, 75, 74,
    /*   170 */ 75, 74, 75, 74, 75, 74, 75, 71, 73, 72,
    /*   180 */ 75, 75, 75, 75, 74, 75, 9, 76, 3, 9,
    /*   190 */ 77, 77, 3, 9, 82, 78, 78, 3, 83, 9,
    /*   200 */ 79, 79, 3, 84, 3, 9, 76, 3, 88, 85,
    /*   210 */ 3, 9, 89, 88, 77, 89, 3, 90, 86, 91,
    /*   220 */ 91, 90, 3, 9, 78, 92, 3, 93, 92, 87,
    /*   230 */ 3, 93, 79, 94, 9, 95, 95, 94, 3, 76,
    /*   240 */ 96, 3, 97, 77, 3, 98, 78, 3, 99, 79,
    /*   250 */ 3, 100, 80, 80, 9, 3, 104, 101, 104, 81,
    /*   260 */ 9, 3, 105, 81, 105, 82, 102, 106, 9, 82,
    /*   270 */ 3, 106, 103, 9, 83, 83, 3, 84, 107, 9,
    /*   280 */ 3, 107, 84, 108, 9, 85, 3, 9, 112, 86,
    /*   290 */ 109, 112, 86, 85, 3, 9, 3, 87, 9, 113,
    /*   300 */ 3, 113, 87, 9, 3, 110, 3, 9, 9, 1,
    /*   310 */ 3, 114, 124, 4, 3, 9, 4, 3, 3, 114,
    /*   320 */ 111, 4, 4, 4, 4, 9, 115, 115, 4, 4,
    /*   330 */ 9, 9, 116, 4, 124, 9, 9, 120, 120, 9,
    /*   340 */ 120, 120, 120, 117, 120, 4, 4, 9, 9, 121,
    /*   350 */ 121, 121, 121, 121, 121, 118, 122, 9, 122, 4,
    /*   360 */ 4, 122, 122, 9, 122, 122, 9, 119, 9, 123,
    /*   370 */ 4, 4, 123, 123, 9, 124, 123, 123, 123, 9,
    /*   380 */ 9, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    /*   390 */ 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    /*   400 */ 4, 4, 4, 4, 4, 4,
};

#define YY_SHIFT_USE_DFLT (-1)
#define YY_SHIFT_MAX 226
static const short yy_shift_ofst[] = {
    /*     0 */ -1, 34, 66, 66, 70, 70, 80, 80, 87, 87,
    /*    10 */ 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    /*    20 */ 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    /*    30 */ 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    /*    40 */ 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    /*    50 */ 27, 27, 27, 27, 109, 177, 177, 185, 180, 180,
    /*    60 */ 189, 184, 184, 194, 190, 190, 199, 201, 196, 201,
    /*    70 */ 177, 196, 204, 207, 202, 207, 180, 202, 213, 219,
    /*    80 */ 214, 219, 184, 214, 223, 227, 225, 227, 190, 225,
    /*    90 */ 235, 177, 238, 180, 241, 184, 244, 190, 247, 109,
    /*   100 */ 245, 109, 245, 252, 185, 251, 185, 251, 258, 189,
    /*   110 */ 259, 189, 259, 267, 194, 264, 194, 264, 273, 199,
    /*   120 */ 270, 199, 270, 277, 204, 275, 204, 275, 283, 213,
    /*   130 */ 278, 213, 278, 291, 223, 286, 223, 286, 293, 289,
    /*   140 */ 289, 289, 289, 289, 289, 297, 294, 294, 294, 294,
    /*   150 */ 294, 294, 301, 298, 298, 298, 298, 298, 298, 303,
    /*   160 */ 299, 299, 299, 299, 299, 299, 308, 307, 309, 311,
    /*   170 */ 312, 314, 317, 315, 318, 306, 319, 316, 320, 321,
    /*   180 */ 324, 322, 325, 329, 326, 327, 330, 341, 342, 338,
    /*   190 */ 339, 348, 355, 356, 354, 357, 359, 366, 367, 365,
    /*   200 */ 370, 371, 377, 378, 379, 380, 381, 382, 383, 384,
    /*   210 */ 385, 386, 387, 388, 389, 390, 391, 392, 393, 394,
    /*   220 */ 395, 396, 397, 398, 399, 400, 401,
};

#define YY_REDUCE_USE_DFLT (-38)
#define YY_REDUCE_MAX 165
static const short yy_reduce_ofst[] = {
    /*     0 */ 2, -37, 26, 29, 20, 25, 37, 47, 41, 45,
    /*    10 */ 42, 18, 28, -32, 43, 44, 49, 50, 51, 55,
    /*    20 */ 56, 59, 60, 62, 64, 67, 69, 72, 74, 75,
    /*    30 */ 76, 81, 82, 83, 86, 89, 90, 93, 95, 97,
    /*    40 */ 99, 101, 106, 107, 105, 110, 16, 33, 46, 53,
    /*    50 */ 77, 85, 92, 108, 15, 40, 111, 21, 113, 114,
    /*    60 */ 112, 117, 118, 115, 121, 122, 119, 120, 123, 125,
    /*    70 */ 130, 126, 124, 127, 128, 131, 137, 129, 132, 133,
    /*    80 */ 134, 136, 146, 138, 142, 139, 140, 143, 153, 141,
    /*    90 */ 144, 163, 145, 166, 147, 168, 149, 170, 151, 172,
    /*   100 */ 152, 173, 154, 156, 178, 157, 182, 159, 164, 183,
    /*   110 */ 161, 187, 165, 169, 191, 171, 192, 174, 175, 193,
    /*   120 */ 176, 198, 179, 181, 200, 186, 208, 188, 195, 203,
    /*   130 */ 197, 206, 205, 209, 210, 211, 215, 212, 216, 217,
    /*   140 */ 218, 220, 221, 222, 224, 226, 228, 229, 230, 231,
    /*   150 */ 232, 233, 237, 234, 236, 239, 240, 242, 243, 248,
    /*   160 */ 246, 249, 250, 253, 254, 255,
};

static const YYACTIONTYPE yy_default[] = {
    /*     0 */ 359, 511, 511, 511, 511, 511, 511, 511, 511, 511,
    /*    10 */ 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
    /*    20 */ 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
    /*    30 */ 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
    /*    40 */ 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
    /*    50 */ 511, 511, 511, 511, 511, 403, 403, 511, 405, 405,
    /*    60 */ 511, 407, 407, 511, 409, 409, 511, 511, 428, 511,
    /*    70 */ 403, 428, 511, 511, 431, 511, 405, 431, 511, 511,
    /*    80 */ 434, 511, 407, 434, 511, 511, 437, 511, 409, 437,
    /*    90 */ 511, 403, 511, 405, 511, 407, 511, 409, 511, 511,
    /*   100 */ 452, 511, 452, 511, 511, 455, 511, 455, 511, 511,
    /*   110 */ 458, 511, 458, 511, 511, 461, 511, 461, 511, 511,
    /*   120 */ 468, 511, 468, 511, 511, 471, 511, 471, 511, 511,
    /*   130 */ 474, 511, 474, 511, 511, 477, 511, 477, 511, 486,
    /*   140 */ 486, 486, 486, 486, 486, 511, 493, 493, 493, 493,
    /*   150 */ 493, 493, 511, 500, 500, 500, 500, 500, 500, 511,
    /*   160 */ 507, 507, 507, 507, 507, 507, 511, 511, 511, 511,
    /*   170 */ 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
    /*   180 */ 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
    /*   190 */ 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
    /*   200 */ 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
    /*   210 */ 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
    /*   220 */ 511, 511, 511, 511, 511, 511, 511, 360, 361, 362,
    /*   230 */ 363, 364, 365, 366, 367, 368, 369, 370, 371, 372,
    /*   240 */ 373, 374, 375, 376, 377, 378, 379, 380, 381, 382,
    /*   250 */ 383, 384, 385, 386, 387, 388, 389, 390, 391, 392,
    /*   260 */ 393, 394, 398, 402, 395, 399, 396, 400, 397, 401,
    /*   270 */ 411, 404, 415, 412, 406, 416, 413, 408, 417, 414,
    /*   280 */ 410, 418, 419, 423, 427, 429, 420, 424, 430, 432,
    /*   290 */ 421, 425, 433, 435, 422, 426, 436, 438, 439, 443,
    /*   300 */ 440, 444, 441, 445, 442, 446, 447, 451, 453, 448,
    /*   310 */ 454, 456, 449, 457, 459, 450, 460, 462, 463, 467,
    /*   320 */ 469, 464, 470, 472, 465, 473, 475, 466, 476, 478,
    /*   330 */ 479, 483, 487, 488, 489, 484, 485, 480, 490, 494,
    /*   340 */ 495, 496, 491, 492, 481, 497, 501, 502, 503, 498,
    /*   350 */ 499, 482, 504, 508, 509, 510, 505, 506,
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
struct yyStackEntry
{
    YYACTIONTYPE stateno;	/* The state-number */
    YYCODETYPE major;		/* The major token value.  This is the code
				 ** number for the token at this stack level */
    YYMINORTYPE minor;		/* The user-supplied minor token value.  This
				 ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser
{
    int yyidx;			/* Index of top element in stack */
#ifdef YYTRACKMAXSTACKDEPTH
    int yyidxMax;		/* Maximum value of yyidx */
#endif
    int yyerrcnt;		/* Shifts left before out of the error */
      ParseARG_SDECL		/* A place to hold %extra_argument */
#if YYSTACKDEPTH<=0
    int yystksz;		/* Current side of the stack */
    yyStackEntry *yystack;	/* The parser's stack */
#else
      yyStackEntry yystack[YYSTACKDEPTH];	/* The parser's stack */
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
void
ParseTrace (FILE * TraceFILE, char *zTracePrompt)
{
    yyTraceFILE = TraceFILE;
    yyTracePrompt = zTracePrompt;
    if (yyTraceFILE == 0)
	yyTracePrompt = 0;
    else if (yyTracePrompt == 0)
	yyTraceFILE = 0;
}
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = {
    "$", "VANUATU_NEWLINE", "VANUATU_POINT", "VANUATU_OPEN_BRACKET",
    "VANUATU_CLOSE_BRACKET", "VANUATU_POINT_M", "VANUATU_POINT_Z",
    "VANUATU_POINT_ZM",
    "VANUATU_NUM", "VANUATU_COMMA", "VANUATU_LINESTRING",
    "VANUATU_LINESTRING_M",
    "VANUATU_LINESTRING_Z", "VANUATU_LINESTRING_ZM", "VANUATU_POLYGON",
    "VANUATU_POLYGON_M",
    "VANUATU_POLYGON_Z", "VANUATU_POLYGON_ZM", "VANUATU_MULTIPOINT",
    "VANUATU_MULTIPOINT_M",
    "VANUATU_MULTIPOINT_Z", "VANUATU_MULTIPOINT_ZM", "VANUATU_MULTILINESTRING",
    "VANUATU_MULTILINESTRING_M",
    "VANUATU_MULTILINESTRING_Z", "VANUATU_MULTILINESTRING_ZM",
    "VANUATU_MULTIPOLYGON", "VANUATU_MULTIPOLYGON_M",
    "VANUATU_MULTIPOLYGON_Z", "VANUATU_MULTIPOLYGON_ZM",
    "VANUATU_GEOMETRYCOLLECTION", "VANUATU_GEOMETRYCOLLECTION_M",
    "VANUATU_GEOMETRYCOLLECTION_Z", "VANUATU_GEOMETRYCOLLECTION_ZM", "error",
    "main",
    "in", "state", "program", "geo_text",
    "geo_textz", "geo_textm", "geo_textzm", "point",
    "linestring", "polygon", "multipoint", "multilinestring",
    "multipolygon", "geocoll", "pointz", "linestringz",
    "polygonz", "multipointz", "multilinestringz", "multipolygonz",
    "geocollz", "pointm", "linestringm", "polygonm",
    "multipointm", "multilinestringm", "multipolygonm", "geocollm",
    "pointzm", "linestringzm", "polygonzm", "multipointzm",
    "multilinestringzm", "multipolygonzm", "geocollzm", "point_coordxy",
    "point_coordxym", "point_coordxyz", "point_coordxyzm", "coord",
    "extra_pointsxy", "extra_pointsxym", "extra_pointsxyz", "extra_pointsxyzm",
    "linestring_text", "linestring_textm", "linestring_textz",
    "linestring_textzm",
    "polygon_text", "polygon_textm", "polygon_textz", "polygon_textzm",
    "ring", "extra_rings", "ringm", "extra_ringsm",
    "ringz", "extra_ringsz", "ringzm", "extra_ringszm",
    "multipoint_text", "multipoint_textm", "multipoint_textz",
    "multipoint_textzm",
    "multilinestring_text", "multilinestring_textm", "multilinestring_textz",
    "multilinestring_textzm",
    "multilinestring_text2", "multilinestring_textm2", "multilinestring_textz2",
    "multilinestring_textzm2",
    "multipolygon_text", "multipolygon_textm", "multipolygon_textz",
    "multipolygon_textzm",
    "multipolygon_text2", "multipolygon_textm2", "multipolygon_textz2",
    "multipolygon_textzm2",
    "geocoll_text", "geocoll_textm", "geocoll_textz", "geocoll_textzm",
    "geocoll_text2", "geocoll_textm2", "geocoll_textz2", "geocoll_textzm2",
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
    /*   0 */ "main ::= in",
    /*   1 */ "in ::=",
    /*   2 */ "in ::= in state VANUATU_NEWLINE",
    /*   3 */ "state ::= program",
    /*   4 */ "program ::= geo_text",
    /*   5 */ "program ::= geo_textz",
    /*   6 */ "program ::= geo_textm",
    /*   7 */ "program ::= geo_textzm",
    /*   8 */ "geo_text ::= point",
    /*   9 */ "geo_text ::= linestring",
    /*  10 */ "geo_text ::= polygon",
    /*  11 */ "geo_text ::= multipoint",
    /*  12 */ "geo_text ::= multilinestring",
    /*  13 */ "geo_text ::= multipolygon",
    /*  14 */ "geo_text ::= geocoll",
    /*  15 */ "geo_textz ::= pointz",
    /*  16 */ "geo_textz ::= linestringz",
    /*  17 */ "geo_textz ::= polygonz",
    /*  18 */ "geo_textz ::= multipointz",
    /*  19 */ "geo_textz ::= multilinestringz",
    /*  20 */ "geo_textz ::= multipolygonz",
    /*  21 */ "geo_textz ::= geocollz",
    /*  22 */ "geo_textm ::= pointm",
    /*  23 */ "geo_textm ::= linestringm",
    /*  24 */ "geo_textm ::= polygonm",
    /*  25 */ "geo_textm ::= multipointm",
    /*  26 */ "geo_textm ::= multilinestringm",
    /*  27 */ "geo_textm ::= multipolygonm",
    /*  28 */ "geo_textm ::= geocollm",
    /*  29 */ "geo_textzm ::= pointzm",
    /*  30 */ "geo_textzm ::= linestringzm",
    /*  31 */ "geo_textzm ::= polygonzm",
    /*  32 */ "geo_textzm ::= multipointzm",
    /*  33 */ "geo_textzm ::= multilinestringzm",
    /*  34 */ "geo_textzm ::= multipolygonzm",
    /*  35 */ "geo_textzm ::= geocollzm",
    /*  36 */
    "point ::= VANUATU_POINT VANUATU_OPEN_BRACKET point_coordxy VANUATU_CLOSE_BRACKET",
    /*  37 */
    "pointm ::= VANUATU_POINT_M VANUATU_OPEN_BRACKET point_coordxym VANUATU_CLOSE_BRACKET",
    /*  38 */
    "pointz ::= VANUATU_POINT_Z VANUATU_OPEN_BRACKET point_coordxyz VANUATU_CLOSE_BRACKET",
    /*  39 */
    "pointzm ::= VANUATU_POINT_ZM VANUATU_OPEN_BRACKET point_coordxyzm VANUATU_CLOSE_BRACKET",
    /*  40 */ "point_coordxy ::= coord coord",
    /*  41 */ "point_coordxym ::= coord coord coord",
    /*  42 */ "point_coordxyz ::= coord coord coord",
    /*  43 */ "point_coordxyzm ::= coord coord coord coord",
    /*  44 */ "coord ::= VANUATU_NUM",
    /*  45 */ "extra_pointsxy ::=",
    /*  46 */ "extra_pointsxy ::= VANUATU_COMMA point_coordxy extra_pointsxy",
    /*  47 */ "extra_pointsxym ::=",
    /*  48 */
    "extra_pointsxym ::= VANUATU_COMMA point_coordxym extra_pointsxym",
    /*  49 */ "extra_pointsxyz ::=",
    /*  50 */
    "extra_pointsxyz ::= VANUATU_COMMA point_coordxyz extra_pointsxyz",
    /*  51 */ "extra_pointsxyzm ::=",
    /*  52 */
    "extra_pointsxyzm ::= VANUATU_COMMA point_coordxyzm extra_pointsxyzm",
    /*  53 */ "linestring ::= VANUATU_LINESTRING linestring_text",
    /*  54 */ "linestringm ::= VANUATU_LINESTRING_M linestring_textm",
    /*  55 */ "linestringz ::= VANUATU_LINESTRING_Z linestring_textz",
    /*  56 */ "linestringzm ::= VANUATU_LINESTRING_ZM linestring_textzm",
    /*  57 */
    "linestring_text ::= VANUATU_OPEN_BRACKET point_coordxy VANUATU_COMMA point_coordxy extra_pointsxy VANUATU_CLOSE_BRACKET",
    /*  58 */
    "linestring_textm ::= VANUATU_OPEN_BRACKET point_coordxym VANUATU_COMMA point_coordxym extra_pointsxym VANUATU_CLOSE_BRACKET",
    /*  59 */
    "linestring_textz ::= VANUATU_OPEN_BRACKET point_coordxyz VANUATU_COMMA point_coordxyz extra_pointsxyz VANUATU_CLOSE_BRACKET",
    /*  60 */
    "linestring_textzm ::= VANUATU_OPEN_BRACKET point_coordxyzm VANUATU_COMMA point_coordxyzm extra_pointsxyzm VANUATU_CLOSE_BRACKET",
    /*  61 */ "polygon ::= VANUATU_POLYGON polygon_text",
    /*  62 */ "polygonm ::= VANUATU_POLYGON_M polygon_textm",
    /*  63 */ "polygonz ::= VANUATU_POLYGON_Z polygon_textz",
    /*  64 */ "polygonzm ::= VANUATU_POLYGON_ZM polygon_textzm",
    /*  65 */
    "polygon_text ::= VANUATU_OPEN_BRACKET ring extra_rings VANUATU_CLOSE_BRACKET",
    /*  66 */
    "polygon_textm ::= VANUATU_OPEN_BRACKET ringm extra_ringsm VANUATU_CLOSE_BRACKET",
    /*  67 */
    "polygon_textz ::= VANUATU_OPEN_BRACKET ringz extra_ringsz VANUATU_CLOSE_BRACKET",
    /*  68 */
    "polygon_textzm ::= VANUATU_OPEN_BRACKET ringzm extra_ringszm VANUATU_CLOSE_BRACKET",
    /*  69 */
    "ring ::= VANUATU_OPEN_BRACKET point_coordxy VANUATU_COMMA point_coordxy VANUATU_COMMA point_coordxy VANUATU_COMMA point_coordxy extra_pointsxy VANUATU_CLOSE_BRACKET",
    /*  70 */ "extra_rings ::=",
    /*  71 */ "extra_rings ::= VANUATU_COMMA ring extra_rings",
    /*  72 */
    "ringm ::= VANUATU_OPEN_BRACKET point_coordxym VANUATU_COMMA point_coordxym VANUATU_COMMA point_coordxym VANUATU_COMMA point_coordxym extra_pointsxym VANUATU_CLOSE_BRACKET",
    /*  73 */ "extra_ringsm ::=",
    /*  74 */ "extra_ringsm ::= VANUATU_COMMA ringm extra_ringsm",
    /*  75 */
    "ringz ::= VANUATU_OPEN_BRACKET point_coordxyz VANUATU_COMMA point_coordxyz VANUATU_COMMA point_coordxyz VANUATU_COMMA point_coordxyz extra_pointsxyz VANUATU_CLOSE_BRACKET",
    /*  76 */ "extra_ringsz ::=",
    /*  77 */ "extra_ringsz ::= VANUATU_COMMA ringz extra_ringsz",
    /*  78 */
    "ringzm ::= VANUATU_OPEN_BRACKET point_coordxyzm VANUATU_COMMA point_coordxyzm VANUATU_COMMA point_coordxyzm VANUATU_COMMA point_coordxyzm extra_pointsxyzm VANUATU_CLOSE_BRACKET",
    /*  79 */ "extra_ringszm ::=",
    /*  80 */ "extra_ringszm ::= VANUATU_COMMA ringzm extra_ringszm",
    /*  81 */ "multipoint ::= VANUATU_MULTIPOINT multipoint_text",
    /*  82 */ "multipointm ::= VANUATU_MULTIPOINT_M multipoint_textm",
    /*  83 */ "multipointz ::= VANUATU_MULTIPOINT_Z multipoint_textz",
    /*  84 */ "multipointzm ::= VANUATU_MULTIPOINT_ZM multipoint_textzm",
    /*  85 */
    "multipoint_text ::= VANUATU_OPEN_BRACKET point_coordxy extra_pointsxy VANUATU_CLOSE_BRACKET",
    /*  86 */
    "multipoint_textm ::= VANUATU_OPEN_BRACKET point_coordxym extra_pointsxym VANUATU_CLOSE_BRACKET",
    /*  87 */
    "multipoint_textz ::= VANUATU_OPEN_BRACKET point_coordxyz extra_pointsxyz VANUATU_CLOSE_BRACKET",
    /*  88 */
    "multipoint_textzm ::= VANUATU_OPEN_BRACKET point_coordxyzm extra_pointsxyzm VANUATU_CLOSE_BRACKET",
    /*  89 */
    "multilinestring ::= VANUATU_MULTILINESTRING multilinestring_text",
    /*  90 */
    "multilinestringm ::= VANUATU_MULTILINESTRING_M multilinestring_textm",
    /*  91 */
    "multilinestringz ::= VANUATU_MULTILINESTRING_Z multilinestring_textz",
    /*  92 */
    "multilinestringzm ::= VANUATU_MULTILINESTRING_ZM multilinestring_textzm",
    /*  93 */
    "multilinestring_text ::= VANUATU_OPEN_BRACKET linestring_text multilinestring_text2 VANUATU_CLOSE_BRACKET",
    /*  94 */ "multilinestring_text2 ::=",
    /*  95 */
    "multilinestring_text2 ::= VANUATU_COMMA linestring_text multilinestring_text2",
    /*  96 */
    "multilinestring_textm ::= VANUATU_OPEN_BRACKET linestring_textm multilinestring_textm2 VANUATU_CLOSE_BRACKET",
    /*  97 */ "multilinestring_textm2 ::=",
    /*  98 */
    "multilinestring_textm2 ::= VANUATU_COMMA linestring_textm multilinestring_textm2",
    /*  99 */
    "multilinestring_textz ::= VANUATU_OPEN_BRACKET linestring_textz multilinestring_textz2 VANUATU_CLOSE_BRACKET",
    /* 100 */ "multilinestring_textz2 ::=",
    /* 101 */
    "multilinestring_textz2 ::= VANUATU_COMMA linestring_textz multilinestring_textz2",
    /* 102 */
    "multilinestring_textzm ::= VANUATU_OPEN_BRACKET linestring_textzm multilinestring_textzm2 VANUATU_CLOSE_BRACKET",
    /* 103 */ "multilinestring_textzm2 ::=",
    /* 104 */
    "multilinestring_textzm2 ::= VANUATU_COMMA linestring_textzm multilinestring_textzm2",
    /* 105 */ "multipolygon ::= VANUATU_MULTIPOLYGON multipolygon_text",
    /* 106 */ "multipolygonm ::= VANUATU_MULTIPOLYGON_M multipolygon_textm",
    /* 107 */ "multipolygonz ::= VANUATU_MULTIPOLYGON_Z multipolygon_textz",
    /* 108 */ "multipolygonzm ::= VANUATU_MULTIPOLYGON_ZM multipolygon_textzm",
    /* 109 */
    "multipolygon_text ::= VANUATU_OPEN_BRACKET polygon_text multipolygon_text2 VANUATU_CLOSE_BRACKET",
    /* 110 */ "multipolygon_text2 ::=",
    /* 111 */
    "multipolygon_text2 ::= VANUATU_COMMA polygon_text multipolygon_text2",
    /* 112 */
    "multipolygon_textm ::= VANUATU_OPEN_BRACKET polygon_textm multipolygon_textm2 VANUATU_CLOSE_BRACKET",
    /* 113 */ "multipolygon_textm2 ::=",
    /* 114 */
    "multipolygon_textm2 ::= VANUATU_COMMA polygon_textm multipolygon_textm2",
    /* 115 */
    "multipolygon_textz ::= VANUATU_OPEN_BRACKET polygon_textz multipolygon_textz2 VANUATU_CLOSE_BRACKET",
    /* 116 */ "multipolygon_textz2 ::=",
    /* 117 */
    "multipolygon_textz2 ::= VANUATU_COMMA polygon_textz multipolygon_textz2",
    /* 118 */
    "multipolygon_textzm ::= VANUATU_OPEN_BRACKET polygon_textzm multipolygon_textzm2 VANUATU_CLOSE_BRACKET",
    /* 119 */ "multipolygon_textzm2 ::=",
    /* 120 */
    "multipolygon_textzm2 ::= VANUATU_COMMA polygon_textzm multipolygon_textzm2",
    /* 121 */ "geocoll ::= VANUATU_GEOMETRYCOLLECTION geocoll_text",
    /* 122 */ "geocollm ::= VANUATU_GEOMETRYCOLLECTION_M geocoll_textm",
    /* 123 */ "geocollz ::= VANUATU_GEOMETRYCOLLECTION_Z geocoll_textz",
    /* 124 */ "geocollzm ::= VANUATU_GEOMETRYCOLLECTION_ZM geocoll_textzm",
    /* 125 */
    "geocoll_text ::= VANUATU_OPEN_BRACKET point geocoll_text2 VANUATU_CLOSE_BRACKET",
    /* 126 */
    "geocoll_text ::= VANUATU_OPEN_BRACKET linestring geocoll_text2 VANUATU_CLOSE_BRACKET",
    /* 127 */
    "geocoll_text ::= VANUATU_OPEN_BRACKET polygon geocoll_text2 VANUATU_CLOSE_BRACKET",
    /* 128 */ "geocoll_text2 ::=",
    /* 129 */ "geocoll_text2 ::= VANUATU_COMMA point geocoll_text2",
    /* 130 */ "geocoll_text2 ::= VANUATU_COMMA linestring geocoll_text2",
    /* 131 */ "geocoll_text2 ::= VANUATU_COMMA polygon geocoll_text2",
    /* 132 */
    "geocoll_textm ::= VANUATU_OPEN_BRACKET pointm geocoll_textm2 VANUATU_CLOSE_BRACKET",
    /* 133 */
    "geocoll_textm ::= VANUATU_OPEN_BRACKET linestringm geocoll_textm2 VANUATU_CLOSE_BRACKET",
    /* 134 */
    "geocoll_textm ::= VANUATU_OPEN_BRACKET polygonm geocoll_textm2 VANUATU_CLOSE_BRACKET",
    /* 135 */ "geocoll_textm2 ::=",
    /* 136 */ "geocoll_textm2 ::= VANUATU_COMMA pointm geocoll_textm2",
    /* 137 */ "geocoll_textm2 ::= VANUATU_COMMA linestringm geocoll_textm2",
    /* 138 */ "geocoll_textm2 ::= VANUATU_COMMA polygonm geocoll_textm2",
    /* 139 */
    "geocoll_textz ::= VANUATU_OPEN_BRACKET pointz geocoll_textz2 VANUATU_CLOSE_BRACKET",
    /* 140 */
    "geocoll_textz ::= VANUATU_OPEN_BRACKET linestringz geocoll_textz2 VANUATU_CLOSE_BRACKET",
    /* 141 */
    "geocoll_textz ::= VANUATU_OPEN_BRACKET polygonz geocoll_textz2 VANUATU_CLOSE_BRACKET",
    /* 142 */ "geocoll_textz2 ::=",
    /* 143 */ "geocoll_textz2 ::= VANUATU_COMMA pointz geocoll_textz2",
    /* 144 */ "geocoll_textz2 ::= VANUATU_COMMA linestringz geocoll_textz2",
    /* 145 */ "geocoll_textz2 ::= VANUATU_COMMA polygonz geocoll_textz2",
    /* 146 */
    "geocoll_textzm ::= VANUATU_OPEN_BRACKET pointzm geocoll_textzm2 VANUATU_CLOSE_BRACKET",
    /* 147 */
    "geocoll_textzm ::= VANUATU_OPEN_BRACKET linestringzm geocoll_textzm2 VANUATU_CLOSE_BRACKET",
    /* 148 */
    "geocoll_textzm ::= VANUATU_OPEN_BRACKET polygonzm geocoll_textzm2 VANUATU_CLOSE_BRACKET",
    /* 149 */ "geocoll_textzm2 ::=",
    /* 150 */ "geocoll_textzm2 ::= VANUATU_COMMA pointzm geocoll_textzm2",
    /* 151 */ "geocoll_textzm2 ::= VANUATU_COMMA linestringzm geocoll_textzm2",
    /* 152 */ "geocoll_textzm2 ::= VANUATU_COMMA polygonzm geocoll_textzm2",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.
*/
static void
yyGrowStack (yyParser * p)
{
    int newSize;
    yyStackEntry *pNew;

    newSize = p->yystksz * 2 + 100;
    pNew = realloc (p->yystack, newSize * sizeof (pNew[0]));
    if (pNew)
      {
	  p->yystack = pNew;
	  p->yystksz = newSize;
#ifndef NDEBUG
	  if (yyTraceFILE)
	    {
		fprintf (yyTraceFILE, "%sStack grows to %d entries!\n",
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
void *
ParseAlloc (void *(*mallocProc) (size_t))
{
    yyParser *pParser;
    pParser = (yyParser *) (*mallocProc) ((size_t) sizeof (yyParser));
    if (pParser)
      {
	  pParser->yyidx = -1;
#ifdef YYTRACKMAXSTACKDEPTH
	  pParser->yyidxMax = 0;
#endif
#if YYSTACKDEPTH<=0
	  pParser->yystack = NULL;
	  pParser->yystksz = 0;
	  yyGrowStack (pParser);
#endif
      }
    return pParser;
}

/* The following function deletes the value associated with a
** symbol.  The symbol can be either a terminal or nonterminal.
** "yymajor" is the symbol code, and "yypminor" is a pointer to
** the value.
*/
static void
yy_destructor (yyParser * yypParser,	/* The parser */
	       YYCODETYPE yymajor,	/* Type code for object to destroy */
	       YYMINORTYPE * yypminor	/* The object to be destroyed */
    )
{
    ParseARG_FETCH;
    switch (yymajor)
      {
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
      default:
	  break;		/* If no destructor action specified: do nothing */
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
static int
yy_pop_parser_stack (yyParser * pParser)
{
    YYCODETYPE yymajor;
    yyStackEntry *yytos = &pParser->yystack[pParser->yyidx];

    if (pParser->yyidx < 0)
	return 0;
#ifndef NDEBUG
    if (yyTraceFILE && pParser->yyidx >= 0)
      {
	  fprintf (yyTraceFILE, "%sPopping %s\n",
		   yyTracePrompt, yyTokenName[yytos->major]);
      }
#endif
    yymajor = yytos->major;
    yy_destructor (pParser, yymajor, &yytos->minor);
    pParser->yyidx--;
    return yymajor;
}

/*
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int
ParseStackPeak (void *p)
{
    yyParser *pParser = (yyParser *) p;
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
static int
yy_find_shift_action (yyParser * pParser,	/* The parser */
		      YYCODETYPE iLookAhead	/* The look-ahead token */
    )
{
    int i;
    int stateno = pParser->yystack[pParser->yyidx].stateno;

    if (stateno > YY_SHIFT_MAX
	|| (i = yy_shift_ofst[stateno]) == YY_SHIFT_USE_DFLT)
      {
	  return yy_default[stateno];
      }
    assert (iLookAhead != YYNOCODE);
    i += iLookAhead;
    if (i < 0 || i >= YY_SZ_ACTTAB || yy_lookahead[i] != iLookAhead)
      {
	  if (iLookAhead > 0)
	    {
#ifdef YYFALLBACK
		YYCODETYPE iFallback;	/* Fallback token */
		if (iLookAhead < sizeof (yyFallback) / sizeof (yyFallback[0])
		    && (iFallback = yyFallback[iLookAhead]) != 0)
		  {
#ifndef NDEBUG
		      if (yyTraceFILE)
			{
			    fprintf (yyTraceFILE, "%sFALLBACK %s => %s\n",
				     yyTracePrompt, yyTokenName[iLookAhead],
				     yyTokenName[iFallback]);
			}
#endif
		      return yy_find_shift_action (pParser, iFallback);
		  }
#endif
#ifdef YYWILDCARD
		{
		    int j = i - iLookAhead + YYWILDCARD;
		    if (j >= 0 && j < YY_SZ_ACTTAB
			&& yy_lookahead[j] == YYWILDCARD)
		      {
#ifndef NDEBUG
			  if (yyTraceFILE)
			    {
				fprintf (yyTraceFILE, "%sWILDCARD %s => %s\n",
					 yyTracePrompt, yyTokenName[iLookAhead],
					 yyTokenName[YYWILDCARD]);
			    }
#endif /* NDEBUG */
			  return yy_action[j];
		      }
		}
#endif /* YYWILDCARD */
	    }
	  return yy_default[stateno];
      }
    else
      {
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
static int
yy_find_reduce_action (int stateno,	/* Current state number */
		       YYCODETYPE iLookAhead	/* The look-ahead token */
    )
{
    int i;
#ifdef YYERRORSYMBOL
    if (stateno > YY_REDUCE_MAX)
      {
	  return yy_default[stateno];
      }
#else
    assert (stateno <= YY_REDUCE_MAX);
#endif
    i = yy_reduce_ofst[stateno];
    assert (i != YY_REDUCE_USE_DFLT);
    assert (iLookAhead != YYNOCODE);
    i += iLookAhead;
#ifdef YYERRORSYMBOL
    if (i < 0 || i >= YY_SZ_ACTTAB || yy_lookahead[i] != iLookAhead)
      {
	  return yy_default[stateno];
      }
#else
    assert (i >= 0 && i < YY_SZ_ACTTAB);
    assert (yy_lookahead[i] == iLookAhead);
#endif
    return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void
yyStackOverflow (yyParser * yypParser, YYMINORTYPE * yypMinor)
{
    ParseARG_FETCH;
    yypParser->yyidx--;
#ifndef NDEBUG
    if (yyTraceFILE)
      {
	  fprintf (yyTraceFILE, "%sStack Overflow!\n", yyTracePrompt);
      }
#endif
    while (yypParser->yyidx >= 0)
	yy_pop_parser_stack (yypParser);
    /* Here code is inserted which will execute if the parser
     ** stack every overflows */

    fprintf (stderr, "Giving up.  Parser stack overflow\n");
    ParseARG_STORE;		/* Suppress warning about unused %extra_argument var */
}

/*
** Perform a shift action.
*/
static void
yy_shift (yyParser * yypParser,	/* The parser to be shifted */
	  int yyNewState,	/* The new state to shift in */
	  int yyMajor,		/* The major token to shift in */
	  YYMINORTYPE * yypMinor	/* Pointer to the minor token to shift in */
    )
{
    yyStackEntry *yytos;
    yypParser->yyidx++;
#ifdef YYTRACKMAXSTACKDEPTH
    if (yypParser->yyidx > yypParser->yyidxMax)
      {
	  yypParser->yyidxMax = yypParser->yyidx;
      }
#endif
#if YYSTACKDEPTH>0
    if (yypParser->yyidx >= YYSTACKDEPTH)
      {
	  yyStackOverflow (yypParser, yypMinor);
	  return;
      }
#else
    if (yypParser->yyidx >= yypParser->yystksz)
      {
	  yyGrowStack (yypParser);
	  if (yypParser->yyidx >= yypParser->yystksz)
	    {
		yyStackOverflow (yypParser, yypMinor);
		return;
	    }
      }
#endif
    yytos = &yypParser->yystack[yypParser->yyidx];
    yytos->stateno = (YYACTIONTYPE) yyNewState;
    yytos->major = (YYCODETYPE) yyMajor;
    yytos->minor = *yypMinor;
#ifndef NDEBUG
    if (yyTraceFILE && yypParser->yyidx > 0)
      {
	  int i;
	  fprintf (yyTraceFILE, "%sShift %d\n", yyTracePrompt, yyNewState);
	  fprintf (yyTraceFILE, "%sStack:", yyTracePrompt);
	  for (i = 1; i <= yypParser->yyidx; i++)
	      fprintf (yyTraceFILE, " %s",
		       yyTokenName[yypParser->yystack[i].major]);
	  fprintf (yyTraceFILE, "\n");
      }
#endif
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct
{
    YYCODETYPE lhs;		/* Symbol on the left-hand side of the rule */
    unsigned char nrhs;		/* Number of right-hand side symbols in the rule */
} yyRuleInfo[] =
{
    {
    35, 1},
    {
    36, 0},
    {
    36, 3},
    {
    37, 1},
    {
    38, 1},
    {
    38, 1},
    {
    38, 1},
    {
    38, 1},
    {
    39, 1},
    {
    39, 1},
    {
    39, 1},
    {
    39, 1},
    {
    39, 1},
    {
    39, 1},
    {
    39, 1},
    {
    40, 1},
    {
    40, 1},
    {
    40, 1},
    {
    40, 1},
    {
    40, 1},
    {
    40, 1},
    {
    40, 1},
    {
    41, 1},
    {
    41, 1},
    {
    41, 1},
    {
    41, 1},
    {
    41, 1},
    {
    41, 1},
    {
    41, 1},
    {
    42, 1},
    {
    42, 1},
    {
    42, 1},
    {
    42, 1},
    {
    42, 1},
    {
    42, 1},
    {
    42, 1},
    {
    43, 4},
    {
    57, 4},
    {
    50, 4},
    {
    64, 4},
    {
    71, 2},
    {
    72, 3},
    {
    73, 3},
    {
    74, 4},
    {
    75, 1},
    {
    76, 0},
    {
    76, 3},
    {
    77, 0},
    {
    77, 3},
    {
    78, 0},
    {
    78, 3},
    {
    79, 0},
    {
    79, 3},
    {
    44, 2},
    {
    58, 2},
    {
    51, 2},
    {
    65, 2},
    {
    80, 6},
    {
    81, 6},
    {
    82, 6},
    {
    83, 6},
    {
    45, 2},
    {
    59, 2},
    {
    52, 2},
    {
    66, 2},
    {
    84, 4},
    {
    85, 4},
    {
    86, 4},
    {
    87, 4},
    {
    88, 10},
    {
    89, 0},
    {
    89, 3},
    {
    90, 10},
    {
    91, 0},
    {
    91, 3},
    {
    92, 10},
    {
    93, 0},
    {
    93, 3},
    {
    94, 10},
    {
    95, 0},
    {
    95, 3},
    {
    46, 2},
    {
    60, 2},
    {
    53, 2},
    {
    67, 2},
    {
    96, 4},
    {
    97, 4},
    {
    98, 4},
    {
    99, 4},
    {
    47, 2},
    {
    61, 2},
    {
    54, 2},
    {
    68, 2},
    {
    100, 4},
    {
    104, 0},
    {
    104, 3},
    {
    101, 4},
    {
    105, 0},
    {
    105, 3},
    {
    102, 4},
    {
    106, 0},
    {
    106, 3},
    {
    103, 4},
    {
    107, 0},
    {
    107, 3},
    {
    48, 2},
    {
    62, 2},
    {
    55, 2},
    {
    69, 2},
    {
    108, 4},
    {
    112, 0},
    {
    112, 3},
    {
    109, 4},
    {
    113, 0},
    {
    113, 3},
    {
    110, 4},
    {
    114, 0},
    {
    114, 3},
    {
    111, 4},
    {
    115, 0},
    {
    115, 3},
    {
    49, 2},
    {
    63, 2},
    {
    56, 2},
    {
    70, 2},
    {
    116, 4},
    {
    116, 4},
    {
    116, 4},
    {
    120, 0},
    {
    120, 3},
    {
    120, 3},
    {
    120, 3},
    {
    117, 4},
    {
    117, 4},
    {
    117, 4},
    {
    121, 0},
    {
    121, 3},
    {
    121, 3},
    {
    121, 3},
    {
    118, 4},
    {
    118, 4},
    {
    118, 4},
    {
    122, 0},
    {
    122, 3},
    {
    122, 3},
    {
    122, 3},
    {
    119, 4},
    {
    119, 4},
    {
    119, 4},
    {
    123, 0},
    {
    123, 3},
    {
    123, 3},
    {
123, 3},};

static void yy_accept (yyParser *);	/* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void
yy_reduce (yyParser * yypParser,	/* The parser */
	   int yyruleno		/* Number of the rule by which to reduce */
    )
{
    int yygoto;			/* The next state */
    int yyact;			/* The next action */
    YYMINORTYPE yygotominor;	/* The LHS of the rule reduced */
    yyStackEntry *yymsp;	/* The top of the parser's stack */
    int yysize;			/* Amount to pop the stack */
    ParseARG_FETCH;
    yymsp = &yypParser->yystack[yypParser->yyidx];
#ifndef NDEBUG
    if (yyTraceFILE && yyruleno >= 0
	&& yyruleno < (int) (sizeof (yyRuleName) / sizeof (yyRuleName[0])))
      {
	  fprintf (yyTraceFILE, "%sReduce [%s].\n", yyTracePrompt,
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
    /*memset(&yygotominor, 0, sizeof(yygotominor)); */
    yygotominor = yyzerominor;


    switch (yyruleno)
      {
	  /* Beginning here are the reduction cases.  A typical example
	   ** follows:
	   **   case 0:
	   **  #line <lineno> <grammarfile>
	   **     { ... }           // User supplied code
	   **  #line <lineno> <thisfile>
	   **     break;
	   */
      case 8:			/* geo_text ::= point */
      case 9:			/* geo_text ::= linestring */
	  yytestcase (yyruleno == 9);
      case 10:			/* geo_text ::= polygon */
	  yytestcase (yyruleno == 10);
      case 11:			/* geo_text ::= multipoint */
	  yytestcase (yyruleno == 11);
      case 12:			/* geo_text ::= multilinestring */
	  yytestcase (yyruleno == 12);
      case 13:			/* geo_text ::= multipolygon */
	  yytestcase (yyruleno == 13);
      case 14:			/* geo_text ::= geocoll */
	  yytestcase (yyruleno == 14);
      case 15:			/* geo_textz ::= pointz */
	  yytestcase (yyruleno == 15);
      case 16:			/* geo_textz ::= linestringz */
	  yytestcase (yyruleno == 16);
      case 17:			/* geo_textz ::= polygonz */
	  yytestcase (yyruleno == 17);
      case 18:			/* geo_textz ::= multipointz */
	  yytestcase (yyruleno == 18);
      case 19:			/* geo_textz ::= multilinestringz */
	  yytestcase (yyruleno == 19);
      case 20:			/* geo_textz ::= multipolygonz */
	  yytestcase (yyruleno == 20);
      case 21:			/* geo_textz ::= geocollz */
	  yytestcase (yyruleno == 21);
      case 22:			/* geo_textm ::= pointm */
	  yytestcase (yyruleno == 22);
      case 23:			/* geo_textm ::= linestringm */
	  yytestcase (yyruleno == 23);
      case 24:			/* geo_textm ::= polygonm */
	  yytestcase (yyruleno == 24);
      case 25:			/* geo_textm ::= multipointm */
	  yytestcase (yyruleno == 25);
      case 26:			/* geo_textm ::= multilinestringm */
	  yytestcase (yyruleno == 26);
      case 27:			/* geo_textm ::= multipolygonm */
	  yytestcase (yyruleno == 27);
      case 28:			/* geo_textm ::= geocollm */
	  yytestcase (yyruleno == 28);
      case 29:			/* geo_textzm ::= pointzm */
	  yytestcase (yyruleno == 29);
      case 30:			/* geo_textzm ::= linestringzm */
	  yytestcase (yyruleno == 30);
      case 31:			/* geo_textzm ::= polygonzm */
	  yytestcase (yyruleno == 31);
      case 32:			/* geo_textzm ::= multipointzm */
	  yytestcase (yyruleno == 32);
      case 33:			/* geo_textzm ::= multilinestringzm */
	  yytestcase (yyruleno == 33);
      case 34:			/* geo_textzm ::= multipolygonzm */
	  yytestcase (yyruleno == 34);
      case 35:			/* geo_textzm ::= geocollzm */
	  yytestcase (yyruleno == 35);
	  {
	      *result = yymsp[0].minor.yy0;
	  }
	  break;
      case 36:			/* point ::= VANUATU_POINT VANUATU_OPEN_BRACKET point_coordxy VANUATU_CLOSE_BRACKET */
	  {
	      yygotominor.yy0 =
		  vanuatu_buildGeomFromPoint ((gaiaPointPtr) yymsp[-1].minor.
					      yy0);
	  }
	  break;
      case 37:			/* pointm ::= VANUATU_POINT_M VANUATU_OPEN_BRACKET point_coordxym VANUATU_CLOSE_BRACKET */
      case 38:			/* pointz ::= VANUATU_POINT_Z VANUATU_OPEN_BRACKET point_coordxyz VANUATU_CLOSE_BRACKET */
	  yytestcase (yyruleno == 38);
      case 39:			/* pointzm ::= VANUATU_POINT_ZM VANUATU_OPEN_BRACKET point_coordxyzm VANUATU_CLOSE_BRACKET */
	  yytestcase (yyruleno == 39);
	  {
	      yygotominor.yy0 =
		  vanuatu_buildGeomFromPoint ((gaiaPointPtr) yymsp[-1].minor.
					      yy0);
	  }
	  break;
      case 40:			/* point_coordxy ::= coord coord */
	  {
	      yygotominor.yy0 =
		  (void *) vanuatu_point_xy ((double *) yymsp[-1].minor.yy0,
					     (double *) yymsp[0].minor.yy0);
	  }
	  break;
      case 41:			/* point_coordxym ::= coord coord coord */
	  {
	      yygotominor.yy0 =
		  (void *) vanuatu_point_xym ((double *) yymsp[-2].minor.yy0,
					      (double *) yymsp[-1].minor.yy0,
					      (double *) yymsp[0].minor.yy0);
	  }
	  break;
      case 42:			/* point_coordxyz ::= coord coord coord */
	  {
	      yygotominor.yy0 =
		  (void *) vanuatu_point_xyz ((double *) yymsp[-2].minor.yy0,
					      (double *) yymsp[-1].minor.yy0,
					      (double *) yymsp[0].minor.yy0);
	  }
	  break;
      case 43:			/* point_coordxyzm ::= coord coord coord coord */
	  {
	      yygotominor.yy0 =
		  (void *) vanuatu_point_xyzm ((double *) yymsp[-3].minor.yy0,
					       (double *) yymsp[-2].minor.yy0,
					       (double *) yymsp[-1].minor.yy0,
					       (double *) yymsp[0].minor.yy0);
	  }
	  break;
      case 44:			/* coord ::= VANUATU_NUM */
      case 81:			/* multipoint ::= VANUATU_MULTIPOINT multipoint_text */
	  yytestcase (yyruleno == 81);
      case 82:			/* multipointm ::= VANUATU_MULTIPOINT_M multipoint_textm */
	  yytestcase (yyruleno == 82);
      case 83:			/* multipointz ::= VANUATU_MULTIPOINT_Z multipoint_textz */
	  yytestcase (yyruleno == 83);
      case 84:			/* multipointzm ::= VANUATU_MULTIPOINT_ZM multipoint_textzm */
	  yytestcase (yyruleno == 84);
      case 89:			/* multilinestring ::= VANUATU_MULTILINESTRING multilinestring_text */
	  yytestcase (yyruleno == 89);
      case 90:			/* multilinestringm ::= VANUATU_MULTILINESTRING_M multilinestring_textm */
	  yytestcase (yyruleno == 90);
      case 91:			/* multilinestringz ::= VANUATU_MULTILINESTRING_Z multilinestring_textz */
	  yytestcase (yyruleno == 91);
      case 92:			/* multilinestringzm ::= VANUATU_MULTILINESTRING_ZM multilinestring_textzm */
	  yytestcase (yyruleno == 92);
      case 105:		/* multipolygon ::= VANUATU_MULTIPOLYGON multipolygon_text */
	  yytestcase (yyruleno == 105);
      case 106:		/* multipolygonm ::= VANUATU_MULTIPOLYGON_M multipolygon_textm */
	  yytestcase (yyruleno == 106);
      case 107:		/* multipolygonz ::= VANUATU_MULTIPOLYGON_Z multipolygon_textz */
	  yytestcase (yyruleno == 107);
      case 108:		/* multipolygonzm ::= VANUATU_MULTIPOLYGON_ZM multipolygon_textzm */
	  yytestcase (yyruleno == 108);
      case 121:		/* geocoll ::= VANUATU_GEOMETRYCOLLECTION geocoll_text */
	  yytestcase (yyruleno == 121);
      case 122:		/* geocollm ::= VANUATU_GEOMETRYCOLLECTION_M geocoll_textm */
	  yytestcase (yyruleno == 122);
      case 123:		/* geocollz ::= VANUATU_GEOMETRYCOLLECTION_Z geocoll_textz */
	  yytestcase (yyruleno == 123);
      case 124:		/* geocollzm ::= VANUATU_GEOMETRYCOLLECTION_ZM geocoll_textzm */
	  yytestcase (yyruleno == 124);
	  {
	      yygotominor.yy0 = yymsp[0].minor.yy0;
	  }
	  break;
      case 45:			/* extra_pointsxy ::= */
      case 47:			/* extra_pointsxym ::= */
	  yytestcase (yyruleno == 47);
      case 49:			/* extra_pointsxyz ::= */
	  yytestcase (yyruleno == 49);
      case 51:			/* extra_pointsxyzm ::= */
	  yytestcase (yyruleno == 51);
      case 70:			/* extra_rings ::= */
	  yytestcase (yyruleno == 70);
      case 73:			/* extra_ringsm ::= */
	  yytestcase (yyruleno == 73);
      case 76:			/* extra_ringsz ::= */
	  yytestcase (yyruleno == 76);
      case 79:			/* extra_ringszm ::= */
	  yytestcase (yyruleno == 79);
      case 94:			/* multilinestring_text2 ::= */
	  yytestcase (yyruleno == 94);
      case 97:			/* multilinestring_textm2 ::= */
	  yytestcase (yyruleno == 97);
      case 100:		/* multilinestring_textz2 ::= */
	  yytestcase (yyruleno == 100);
      case 103:		/* multilinestring_textzm2 ::= */
	  yytestcase (yyruleno == 103);
      case 110:		/* multipolygon_text2 ::= */
	  yytestcase (yyruleno == 110);
      case 113:		/* multipolygon_textm2 ::= */
	  yytestcase (yyruleno == 113);
      case 116:		/* multipolygon_textz2 ::= */
	  yytestcase (yyruleno == 116);
      case 119:		/* multipolygon_textzm2 ::= */
	  yytestcase (yyruleno == 119);
      case 128:		/* geocoll_text2 ::= */
	  yytestcase (yyruleno == 128);
      case 135:		/* geocoll_textm2 ::= */
	  yytestcase (yyruleno == 135);
      case 142:		/* geocoll_textz2 ::= */
	  yytestcase (yyruleno == 142);
      case 149:		/* geocoll_textzm2 ::= */
	  yytestcase (yyruleno == 149);
	  {
	      yygotominor.yy0 = NULL;
	  }
	  break;
      case 46:			/* extra_pointsxy ::= VANUATU_COMMA point_coordxy extra_pointsxy */
      case 48:			/* extra_pointsxym ::= VANUATU_COMMA point_coordxym extra_pointsxym */
	  yytestcase (yyruleno == 48);
      case 50:			/* extra_pointsxyz ::= VANUATU_COMMA point_coordxyz extra_pointsxyz */
	  yytestcase (yyruleno == 50);
      case 52:			/* extra_pointsxyzm ::= VANUATU_COMMA point_coordxyzm extra_pointsxyzm */
	  yytestcase (yyruleno == 52);
	  {
	      ((gaiaPointPtr) yymsp[-1].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[0].minor.yy0;
	      yygotominor.yy0 = yymsp[-1].minor.yy0;
	  }
	  break;
      case 53:			/* linestring ::= VANUATU_LINESTRING linestring_text */
      case 54:			/* linestringm ::= VANUATU_LINESTRING_M linestring_textm */
	  yytestcase (yyruleno == 54);
      case 55:			/* linestringz ::= VANUATU_LINESTRING_Z linestring_textz */
	  yytestcase (yyruleno == 55);
      case 56:			/* linestringzm ::= VANUATU_LINESTRING_ZM linestring_textzm */
	  yytestcase (yyruleno == 56);
	  {
	      yygotominor.yy0 =
		  vanuatu_buildGeomFromLinestring ((gaiaLinestringPtr)
						   yymsp[0].minor.yy0);
	  }
	  break;
      case 57:			/* linestring_text ::= VANUATU_OPEN_BRACKET point_coordxy VANUATU_COMMA point_coordxy extra_pointsxy VANUATU_CLOSE_BRACKET */
	  {
	      ((gaiaPointPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-1].minor.yy0;
	      ((gaiaPointPtr) yymsp[-4].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-2].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_linestring_xy ((gaiaPointPtr)
						  yymsp[-4].minor.yy0);
	  }
	  break;
      case 58:			/* linestring_textm ::= VANUATU_OPEN_BRACKET point_coordxym VANUATU_COMMA point_coordxym extra_pointsxym VANUATU_CLOSE_BRACKET */
	  {
	      ((gaiaPointPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-1].minor.yy0;
	      ((gaiaPointPtr) yymsp[-4].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-2].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_linestring_xym ((gaiaPointPtr)
						   yymsp[-4].minor.yy0);
	  }
	  break;
      case 59:			/* linestring_textz ::= VANUATU_OPEN_BRACKET point_coordxyz VANUATU_COMMA point_coordxyz extra_pointsxyz VANUATU_CLOSE_BRACKET */
	  {
	      ((gaiaPointPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-1].minor.yy0;
	      ((gaiaPointPtr) yymsp[-4].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-2].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_linestring_xyz ((gaiaPointPtr)
						   yymsp[-4].minor.yy0);
	  }
	  break;
      case 60:			/* linestring_textzm ::= VANUATU_OPEN_BRACKET point_coordxyzm VANUATU_COMMA point_coordxyzm extra_pointsxyzm VANUATU_CLOSE_BRACKET */
	  {
	      ((gaiaPointPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-1].minor.yy0;
	      ((gaiaPointPtr) yymsp[-4].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-2].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_linestring_xyzm ((gaiaPointPtr)
						    yymsp[-4].minor.yy0);
	  }
	  break;
      case 61:			/* polygon ::= VANUATU_POLYGON polygon_text */
      case 62:			/* polygonm ::= VANUATU_POLYGON_M polygon_textm */
	  yytestcase (yyruleno == 62);
      case 63:			/* polygonz ::= VANUATU_POLYGON_Z polygon_textz */
	  yytestcase (yyruleno == 63);
      case 64:			/* polygonzm ::= VANUATU_POLYGON_ZM polygon_textzm */
	  yytestcase (yyruleno == 64);
	  {
	      yygotominor.yy0 =
		  buildGeomFromPolygon ((gaiaPolygonPtr) yymsp[0].minor.yy0);
	  }
	  break;
      case 65:			/* polygon_text ::= VANUATU_OPEN_BRACKET ring extra_rings VANUATU_CLOSE_BRACKET */
	  {
	      ((gaiaRingPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaRingPtr) yymsp[-1].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_polygon_xy ((gaiaRingPtr) yymsp[-2].minor.
					       yy0);
	  }
	  break;
      case 66:			/* polygon_textm ::= VANUATU_OPEN_BRACKET ringm extra_ringsm VANUATU_CLOSE_BRACKET */
	  {
	      ((gaiaRingPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaRingPtr) yymsp[-1].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_polygon_xym ((gaiaRingPtr) yymsp[-2].minor.
						yy0);
	  }
	  break;
      case 67:			/* polygon_textz ::= VANUATU_OPEN_BRACKET ringz extra_ringsz VANUATU_CLOSE_BRACKET */
	  {
	      ((gaiaRingPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaRingPtr) yymsp[-1].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_polygon_xyz ((gaiaRingPtr) yymsp[-2].minor.
						yy0);
	  }
	  break;
      case 68:			/* polygon_textzm ::= VANUATU_OPEN_BRACKET ringzm extra_ringszm VANUATU_CLOSE_BRACKET */
	  {
	      ((gaiaRingPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaRingPtr) yymsp[-1].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_polygon_xyzm ((gaiaRingPtr) yymsp[-2].minor.
						 yy0);
	  }
	  break;
      case 69:			/* ring ::= VANUATU_OPEN_BRACKET point_coordxy VANUATU_COMMA point_coordxy VANUATU_COMMA point_coordxy VANUATU_COMMA point_coordxy extra_pointsxy VANUATU_CLOSE_BRACKET */
	  {
	      ((gaiaPointPtr) yymsp[-8].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-6].minor.yy0;
	      ((gaiaPointPtr) yymsp[-6].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-4].minor.yy0;
	      ((gaiaPointPtr) yymsp[-4].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-2].minor.yy0;
	      ((gaiaPointPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-1].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_ring_xy ((gaiaPointPtr) yymsp[-8].minor.yy0);
	  }
	  break;
      case 71:			/* extra_rings ::= VANUATU_COMMA ring extra_rings */
      case 74:			/* extra_ringsm ::= VANUATU_COMMA ringm extra_ringsm */
	  yytestcase (yyruleno == 74);
      case 77:			/* extra_ringsz ::= VANUATU_COMMA ringz extra_ringsz */
	  yytestcase (yyruleno == 77);
      case 80:			/* extra_ringszm ::= VANUATU_COMMA ringzm extra_ringszm */
	  yytestcase (yyruleno == 80);
	  {
	      ((gaiaRingPtr) yymsp[-1].minor.yy0)->Next =
		  (gaiaRingPtr) yymsp[0].minor.yy0;
	      yygotominor.yy0 = yymsp[-1].minor.yy0;
	  }
	  break;
      case 72:			/* ringm ::= VANUATU_OPEN_BRACKET point_coordxym VANUATU_COMMA point_coordxym VANUATU_COMMA point_coordxym VANUATU_COMMA point_coordxym extra_pointsxym VANUATU_CLOSE_BRACKET */
	  {
	      ((gaiaPointPtr) yymsp[-8].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-6].minor.yy0;
	      ((gaiaPointPtr) yymsp[-6].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-4].minor.yy0;
	      ((gaiaPointPtr) yymsp[-4].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-2].minor.yy0;
	      ((gaiaPointPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-1].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_ring_xym ((gaiaPointPtr) yymsp[-8].minor.
					     yy0);
	  }
	  break;
      case 75:			/* ringz ::= VANUATU_OPEN_BRACKET point_coordxyz VANUATU_COMMA point_coordxyz VANUATU_COMMA point_coordxyz VANUATU_COMMA point_coordxyz extra_pointsxyz VANUATU_CLOSE_BRACKET */
	  {
	      ((gaiaPointPtr) yymsp[-8].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-6].minor.yy0;
	      ((gaiaPointPtr) yymsp[-6].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-4].minor.yy0;
	      ((gaiaPointPtr) yymsp[-4].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-2].minor.yy0;
	      ((gaiaPointPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-1].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_ring_xyz ((gaiaPointPtr) yymsp[-8].minor.
					     yy0);
	  }
	  break;
      case 78:			/* ringzm ::= VANUATU_OPEN_BRACKET point_coordxyzm VANUATU_COMMA point_coordxyzm VANUATU_COMMA point_coordxyzm VANUATU_COMMA point_coordxyzm extra_pointsxyzm VANUATU_CLOSE_BRACKET */
	  {
	      ((gaiaPointPtr) yymsp[-8].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-6].minor.yy0;
	      ((gaiaPointPtr) yymsp[-6].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-4].minor.yy0;
	      ((gaiaPointPtr) yymsp[-4].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-2].minor.yy0;
	      ((gaiaPointPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-1].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_ring_xyzm ((gaiaPointPtr) yymsp[-8].minor.
					      yy0);
	  }
	  break;
      case 85:			/* multipoint_text ::= VANUATU_OPEN_BRACKET point_coordxy extra_pointsxy VANUATU_CLOSE_BRACKET */
	  {
	      ((gaiaPointPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-1].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_multipoint_xy ((gaiaPointPtr)
						  yymsp[-2].minor.yy0);
	  }
	  break;
      case 86:			/* multipoint_textm ::= VANUATU_OPEN_BRACKET point_coordxym extra_pointsxym VANUATU_CLOSE_BRACKET */
	  {
	      ((gaiaPointPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-1].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_multipoint_xym ((gaiaPointPtr)
						   yymsp[-2].minor.yy0);
	  }
	  break;
      case 87:			/* multipoint_textz ::= VANUATU_OPEN_BRACKET point_coordxyz extra_pointsxyz VANUATU_CLOSE_BRACKET */
	  {
	      ((gaiaPointPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-1].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_multipoint_xyz ((gaiaPointPtr)
						   yymsp[-2].minor.yy0);
	  }
	  break;
      case 88:			/* multipoint_textzm ::= VANUATU_OPEN_BRACKET point_coordxyzm extra_pointsxyzm VANUATU_CLOSE_BRACKET */
	  {
	      ((gaiaPointPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaPointPtr) yymsp[-1].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_multipoint_xyzm ((gaiaPointPtr)
						    yymsp[-2].minor.yy0);
	  }
	  break;
      case 93:			/* multilinestring_text ::= VANUATU_OPEN_BRACKET linestring_text multilinestring_text2 VANUATU_CLOSE_BRACKET */
	  {
	      ((gaiaLinestringPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaLinestringPtr) yymsp[-1].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_multilinestring_xy ((gaiaLinestringPtr)
						       yymsp[-2].minor.yy0);
	  }
	  break;
      case 95:			/* multilinestring_text2 ::= VANUATU_COMMA linestring_text multilinestring_text2 */
      case 98:			/* multilinestring_textm2 ::= VANUATU_COMMA linestring_textm multilinestring_textm2 */
	  yytestcase (yyruleno == 98);
      case 101:		/* multilinestring_textz2 ::= VANUATU_COMMA linestring_textz multilinestring_textz2 */
	  yytestcase (yyruleno == 101);
      case 104:		/* multilinestring_textzm2 ::= VANUATU_COMMA linestring_textzm multilinestring_textzm2 */
	  yytestcase (yyruleno == 104);
	  {
	      ((gaiaLinestringPtr) yymsp[-1].minor.yy0)->Next =
		  (gaiaLinestringPtr) yymsp[0].minor.yy0;
	      yygotominor.yy0 = yymsp[-1].minor.yy0;
	  }
	  break;
      case 96:			/* multilinestring_textm ::= VANUATU_OPEN_BRACKET linestring_textm multilinestring_textm2 VANUATU_CLOSE_BRACKET */
	  {
	      ((gaiaLinestringPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaLinestringPtr) yymsp[-1].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_multilinestring_xym ((gaiaLinestringPtr)
							yymsp[-2].minor.yy0);
	  }
	  break;
      case 99:			/* multilinestring_textz ::= VANUATU_OPEN_BRACKET linestring_textz multilinestring_textz2 VANUATU_CLOSE_BRACKET */
	  {
	      ((gaiaLinestringPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaLinestringPtr) yymsp[-1].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_multilinestring_xyz ((gaiaLinestringPtr)
							yymsp[-2].minor.yy0);
	  }
	  break;
      case 102:		/* multilinestring_textzm ::= VANUATU_OPEN_BRACKET linestring_textzm multilinestring_textzm2 VANUATU_CLOSE_BRACKET */
	  {
	      ((gaiaLinestringPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaLinestringPtr) yymsp[-1].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_multilinestring_xyzm ((gaiaLinestringPtr)
							 yymsp[-2].minor.yy0);
	  }
	  break;
      case 109:		/* multipolygon_text ::= VANUATU_OPEN_BRACKET polygon_text multipolygon_text2 VANUATU_CLOSE_BRACKET */
	  {
	      ((gaiaPolygonPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaPolygonPtr) yymsp[-1].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_multipolygon_xy ((gaiaPolygonPtr)
						    yymsp[-2].minor.yy0);
	  }
	  break;
      case 111:		/* multipolygon_text2 ::= VANUATU_COMMA polygon_text multipolygon_text2 */
      case 114:		/* multipolygon_textm2 ::= VANUATU_COMMA polygon_textm multipolygon_textm2 */
	  yytestcase (yyruleno == 114);
      case 117:		/* multipolygon_textz2 ::= VANUATU_COMMA polygon_textz multipolygon_textz2 */
	  yytestcase (yyruleno == 117);
      case 120:		/* multipolygon_textzm2 ::= VANUATU_COMMA polygon_textzm multipolygon_textzm2 */
	  yytestcase (yyruleno == 120);
	  {
	      ((gaiaPolygonPtr) yymsp[-1].minor.yy0)->Next =
		  (gaiaPolygonPtr) yymsp[0].minor.yy0;
	      yygotominor.yy0 = yymsp[-1].minor.yy0;
	  }
	  break;
      case 112:		/* multipolygon_textm ::= VANUATU_OPEN_BRACKET polygon_textm multipolygon_textm2 VANUATU_CLOSE_BRACKET */
	  {
	      ((gaiaPolygonPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaPolygonPtr) yymsp[-1].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_multipolygon_xym ((gaiaPolygonPtr)
						     yymsp[-2].minor.yy0);
	  }
	  break;
      case 115:		/* multipolygon_textz ::= VANUATU_OPEN_BRACKET polygon_textz multipolygon_textz2 VANUATU_CLOSE_BRACKET */
	  {
	      ((gaiaPolygonPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaPolygonPtr) yymsp[-1].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_multipolygon_xyz ((gaiaPolygonPtr)
						     yymsp[-2].minor.yy0);
	  }
	  break;
      case 118:		/* multipolygon_textzm ::= VANUATU_OPEN_BRACKET polygon_textzm multipolygon_textzm2 VANUATU_CLOSE_BRACKET */
	  {
	      ((gaiaPolygonPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaPolygonPtr) yymsp[-1].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_multipolygon_xyzm ((gaiaPolygonPtr)
						      yymsp[-2].minor.yy0);
	  }
	  break;
      case 125:		/* geocoll_text ::= VANUATU_OPEN_BRACKET point geocoll_text2 VANUATU_CLOSE_BRACKET */
      case 126:		/* geocoll_text ::= VANUATU_OPEN_BRACKET linestring geocoll_text2 VANUATU_CLOSE_BRACKET */
	  yytestcase (yyruleno == 126);
      case 127:		/* geocoll_text ::= VANUATU_OPEN_BRACKET polygon geocoll_text2 VANUATU_CLOSE_BRACKET */
	  yytestcase (yyruleno == 127);
	  {
	      ((gaiaGeomCollPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaGeomCollPtr) yymsp[-1].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_geomColl_xy ((gaiaGeomCollPtr)
						yymsp[-2].minor.yy0);
	  }
	  break;
      case 129:		/* geocoll_text2 ::= VANUATU_COMMA point geocoll_text2 */
      case 130:		/* geocoll_text2 ::= VANUATU_COMMA linestring geocoll_text2 */
	  yytestcase (yyruleno == 130);
      case 131:		/* geocoll_text2 ::= VANUATU_COMMA polygon geocoll_text2 */
	  yytestcase (yyruleno == 131);
      case 136:		/* geocoll_textm2 ::= VANUATU_COMMA pointm geocoll_textm2 */
	  yytestcase (yyruleno == 136);
      case 137:		/* geocoll_textm2 ::= VANUATU_COMMA linestringm geocoll_textm2 */
	  yytestcase (yyruleno == 137);
      case 138:		/* geocoll_textm2 ::= VANUATU_COMMA polygonm geocoll_textm2 */
	  yytestcase (yyruleno == 138);
      case 143:		/* geocoll_textz2 ::= VANUATU_COMMA pointz geocoll_textz2 */
	  yytestcase (yyruleno == 143);
      case 144:		/* geocoll_textz2 ::= VANUATU_COMMA linestringz geocoll_textz2 */
	  yytestcase (yyruleno == 144);
      case 145:		/* geocoll_textz2 ::= VANUATU_COMMA polygonz geocoll_textz2 */
	  yytestcase (yyruleno == 145);
      case 150:		/* geocoll_textzm2 ::= VANUATU_COMMA pointzm geocoll_textzm2 */
	  yytestcase (yyruleno == 150);
      case 151:		/* geocoll_textzm2 ::= VANUATU_COMMA linestringzm geocoll_textzm2 */
	  yytestcase (yyruleno == 151);
      case 152:		/* geocoll_textzm2 ::= VANUATU_COMMA polygonzm geocoll_textzm2 */
	  yytestcase (yyruleno == 152);
	  {
	      ((gaiaGeomCollPtr) yymsp[-1].minor.yy0)->Next =
		  (gaiaGeomCollPtr) yymsp[0].minor.yy0;
	      yygotominor.yy0 = yymsp[-1].minor.yy0;
	  }
	  break;
      case 132:		/* geocoll_textm ::= VANUATU_OPEN_BRACKET pointm geocoll_textm2 VANUATU_CLOSE_BRACKET */
      case 133:		/* geocoll_textm ::= VANUATU_OPEN_BRACKET linestringm geocoll_textm2 VANUATU_CLOSE_BRACKET */
	  yytestcase (yyruleno == 133);
      case 134:		/* geocoll_textm ::= VANUATU_OPEN_BRACKET polygonm geocoll_textm2 VANUATU_CLOSE_BRACKET */
	  yytestcase (yyruleno == 134);
	  {
	      ((gaiaGeomCollPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaGeomCollPtr) yymsp[-1].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_geomColl_xym ((gaiaGeomCollPtr)
						 yymsp[-2].minor.yy0);
	  }
	  break;
      case 139:		/* geocoll_textz ::= VANUATU_OPEN_BRACKET pointz geocoll_textz2 VANUATU_CLOSE_BRACKET */
      case 140:		/* geocoll_textz ::= VANUATU_OPEN_BRACKET linestringz geocoll_textz2 VANUATU_CLOSE_BRACKET */
	  yytestcase (yyruleno == 140);
      case 141:		/* geocoll_textz ::= VANUATU_OPEN_BRACKET polygonz geocoll_textz2 VANUATU_CLOSE_BRACKET */
	  yytestcase (yyruleno == 141);
	  {
	      ((gaiaGeomCollPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaGeomCollPtr) yymsp[-1].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_geomColl_xyz ((gaiaGeomCollPtr)
						 yymsp[-2].minor.yy0);
	  }
	  break;
      case 146:		/* geocoll_textzm ::= VANUATU_OPEN_BRACKET pointzm geocoll_textzm2 VANUATU_CLOSE_BRACKET */
      case 147:		/* geocoll_textzm ::= VANUATU_OPEN_BRACKET linestringzm geocoll_textzm2 VANUATU_CLOSE_BRACKET */
	  yytestcase (yyruleno == 147);
      case 148:		/* geocoll_textzm ::= VANUATU_OPEN_BRACKET polygonzm geocoll_textzm2 VANUATU_CLOSE_BRACKET */
	  yytestcase (yyruleno == 148);
	  {
	      ((gaiaGeomCollPtr) yymsp[-2].minor.yy0)->Next =
		  (gaiaGeomCollPtr) yymsp[-1].minor.yy0;
	      yygotominor.yy0 =
		  (void *) vanuatu_geomColl_xyzm ((gaiaGeomCollPtr)
						  yymsp[-2].minor.yy0);
	  }
	  break;
      default:
	  /* (0) main ::= in */ yytestcase (yyruleno == 0);
	  /* (1) in ::= */ yytestcase (yyruleno == 1);
	  /* (2) in ::= in state VANUATU_NEWLINE */ yytestcase (yyruleno == 2);
	  /* (3) state ::= program */ yytestcase (yyruleno == 3);
	  /* (4) program ::= geo_text */ yytestcase (yyruleno == 4);
	  /* (5) program ::= geo_textz */ yytestcase (yyruleno == 5);
	  /* (6) program ::= geo_textm */ yytestcase (yyruleno == 6);
	  /* (7) program ::= geo_textzm */ yytestcase (yyruleno == 7);
	  break;
      };
    yygoto = yyRuleInfo[yyruleno].lhs;
    yysize = yyRuleInfo[yyruleno].nrhs;
    yypParser->yyidx -= yysize;
    yyact = yy_find_reduce_action (yymsp[-yysize].stateno, (YYCODETYPE) yygoto);
    if (yyact < YYNSTATE)
      {
#ifdef NDEBUG
	  /* If we are not debugging and the reduce action popped at least
	   ** one element off the stack, then we can push the new element back
	   ** onto the stack here, and skip the stack overflow test in yy_shift().
	   ** That gives a significant speed improvement. */
	  if (yysize)
	    {
		yypParser->yyidx++;
		yymsp -= yysize - 1;
		yymsp->stateno = (YYACTIONTYPE) yyact;
		yymsp->major = (YYCODETYPE) yygoto;
		yymsp->minor = yygotominor;
	    }
	  else
#endif
	    {
		yy_shift (yypParser, yyact, yygoto, &yygotominor);
	    }
      }
    else
      {
	  assert (yyact == YYNSTATE + YYNRULE + 1);
	  yy_accept (yypParser);
      }
}

/*
** The following code executes when the parse fails
*/
#ifndef YYNOERRORRECOVERY
static void
yy_parse_failed (yyParser * yypParser	/* The parser */
    )
{
    ParseARG_FETCH;
#ifndef NDEBUG
    if (yyTraceFILE)
      {
	  fprintf (yyTraceFILE, "%sFail!\n", yyTracePrompt);
      }
#endif
    while (yypParser->yyidx >= 0)
	yy_pop_parser_stack (yypParser);
    /* Here code is inserted which will be executed whenever the
     ** parser fails */
    ParseARG_STORE;		/* Suppress warning about unused %extra_argument variable */
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
static void
yy_syntax_error (yyParser * yypParser,	/* The parser */
		 int yymajor,	/* The major type of the error token */
		 YYMINORTYPE yyminor	/* The minor type of the error token */
    )
{
    ParseARG_FETCH;
#define TOKEN (yyminor.yy0)

/* 
** Sandro Furieri 2010 Apr 4
** when the LEMON parser encounters an error
** then this global variable is set 
*/
    vanuatu_parse_error = 1;
    *result = NULL;
    ParseARG_STORE;		/* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void
yy_accept (yyParser * yypParser	/* The parser */
    )
{
    ParseARG_FETCH;
#ifndef NDEBUG
    if (yyTraceFILE)
      {
	  fprintf (yyTraceFILE, "%sAccept!\n", yyTracePrompt);
      }
#endif
    while (yypParser->yyidx >= 0)
	yy_pop_parser_stack (yypParser);
    /* Here code is inserted which will be executed whenever the
     ** parser accepts */
    ParseARG_STORE;		/* Suppress warning about unused %extra_argument variable */
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
void
Parse (void *yyp,		/* The parser */
       int yymajor,		/* The major token code number */
       ParseTOKENTYPE yyminor	/* The value for the token */
       ParseARG_PDECL		/* Optional %extra_argument parameter */
    )
{
    YYMINORTYPE yyminorunion;
    int yyact;			/* The parser action. */
    int yyendofinput;		/* True if we are at the end of input */
#ifdef YYERRORSYMBOL
    int yyerrorhit = 0;		/* True if yymajor has invoked an error */
#endif
    yyParser *yypParser;	/* The parser */

    /* (re)initialize the parser, if necessary */
    yypParser = (yyParser *) yyp;
    if (yypParser->yyidx < 0)
      {
#if YYSTACKDEPTH<=0
	  if (yypParser->yystksz <= 0)
	    {
		/*memset(&yyminorunion, 0, sizeof(yyminorunion)); */
		yyminorunion = yyzerominor;
		yyStackOverflow (yypParser, &yyminorunion);
		return;
	    }
#endif
	  yypParser->yyidx = 0;
	  yypParser->yyerrcnt = -1;
	  yypParser->yystack[0].stateno = 0;
	  yypParser->yystack[0].major = 0;
      }
    yyminorunion.yy0 = yyminor;
    yyendofinput = (yymajor == 0);
    ParseARG_STORE;

#ifndef NDEBUG
    if (yyTraceFILE)
      {
	  fprintf (yyTraceFILE, "%sInput %s\n", yyTracePrompt,
		   yyTokenName[yymajor]);
      }
#endif

    do
      {
	  yyact = yy_find_shift_action (yypParser, (YYCODETYPE) yymajor);
	  if (yyact < YYNSTATE)
	    {
		assert (!yyendofinput);	/* Impossible to shift the $ token */
		yy_shift (yypParser, yyact, yymajor, &yyminorunion);
		yypParser->yyerrcnt--;
		yymajor = YYNOCODE;
	    }
	  else if (yyact < YYNSTATE + YYNRULE)
	    {
		yy_reduce (yypParser, yyact - YYNSTATE);
	    }
	  else
	    {
		assert (yyact == YY_ERROR_ACTION);
#ifdef YYERRORSYMBOL
		int yymx;
#endif
#ifndef NDEBUG
		if (yyTraceFILE)
		  {
		      fprintf (yyTraceFILE, "%sSyntax Error!\n", yyTracePrompt);
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
		if (yypParser->yyerrcnt < 0)
		  {
		      yy_syntax_error (yypParser, yymajor, yyminorunion);
		  }
		yymx = yypParser->yystack[yypParser->yyidx].major;
		if (yymx == YYERRORSYMBOL || yyerrorhit)
		  {
#ifndef NDEBUG
		      if (yyTraceFILE)
			{
			    fprintf (yyTraceFILE, "%sDiscard input token %s\n",
				     yyTracePrompt, yyTokenName[yymajor]);
			}
#endif
		      yy_destructor (yypParser, (YYCODETYPE) yymajor,
				     &yyminorunion);
		      yymajor = YYNOCODE;
		  }
		else
		  {
		      while (yypParser->yyidx >= 0 &&
			     yymx != YYERRORSYMBOL &&
			     (yyact =
			      yy_find_reduce_action (yypParser->yystack
						     [yypParser->yyidx].stateno,
						     YYERRORSYMBOL)) >=
			     YYNSTATE)
			{
			    yy_pop_parser_stack (yypParser);
			}
		      if (yypParser->yyidx < 0 || yymajor == 0)
			{
			    yy_destructor (yypParser, (YYCODETYPE) yymajor,
					   &yyminorunion);
			    yy_parse_failed (yypParser);
			    yymajor = YYNOCODE;
			}
		      else if (yymx != YYERRORSYMBOL)
			{
			    YYMINORTYPE u2;
			    u2.YYERRSYMDT = 0;
			    yy_shift (yypParser, yyact, YYERRORSYMBOL, &u2);
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
		yy_syntax_error (yypParser, yymajor, yyminorunion);
		yy_destructor (yypParser, (YYCODETYPE) yymajor, &yyminorunion);
		yymajor = YYNOCODE;

#else /* YYERRORSYMBOL is not defined */
		/* This is what we do if the grammar does not define ERROR:
		 **
		 **  * Report an error message, and throw away the input token.
		 **
		 **  * If the input token is $, then fail the parse.
		 **
		 ** As before, subsequent error messages are suppressed until
		 ** three input tokens have been successfully shifted.
		 */
		if (yypParser->yyerrcnt <= 0)
		  {
		      yy_syntax_error (yypParser, yymajor, yyminorunion);
		  }
		yypParser->yyerrcnt = 3;
		yy_destructor (yypParser, (YYCODETYPE) yymajor, &yyminorunion);
		if (yyendofinput)
		  {
		      yy_parse_failed (yypParser);
		  }
		yymajor = YYNOCODE;
#endif
	    }
      }
    while (yymajor != YYNOCODE && yypParser->yyidx >= 0);
    return;
}

/*
 VANUATU_LEMON_END - LEMON generated code ends here 
*/

















/*
** CAVEAT: there is an incompatibility between LEMON and FLEX
** this macro resolves the issue
*/
#undef yy_accept
















/*
 VANUATU_FLEX_START - FLEX generated code starts here 
*/


#line 3 "lex.VanuatuWkt.c"

#define  YY_INT_ALIGNED short int

/* A lexical scanner generated by flex */

#define yy_create_buffer VanuatuWkt_create_buffer
#define yy_delete_buffer VanuatuWkt_delete_buffer
#define yy_flex_debug VanuatuWkt_flex_debug
#define yy_init_buffer VanuatuWkt_init_buffer
#define yy_flush_buffer VanuatuWkt_flush_buffer
#define yy_load_buffer_state VanuatuWkt_load_buffer_state
#define yy_switch_to_buffer VanuatuWkt_switch_to_buffer
#define yyin VanuatuWktin
#define yyleng VanuatuWktleng
#define yylex VanuatuWktlex
#define yylineno VanuatuWktlineno
#define yyout VanuatuWktout
#define yyrestart VanuatuWktrestart
#define yytext VanuatuWkttext
#define yywrap VanuatuWktwrap
#define yyalloc VanuatuWktalloc
#define yyrealloc VanuatuWktrealloc
#define yyfree VanuatuWktfree

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
#define YY_NEW_FILE VanuatuWktrestart(VanuatuWktin  )

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

extern int VanuatuWktleng;

extern FILE *VanuatuWktin, *VanuatuWktout;

#define EOB_ACT_CONTINUE_SCAN 0
#define EOB_ACT_END_OF_FILE 1
#define EOB_ACT_LAST_MATCH 2

    #define YY_LESS_LINENO(n)
    
/* Return all but the first "n" matched characters back to the input stream. */
#define yyless(n) \
	do \
		{ \
		/* Undo effects of setting up VanuatuWkttext. */ \
        int yyless_macro_arg = (n); \
        YY_LESS_LINENO(yyless_macro_arg);\
		*yy_cp = (yy_hold_char); \
		YY_RESTORE_YY_MORE_OFFSET \
		(yy_c_buf_p) = yy_cp = yy_bp + yyless_macro_arg - YY_MORE_ADJ; \
		YY_DO_BEFORE_ACTION; /* set up VanuatuWkttext again */ \
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
	 * (via VanuatuWktrestart()), so that the user can continue scanning by
	 * just pointing VanuatuWktin at a new input file.
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

/* yy_hold_char holds the character lost when VanuatuWkttext is formed. */
static char yy_hold_char;
static int yy_n_chars;		/* number of characters read into yy_ch_buf */
int VanuatuWktleng;

/* Points to current character in buffer. */
static char *yy_c_buf_p = (char *) 0;
static int yy_init = 0;		/* whether we need to initialize */
static int yy_start = 0;	/* start state number */

/* Flag which is used to allow VanuatuWktwrap()'s to do buffer switches
 * instead of setting up a fresh VanuatuWktin.  A bit of a hack ...
 */
static int yy_did_buffer_switch_on_eof;

void VanuatuWktrestart (FILE *input_file  );
void VanuatuWkt_switch_to_buffer (YY_BUFFER_STATE new_buffer  );
YY_BUFFER_STATE VanuatuWkt_create_buffer (FILE *file,int size  );
void VanuatuWkt_delete_buffer (YY_BUFFER_STATE b  );
void VanuatuWkt_flush_buffer (YY_BUFFER_STATE b  );
void VanuatuWktpush_buffer_state (YY_BUFFER_STATE new_buffer  );
void VanuatuWktpop_buffer_state (void );

static void VanuatuWktensure_buffer_stack (void );
static void VanuatuWkt_load_buffer_state (void );
static void VanuatuWkt_init_buffer (YY_BUFFER_STATE b,FILE *file  );

#define YY_FLUSH_BUFFER VanuatuWkt_flush_buffer(YY_CURRENT_BUFFER )

YY_BUFFER_STATE VanuatuWkt_scan_buffer (char *base,yy_size_t size  );
YY_BUFFER_STATE VanuatuWkt_scan_string (yyconst char *yy_str  );
YY_BUFFER_STATE VanuatuWkt_scan_bytes (yyconst char *bytes,int len  );

void *VanuatuWktalloc (yy_size_t  );
void *VanuatuWktrealloc (void *,yy_size_t  );
void VanuatuWktfree (void *  );

#define yy_new_buffer VanuatuWkt_create_buffer

#define yy_set_interactive(is_interactive) \
	{ \
	if ( ! YY_CURRENT_BUFFER ){ \
        VanuatuWktensure_buffer_stack (); \
		YY_CURRENT_BUFFER_LVALUE =    \
            VanuatuWkt_create_buffer(VanuatuWktin,YY_BUF_SIZE ); \
	} \
	YY_CURRENT_BUFFER_LVALUE->yy_is_interactive = is_interactive; \
	}

#define yy_set_bol(at_bol) \
	{ \
	if ( ! YY_CURRENT_BUFFER ){\
        VanuatuWktensure_buffer_stack (); \
		YY_CURRENT_BUFFER_LVALUE =    \
            VanuatuWkt_create_buffer(VanuatuWktin,YY_BUF_SIZE ); \
	} \
	YY_CURRENT_BUFFER_LVALUE->yy_at_bol = at_bol; \
	}

#define YY_AT_BOL() (YY_CURRENT_BUFFER_LVALUE->yy_at_bol)

/* Begin user sect3 */

typedef unsigned char YY_CHAR;

FILE *VanuatuWktin = (FILE *) 0, *VanuatuWktout = (FILE *) 0;

typedef int yy_state_type;

extern int VanuatuWktlineno;

int VanuatuWktlineno = 1;

extern char *VanuatuWkttext;
#define yytext_ptr VanuatuWkttext

static yy_state_type yy_get_previous_state (void );
static yy_state_type yy_try_NUL_trans (yy_state_type current_state  );
static int yy_get_next_buffer (void );
static void yy_fatal_error (yyconst char msg[]  );

/* Done after the current pattern has been matched and before the
 * corresponding action - sets up VanuatuWkttext.
 */
#define YY_DO_BEFORE_ACTION \
	(yytext_ptr) = yy_bp; \
	VanuatuWktleng = (size_t) (yy_cp - yy_bp); \
	(yy_hold_char) = *yy_cp; \
	*yy_cp = '\0'; \
	(yy_c_buf_p) = yy_cp;

#define YY_NUM_RULES 36
#define YY_END_OF_BUFFER 37
/* This struct is not used in this scanner,
   but its presence is necessary. */
struct yy_trans_info
	{
	flex_int32_t yy_verify;
	flex_int32_t yy_nxt;
	};
static yyconst flex_int16_t yy_accept[114] =
    {   0,
        0,    0,   37,   35,   33,   34,    3,    4,   35,    2,
       35,    1,   35,   35,   35,   35,    1,    1,    1,    1,
        0,    0,    0,    0,    1,    1,    1,    0,    0,    0,
        0,    0,    1,    1,    0,    0,    0,    0,    0,    0,
        0,    0,    5,    0,    0,    0,    0,    0,    0,    7,
        6,    0,    0,    0,    0,    0,    8,   13,    0,    0,
        0,    0,    0,    0,   15,   14,    0,    0,    0,    0,
        0,   16,    0,    9,    0,   17,    0,    0,    0,   11,
       10,    0,    0,   19,   18,    0,    0,   12,    0,   20,
       25,    0,    0,    0,   27,   26,    0,    0,   28,    0,

       21,    0,    0,   23,   22,    0,   24,   29,    0,   31,
       30,   32,    0
    } ;

static yyconst flex_int32_t yy_ec[256] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    2,    3,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    4,    1,    1,    1,    1,    1,    1,    1,    5,
        6,    1,    7,    8,    9,   10,    1,   11,   11,   11,
       11,   11,   11,   11,   11,   11,   11,    1,    1,    1,
        1,    1,    1,    1,    1,    1,   12,    1,   13,    1,
       14,    1,   15,    1,    1,   16,   17,   18,   19,   20,
        1,   21,   22,   23,   24,    1,    1,    1,   25,   26,
        1,    1,    1,    1,    1,    1,    1,    1,   27,    1,

       28,    1,   29,    1,   30,    1,    1,   31,   32,   33,
       34,   35,    1,   36,   37,   38,   39,    1,    1,    1,
       40,   41,    1,    1,    1,    1,    1,    1,    1,    1,
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

static yyconst flex_int32_t yy_meta[42] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1
    } ;

static yyconst flex_int16_t yy_base[114] =
    {   0,
        0,    0,  246,  270,  270,  270,  270,  270,  230,  270,
      228,   32,   31,   30,   22,   28,   38,   40,  227,   42,
       35,   37,   40,   42,  226,  159,  123,   46,   51,   42,
       48,   42,   84,   75,   55,   52,   60,   53,   63,   61,
       62,   77,   84,   68,   73,   75,   83,   84,   88,  270,
       87,   88,   82,  100,   99,  108,  270,  123,  114,  110,
      118,  115,  110,  119,  270,  120,  123,  130,  125,  130,
      140,  270,  140,  157,  135,  159,  146,  150,  158,  270,
      150,  151,  160,  270,  161,  161,  175,  270,  180,  270,
      192,  185,  184,  187,  270,  188,  183,  193,  270,  193,

      210,  192,  199,  270,  198,  211,  270,  228,  217,  270,
      218,  270,  270
    } ;

static yyconst flex_int16_t yy_def[114] =
    {   0,
      113,    1,  113,  113,  113,  113,  113,  113,  113,  113,
      113,  113,  113,  113,  113,  113,  113,  113,  113,  113,
      113,  113,  113,  113,  113,  113,  113,  113,  113,  113,
      113,  113,  113,  113,  113,  113,  113,  113,  113,  113,
      113,  113,  113,  113,  113,  113,  113,  113,  113,  113,
      113,  113,  113,  113,  113,  113,  113,  113,  113,  113,
      113,  113,  113,  113,  113,  113,  113,  113,  113,  113,
      113,  113,  113,  113,  113,  113,  113,  113,  113,  113,
      113,  113,  113,  113,  113,  113,  113,  113,  113,  113,
      113,  113,  113,  113,  113,  113,  113,  113,  113,  113,

      113,  113,  113,  113,  113,  113,  113,  113,  113,  113,
      113,  113,    0
    } ;

static yyconst flex_int16_t yy_nxt[312] =
    {   0,
        4,    5,    6,    5,    7,    8,    9,   10,   11,    4,
       12,    4,    4,   13,    4,   14,   15,    4,    4,   16,
        4,    4,    4,    4,    4,    4,    4,    4,   13,    4,
       14,   15,    4,    4,   16,    4,    4,    4,    4,    4,
        4,   19,   20,   21,   22,   23,   24,   25,   17,   26,
       18,   19,   20,   28,   29,   30,   31,   32,   21,   22,
       23,   24,   35,   36,   37,   38,   39,   40,   28,   29,
       30,   31,   32,   41,   42,   43,   44,   35,   36,   37,
       38,   39,   40,   45,   46,   34,   52,   49,   41,   42,
       43,   44,   47,   53,   33,   54,   48,   55,   45,   46,

       50,   52,   56,   57,   50,   58,   59,   47,   53,   51,
       54,   48,   55,   51,   60,   50,   61,   56,   57,   50,
       58,   59,   62,   63,   51,   67,   64,   68,   51,   60,
       69,   61,   70,   27,   71,   65,   72,   62,   63,   65,
       67,   73,   68,   74,   66,   69,   75,   70,   66,   71,
       65,   72,   76,   77,   65,   78,   73,   82,   74,   66,
       79,   75,   83,   66,   86,   87,   88,   76,   77,   34,
       78,   89,   82,   80,   80,   84,   84,   90,   91,   86,
       87,   88,   81,   81,   85,   85,   89,   92,   80,   80,
       84,   84,   90,   91,   93,   94,   97,   81,   81,   85,

       85,   98,   92,   95,   99,  100,  101,  102,   95,   93,
      106,   97,   96,  103,  107,  104,   98,   96,   95,   99,
      100,  101,  102,   95,  105,  106,  104,   96,  108,  107,
      104,  109,   96,  110,  112,  105,   33,   27,   18,  105,
       17,  104,  111,  108,  110,  113,  113,  113,  110,  112,
      105,  113,  113,  111,  113,  113,  113,  111,  113,  110,
      113,  113,  113,  113,  113,  113,  113,  113,  111,    3,
      113,  113,  113,  113,  113,  113,  113,  113,  113,  113,
      113,  113,  113,  113,  113,  113,  113,  113,  113,  113,
      113,  113,  113,  113,  113,  113,  113,  113,  113,  113,

      113,  113,  113,  113,  113,  113,  113,  113,  113,  113,
      113
    } ;

static yyconst flex_int16_t yy_chk[312] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,   12,   12,   13,   14,   15,   16,   17,   17,   18,
       18,   20,   20,   21,   22,   23,   24,   24,   13,   14,
       15,   16,   28,   29,   30,   31,   32,   35,   21,   22,
       23,   24,   24,   36,   37,   38,   39,   28,   29,   30,
       31,   32,   35,   40,   41,   34,   44,   43,   36,   37,
       38,   39,   42,   45,   33,   46,   42,   47,   40,   41,

       43,   44,   48,   51,   49,   52,   53,   42,   45,   43,
       46,   42,   47,   49,   54,   43,   55,   48,   51,   49,
       52,   53,   56,   56,   43,   59,   58,   60,   49,   54,
       61,   55,   62,   27,   63,   64,   66,   56,   56,   58,
       59,   67,   60,   68,   64,   61,   69,   62,   58,   63,
       64,   66,   70,   71,   58,   73,   67,   75,   68,   64,
       74,   69,   76,   58,   77,   78,   81,   70,   71,   26,
       73,   82,   75,   74,   79,   76,   83,   85,   86,   77,
       78,   81,   74,   79,   76,   83,   82,   87,   74,   79,
       76,   83,   85,   86,   89,   91,   92,   74,   79,   76,

       83,   93,   87,   94,   96,   97,   98,  100,   91,   89,
      102,   92,   94,  101,  105,  103,   93,   91,   94,   96,
       97,   98,  100,   91,  103,  102,  101,   94,  106,  105,
      103,  108,   91,  109,  111,  101,   25,   19,   11,  103,
        9,  101,  109,  106,  108,    3,    0,    0,  109,  111,
      101,    0,    0,  108,    0,    0,    0,  109,    0,  108,
        0,    0,    0,    0,    0,    0,    0,    0,  108,  113,
      113,  113,  113,  113,  113,  113,  113,  113,  113,  113,
      113,  113,  113,  113,  113,  113,  113,  113,  113,  113,
      113,  113,  113,  113,  113,  113,  113,  113,  113,  113,

      113,  113,  113,  113,  113,  113,  113,  113,  113,  113,
      113
    } ;

static yy_state_type yy_last_accepting_state;
static char *yy_last_accepting_cpos;

extern int VanuatuWkt_flex_debug;
int VanuatuWkt_flex_debug = 0;

/* The intent behind this definition is that it'll catch
 * any uses of REJECT which flex missed.
 */
#define REJECT reject_used_but_not_detected
#define yymore() yymore_used_but_not_detected
#define YY_MORE_ADJ 0
#define YY_RESTORE_YY_MORE_OFFSET
char *VanuatuWkttext;
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

/* For debugging purposes */
int line = 1, col = 1;

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

int VanuatuWktlex_destroy (void );

int VanuatuWktget_debug (void );

void VanuatuWktset_debug (int debug_flag  );

YY_EXTRA_TYPE VanuatuWktget_extra (void );

void VanuatuWktset_extra (YY_EXTRA_TYPE user_defined  );

FILE *VanuatuWktget_in (void );

void VanuatuWktset_in  (FILE * in_str  );

FILE *VanuatuWktget_out (void );

void VanuatuWktset_out  (FILE * out_str  );

int VanuatuWktget_leng (void );

char *VanuatuWktget_text (void );

int VanuatuWktget_lineno (void );

void VanuatuWktset_lineno (int line_number  );

/* Macros after this point can all be overridden by user definitions in
 * section 1.
 */

#ifndef YY_SKIP_YYWRAP
#ifdef __cplusplus
extern "C" int VanuatuWktwrap (void );
#else
extern int VanuatuWktwrap (void );
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
#define ECHO do { if (fwrite( VanuatuWkttext, VanuatuWktleng, 1, VanuatuWktout )) {} } while (0)
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
			     (c = getc( VanuatuWktin )) != EOF && c != '\n'; ++n ) \
			buf[n] = (char) c; \
		if ( c == '\n' ) \
			buf[n++] = (char) c; \
		if ( c == EOF && ferror( VanuatuWktin ) ) \
			YY_FATAL_ERROR( "input in flex scanner failed" ); \
		result = n; \
		} \
	else \
		{ \
		errno=0; \
		while ( (result = fread(buf, 1, max_size, VanuatuWktin))==0 && ferror(VanuatuWktin)) \
			{ \
			if( errno != EINTR) \
				{ \
				YY_FATAL_ERROR( "input in flex scanner failed" ); \
				break; \
				} \
			errno=0; \
			clearerr(VanuatuWktin); \
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

extern int VanuatuWktlex (void);

#define YY_DECL int VanuatuWktlex (void)
#endif /* !YY_DECL */

/* Code executed at the beginning of each rule, after VanuatuWkttext and VanuatuWktleng
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

		if ( ! VanuatuWktin )
			VanuatuWktin = stdin;

		if ( ! VanuatuWktout )
			VanuatuWktout = stdout;

		if ( ! YY_CURRENT_BUFFER ) {
			VanuatuWktensure_buffer_stack ();
			YY_CURRENT_BUFFER_LVALUE =
				VanuatuWkt_create_buffer(VanuatuWktin,YY_BUF_SIZE );
		}

		VanuatuWkt_load_buffer_state( );
		}

	while ( 1 )		/* loops until end-of-file is reached */
		{
		yy_cp = (yy_c_buf_p);

		/* Support of VanuatuWkttext. */
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
				if ( yy_current_state >= 114 )
					yy_c = yy_meta[(unsigned int) yy_c];
				}
			yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
			++yy_cp;
			}
		while ( yy_base[yy_current_state] != 270 );

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
{ col += (int) strlen(VanuatuWkttext);  VanuatuWktlval.dval = atof(VanuatuWkttext); return VANUATU_NUM; }
	YY_BREAK
case 2:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_COMMA; }
	YY_BREAK
case 3:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_OPEN_BRACKET; }
	YY_BREAK
case 4:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_CLOSE_BRACKET; }
	YY_BREAK
case 5:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_POINT; }
	YY_BREAK
case 6:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_POINT_Z; }
	YY_BREAK
case 7:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_POINT_M; }
	YY_BREAK
case 8:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_POINT_ZM; }
	YY_BREAK
case 9:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_LINESTRING; }
	YY_BREAK
case 10:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_LINESTRING_Z; }
	YY_BREAK
case 11:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_LINESTRING_M; }
	YY_BREAK
case 12:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_LINESTRING_ZM; }
	YY_BREAK
case 13:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_POLYGON; }
	YY_BREAK
case 14:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_POLYGON_Z; }
	YY_BREAK
case 15:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_POLYGON_M; }
	YY_BREAK
case 16:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_POLYGON_ZM; }
	YY_BREAK
case 17:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_MULTIPOINT; }
	YY_BREAK
case 18:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_MULTIPOINT_Z; }
	YY_BREAK
case 19:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_MULTIPOINT_M; }
	YY_BREAK
case 20:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_MULTIPOINT_ZM; }
	YY_BREAK
case 21:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_MULTILINESTRING; }
	YY_BREAK
case 22:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_MULTILINESTRING_Z; }
	YY_BREAK
case 23:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_MULTILINESTRING_M; }
	YY_BREAK
case 24:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_MULTILINESTRING_ZM; }	
	YY_BREAK
case 25:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_MULTIPOLYGON; }
	YY_BREAK
case 26:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_MULTIPOLYGON_Z; }
	YY_BREAK
case 27:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_MULTIPOLYGON_M; }
	YY_BREAK
case 28:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_MULTIPOLYGON_ZM; }
	YY_BREAK
case 29:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_GEOMETRYCOLLECTION; }
	YY_BREAK
case 30:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_GEOMETRYCOLLECTION_Z; }
	YY_BREAK
case 31:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_GEOMETRYCOLLECTION_M; }
	YY_BREAK
case 32:
YY_RULE_SETUP
{ VanuatuWktlval.dval = 0; return VANUATU_GEOMETRYCOLLECTION_ZM; }
	YY_BREAK
case 33:
YY_RULE_SETUP
{ col += (int) strlen(VanuatuWkttext); }               /* ignore but count white space */
	YY_BREAK
case 34:
/* rule 34 can match eol */
YY_RULE_SETUP
{ col = 0; ++line; return VANUATU_NEWLINE; }
	YY_BREAK
case 35:
YY_RULE_SETUP
{ col += (int) strlen(VanuatuWkttext); return -1; }
	YY_BREAK
case 36:
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
			 * just pointed VanuatuWktin at a new source and called
			 * VanuatuWktlex().  If so, then we have to assure
			 * consistency between YY_CURRENT_BUFFER and our
			 * globals.  Here is the right place to do so, because
			 * this is the first action (other than possibly a
			 * back-up) that will match for the new input source.
			 */
			(yy_n_chars) = YY_CURRENT_BUFFER_LVALUE->yy_n_chars;
			YY_CURRENT_BUFFER_LVALUE->yy_input_file = VanuatuWktin;
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

				if ( VanuatuWktwrap( ) )
					{
					/* Note: because we've taken care in
					 * yy_get_next_buffer() to have set up
					 * VanuatuWkttext, we can now set up
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
} /* end of VanuatuWktlex */

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
					VanuatuWktrealloc((void *) b->yy_ch_buf,b->yy_buf_size + 2  );
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
			VanuatuWktrestart(VanuatuWktin  );
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
		YY_CURRENT_BUFFER_LVALUE->yy_ch_buf = (char *) VanuatuWktrealloc((void *) YY_CURRENT_BUFFER_LVALUE->yy_ch_buf,new_size  );
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
			if ( yy_current_state >= 114 )
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
		if ( yy_current_state >= 114 )
			yy_c = yy_meta[(unsigned int) yy_c];
		}
	yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
	yy_is_jam = (yy_current_state == 113);

	return yy_is_jam ? 0 : yy_current_state;
}

    static void yyunput (int c, register char * yy_bp )
{
	register char *yy_cp;
    
    yy_cp = (yy_c_buf_p);

	/* undo effects of setting up VanuatuWkttext */
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
					VanuatuWktrestart(VanuatuWktin );

					/*FALLTHROUGH*/

				case EOB_ACT_END_OF_FILE:
					{
					if ( VanuatuWktwrap( ) )
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
	*(yy_c_buf_p) = '\0';	/* preserve VanuatuWkttext */
	(yy_hold_char) = *++(yy_c_buf_p);

	return c;
}
#endif	/* ifndef YY_NO_INPUT */

/** Immediately switch to a different input stream.
 * @param input_file A readable stream.
 * 
 * @note This function does not reset the start condition to @c INITIAL .
 */
    void VanuatuWktrestart  (FILE * input_file )
{
    
	if ( ! YY_CURRENT_BUFFER ){
        VanuatuWktensure_buffer_stack ();
		YY_CURRENT_BUFFER_LVALUE =
            VanuatuWkt_create_buffer(VanuatuWktin,YY_BUF_SIZE );
	}

	VanuatuWkt_init_buffer(YY_CURRENT_BUFFER,input_file );
	VanuatuWkt_load_buffer_state( );
}

/** Switch to a different input buffer.
 * @param new_buffer The new input buffer.
 * 
 */
    void VanuatuWkt_switch_to_buffer  (YY_BUFFER_STATE  new_buffer )
{
    
	/* TODO. We should be able to replace this entire function body
	 * with
	 *		VanuatuWktpop_buffer_state();
	 *		VanuatuWktpush_buffer_state(new_buffer);
     */
	VanuatuWktensure_buffer_stack ();
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
	VanuatuWkt_load_buffer_state( );

	/* We don't actually know whether we did this switch during
	 * EOF (VanuatuWktwrap()) processing, but the only time this flag
	 * is looked at is after VanuatuWktwrap() is called, so it's safe
	 * to go ahead and always set it.
	 */
	(yy_did_buffer_switch_on_eof) = 1;
}

static void VanuatuWkt_load_buffer_state  (void)
{
    	(yy_n_chars) = YY_CURRENT_BUFFER_LVALUE->yy_n_chars;
	(yytext_ptr) = (yy_c_buf_p) = YY_CURRENT_BUFFER_LVALUE->yy_buf_pos;
	VanuatuWktin = YY_CURRENT_BUFFER_LVALUE->yy_input_file;
	(yy_hold_char) = *(yy_c_buf_p);
}

/** Allocate and initialize an input buffer state.
 * @param file A readable stream.
 * @param size The character buffer size in bytes. When in doubt, use @c YY_BUF_SIZE.
 * 
 * @return the allocated buffer state.
 */
    YY_BUFFER_STATE VanuatuWkt_create_buffer  (FILE * file, int  size )
{
	YY_BUFFER_STATE b;
    
	b = (YY_BUFFER_STATE) VanuatuWktalloc(sizeof( struct yy_buffer_state )  );
	if ( ! b )
		YY_FATAL_ERROR( "out of dynamic memory in VanuatuWkt_create_buffer()" );

	b->yy_buf_size = size;

	/* yy_ch_buf has to be 2 characters longer than the size given because
	 * we need to put in 2 end-of-buffer characters.
	 */
	b->yy_ch_buf = (char *) VanuatuWktalloc(b->yy_buf_size + 2  );
	if ( ! b->yy_ch_buf )
		YY_FATAL_ERROR( "out of dynamic memory in VanuatuWkt_create_buffer()" );

	b->yy_is_our_buffer = 1;

	VanuatuWkt_init_buffer(b,file );

	return b;
}

/** Destroy the buffer.
 * @param b a buffer created with VanuatuWkt_create_buffer()
 * 
 */
    void VanuatuWkt_delete_buffer (YY_BUFFER_STATE  b )
{
    
	if ( ! b )
		return;

	if ( b == YY_CURRENT_BUFFER ) /* Not sure if we should pop here. */
		YY_CURRENT_BUFFER_LVALUE = (YY_BUFFER_STATE) 0;

	if ( b->yy_is_our_buffer )
		VanuatuWktfree((void *) b->yy_ch_buf  );

	VanuatuWktfree((void *) b  );
}

#ifndef __cplusplus
extern int isatty (int );
#endif /* __cplusplus */
    
/* Initializes or reinitializes a buffer.
 * This function is sometimes called more than once on the same buffer,
 * such as during a VanuatuWktrestart() or at EOF.
 */
    static void VanuatuWkt_init_buffer  (YY_BUFFER_STATE  b, FILE * file )

{
	int oerrno = errno;
    
	VanuatuWkt_flush_buffer(b );

	b->yy_input_file = file;
	b->yy_fill_buffer = 1;

    /* If b is the current buffer, then VanuatuWkt_init_buffer was _probably_
     * called from VanuatuWktrestart() or through yy_get_next_buffer.
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
    void VanuatuWkt_flush_buffer (YY_BUFFER_STATE  b )
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
		VanuatuWkt_load_buffer_state( );
}

/** Pushes the new state onto the stack. The new state becomes
 *  the current state. This function will allocate the stack
 *  if necessary.
 *  @param new_buffer The new state.
 *  
 */
void VanuatuWktpush_buffer_state (YY_BUFFER_STATE new_buffer )
{
    	if (new_buffer == NULL)
		return;

	VanuatuWktensure_buffer_stack();

	/* This block is copied from VanuatuWkt_switch_to_buffer. */
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

	/* copied from VanuatuWkt_switch_to_buffer. */
	VanuatuWkt_load_buffer_state( );
	(yy_did_buffer_switch_on_eof) = 1;
}

/** Removes and deletes the top of the stack, if present.
 *  The next element becomes the new top.
 *  
 */
void VanuatuWktpop_buffer_state (void)
{
    	if (!YY_CURRENT_BUFFER)
		return;

	VanuatuWkt_delete_buffer(YY_CURRENT_BUFFER );
	YY_CURRENT_BUFFER_LVALUE = NULL;
	if ((yy_buffer_stack_top) > 0)
		--(yy_buffer_stack_top);

	if (YY_CURRENT_BUFFER) {
		VanuatuWkt_load_buffer_state( );
		(yy_did_buffer_switch_on_eof) = 1;
	}
}

/* Allocates the stack if it does not exist.
 *  Guarantees space for at least one push.
 */
static void VanuatuWktensure_buffer_stack (void)
{
	int num_to_alloc;
    
	if (!(yy_buffer_stack)) {

		/* First allocation is just for 2 elements, since we don't know if this
		 * scanner will even need a stack. We use 2 instead of 1 to avoid an
		 * immediate realloc on the next call.
         */
		num_to_alloc = 1;
		(yy_buffer_stack) = (struct yy_buffer_state**)VanuatuWktalloc
								(num_to_alloc * sizeof(struct yy_buffer_state*)
								);
		if ( ! (yy_buffer_stack) )
			YY_FATAL_ERROR( "out of dynamic memory in VanuatuWktensure_buffer_stack()" );
								  
		memset((yy_buffer_stack), 0, num_to_alloc * sizeof(struct yy_buffer_state*));
				
		(yy_buffer_stack_max) = num_to_alloc;
		(yy_buffer_stack_top) = 0;
		return;
	}

	if ((yy_buffer_stack_top) >= ((yy_buffer_stack_max)) - 1){

		/* Increase the buffer to prepare for a possible push. */
		int grow_size = 8 /* arbitrary grow size */;

		num_to_alloc = (yy_buffer_stack_max) + grow_size;
		(yy_buffer_stack) = (struct yy_buffer_state**)VanuatuWktrealloc
								((yy_buffer_stack),
								num_to_alloc * sizeof(struct yy_buffer_state*)
								);
		if ( ! (yy_buffer_stack) )
			YY_FATAL_ERROR( "out of dynamic memory in VanuatuWktensure_buffer_stack()" );

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
YY_BUFFER_STATE VanuatuWkt_scan_buffer  (char * base, yy_size_t  size )
{
	YY_BUFFER_STATE b;
    
	if ( size < 2 ||
	     base[size-2] != YY_END_OF_BUFFER_CHAR ||
	     base[size-1] != YY_END_OF_BUFFER_CHAR )
		/* They forgot to leave room for the EOB's. */
		return 0;

	b = (YY_BUFFER_STATE) VanuatuWktalloc(sizeof( struct yy_buffer_state )  );
	if ( ! b )
		YY_FATAL_ERROR( "out of dynamic memory in VanuatuWkt_scan_buffer()" );

	b->yy_buf_size = size - 2;	/* "- 2" to take care of EOB's */
	b->yy_buf_pos = b->yy_ch_buf = base;
	b->yy_is_our_buffer = 0;
	b->yy_input_file = 0;
	b->yy_n_chars = b->yy_buf_size;
	b->yy_is_interactive = 0;
	b->yy_at_bol = 1;
	b->yy_fill_buffer = 0;
	b->yy_buffer_status = YY_BUFFER_NEW;

	VanuatuWkt_switch_to_buffer(b  );

	return b;
}

/** Setup the input buffer state to scan a string. The next call to VanuatuWktlex() will
 * scan from a @e copy of @a str.
 * @param yystr a NUL-terminated string to scan
 * 
 * @return the newly allocated buffer state object.
 * @note If you want to scan bytes that may contain NUL values, then use
 *       VanuatuWkt_scan_bytes() instead.
 */
YY_BUFFER_STATE VanuatuWkt_scan_string (yyconst char * yystr )
{
    
	return VanuatuWkt_scan_bytes(yystr,strlen(yystr) );
}

/** Setup the input buffer state to scan the given bytes. The next call to VanuatuWktlex() will
 * scan from a @e copy of @a bytes.
 * @param yybytes the byte buffer to scan
 * @param _yybytes_len the number of bytes in the buffer pointed to by @a bytes.
 * 
 * @return the newly allocated buffer state object.
 */
YY_BUFFER_STATE VanuatuWkt_scan_bytes  (yyconst char * yybytes, int  _yybytes_len )
{
	YY_BUFFER_STATE b;
	char *buf;
	yy_size_t n;
	int i;
    
	/* Get memory for full buffer, including space for trailing EOB's. */
	n = _yybytes_len + 2;
	buf = (char *) VanuatuWktalloc(n  );
	if ( ! buf )
		YY_FATAL_ERROR( "out of dynamic memory in VanuatuWkt_scan_bytes()" );

	for ( i = 0; i < _yybytes_len; ++i )
		buf[i] = yybytes[i];

	buf[_yybytes_len] = buf[_yybytes_len+1] = YY_END_OF_BUFFER_CHAR;

	b = VanuatuWkt_scan_buffer(buf,n );
	if ( ! b )
		YY_FATAL_ERROR( "bad buffer in VanuatuWkt_scan_bytes()" );

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
		/* Undo effects of setting up VanuatuWkttext. */ \
        int yyless_macro_arg = (n); \
        YY_LESS_LINENO(yyless_macro_arg);\
		VanuatuWkttext[VanuatuWktleng] = (yy_hold_char); \
		(yy_c_buf_p) = VanuatuWkttext + yyless_macro_arg; \
		(yy_hold_char) = *(yy_c_buf_p); \
		*(yy_c_buf_p) = '\0'; \
		VanuatuWktleng = yyless_macro_arg; \
		} \
	while ( 0 )

/* Accessor  methods (get/set functions) to struct members. */

/** Get the current line number.
 * 
 */
int VanuatuWktget_lineno  (void)
{
        
    return VanuatuWktlineno;
}

/** Get the input stream.
 * 
 */
FILE *VanuatuWktget_in  (void)
{
        return VanuatuWktin;
}

/** Get the output stream.
 * 
 */
FILE *VanuatuWktget_out  (void)
{
        return VanuatuWktout;
}

/** Get the length of the current token.
 * 
 */
int VanuatuWktget_leng  (void)
{
        return VanuatuWktleng;
}

/** Get the current token.
 * 
 */

char *VanuatuWktget_text  (void)
{
        return VanuatuWkttext;
}

/** Set the current line number.
 * @param line_number
 * 
 */
void VanuatuWktset_lineno (int  line_number )
{
    
    VanuatuWktlineno = line_number;
}

/** Set the input stream. This does not discard the current
 * input buffer.
 * @param in_str A readable stream.
 * 
 * @see VanuatuWkt_switch_to_buffer
 */
void VanuatuWktset_in (FILE *  in_str )
{
        VanuatuWktin = in_str ;
}

void VanuatuWktset_out (FILE *  out_str )
{
        VanuatuWktout = out_str ;
}

int VanuatuWktget_debug  (void)
{
        return VanuatuWkt_flex_debug;
}

void VanuatuWktset_debug (int  bdebug )
{
        VanuatuWkt_flex_debug = bdebug ;
}

static int yy_init_globals (void)
{
        /* Initialization is the same as for the non-reentrant scanner.
     * This function is called from VanuatuWktlex_destroy(), so don't allocate here.
     */

    (yy_buffer_stack) = 0;
    (yy_buffer_stack_top) = 0;
    (yy_buffer_stack_max) = 0;
    (yy_c_buf_p) = (char *) 0;
    (yy_init) = 0;
    (yy_start) = 0;

/* Defined in main.c */
#ifdef YY_STDINIT
    VanuatuWktin = stdin;
    VanuatuWktout = stdout;
#else
    VanuatuWktin = (FILE *) 0;
    VanuatuWktout = (FILE *) 0;
#endif

    /* For future reference: Set errno on error, since we are called by
     * VanuatuWktlex_init()
     */
    return 0;
}

/* VanuatuWktlex_destroy is for both reentrant and non-reentrant scanners. */
int VanuatuWktlex_destroy  (void)
{
    
    /* Pop the buffer stack, destroying each element. */
	while(YY_CURRENT_BUFFER){
		VanuatuWkt_delete_buffer(YY_CURRENT_BUFFER  );
		YY_CURRENT_BUFFER_LVALUE = NULL;
		VanuatuWktpop_buffer_state();
	}

	/* Destroy the stack itself. */
	VanuatuWktfree((yy_buffer_stack) );
	(yy_buffer_stack) = NULL;

    /* Reset the globals. This is important in a non-reentrant scanner so the next time
     * VanuatuWktlex() is called, initialization will occur. */
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

void *VanuatuWktalloc (yy_size_t  size )
{
	return (void *) malloc( size );
}

void *VanuatuWktrealloc  (void * ptr, yy_size_t  size )
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

void VanuatuWktfree (void * ptr )
{
	free( (char *) ptr );	/* see VanuatuWktrealloc() for (char *) cast */
}

#define YYTABLES_NAME "yytables"

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

int VanuatuWktwrap(void)
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


/* 
 VANUATU_FLEX_END - FLEX generated code ends here 
*/









/*
** This is a linked-list struct to store all the values for each token.
** All tokens will have a value of 0, except tokens denoted as NUM.
** NUM tokens are geometry coordinates and will contain the floating
** point number.
*/
typedef struct gaiaFlexTokenStruct
{
    double value;
    struct gaiaFlexTokenStruct *Next;
} gaiaFlexToken;

/*
** Function to clean up the linked-list of token values.
*/
static int
vanuatu_cleanup (gaiaFlexToken * token)
{
    gaiaFlexToken *ptok;
    gaiaFlexToken *ptok_n;
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
void
ParseFree (void *p,		/* The parser to be deleted */
	   void (*freeProc) (void *)	/* Function used to reclaim memory */
    )
{
    yyParser *pParser = (yyParser *) p;
    if (pParser == 0)
	return;
    while (pParser->yyidx >= 0)
	yy_pop_parser_stack (pParser);
#if YYSTACKDEPTH<=0
    free (pParser->yystack);
#endif
    VanuatuWktlex_destroy ();
    (*freeProc) ((void *) pParser);
}

gaiaGeomCollPtr
gaiaParseWkt (const unsigned char *dirty_buffer, short type)
{
    void *pParser = ParseAlloc (malloc);
    /* Linked-list of token values */
    gaiaFlexToken *tokens = malloc (sizeof (gaiaFlexToken));
    /* Pointer to the head of the list */
    gaiaFlexToken *head = tokens;
    int yv;
    gaiaGeomCollPtr result = NULL;

    /*
     ** Sandro Furieri 2010 Apr 4
     ** unsetting the parser error flag
     */
    vanuatu_parse_error = 0;

    VanuatuWkt_scan_string ((char *) dirty_buffer);

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
	  tokens->Next = malloc (sizeof (gaiaFlexToken));
	  /*
	     /VanuatuWktlval is a global variable from FLEX.
	     /VanuatuWktlval is defined in vanuatuLexglobal.h
	   */
	  tokens->Next->value = VanuatuWktlval.dval;
	  /* Pass the token to the wkt parser created from lemon */
	  Parse (pParser, yv, &(tokens->Next->value), &result);
	  tokens = tokens->Next;
      }
    /* This denotes the end of a line as well as the end of the parser */
    Parse (pParser, VANUATU_NEWLINE, 0, &result);
    ParseFree (pParser, free);

    /* Assigning the token as the end to avoid seg faults while cleaning */
    tokens->Next = NULL;
    vanuatu_cleanup (head);

    /*
     ** Sandro Furieri 2010 Apr 4
     ** checking if any parsing error was encountered 
     */
    if (vanuatu_parse_error)
      {
	  if (result)
	      gaiaFreeGeomColl (result);
	  return NULL;
      }

    /*
     ** Sandro Furieri 2010 Apr 4
     ** final checkup for validity
     */
    if (!result)
	return NULL;
    if (!checkValidity (result))
      {
	  gaiaFreeGeomColl (result);
	  return NULL;
      }
    if (type < 0)
	;			/* no restrinction about GEOMETRY CLASS TYPE */
    else
      {
	  if (result->DeclaredType != type)
	    {
		/* invalid CLASS TYPE for request */
		gaiaFreeGeomColl (result);
		return NULL;
	    }
      }

    gaiaMbrGeometry (result);

    return result;
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
