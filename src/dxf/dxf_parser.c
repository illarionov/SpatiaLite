/*

 dxf_parser.c -- implements DXF support [parsing]

 version 4.1, 2013 May 14

 Author: Sandro Furieri a.furieri@lqt.it

 -----------------------------------------------------------------------------
 
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
 
Portions created by the Initial Developer are Copyright (C) 2008-2013
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#include <spatialite/sqlite.h>
#include <spatialite/debug.h>

#include <spatialite/gaiageo.h>
#include <spatialite/gg_dxf.h>
#include <spatialite.h>

typedef struct dxf_segment
{
/* a DXF segment */
    int valid;
    double ax;
    double ay;
    double az;
    double bx;
    double by;
    double bz;
} dxfSegment;
typedef dxfSegment *dxfSegmentPtr;

typedef struct dxf_linked_segments
{
/* a collection of DXF segments */
    int count;
    dxfSegmentPtr segments;
} dxfLinkedSegments;
typedef dxfLinkedSegments *dxfLinkedSegmentsPtr;

typedef struct dxf_rings_collection
{
/* a collection of rings */
    gaiaDxfPolylinePtr first;
    gaiaDxfPolylinePtr last;
} dxfRingsCollection;
typedef dxfRingsCollection *dxfRingsCollectionPtr;



static gaiaDxfHolePtr
alloc_dxf_hole (int points)
{
/* allocating and initializing a DXF Polygon Hole object */
    int i;
    gaiaDxfHolePtr hole = malloc (sizeof (gaiaDxfHole));
    hole->points = points;
    hole->x = malloc (sizeof (double) * points);
    hole->y = malloc (sizeof (double) * points);
    hole->z = malloc (sizeof (double) * points);
    for (i = 0; i < points; i++)
      {
	  *(hole->x + i) = 0.0;
	  *(hole->y + i) = 0.0;
	  *(hole->z + i) = 0.0;
      }
    hole->next = NULL;
    return hole;
}

static void
insert_dxf_hole (gaiaDxfPolylinePtr line, gaiaDxfHolePtr hole)
{
/* inserting an Hole into a Polygon */
    if (line->first_hole == NULL)
	line->first_hole = hole;
    if (line->last_hole != NULL)
	line->last_hole->next = hole;
    line->last_hole = hole;
}

static void
destroy_dxf_hole (gaiaDxfHolePtr hole)
{
/* memory cleanup - destroying a DXF Hole object */
    if (hole == NULL)
	return;
    if (hole->x != NULL)
	free (hole->x);
    if (hole->y != NULL)
	free (hole->y);
    if (hole->z != NULL)
	free (hole->z);
    free (hole);
}

static void
linked_rings (gaiaDxfPolylinePtr line)
{
/* attempt to identify linked Polygon rings */
    int i;
    int i2;
    int match = 0;
    double x;
    double y;
    double z;
    gaiaDxfHolePtr hole;
    dxfLinkedSegmentsPtr coll;
    dxfSegmentPtr seg;
    dxfSegmentPtr seg2;
    gaiaGeomCollPtr geom;
    gaiaGeomCollPtr result;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    int pgs;
    int ok;

    if (line == NULL)
	return;
    if (line->points <= 0)
	return;

    coll = malloc (sizeof (dxfLinkedSegments));
    coll->count = line->points - 1;
    coll->segments = malloc (sizeof (dxfSegment) * coll->count);
    x = *(line->x + 0);
    y = *(line->y + 0);
    z = *(line->z + 0);
    for (i2 = 0, i = 1; i < line->points; i++, i2++)
      {
	  /* initializing all segments */
	  seg = &(coll->segments[i2]);
	  seg->valid = 1;
	  seg->ax = x;
	  seg->ay = y;
	  seg->az = z;
	  x = *(line->x + i);
	  y = *(line->y + i);
	  z = *(line->z + i);
	  seg->bx = x;
	  seg->by = y;
	  seg->bz = z;
      }

    for (i = 0; i < coll->count - 1; i++)
      {
	  /* testing for linked polygon holes */
	  seg = &(coll->segments[i]);
	  if (seg->valid == 0)
	      continue;
	  for (i2 = i + 1; i2 < coll->count; i2++)
	    {
		seg2 = &(coll->segments[i2]);
		if (seg2->valid == 0)
		    continue;
		if ((seg->ax == seg2->ax && seg->ay == seg2->ay
		     && seg->az == seg2->az && seg->bx == seg2->bx
		     && seg->by == seg2->by && seg->bz == seg2->bz)
		    || (seg->ax == seg2->bx && seg->ay == seg2->by
			&& seg->az == seg2->bz && seg->bx == seg2->ax
			&& seg->by == seg2->ay && seg->bz == seg2->az))
		  {
		      /* found a linked segment */
		      seg->valid = 0;
		      seg2->valid = 0;
		      match = 1;
		  }
	    }
      }

    if (match == 0)
      {
	  /* no link segment was found - quitting */
	  free (coll->segments);
	  free (coll);
	  return;
      }

/* building a candidate geometry (multilinestring) */
    geom = gaiaAllocGeomCollXYZ ();
    for (i = 0; i < coll->count; i++)
      {
	  seg = &(coll->segments[i]);
	  if (seg->valid)
	    {
		/* inserting a valid segment into the collection */
		gaiaLinestringPtr ln = gaiaAddLinestringToGeomColl (geom, 2);
		gaiaSetPointXYZ (ln->Coords, 0, seg->ax, seg->ay, seg->az);
		gaiaSetPointXYZ (ln->Coords, 1, seg->bx, seg->by, seg->bz);
	    }
      }
/* freeing the linked segments list */
    free (coll->segments);
    free (coll);

/* attempting to reassemble a polygon */
    result = gaiaPolygonize (geom, 0);
    gaiaFreeGeomColl (geom);
    if (result == NULL)
	return;

/* checking the returned polygon for validity */
    pgs = 0;
    ok = 1;
    pg = result->FirstPolygon;
    while (pg != NULL)
      {
	  pgs++;
	  if (pg->NumInteriors == 0)
	      ok = 0;
	  pg = pg->Next;
      }
    if (ok == 1 && pgs == 1)
      {
	  /* found a valid Polygon with internal holes(s) */
	  pg = result->FirstPolygon;
	  rng = pg->Exterior;
	  /* rebuilding the exterior ring */
	  free (line->x);
	  free (line->y);
	  free (line->z);
	  line->points = rng->Points;
	  line->x = malloc (sizeof (double) * line->points);
	  line->y = malloc (sizeof (double) * line->points);
	  line->z = malloc (sizeof (double) * line->points);
	  for (i = 0; i < line->points; i++)
	    {
		/* setting the exterior ring points */
		gaiaGetPointXYZ (rng->Coords, i, &x, &y, &z);
		*(line->x + i) = x;
		*(line->y + i) = y;
		*(line->z + i) = z;
	    }
	  for (i2 = 0; i2 < pg->NumInteriors; i2++)
	    {
		/* saving the Holes */
		rng = pg->Interiors + i2;
		hole = alloc_dxf_hole (rng->Points);
		insert_dxf_hole (line, hole);
		for (i = 0; i < hole->points; i++)
		  {
		      /* setting the interior ring points */
		      gaiaGetPointXYZ (rng->Coords, i, &x, &y, &z);
		      *(hole->x + i) = x;
		      *(hole->y + i) = y;
		      *(hole->z + i) = z;
		  }
	    }
      }
    gaiaFreeGeomColl (result);
/* forcing the closure flag */
    line->is_closed = 1;
}

static gaiaDxfExtraAttrPtr
alloc_dxf_extra ()
{
/* allocating and initializing a DXF Extra Attribute object */
    gaiaDxfExtraAttrPtr ext = malloc (sizeof (gaiaDxfExtraAttr));
    ext->key = NULL;
    ext->value = NULL;
    ext->next = NULL;
    return ext;
}

static void
destroy_dxf_extra (gaiaDxfExtraAttrPtr ext)
{
/* memory cleanup - destroying a DXF Extra Attribute object */
    if (ext == NULL)
	return;
    if (ext->key != NULL)
	free (ext->key);
    if (ext->value != NULL)
	free (ext->value);
    free (ext);
}

static gaiaDxfTextPtr
alloc_dxf_text (const char *label, double x, double y, double z, double angle)
{
/* allocating and initializing a DXF Text object */
    int len;
    gaiaDxfTextPtr txt = malloc (sizeof (gaiaDxfText));
    len = strlen (label);
    txt->label = malloc (len + 1);
    strcpy (txt->label, label);
    txt->x = x;
    txt->y = y;
    txt->z = z;
    txt->angle = angle;
    txt->first = NULL;
    txt->last = NULL;
    txt->next = NULL;
    return txt;
}

static void
destroy_dxf_text (gaiaDxfTextPtr txt)
{
/* memory cleanup - destroying a DXF Text object */
    gaiaDxfExtraAttrPtr ext;
    gaiaDxfExtraAttrPtr n_ext;
    if (txt == NULL)
	return;
    if (txt->label != NULL)
	free (txt->label);
    ext = txt->first;
    while (ext != NULL)
      {
	  n_ext = ext->next;
	  destroy_dxf_extra (ext);
	  ext = n_ext;
      }
    free (txt);
}

static int
is_3d_text (gaiaDxfTextPtr txt)
{
/* testing if it's really a 3D Text */
    if (txt->z == 0.0)
	return 0;
    return 1;
}

static void
insert_dxf_text (gaiaDxfParserPtr dxf, const char *layer_name,
		 gaiaDxfTextPtr txt)
{
/* inserting a TEXT object into the appropriate Layer */
    gaiaDxfLayerPtr lyr = dxf->first;
    while (lyr != NULL)
      {
	  if (strcmp (lyr->layer_name, layer_name) == 0)
	    {
		/* found the matching Layer */
		if (lyr->first_text == NULL)
		    lyr->first_text = txt;
		if (lyr->last_text != NULL)
		    lyr->last_text->next = txt;
		lyr->last_text = txt;
		if (dxf->force_dims == GAIA_DXF_FORCE_2D
		    || dxf->force_dims == GAIA_DXF_FORCE_3D)
		    ;
		else
		  {
		      if (is_3d_text (txt))
			  lyr->is3Dtext = 1;
		  }
		txt->first = dxf->first_ext;
		txt->last = dxf->last_ext;
		dxf->first_ext = NULL;
		dxf->last_ext = NULL;
		if (txt->first != NULL)
		    lyr->hasExtraText = 1;
		return;
	    }
	  lyr = lyr->next;
      }
    destroy_dxf_text (txt);
}

static gaiaDxfPointPtr
alloc_dxf_point (double x, double y, double z)
{
/* allocating and initializing a DXF Point object */
    gaiaDxfPointPtr pt = malloc (sizeof (gaiaDxfPoint));
    pt->x = x;
    pt->y = y;
    pt->z = z;
    pt->next = NULL;
    pt->first = NULL;
    pt->last = NULL;
    return pt;
}

static void
destroy_dxf_point (gaiaDxfPointPtr pt)
{
/* memory cleanup - destroying a DXF Point object */
    gaiaDxfExtraAttrPtr ext;
    gaiaDxfExtraAttrPtr n_ext;
    if (pt == NULL)
	return;
    ext = pt->first;
    while (ext != NULL)
      {
	  n_ext = ext->next;
	  destroy_dxf_extra (ext);
	  ext = n_ext;
      }
    free (pt);
}

static int
is_3d_point (gaiaDxfPointPtr pt)
{
/* testing if it's really a 3D Point */
    if (pt->z == 0.0)
	return 0;
    return 1;
}

static void
insert_dxf_point (gaiaDxfParserPtr dxf, const char *layer_name,
		  gaiaDxfPointPtr pt)
{
/* inserting a POINT object into the appropriate Layer */
    gaiaDxfLayerPtr lyr = dxf->first;
    while (lyr != NULL)
      {
	  if (strcmp (lyr->layer_name, layer_name) == 0)
	    {
		/* found the matching Layer */
		if (lyr->first_point == NULL)
		    lyr->first_point = pt;
		if (lyr->last_point != NULL)
		    lyr->last_point->next = pt;
		lyr->last_point = pt;
		if (dxf->force_dims == GAIA_DXF_FORCE_2D
		    || dxf->force_dims == GAIA_DXF_FORCE_3D)
		    ;
		else
		  {
		      if (is_3d_point (pt))
			  lyr->is3Dpoint = 1;
		  }
		pt->first = dxf->first_ext;
		pt->last = dxf->last_ext;
		dxf->first_ext = NULL;
		dxf->last_ext = NULL;
		if (pt->first != NULL)
		    lyr->hasExtraPoint = 1;
		return;
	    }
	  lyr = lyr->next;
      }
    destroy_dxf_point (pt);
}

static gaiaDxfPolylinePtr
alloc_dxf_polyline (int is_closed, int points)
{
/* allocating and initializing a DXF Polyline object */
    int i;
    gaiaDxfPolylinePtr ln = malloc (sizeof (gaiaDxfPolyline));
    ln->is_closed = is_closed;
    ln->points = points;
    ln->x = malloc (sizeof (double) * points);
    ln->y = malloc (sizeof (double) * points);
    ln->z = malloc (sizeof (double) * points);
    for (i = 0; i < points; i++)
      {
	  *(ln->x + i) = 0.0;
	  *(ln->y + i) = 0.0;
	  *(ln->z + i) = 0.0;
      }
    ln->first_hole = NULL;
    ln->last_hole = NULL;
    ln->first = NULL;
    ln->last = NULL;
    ln->next = NULL;
    return ln;
}

static gaiaDxfPolylinePtr
clone_dxf_polyline (gaiaDxfPolylinePtr in)
{
/* cloning a DXF Polyline object */
    int i;
    gaiaDxfPolylinePtr ln = malloc (sizeof (gaiaDxfPolyline));
    ln->is_closed = in->is_closed;
    ln->points = in->points;
    ln->x = malloc (sizeof (double) * in->points);
    ln->y = malloc (sizeof (double) * in->points);
    ln->z = malloc (sizeof (double) * in->points);
    for (i = 0; i < in->points; i++)
      {
	  *(ln->x + i) = *(in->x + i);
	  *(ln->y + i) = *(in->y + i);
	  *(ln->z + i) = *(in->z + i);
      }
    ln->first_hole = NULL;
    ln->last_hole = NULL;
    ln->first = NULL;
    ln->last = NULL;
    ln->next = NULL;
    return ln;
}

static void
destroy_dxf_polyline (gaiaDxfPolylinePtr ln)
{
/* memory cleanup - destroying a DXF Polyline object */
    gaiaDxfExtraAttrPtr ext;
    gaiaDxfExtraAttrPtr n_ext;
    gaiaDxfHolePtr hole;
    gaiaDxfHolePtr n_hole;
    if (ln == NULL)
	return;
    if (ln->x != NULL)
	free (ln->x);
    if (ln->y != NULL)
	free (ln->y);
    if (ln->z != NULL)
	free (ln->z);
    ext = ln->first;
    while (ext != NULL)
      {
	  n_ext = ext->next;
	  destroy_dxf_extra (ext);
	  ext = n_ext;
      }
    hole = ln->first_hole;
    while (hole != NULL)
      {
	  n_hole = hole->next;
	  destroy_dxf_hole (hole);
	  hole = n_hole;
      }
    free (ln);
}

static int
is_3d_line (gaiaDxfPolylinePtr ln)
{
/* testing if it's really a 3D Polyline */
    gaiaDxfHolePtr hole;
    int i;
    for (i = 0; i < ln->points; i++)
      {
	  if (*(ln->z + i) != 0.0)
	      return 1;
      }
    hole = ln->first_hole;
    while (hole != NULL)
      {
	  for (i = 0; i < hole->points; i++)
	    {
		if (*(hole->z + i) != 0.0)
		    return 1;
	    }
      }
    return 0;
}

static dxfRingsCollectionPtr
alloc_dxf_rings ()
{
/* allocating an empty Rings Collection */
    dxfRingsCollectionPtr coll = malloc (sizeof (dxfRingsCollection));
    coll->first = NULL;
    coll->last = NULL;
    return coll;
}

static void
destroy_dxf_rings (dxfRingsCollectionPtr coll)
{
/* memory cleanup - destroying a Rings Collection */
    gaiaDxfPolylinePtr ln;
    gaiaDxfPolylinePtr n_ln;
    if (coll == NULL)
	return;
    ln = coll->first;
    while (ln != NULL)
      {
	  n_ln = ln->next;
	  destroy_dxf_polyline (ln);
	  ln = n_ln;
      }
    free (coll);
}

static void
insert_dxf_ring (dxfRingsCollectionPtr coll, gaiaDxfPolylinePtr line, int start,
		 int end)
{
/* inserting a Ring into the collection */
    int i;
    int i2;
    int points = end - start + 1;
    gaiaDxfPolylinePtr out = alloc_dxf_polyline (1, points);
    for (i2 = 0, i = start; i <= end; i++, i2++)
      {
	  *(out->x + i2) = *(line->x + i);
	  *(out->y + i2) = *(line->y + i);
	  *(out->z + i2) = *(line->z + i);
      }
    if (coll->first == NULL)
	coll->first = out;
    if (coll->last != NULL)
	coll->last->next = out;
    coll->last = out;
}

static void
unlinked_rings (gaiaDxfPolylinePtr line)
{
/* attempt to identify unlinked Polygon rings */
    int invalid;
    int start;
    int count;
    double x;
    double y;
    double z;
    int i;
    int i2;
    gaiaDxfHolePtr hole;
    dxfRingsCollectionPtr coll;
    gaiaDxfPolylinePtr ring;
    gaiaGeomCollPtr geom;
    gaiaGeomCollPtr result;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    int pgs;
    int ok;

    if (line == NULL)
	return;
    if (line->points <= 0)
	return;

    coll = alloc_dxf_rings ();
    start = 0;
    while (start < line->points - 1)
      {
	  /* looping on candidate rings */
	  x = *(line->x + start);
	  y = *(line->y + start);
	  z = *(line->z + start);
	  invalid = 1;
	  for (i = start + 1; i < line->points; i++)
	    {
		if (*(line->x + i) == x && *(line->y + i) == y
		    && *(line->z + i) == z)
		  {
		      insert_dxf_ring (coll, line, start, i);
		      start = i + 1;
		      invalid = 0;
		      break;
		  }
	    }
	  if (invalid)
	      break;
      }

    count = 0;
    ring = coll->first;
    while (ring != NULL)
      {
	  count++;
	  ring = ring->next;
      }
    if (count < 2)
	invalid = 1;
    if (invalid)
      {
	  /* no unlinked rings were found - quitting */
	  destroy_dxf_rings (coll);
	  return;
      }

/* building a candidate geometry (multilinestring) */
    geom = gaiaAllocGeomCollXYZ ();
    ring = coll->first;
    while (ring != NULL)
      {
	  /* inserting a ring into the collection */
	  gaiaLinestringPtr ln =
	      gaiaAddLinestringToGeomColl (geom, ring->points);
	  for (i = 0; i < ring->points; i++)
	    {
		gaiaSetPointXYZ (ln->Coords, i, *(ring->x + i), *(ring->y + i),
				 *(ring->z + i));
	    }
	  ring = ring->next;
      }
/* freeing the rings list */
    destroy_dxf_rings (coll);

/* attempting to reassemble a polygon */
    result = gaiaPolygonize (geom, 0);
    gaiaFreeGeomColl (geom);
    if (result == NULL)
	return;

/* checking the returned polygon for validity */
    pgs = 0;
    ok = 1;
    pg = result->FirstPolygon;
    while (pg != NULL)
      {
	  pgs++;
	  if (pg->NumInteriors == 0)
	      ok = 0;
	  pg = pg->Next;
      }
    if (ok == 1 && pgs == 1)
      {
	  /* found a valid Polygon with internal holes(s) */
	  pg = result->FirstPolygon;
	  rng = pg->Exterior;
	  /* rebuilding the exterior ring */
	  free (line->x);
	  free (line->y);
	  free (line->z);
	  line->points = rng->Points;
	  line->x = malloc (sizeof (double) * line->points);
	  line->y = malloc (sizeof (double) * line->points);
	  line->z = malloc (sizeof (double) * line->points);
	  for (i = 0; i < line->points; i++)
	    {
		/* setting the exterior ring points */
		gaiaGetPointXYZ (rng->Coords, i, &x, &y, &z);
		*(line->x + i) = x;
		*(line->y + i) = y;
		*(line->z + i) = z;
	    }
	  for (i2 = 0; i2 < pg->NumInteriors; i2++)
	    {
		/* saving the Holes */
		rng = pg->Interiors + i2;
		hole = alloc_dxf_hole (rng->Points);
		insert_dxf_hole (line, hole);
		for (i = 0; i < hole->points; i++)
		  {
		      /* setting the interior ring points */
		      gaiaGetPointXYZ (rng->Coords, i, &x, &y, &z);
		      *(hole->x + i) = x;
		      *(hole->y + i) = y;
		      *(hole->z + i) = z;
		  }
	    }
      }
    gaiaFreeGeomColl (result);
/* forcing the closure flag */
    line->is_closed = 1;
}

static void
insert_dxf_polyline (gaiaDxfParserPtr dxf, const char *layer_name,
		     gaiaDxfPolylinePtr ln)
{
/* inserting a POLYLINE object into the appropriate Layer */
    gaiaDxfLayerPtr lyr = dxf->first;
    while (lyr != NULL)
      {
	  if (strcmp (lyr->layer_name, layer_name) == 0)
	    {
		/* found the matching Layer */
		if (dxf->linked_rings)
		    linked_rings (ln);
		if (dxf->unlinked_rings)
		    unlinked_rings (ln);
		if (ln->is_closed)
		  {
		      /* it's a Ring */
		      if (lyr->first_polyg == NULL)
			  lyr->first_polyg = ln;
		      if (lyr->last_polyg != NULL)
			  lyr->last_polyg->next = ln;
		      lyr->last_polyg = ln;
		      if (dxf->force_dims == GAIA_DXF_FORCE_2D
			  || dxf->force_dims == GAIA_DXF_FORCE_3D)
			  ;
		      else
			{
			    if (is_3d_line (ln))
				lyr->is3Dpolyg = 1;
			}
		  }
		else
		  {
		      /* it's a Linestring */
		      if (lyr->first_line == NULL)
			  lyr->first_line = ln;
		      if (lyr->last_line != NULL)
			  lyr->last_line->next = ln;
		      lyr->last_line = ln;
		      if (dxf->force_dims == GAIA_DXF_FORCE_2D
			  || dxf->force_dims == GAIA_DXF_FORCE_3D)
			  ;
		      else
			{
			    if (is_3d_line (ln))
				lyr->is3Dline = 1;
			}
		  }
		ln->first = dxf->first_ext;
		ln->last = dxf->last_ext;
		dxf->first_ext = NULL;
		dxf->last_ext = NULL;
		if (ln->is_closed && ln->first != NULL)
		    lyr->hasExtraPolyg = 1;
		if (ln->is_closed == 0 && ln->first != NULL)
		    lyr->hasExtraLine = 1;
		return;
	    }
	  lyr = lyr->next;
      }
    destroy_dxf_polyline (ln);
}

static gaiaDxfBlockPtr
alloc_dxf_block (const char *layer, const char *id, gaiaDxfTextPtr text,
		 gaiaDxfPointPtr point, gaiaDxfPolylinePtr line)
{
/* allocating and initializing a DXF Block object */
    int len;
    gaiaDxfBlockPtr blk = malloc (sizeof (gaiaDxfBlock));
    len = strlen (layer);
    blk->layer_name = malloc (len + 1);
    strcpy (blk->layer_name, layer);
    len = strlen (id);
    blk->block_id = malloc (len + 1);
    strcpy (blk->block_id, id);
    blk->text = text;
    blk->point = point;
    blk->line = line;
    blk->next = NULL;
    return blk;
}

static gaiaDxfBlockPtr
alloc_dxf_text_block (const char *layer, const char *id, gaiaDxfTextPtr text)
{
/* convenience method - Text Block */
    return alloc_dxf_block (layer, id, text, NULL, NULL);
}

static gaiaDxfBlockPtr
alloc_dxf_point_block (const char *layer, const char *id, gaiaDxfPointPtr point)
{
/* convenience method - Point Block */
    return alloc_dxf_block (layer, id, NULL, point, NULL);
}

static gaiaDxfBlockPtr
alloc_dxf_line_block (const char *layer, const char *id,
		      gaiaDxfPolylinePtr line)
{
/* convenience method - Line Block */
    return alloc_dxf_block (layer, id, NULL, NULL, line);
}

static void
destroy_dxf_block (gaiaDxfBlockPtr blk)
{
/* memory cleanup - destroying a DXF Block object */
    if (blk == NULL)
	return;
    if (blk->layer_name != NULL)
	free (blk->layer_name);
    if (blk->block_id != NULL)
	free (blk->block_id);
    if (blk->text != NULL)
	destroy_dxf_text (blk->text);
    if (blk->point != NULL)
	destroy_dxf_point (blk->point);
    if (blk->line != NULL)
	destroy_dxf_polyline (blk->line);
    free (blk);
}

static void
insert_dxf_block (gaiaDxfParserPtr dxf, gaiaDxfBlockPtr blk)
{
/* inserting a DXF Block object */
    if (dxf->first_blk == NULL)
	dxf->first_blk = blk;
    if (dxf->last_blk != NULL)
	dxf->last_blk->next = blk;
    dxf->last_blk = blk;
}

static gaiaDxfBlockPtr
find_dxf_block (gaiaDxfParserPtr dxf, const char *layer_name,
		const char *block_id)
{
/* attempting to find a Block object by its Id */
    gaiaDxfBlockPtr blk = dxf->first_blk;
    while (blk != NULL)
      {
	  if (layer_name != NULL && block_id != NULL)
	    {
		if (strcmp (blk->layer_name, layer_name) == 0
		    && strcmp (blk->block_id, block_id) == 0)
		  {
		      /* ok, matching item found */
		      return blk;
		  }
	    }
	  blk = blk->next;
      }
    return NULL;
}

static gaiaDxfLayerPtr
alloc_dxf_layer (const char *name, int force_dims)
{
/* allocating and initializing a DXF Layer object */
    int len;
    gaiaDxfLayerPtr lyr = malloc (sizeof (gaiaDxfLayer));
    len = strlen (name);
    lyr->layer_name = malloc (len + 1);
    strcpy (lyr->layer_name, name);
    lyr->first_text = NULL;
    lyr->last_text = NULL;
    lyr->first_point = NULL;
    lyr->last_point = NULL;
    lyr->first_line = NULL;
    lyr->last_line = NULL;
    lyr->first_polyg = NULL;
    lyr->last_polyg = NULL;
    if (force_dims == GAIA_DXF_FORCE_3D)
      {
	  lyr->is3Dtext = 1;
	  lyr->is3Dpoint = 1;
	  lyr->is3Dline = 1;
	  lyr->is3Dpolyg = 1;
      }
    else
      {
	  lyr->is3Dtext = 0;
	  lyr->is3Dpoint = 0;
	  lyr->is3Dline = 0;
	  lyr->is3Dpolyg = 0;
      }
    lyr->hasExtraText = 0;
    lyr->hasExtraPoint = 0;
    lyr->hasExtraLine = 0;
    lyr->hasExtraPolyg = 0;
    lyr->next = NULL;
    return lyr;
}

static void
destroy_dxf_layer (gaiaDxfLayerPtr lyr)
{
/* memory cleanup - destroying a DXF Layer object */
    gaiaDxfTextPtr txt;
    gaiaDxfTextPtr n_txt;
    gaiaDxfPointPtr pt;
    gaiaDxfPointPtr n_pt;
    gaiaDxfPolylinePtr ln;
    gaiaDxfPolylinePtr n_ln;
    if (lyr == NULL)
	return;
    if (lyr->layer_name != NULL)
	free (lyr->layer_name);
    txt = lyr->first_text;
    while (txt != NULL)
      {
	  n_txt = txt->next;
	  destroy_dxf_text (txt);
	  txt = n_txt;
      }
    pt = lyr->first_point;
    while (pt != NULL)
      {
	  n_pt = pt->next;
	  destroy_dxf_point (pt);
	  pt = n_pt;
      }
    ln = lyr->first_line;
    while (ln != NULL)
      {
	  n_ln = ln->next;
	  destroy_dxf_polyline (ln);
	  ln = n_ln;
      }
    ln = lyr->first_polyg;
    while (ln != NULL)
      {
	  n_ln = ln->next;
	  destroy_dxf_polyline (ln);
	  ln = n_ln;
      }
    free (lyr);
}

static void
insert_dxf_layer (gaiaDxfParserPtr dxf, gaiaDxfLayerPtr lyr)
{
/* inserting a Layer object into the DXF struct */
    if (dxf->first == NULL)
	dxf->first = lyr;
    if (dxf->last != NULL)
	dxf->last->next = lyr;
    dxf->last = lyr;
}

static void
force_missing_layer (gaiaDxfParserPtr dxf)
{
/* forcing undeclared layers [missing TABLES section] */
    int ok_layer = 1;
    if (dxf->undeclared_layers == 0)
	return;
    if (dxf->selected_layer != NULL)
      {
	  ok_layer = 0;
	  if (strcmp (dxf->selected_layer, dxf->curr_layer_name) == 0)
	      ok_layer = 1;
      }
    if (ok_layer)
      {
	  int already_defined = 0;
	  gaiaDxfLayerPtr lyr = dxf->first;
	  while (lyr != NULL)
	    {
		if (strcmp (lyr->layer_name, dxf->curr_layer_name) == 0)
		  {
		      already_defined = 1;
		      break;
		  }
		lyr = lyr->next;
	    }
	  if (already_defined)
	      return;
	  lyr = alloc_dxf_layer (dxf->curr_layer_name, dxf->force_dims);
	  insert_dxf_layer (dxf, lyr);
      }
}

static void
set_dxf_vertex (gaiaDxfParserPtr dxf)
{
/* saving the current Polyline Vertex */
    gaiaDxfPointPtr pt = malloc (sizeof (gaiaDxfPoint));
    pt->x = dxf->curr_point.x;
    pt->y = dxf->curr_point.y;
    pt->z = dxf->curr_point.z;
    pt->first = NULL;
    pt->last = NULL;
    pt->next = NULL;
    if (dxf->first_pt == NULL)
	dxf->first_pt = pt;
    if (dxf->last_pt != NULL)
	dxf->last_pt->next = pt;
    dxf->last_pt = pt;
    dxf->curr_point.x = 0.0;
    dxf->curr_point.y = 0.0;
    dxf->curr_point.z = 0.0;
}

static void
save_current_polyline (gaiaDxfParserPtr dxf)
{
/* saving the current Polyline */
    int points = 0;
    gaiaDxfPolylinePtr ln;
    gaiaDxfPointPtr n_pt;
    gaiaDxfPointPtr pt;
    if (dxf->curr_layer_name == NULL)
	goto clear;
    pt = dxf->first_pt;
    while (pt != NULL)
      {
	  /* counting how many vertices are into the polyline */
	  points++;
	  pt = pt->next;
      }
    ln = alloc_dxf_polyline (dxf->is_closed_polyline, points);
    points = 0;
    pt = dxf->first_pt;
    while (pt != NULL)
      {
	  /* setting vertices into the polyline */
	  *(ln->x + points) = pt->x;
	  *(ln->y + points) = pt->y;
	  *(ln->z + points) = pt->z;
	  points++;
	  pt = pt->next;
      }
    force_missing_layer (dxf);
    insert_dxf_polyline (dxf, dxf->curr_layer_name, ln);
    /* resetting the current polyline */
  clear:
    pt = dxf->first_pt;
    while (pt != NULL)
      {
	  n_pt = pt->next;
	  destroy_dxf_point (pt);
	  pt = n_pt;
	  dxf->is_insert = 0;
	  /* resetting curr_layer */
	  if (dxf->curr_layer_name != NULL)
	      free (dxf->curr_layer_name);
	  dxf->curr_layer_name = NULL;
      }
    dxf->first_pt = NULL;
    dxf->last_pt = NULL;
}

static void
save_current_block_polyline (gaiaDxfParserPtr dxf)
{
/* saving the current Polyline Block */
    int points = 0;
    gaiaDxfBlockPtr blk;
    gaiaDxfPolylinePtr ln;
    gaiaDxfPointPtr n_pt;
    gaiaDxfPointPtr pt = dxf->first_pt;
    if (dxf->curr_block_id != NULL)
      {
	  /* saving the current Polyline Block */
	  while (pt != NULL)
	    {
		/* counting how many vertices are into the polyline */
		points++;
		pt = pt->next;
	    }
	  ln = alloc_dxf_polyline (dxf->is_closed_polyline, points);
	  points = 0;
	  pt = dxf->first_pt;
	  while (pt != NULL)
	    {
		/* setting vertices into the polyline */
		*(ln->x + points) = pt->x;
		*(ln->y + points) = pt->y;
		*(ln->z + points) = pt->z;
		points++;
		pt = pt->next;
	    }
	  blk =
	      alloc_dxf_line_block (dxf->curr_layer_name, dxf->curr_block_id,
				    ln);
	  insert_dxf_block (dxf, blk);
      }
    /* resetting the current polyline */
    pt = dxf->first_pt;
    while (pt != NULL)
      {
	  n_pt = pt->next;
	  destroy_dxf_point (pt);
	  pt = n_pt;
      }
    dxf->first_pt = NULL;
    dxf->last_pt = NULL;
}

static void
reset_dxf_polyline (gaiaDxfParserPtr dxf)
{
/* resetting the current DXF polyline */
    if (dxf->is_polyline && dxf->is_block == 0)
      {
	  if (dxf->first_pt != NULL)
	      save_current_polyline (dxf);
	  dxf->is_polyline = 0;
      }
}

static void
reset_dxf_block (gaiaDxfParserPtr dxf)
{
/* resetting the current DXF block */
    gaiaDxfExtraAttrPtr ext;
    gaiaDxfExtraAttrPtr n_ext;
    gaiaDxfBlockPtr blk;
    if (dxf->is_vertex)
      {
	  /* saving the current Vertex */
	  set_dxf_vertex (dxf);
	  dxf->is_vertex = 0;
      }
    if (dxf->is_block == 1)
      {
	  if (dxf->is_text)
	    {
		if (dxf->curr_block_id != NULL)
		  {
		      /* saving the current Text Block */
		      gaiaDxfTextPtr txt = alloc_dxf_text (dxf->curr_text.label,
							   dxf->curr_text.x,
							   dxf->curr_text.y,
							   dxf->curr_text.z,
							   dxf->curr_text.
							   angle);
		      blk =
			  alloc_dxf_text_block (dxf->curr_layer_name,
						dxf->curr_block_id, txt);
		      insert_dxf_block (dxf, blk);
		  }
		/* resetting curr_text */
		dxf->curr_text.x = 0.0;
		dxf->curr_text.y = 0.0;
		dxf->curr_text.z = 0.0;
		dxf->curr_text.angle = 0.0;
		if (dxf->curr_text.label != NULL)
		    free (dxf->curr_text.label);
		dxf->curr_text.label = NULL;
		dxf->is_text = 0;
		dxf->is_block = 0;
		/* resetting curr_block_id */
		if (dxf->curr_block_id != NULL)
		    free (dxf->curr_block_id);
		dxf->curr_block_id = NULL;
	    }
	  if (dxf->is_point)
	    {
		if (dxf->curr_block_id != NULL)
		  {
		      /* saving the current Point Block */
		      gaiaDxfPointPtr pt =
			  alloc_dxf_point (dxf->curr_point.x, dxf->curr_point.y,
					   dxf->curr_point.z);
		      blk =
			  alloc_dxf_point_block (dxf->curr_layer_name,
						 dxf->curr_block_id, pt);
		      insert_dxf_block (dxf, blk);
		  }
		/* resetting curr_point */
		dxf->curr_point.x = 0.0;
		dxf->curr_point.y = 0.0;
		dxf->curr_point.z = 0.0;
		dxf->is_text = 0;
		dxf->is_block = 0;
		/* resetting curr_block_id */
		if (dxf->curr_block_id != NULL)
		    free (dxf->curr_block_id);
		dxf->curr_block_id = NULL;
	    }
	  if (dxf->is_polyline)
	    {
		if (dxf->curr_block_id != NULL)
		  {
		      /* saving the current Polyline Block */
		      save_current_block_polyline (dxf);
		      dxf->is_polyline = 0;
		  }
	    }
	  if (dxf->is_lwpolyline)
	    {
		if (dxf->curr_block_id != NULL)
		  {
		      /* saving the current LWPolyline Block */
		      save_current_block_polyline (dxf);
		      dxf->is_lwpolyline = 0;
		  }
	    }
      }
    if (dxf->extra_key != NULL)
	free (dxf->extra_key);
    if (dxf->extra_value != NULL)
	free (dxf->extra_value);
    ext = dxf->first_ext;
    while (ext != NULL)
      {
	  n_ext = ext->next;
	  destroy_dxf_extra (ext);
	  ext = n_ext;
      }
    dxf->first_ext = NULL;
    dxf->last_ext = NULL;
    if (dxf->curr_block_id != NULL)
	free (dxf->curr_block_id);
    dxf->curr_block_id = NULL;
}

static void
insert_dxf_indirect_entity (gaiaDxfParserPtr dxf, gaiaDxfBlockPtr blk)
{
/* inderting an entity indirectly referencing a Block */
    if (blk->text != NULL)
      {
	  /* saving the Text Block */
	  gaiaDxfTextPtr in = blk->text;
	  gaiaDxfTextPtr txt =
	      alloc_dxf_text (in->label, in->x, in->y, in->z, in->angle);
	  force_missing_layer (dxf);
	  insert_dxf_text (dxf, dxf->curr_layer_name, txt);
      }
    if (blk->point)
      {
	  /* saving the Point Block */
	  gaiaDxfPointPtr in = blk->point;
	  gaiaDxfPointPtr pt = alloc_dxf_point (in->x, in->y, in->z);
	  force_missing_layer (dxf);
	  insert_dxf_point (dxf, dxf->curr_layer_name, pt);
      }
    if (blk->line)
      {
	  /* saving the Polyline Block */
	  gaiaDxfPolylinePtr ln = clone_dxf_polyline (blk->line);
	  force_missing_layer (dxf);
	  insert_dxf_polyline (dxf, dxf->curr_layer_name, ln);
      }
    dxf->is_insert = 0;
    /* resetting curr_layer */
    if (dxf->curr_layer_name != NULL)
	free (dxf->curr_layer_name);
    dxf->curr_layer_name = NULL;
}

static void
reset_dxf_entity (gaiaDxfParserPtr dxf)
{
/* resetting the current DXF entity */
    gaiaDxfExtraAttrPtr ext;
    gaiaDxfExtraAttrPtr n_ext;
    if (dxf->is_vertex)
      {
	  /* saving the current Vertex */
	  set_dxf_vertex (dxf);
	  dxf->is_vertex = 0;
	  return;
      }
    if (dxf->is_polyline)
	return;
    if (dxf->is_layer)
      {
	  /* saving the current Table aka Layer */
	  int ok_layer = 1;
	  if (dxf->selected_layer != NULL)
	    {
		ok_layer = 0;
		if (strcmp (dxf->selected_layer, dxf->curr_layer_name) == 0)
		    ok_layer = 1;
	    }
	  if (ok_layer)
	    {
		gaiaDxfLayerPtr lyr =
		    alloc_dxf_layer (dxf->curr_layer_name, dxf->force_dims);
		insert_dxf_layer (dxf, lyr);
		dxf->undeclared_layers = 0;
	    }
	  /* resetting curr_layer */
	  if (dxf->curr_layer_name != NULL)
	      free (dxf->curr_layer_name);
	  dxf->curr_layer_name = NULL;
	  dxf->is_layer = 0;
      }
    if (dxf->is_insert)
      {
	  /* attempting to replace the current Insert */
	  gaiaDxfBlockPtr blk =
	      find_dxf_block (dxf, dxf->curr_layer_name, dxf->curr_block_id);
	  if (blk != NULL)
	      insert_dxf_indirect_entity (dxf, blk);
	  if (dxf->curr_block_id != NULL)
	      free (dxf->curr_block_id);
	  dxf->curr_block_id = NULL;
      }
    if (dxf->is_text)
      {
	  /* saving the current Text */
	  gaiaDxfTextPtr txt =
	      alloc_dxf_text (dxf->curr_text.label, dxf->curr_text.x,
			      dxf->curr_text.y, dxf->curr_text.z,
			      dxf->curr_text.angle);
	  force_missing_layer (dxf);
	  insert_dxf_text (dxf, dxf->curr_layer_name, txt);
	  /* resetting curr_text */
	  dxf->curr_text.x = 0.0;
	  dxf->curr_text.y = 0.0;
	  dxf->curr_text.z = 0.0;
	  dxf->curr_text.angle = 0.0;
	  if (dxf->curr_text.label != NULL)
	      free (dxf->curr_text.label);
	  dxf->curr_text.label = NULL;
	  dxf->is_text = 0;
	  /* resetting curr_layer */
	  if (dxf->curr_layer_name != NULL)
	      free (dxf->curr_layer_name);
	  dxf->curr_layer_name = NULL;
      }
    if (dxf->is_point)
      {
	  /* saving the current Point */
	  gaiaDxfPointPtr pt =
	      alloc_dxf_point (dxf->curr_point.x, dxf->curr_point.y,
			       dxf->curr_point.z);
	  force_missing_layer (dxf);
	  insert_dxf_point (dxf, dxf->curr_layer_name, pt);
	  /* resetting curr_point */
	  dxf->curr_point.x = 0.0;
	  dxf->curr_point.y = 0.0;
	  dxf->curr_point.z = 0.0;
	  dxf->is_point = 0;
	  /* resetting curr_layer */
	  if (dxf->curr_layer_name != NULL)
	      free (dxf->curr_layer_name);
	  dxf->curr_layer_name = NULL;
      }
    if (dxf->is_lwpolyline && dxf->is_block == 0)
      {
	  /* saving the current Polyline */
	  save_current_polyline (dxf);
	  dxf->is_lwpolyline = 0;
      }
    if (dxf->extra_key != NULL)
	free (dxf->extra_key);
    if (dxf->extra_value != NULL)
	free (dxf->extra_value);
    ext = dxf->first_ext;
    while (ext != NULL)
      {
	  n_ext = ext->next;
	  destroy_dxf_extra (ext);
	  ext = n_ext;
      }
    dxf->first_ext = NULL;
    dxf->last_ext = NULL;
}

static void
set_dxf_layer_name (gaiaDxfParserPtr dxf, const char *name)
{
/* saving the current Layer Name */
    int len;
    if (dxf->curr_layer_name != NULL)
	free (dxf->curr_layer_name);
    len = strlen (name);
    dxf->curr_layer_name = malloc (len + 1);
    strcpy (dxf->curr_layer_name, name);
}

static void
set_dxf_block_id (gaiaDxfParserPtr dxf, const char *id)
{
/* saving the current Block Id */
    int len;
    if (dxf->curr_block_id != NULL)
	free (dxf->curr_block_id);
    len = strlen (id);
    dxf->curr_block_id = malloc (len + 1);
    strcpy (dxf->curr_block_id, id);
}

static void
set_dxf_label (gaiaDxfParserPtr dxf, const char *label)
{
/* saving the current Text Label */
    int len;
    if (dxf->curr_text.label != NULL)
	free (dxf->curr_text.label);
    len = strlen (label);
    dxf->curr_text.label = malloc (len + 1);
    strcpy (dxf->curr_text.label, label);
}

static void
set_dxf_extra_attr (gaiaDxfParserPtr dxf)
{
/* saving the current Extra Attribute */
    gaiaDxfExtraAttrPtr ext = alloc_dxf_extra ();
    ext->key = dxf->extra_key;
    ext->value = dxf->extra_value;
    if (dxf->first_ext == NULL)
	dxf->first_ext = ext;
    if (dxf->last_ext != NULL)
	dxf->last_ext->next = ext;
    dxf->last_ext = ext;
    dxf->extra_key = NULL;
    dxf->extra_value = NULL;
}

static void
set_dxf_extra_key (gaiaDxfParserPtr dxf, const char *key)
{
/* saving the current Extra Attribute Key */
    int len;
    if (dxf->extra_key != NULL)
	free (dxf->extra_key);
    len = strlen (key);
    dxf->extra_key = malloc (len + 1);
    strcpy (dxf->extra_key, key);
    if (dxf->extra_key != NULL && dxf->extra_value != NULL)
	set_dxf_extra_attr (dxf);
}

static void
set_dxf_extra_value (gaiaDxfParserPtr dxf, const char *value)
{
/* saving the current Extra Attribute Value */
    int len;
    if (dxf->extra_value != NULL)
	free (dxf->extra_value);
    len = strlen (value);
    dxf->extra_value = malloc (len + 1);
    strcpy (dxf->extra_value, value);
    if (dxf->extra_key != NULL && dxf->extra_value != NULL)
	set_dxf_extra_attr (dxf);
}

static int
op_code_line (const char *line)
{
/* checking for a valid op-code */
    int numdigit = 0;
    int invalid = 0;
    const char *p = line;
    while (*p != '\0')
      {
	  /* skipping leading white-spaces */
	  if (*p == ' ' || *p == '\t')
	    {
		p++;
		continue;
	    }
	  else
	      break;
      }
    while (*p != '\0')
      {
	  /* evaluating each character */
	  if (*p >= '0' && *p <= '9')
	    {
		numdigit++;
		p++;
		continue;
	    }
	  else
	    {
		invalid = 1;
		break;
	    }
      }
    if (numdigit > 0 && invalid == 0)
	return 1;
    return 0;
}

static int
parse_dxf_line (gaiaDxfParserPtr dxf, const char *line)
{
/* parsing a DXF line */
    dxf->line_no += 1;

    if (dxf->tables || dxf->entities || dxf->blocks)
      {
	  /* handling OP-CODE lines */
	  if (dxf->op_code_line)
	    {
		if (!op_code_line (line))
		  {
		      /* unexpected value */
		      spatialite_e
			  ("ERROR on line %d: expected on OP-CODE to be found\n",
			   dxf->line_no);
		      return 0;
		  }
		dxf->op_code = atoi (line);
		if (dxf->op_code == 0)
		    reset_dxf_entity (dxf);
		dxf->op_code_line = 0;
		return 1;
	    }
	  dxf->op_code_line = 1;
      }
    if (dxf->eof)
      {
	  /* reading past the end */
	  spatialite_e ("ERROR on line %d: attempting to read past EOF\n",
			dxf->line_no);
	  return 0;
      }
    if (strcmp (line, "SECTION") == 0)
      {
	  /* start SECTION tag */
	  reset_dxf_polyline (dxf);
	  if (dxf->section)
	    {
		spatialite_e ("ERROR on line %d: unexpected SECTION\n",
			      dxf->line_no);
		dxf->error = 1;
		return 0;
	    }
	  dxf->section = 1;
	  return 1;
      }
    if (strcmp (line, "ENDSEC") == 0)
      {
	  /* end SECTION tag */
	  reset_dxf_polyline (dxf);
	  if (!dxf->section)
	    {
		spatialite_e ("ERROR on line %d: unexpected ENDSEC\n",
			      dxf->line_no);
		dxf->error = 1;
		return 0;
	    }
	  dxf->section = 0;
	  dxf->tables = 0;
	  dxf->blocks = 0;
	  dxf->is_block = 0;
	  dxf->entities = 0;
	  return 1;
      }
    if (strcmp (line, "TABLES") == 0)
      {
	  /* start TABLES tag */
	  reset_dxf_polyline (dxf);
	  if (dxf->section)
	    {
		dxf->tables = 1;
		dxf->op_code_line = 1;
		return 1;
	    }
      }
    if (strcmp (line, "BLOCKS") == 0)
      {
	  /* start BLOCKS tag */
	  reset_dxf_polyline (dxf);
	  if (dxf->section)
	    {
		dxf->blocks = 1;
		dxf->op_code_line = 1;
		return 1;
	    }
      }
    if (strcmp (line, "BLOCK") == 0)
      {
	  /* start BLOCK tag */
	  reset_dxf_polyline (dxf);
	  if (dxf->blocks)
	    {
		dxf->is_block = 1;
		dxf->op_code_line = 1;
		return 1;
	    }
      }
    if (strcmp (line, "ENDBLK") == 0)
      {
	  /* end BLOCK tag */
	  if (dxf->is_block)
	    {
		reset_dxf_block (dxf);
		dxf->is_block = 0;
		dxf->op_code_line = 1;
		return 1;
	    }
      }
    if (strcmp (line, "ENTITIES") == 0)
      {
	  /* start ENTITIES tag */
	  reset_dxf_polyline (dxf);
	  if (dxf->section)
	    {
		dxf->entities = 1;
		dxf->op_code_line = 1;
		return 1;
	    }
      }
    if (strcmp (line, "LAYER") == 0)
      {
	  /* start LAYER tag */
	  reset_dxf_polyline (dxf);
	  if (dxf->tables && dxf->op_code == 0)
	    {
		dxf->is_layer = 1;
		return 1;
	    }
      }
    if (strcmp (line, "INSERT") == 0)
      {
	  /* start INSERT tag */
	  reset_dxf_polyline (dxf);
	  if (dxf->entities && dxf->op_code == 0)
	    {
		dxf->is_insert = 1;
		return 1;
	    }
      }
    if (strcmp (line, "TEXT") == 0)
      {
	  /* start TEXT tag */
	  reset_dxf_polyline (dxf);
	  if (dxf->entities && dxf->op_code == 0)
	    {
		dxf->is_text = 1;
		return 1;
	    }
	  if (dxf->is_block && dxf->op_code == 0)
	    {
		dxf->is_text = 1;
		return 1;
	    }
      }
    if (strcmp (line, "POINT") == 0)
      {
	  /* start POINT tag */
	  reset_dxf_polyline (dxf);
	  if (dxf->entities && dxf->op_code == 0)
	    {
		dxf->is_point = 1;
		return 1;
	    }
	  if (dxf->is_block && dxf->op_code == 0)
	    {
		dxf->is_point = 1;
		return 1;
	    }
      }
    if (strcmp (line, "POLYLINE") == 0)
      {
	  /* start POLYLINE tag */
	  reset_dxf_polyline (dxf);
	  if (dxf->entities && dxf->op_code == 0)
	    {
		dxf->is_polyline = 1;
		return 1;
	    }
	  if (dxf->is_block && dxf->op_code == 0)
	    {
		dxf->is_polyline = 1;
		return 1;
	    }
      }
    if (strcmp (line, "LWPOLYLINE") == 0)
      {
	  /* start LWPOLYLINE tag */
	  reset_dxf_polyline (dxf);
	  if (dxf->entities && dxf->op_code == 0)
	    {
		dxf->is_lwpolyline = 1;
		return 1;
	    }
	  if (dxf->is_block && dxf->op_code == 0)
	    {
		dxf->is_lwpolyline = 1;
		return 1;
	    }
      }
    if (strcmp (line, "VERTEX") == 0)
      {
	  /* start VERTEX tag */
	  if (dxf->is_polyline && dxf->op_code == 0)
	    {
		dxf->is_vertex = 1;
		return 1;
	    }
      }
    if (strcmp (line, "EOF") == 0)
      {
	  /* end of file marker tag */
	  reset_dxf_block (dxf);
	  reset_dxf_polyline (dxf);
	  dxf->eof = 1;
	  return 1;
      }
    if (dxf->is_layer)
      {
	  /* parsing Table attributes */
	  switch (dxf->op_code)
	    {
	    case 2:
		set_dxf_layer_name (dxf, line);
		break;
	    };
      }
    if (dxf->is_block)
      {
	  /* parsing Block attributes */
	  switch (dxf->op_code)
	    {
	    case 2:
		set_dxf_block_id (dxf, line);
		break;
	    case 8:
		set_dxf_layer_name (dxf, line);
		break;
	    };
      }
    if (dxf->is_insert)
      {
	  /* parsing Insert attributes */
	  switch (dxf->op_code)
	    {
	    case 2:
		set_dxf_block_id (dxf, line);
		break;
	    case 8:
		set_dxf_layer_name (dxf, line);
		break;
	    };
      }
    if (dxf->is_text)
      {
	  /* parsing Text attributes */
	  switch (dxf->op_code)
	    {
	    case 1:
		set_dxf_label (dxf, line);
		break;
	    case 8:
		set_dxf_layer_name (dxf, line);
		break;
	    case 10:
		dxf->curr_text.x = atof (line);
		break;
	    case 20:
		dxf->curr_text.y = atof (line);
		break;
	    case 30:
		dxf->curr_text.z = atof (line);
		break;
	    case 50:
		dxf->curr_text.angle = atof (line);
		break;
	    case 1000:
		set_dxf_extra_value (dxf, line);
		break;
	    case 1001:
		set_dxf_extra_key (dxf, line);
		break;
	    };
      }
    if (dxf->is_point)
      {
	  /* parsing Point attributes */
	  switch (dxf->op_code)
	    {
	    case 8:
		set_dxf_layer_name (dxf, line);
		break;
	    case 10:
		dxf->curr_point.x = atof (line);
		break;
	    case 20:
		dxf->curr_point.y = atof (line);
		break;
	    case 30:
		dxf->curr_point.z = atof (line);
		break;
	    case 1000:
		set_dxf_extra_value (dxf, line);
		break;
	    case 1001:
		set_dxf_extra_key (dxf, line);
		break;
	    };
      }
    if (dxf->is_lwpolyline)
      {
	  /* parsing LwPolyline attributes */
	  switch (dxf->op_code)
	    {
	    case 8:
		set_dxf_layer_name (dxf, line);
		break;
	    case 10:
		dxf->curr_point.x = atof (line);
		break;
	    case 20:
		dxf->curr_point.y = atof (line);
		set_dxf_vertex (dxf);
		break;
	    case 70:
		if ((atoi (line) & 0x01) == 0x01)
		    dxf->is_closed_polyline = 1;
		else
		    dxf->is_closed_polyline = 0;
		break;
	    case 1000:
		set_dxf_extra_value (dxf, line);
		break;
	    case 1001:
		set_dxf_extra_key (dxf, line);
		break;
	    };
      }
    if (dxf->is_polyline && dxf->is_vertex != 1)
      {
	  /* parsing Polyline attributes */
	  switch (dxf->op_code)
	    {
	    case 8:
		set_dxf_layer_name (dxf, line);
		break;
	    case 70:
		if ((atoi (line) & 0x01) == 0x01)
		    dxf->is_closed_polyline = 1;
		else
		    dxf->is_closed_polyline = 0;
		break;
	    case 1000:
		set_dxf_extra_value (dxf, line);
		break;
	    case 1001:
		set_dxf_extra_key (dxf, line);
		break;
	    };
      }
    if (dxf->is_vertex)
      {
	  /* parsing Vertex attributes */
	  switch (dxf->op_code)
	    {
	    case 10:
		dxf->curr_point.x = atof (line);
		break;
	    case 20:
		dxf->curr_point.y = atof (line);
		break;
	    case 30:
		dxf->curr_point.z = atof (line);
		break;
	    };
      }
    return 1;
}

GAIAGEO_DECLARE gaiaDxfParserPtr
gaiaCreateDxfParser (int srid,
		     int force_dims,
		     const char *prefix,
		     const char *selected_layer, int special_rings)
{
/* allocating and initializing a DXF parser object */
    gaiaDxfParserPtr dxf = malloc (sizeof (gaiaDxfParser));
    dxf->line_no = 0;
    dxf->op_code_line = 0;
    dxf->op_code = -1;
    dxf->section = 0;
    dxf->tables = 0;
    dxf->blocks = 0;
    dxf->entities = 0;
    dxf->is_layer = 0;
    dxf->is_block = 0;
    dxf->is_text = 0;
    dxf->is_point = 0;
    dxf->is_polyline = 0;
    dxf->is_lwpolyline = 0;
    dxf->is_vertex = 0;
    dxf->is_insert = 0;
    dxf->eof = 0;
    dxf->error = 0;
    dxf->curr_block_id = NULL;
    dxf->curr_layer_name = NULL;
    dxf->curr_text.x = 0.0;
    dxf->curr_text.y = 0.0;
    dxf->curr_text.z = 0.0;
    dxf->curr_text.angle = 0.0;
    dxf->curr_text.label = NULL;
    dxf->curr_point.x = 0.0;
    dxf->curr_point.y = 0.0;
    dxf->curr_point.z = 0.0;
    dxf->is_closed_polyline = 0;
    dxf->extra_key = NULL;
    dxf->extra_value = NULL;
    dxf->first_pt = NULL;
    dxf->last_pt = NULL;
    dxf->first_ext = NULL;
    dxf->last_ext = NULL;
    dxf->first = NULL;
    dxf->last = NULL;
    dxf->first_blk = NULL;
    dxf->last_blk = NULL;
    dxf->force_dims = force_dims;
    if (srid <= 0)
	srid = -1;
    dxf->srid = srid;
    dxf->prefix = prefix;
    dxf->selected_layer = selected_layer;
    dxf->linked_rings = 0;
    dxf->unlinked_rings = 0;
    if (special_rings == GAIA_DXF_RING_LINKED)
	dxf->linked_rings = 1;
    if (special_rings == GAIA_DXF_RING_UNLINKED)
	dxf->unlinked_rings = 1;
    dxf->undeclared_layers = 1;
    return dxf;
}

GAIAGEO_DECLARE void
gaiaDestroyDxfParser (gaiaDxfParserPtr dxf)
{
/* memory cleanup: destroying a DXF parser object */
    gaiaDxfLayerPtr lyr;
    gaiaDxfLayerPtr n_lyr;
    gaiaDxfPointPtr pt;
    gaiaDxfPointPtr n_pt;
    gaiaDxfExtraAttrPtr ext;
    gaiaDxfExtraAttrPtr n_ext;
    gaiaDxfBlockPtr blk;
    gaiaDxfBlockPtr n_blk;
    if (dxf == NULL)
	return;
    if (dxf->curr_text.label != NULL)
	free (dxf->curr_text.label);
    if (dxf->curr_layer_name != NULL)
	free (dxf->curr_layer_name);
    if (dxf->curr_block_id != NULL)
	free (dxf->curr_block_id);
    lyr = dxf->first;
    while (lyr != NULL)
      {
	  n_lyr = lyr->next;
	  destroy_dxf_layer (lyr);
	  lyr = n_lyr;
      }
    pt = dxf->first_pt;
    while (pt != NULL)
      {
	  n_pt = pt->next;
	  destroy_dxf_point (pt);
	  pt = n_pt;
      }
    if (dxf->extra_key != NULL)
	free (dxf->extra_key);
    if (dxf->extra_value != NULL)
	free (dxf->extra_value);
    ext = dxf->first_ext;
    while (ext != NULL)
      {
	  n_ext = ext->next;
	  destroy_dxf_extra (ext);
	  ext = n_ext;
      }
    blk = dxf->first_blk;
    while (blk != NULL)
      {
	  n_blk = blk->next;
	  destroy_dxf_block (blk);
	  blk = n_blk;
      }
    free (dxf);
}

GAIAGEO_DECLARE int
gaiaParseDxfFile (gaiaDxfParserPtr dxf, const char *path)
{
/* parsing the whole DXF file */
    int c;
    char line[4192];
    char *p = line;
    FILE *fl;

    if (dxf == NULL)
	return 0;
    if (dxf->first != NULL)
	return 0;

/* attempting to open the input file */
    fl = fopen (path, "rb");
    if (fl == NULL)
	return 0;

/* scanning the DXF file */
    while ((c = getc (fl)) != EOF)
      {
	  if (c == '\r')
	    {
		/* ignoring any CR */
		continue;
	    }
	  if (c == '\n')
	    {
		/* end line found */
		*p = '\0';
		if (!parse_dxf_line (dxf, line))
		    goto stop;
		p = line;
		continue;
	    }
	  *p++ = c;
      }

    fclose (fl);
    return 1;
  stop:
    fclose (fl);
    return 0;
}
