/* 
/ spatialite_tool
/
/ an utility CLI tool for DXF import
/
/ version 1.0, 2013 May 11
/
/ Author: Sandro Furieri a.furieri@lqt.it
/
/ Copyright (C) 2013  Alessandro Furieri
/
/    This program is free software: you can redistribute it and/or modify
/    it under the terms of the GNU General Public License as published by
/    the Free Software Foundation, either version 3 of the License, or
/    (at your option) any later version.
/
/    This program is distributed in the hope that it will be useful,
/    but WITHOUT ANY WARRANTY; without even the implied warranty of
/    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/    GNU General Public License for more details.
/
/    You should have received a copy of the GNU General Public License
/    along with this program.  If not, see <http://www.gnu.org/licenses/>.
/
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>

#include <spatialite/gaiageo.h>
#include <spatialite/gaiaaux.h>
#include <spatialite.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#define strcasecmp	_stricmp
#endif /* not WIN32 */

#define ARG_NONE		0
#define ARG_DB_PATH		1
#define ARG_DXF_PATH		2
#define ARG_SRID		3
#define ARG_DIMS		4
#define ARG_MODE		5
#define ARG_LAYER		6
#define ARG_PREFIX		7
#define ARG_CACHE_SIZE		8
#define ARG_LINKED_RINGS	9
#define ARG_UNLINKED_RINGS	10

#define IMPORT_BY_LAYER	1
#define IMPORT_MIXED	2
#define AUTO_2D_3D	3
#define FORCE_2D	4
#define FORCE_3D	5

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

typedef struct dxf_extra_attr
{
/* a DXF extra attribute */
    char *key;
    char *value;
    struct dxf_extra_attr *next;
} dxfExtraAttr;
typedef dxfExtraAttr *dxfExtraAttrPtr;

typedef struct dxf_text
{
/* a DXF Text object */
    char *label;
    double x;
    double y;
    double z;
    double angle;
    dxfExtraAttrPtr first;
    dxfExtraAttrPtr last;
    struct dxf_text *next;
} dxfText;
typedef dxfText *dxfTextPtr;

typedef struct dxf_point
{
/* a DXF Point object */
    double x;
    double y;
    double z;
    dxfExtraAttrPtr first;
    dxfExtraAttrPtr last;
    struct dxf_point *next;
} dxfPoint;
typedef dxfPoint *dxfPointPtr;

typedef struct dxf_hole
{
    int points;
    double *x;
    double *y;
    double *z;
    struct dxf_hole *next;
} dxfHole;
typedef dxfHole *dxfHolePtr;

typedef struct dxf_polyline
{
/* a DXF Polyline (or Ring) Object */
    int is_closed;
    int points;
    double *x;
    double *y;
    double *z;
    dxfHolePtr first_hole;
    dxfHolePtr last_hole;
    dxfExtraAttrPtr first;
    dxfExtraAttrPtr last;
    struct dxf_polyline *next;
} dxfPolyline;
typedef dxfPolyline *dxfPolylinePtr;

typedef struct dxf_rings_collection
{
/* a collection of rings */
    dxfPolylinePtr first;
    dxfPolylinePtr last;
} dxfRingsCollection;
typedef dxfRingsCollection *dxfRingsCollectionPtr;

typedef struct dxf_block
{
/* a DXF Block object */
    char *layer_name;
    char *block_id;
    dxfTextPtr text;
    dxfPointPtr point;
    dxfPolylinePtr line;
    struct dxf_block *next;
} dxfBlock;
typedef dxfBlock *dxfBlockPtr;

typedef struct dxf_layer
{
/* a DXF Layer object */
    char *layer_name;
    dxfTextPtr first_text;
    dxfTextPtr last_text;
    dxfPointPtr first_point;
    dxfPointPtr last_point;
    dxfPolylinePtr first_line;
    dxfPolylinePtr last_line;
    dxfPolylinePtr first_polyg;
    dxfPolylinePtr last_polyg;
    int is3Dtext;
    int is3Dpoint;
    int is3Dline;
    int is3Dpolyg;
    int hasExtraText;
    int hasExtraPoint;
    int hasExtraLine;
    int hasExtraPolyg;
    struct dxf_layer *next;
} dxfLayer;
typedef dxfLayer *dxfLayerPtr;

typedef struct dxf_parser
{
/* the DXF parser main struct */
    int line_no;
    int op_code_line;
    int op_code;
    int section;
    int tables;
    int blocks;
    int entities;
    int is_layer;
    int is_block;
    int is_text;
    int is_point;
    int is_polyline;
    int is_lwpolyline;
    int is_vertex;
    int is_insert;
    int eof;
    int error;
    char *curr_block_id;
    char *curr_layer_name;
    dxfText curr_text;
    dxfPoint curr_point;
    int is_closed_polyline;
    dxfPointPtr first_pt;
    dxfPointPtr last_pt;
    char *extra_key;
    char *extra_value;
    dxfExtraAttrPtr first_ext;
    dxfExtraAttrPtr last_ext;
    dxfLayerPtr first;
    dxfLayerPtr last;
    dxfBlockPtr first_blk;
    dxfBlockPtr last_blk;
    int force_dims;
    int srid;
    const char *selected_layer;
    const char *prefix;
    int linked_rings;
    int unlinked_rings;
    int undeclared_layers;
} dxfParser;
typedef dxfParser *dxfParserPtr;


static dxfHolePtr
alloc_dxf_hole (int points)
{
/* allocating and initializing a DXF Polygon Hole object */
    int i;
    dxfHolePtr hole = malloc (sizeof (dxfHole));
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
insert_dxf_hole (dxfPolylinePtr line, dxfHolePtr hole)
{
/* inserting an Hole into a Polygon */
    if (line->first_hole == NULL)
	line->first_hole = hole;
    if (line->last_hole != NULL)
	line->last_hole->next = hole;
    line->last_hole = hole;
}

static void
destroy_dxf_hole (dxfHolePtr hole)
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
linked_rings (dxfPolylinePtr line)
{
/* attempt to identify linked Polygon rings */
    int i;
    int i2;
    int match = 0;
    double x;
    double y;
    double z;
    dxfHolePtr hole;
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

static dxfExtraAttrPtr
alloc_dxf_extra ()
{
/* allocating and initializing a DXF Extra Attribute object */
    dxfExtraAttrPtr ext = malloc (sizeof (dxfExtraAttr));
    ext->key = NULL;
    ext->value = NULL;
    ext->next = NULL;
    return ext;
}

static void
destroy_dxf_extra (dxfExtraAttrPtr ext)
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

static dxfTextPtr
alloc_dxf_text (const char *label, double x, double y, double z, double angle)
{
/* allocating and initializing a DXF Text object */
    int len;
    dxfTextPtr txt = malloc (sizeof (dxfText));
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
destroy_dxf_text (dxfTextPtr txt)
{
/* memory cleanup - destroying a DXF Text object */
    dxfExtraAttrPtr ext;
    dxfExtraAttrPtr n_ext;
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
is_3d_text (dxfTextPtr txt)
{
/* testing if it's really a 3D Text */
    if (txt->z == 0.0)
	return 0;
    return 1;
}

static void
insert_dxf_text (dxfParserPtr dxf, const char *layer_name, dxfTextPtr txt)
{
/* inserting a TEXT object into the appropriate Layer */
    dxfLayerPtr lyr = dxf->first;
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
		if (dxf->force_dims == FORCE_2D || dxf->force_dims == FORCE_3D)
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

static dxfPointPtr
alloc_dxf_point (double x, double y, double z)
{
/* allocating and initializing a DXF Point object */
    dxfPointPtr pt = malloc (sizeof (dxfPoint));
    pt->x = x;
    pt->y = y;
    pt->z = z;
    pt->next = NULL;
    pt->first = NULL;
    pt->last = NULL;
    return pt;
}

static void
destroy_dxf_point (dxfPointPtr pt)
{
/* memory cleanup - destroying a DXF Point object */
    dxfExtraAttrPtr ext;
    dxfExtraAttrPtr n_ext;
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
is_3d_point (dxfPointPtr pt)
{
/* testing if it's really a 3D Point */
    if (pt->z == 0.0)
	return 0;
    return 1;
}

static void
insert_dxf_point (dxfParserPtr dxf, const char *layer_name, dxfPointPtr pt)
{
/* inserting a POINT object into the appropriate Layer */
    dxfLayerPtr lyr = dxf->first;
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
		if (dxf->force_dims == FORCE_2D || dxf->force_dims == FORCE_3D)
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

static dxfPolylinePtr
alloc_dxf_polyline (int is_closed, int points)
{
/* allocating and initializing a DXF Polyline object */
    int i;
    dxfPolylinePtr ln = malloc (sizeof (dxfPolyline));
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

static dxfPolylinePtr
clone_dxf_polyline (dxfPolylinePtr in)
{
/* cloning a DXF Polyline object */
    int i;
    dxfPolylinePtr ln = malloc (sizeof (dxfPolyline));
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
destroy_dxf_polyline (dxfPolylinePtr ln)
{
/* memory cleanup - destroying a DXF Polyline object */
    dxfExtraAttrPtr ext;
    dxfExtraAttrPtr n_ext;
    dxfHolePtr hole;
    dxfHolePtr n_hole;
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
is_3d_line (dxfPolylinePtr ln)
{
/* testing if it's really a 3D Polyline */
    dxfHolePtr hole;
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
    dxfPolylinePtr ln;
    dxfPolylinePtr n_ln;
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
insert_dxf_ring (dxfRingsCollectionPtr coll, dxfPolylinePtr line, int start,
		 int end)
{
/* inserting a Ring into the collection */
    int i;
    int i2;
    int points = end - start + 1;
    dxfPolylinePtr out = alloc_dxf_polyline (1, points);
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
unlinked_rings (dxfPolylinePtr line)
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
    dxfHolePtr hole;
    dxfRingsCollectionPtr coll;
    dxfPolylinePtr ring;
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
    fprintf (stderr, "go\n");

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
insert_dxf_polyline (dxfParserPtr dxf, const char *layer_name,
		     dxfPolylinePtr ln)
{
/* inserting a POLYLINE object into the appropriate Layer */
    dxfLayerPtr lyr = dxf->first;
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
		      if (dxf->force_dims == FORCE_2D
			  || dxf->force_dims == FORCE_3D)
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
		      if (dxf->force_dims == FORCE_2D
			  || dxf->force_dims == FORCE_3D)
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

static dxfBlockPtr
alloc_dxf_block (const char *layer, const char *id, dxfTextPtr text,
		 dxfPointPtr point, dxfPolylinePtr line)
{
/* allocating and initializing a DXF Block object */
    int len;
    dxfBlockPtr blk = malloc (sizeof (dxfBlock));
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

static dxfBlockPtr
alloc_dxf_text_block (const char *layer, const char *id, dxfTextPtr text)
{
/* convenience method - Text Block */
    return alloc_dxf_block (layer, id, text, NULL, NULL);
}

static dxfBlockPtr
alloc_dxf_point_block (const char *layer, const char *id, dxfPointPtr point)
{
/* convenience method - Point Block */
    return alloc_dxf_block (layer, id, NULL, point, NULL);
}

static dxfBlockPtr
alloc_dxf_line_block (const char *layer, const char *id, dxfPolylinePtr line)
{
/* convenience method - Line Block */
    return alloc_dxf_block (layer, id, NULL, NULL, line);
}

static void
destroy_dxf_block (dxfBlockPtr blk)
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
insert_dxf_block (dxfParserPtr dxf, dxfBlockPtr blk)
{
/* inserting a DXF Block object */
    if (dxf->first_blk == NULL)
	dxf->first_blk = blk;
    if (dxf->last_blk != NULL)
	dxf->last_blk->next = blk;
    dxf->last_blk = blk;
}

static dxfBlockPtr
find_dxf_block (dxfParserPtr dxf, const char *layer_name, const char *block_id)
{
/* attempting to find a Block object by its Id */
    dxfBlockPtr blk = dxf->first_blk;
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

static dxfLayerPtr
alloc_dxf_layer (const char *name, int force_dims)
{
/* allocating and initializing a DXF Layer object */
    int len;
    dxfLayerPtr lyr = malloc (sizeof (dxfLayer));
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
    if (force_dims == FORCE_3D)
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
destroy_dxf_layer (dxfLayerPtr lyr)
{
/* memory cleanup - destroying a DXF Layer object */
    dxfTextPtr txt;
    dxfTextPtr n_txt;
    dxfPointPtr pt;
    dxfPointPtr n_pt;
    dxfPolylinePtr ln;
    dxfPolylinePtr n_ln;
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
insert_dxf_layer (dxfParserPtr dxf, dxfLayerPtr lyr)
{
/* inserting a Layer object into the DXF struct */
    if (dxf->first == NULL)
	dxf->first = lyr;
    if (dxf->last != NULL)
	dxf->last->next = lyr;
    dxf->last = lyr;
}

static dxfParserPtr
alloc_dxf_parser (int srid, int force_dims, const char *prefix,
		  const char *selected_layer, int linked_rings,
		  int unlinked_rings)
{
/* allocating and initializing a DXF parser object */
    dxfParserPtr dxf = malloc (sizeof (dxfParser));
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
    dxf->linked_rings = linked_rings;
    dxf->unlinked_rings = unlinked_rings;
    dxf->undeclared_layers = 1;
    return dxf;
}

static void
destroy_dxf_parser (dxfParserPtr dxf)
{
/* memory cleanup: destroying a DXF parser object */
    dxfLayerPtr lyr;
    dxfLayerPtr n_lyr;
    dxfPointPtr pt;
    dxfPointPtr n_pt;
    dxfExtraAttrPtr ext;
    dxfExtraAttrPtr n_ext;
    dxfBlockPtr blk;
    dxfBlockPtr n_blk;
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

static void
force_missing_layer (dxfParserPtr dxf)
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
	  dxfLayerPtr lyr = dxf->first;
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
set_dxf_vertex (dxfParserPtr dxf)
{
/* saving the current Polyline Vertex */
    dxfPointPtr pt = malloc (sizeof (dxfPoint));
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
save_current_polyline (dxfParserPtr dxf)
{
/* saving the current Polyline */
    int points = 0;
    dxfPolylinePtr ln;
    dxfPointPtr n_pt;
    dxfPointPtr pt;
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
save_current_block_polyline (dxfParserPtr dxf)
{
/* saving the current Polyline Block */
    int points = 0;
    dxfBlockPtr blk;
    dxfPolylinePtr ln;
    dxfPointPtr n_pt;
    dxfPointPtr pt = dxf->first_pt;
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
reset_dxf_polyline (dxfParserPtr dxf)
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
reset_dxf_block (dxfParserPtr dxf)
{
/* resetting the current DXF block */
    dxfExtraAttrPtr ext;
    dxfExtraAttrPtr n_ext;
    dxfBlockPtr blk;
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
		      dxfTextPtr txt = alloc_dxf_text (dxf->curr_text.label,
						       dxf->curr_text.x,
						       dxf->curr_text.y,
						       dxf->curr_text.z,
						       dxf->curr_text.angle);
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
		      dxfPointPtr pt =
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
insert_dxf_indirect_entity (dxfParserPtr dxf, dxfBlockPtr blk)
{
/* inderting an entity indirectly referencing a Block */
    if (blk->text != NULL)
      {
	  /* saving the Text Block */
	  dxfTextPtr in = blk->text;
	  dxfTextPtr txt =
	      alloc_dxf_text (in->label, in->x, in->y, in->z, in->angle);
	  force_missing_layer (dxf);
	  insert_dxf_text (dxf, dxf->curr_layer_name, txt);
      }
    if (blk->point)
      {
	  /* saving the Point Block */
	  dxfPointPtr in = blk->point;
	  dxfPointPtr pt = alloc_dxf_point (in->x, in->y, in->z);
	  force_missing_layer (dxf);
	  insert_dxf_point (dxf, dxf->curr_layer_name, pt);
      }
    if (blk->line)
      {
	  /* saving the Polyline Block */
	  dxfPolylinePtr ln = clone_dxf_polyline (blk->line);
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
reset_dxf_entity (dxfParserPtr dxf)
{
/* resetting the current DXF entity */
    dxfExtraAttrPtr ext;
    dxfExtraAttrPtr n_ext;
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
		dxfLayerPtr lyr =
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
	  dxfBlockPtr blk =
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
	  dxfTextPtr txt =
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
	  dxfPointPtr pt =
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
set_dxf_layer_name (dxfParserPtr dxf, const char *name)
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
set_dxf_block_id (dxfParserPtr dxf, const char *id)
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
set_dxf_label (dxfParserPtr dxf, const char *label)
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
set_dxf_extra_attr (dxfParserPtr dxf)
{
/* saving the current Extra Attribute */
    dxfExtraAttrPtr ext = alloc_dxf_extra ();
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
set_dxf_extra_key (dxfParserPtr dxf, const char *key)
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
set_dxf_extra_value (dxfParserPtr dxf, const char *value)
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
parse_dxf_line (dxfParserPtr dxf, const char *line)
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
		      fprintf (stderr,
			       "ERROR on line %d: expected on OP-CODE to be found\n",
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
	  fprintf (stderr, "ERROR on line %d: attempting to read past EOF\n",
		   dxf->line_no);
	  return 0;
      }
    if (strcmp (line, "SECTION") == 0)
      {
	  /* start SECTION tag */
	  reset_dxf_polyline (dxf);
	  if (dxf->section)
	    {
		fprintf (stderr, "ERROR on line %d: unexpected SECTION\n",
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
		fprintf (stderr, "ERROR on line %d: unexpected ENDSEC\n",
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
    if (dxf->is_polyline)
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

static int
parse_dxf_file (dxfParserPtr dxf, const char *path)
{
/* parsing the whole DXF file */
    int c;
    char line[4192];
    char *p = line;

/* attempting to open the input file */
    FILE *fl = fopen (path, "rb");
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

static void
import_mixed (sqlite3 * handle, dxfParserPtr dxf)
{
/* populating the target DB - all layers mixed altogether */
    int text = 0;
    int point = 0;
    int line = 0;
    int polyg = 0;
    int hasExtraText = 0;
    int hasExtraPoint = 0;
    int hasExtraLine = 0;
    int hasExtraPolyg = 0;
    int text3D = 0;
    int point3D = 0;
    int line3D = 0;
    int polyg3D = 0;
    char *sql;
    int ret;
    sqlite3_stmt *stmt;
    sqlite3_stmt *stmt_ext;
    unsigned char *blob;
    int blob_size;
    gaiaGeomCollPtr geom;
    char *name;
    char *xname;
    char *extra_name;
    char *xextra_name;
    char *fk_name;
    char *xfk_name;
    char *idx_name;
    char *xidx_name;
    char *view_name;
    char *xview_name;

    dxfLayerPtr lyr = dxf->first;
    while (lyr != NULL)
      {
	  /* exploring Layers by type */
	  if (lyr->first_text != NULL)
	      text = 1;
	  if (lyr->first_point != NULL)
	      point = 1;
	  if (lyr->first_line != NULL)
	      line = 1;
	  if (lyr->first_polyg != NULL)
	      polyg = 1;
	  if (lyr->hasExtraText)
	      hasExtraText = 1;
	  if (lyr->hasExtraPoint)
	      hasExtraPoint = 1;
	  if (lyr->hasExtraLine)
	      hasExtraLine = 1;
	  if (lyr->hasExtraPolyg)
	      hasExtraPolyg = 1;
	  if (lyr->is3Dtext)
	      text3D = 1;
	  if (lyr->is3Dpoint)
	      point3D = 1;
	  if (lyr->is3Dline)
	      line3D = 1;
	  if (lyr->is3Dpolyg)
	      polyg3D = 1;
	  lyr = lyr->next;
      }

    if (text)
      {
	  /* creating and populating the TEXT layer */
	  stmt_ext = NULL;
	  extra_name = NULL;
	  if (dxf->prefix == NULL)
	      name = sqlite3_mprintf ("text_layer");
	  else
	      name = sqlite3_mprintf ("%stext_layer", dxf->prefix);
	  xname = gaiaDoubleQuotedSql (name);
	  sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
				 "    feature_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
				 "    layer TEXT NOT NULL,\n"
				 "    label TEXT NOT NULL,\n"
				 "    rotation DOUBLE NOT NULL)", xname);
	  free (xname);
	  ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "CREATE TABLE %s error: %s\n", name,
			 sqlite3_errmsg (handle));
		return;
	    }
	  sql =
	      sqlite3_mprintf
	      ("SELECT AddGeometryColumn(%Q, 'geometry', "
	       "%d, 'POINT', %Q)", name, dxf->srid, text3D ? "XYZ" : "XY");
	  ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "ADD GEOMETRY %s error: %s\n", name,
			 sqlite3_errmsg (handle));
		return;
	    }
	  sql =
	      sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'geometry')",
			       name);
	  ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "CREATE SPATIAL INDEX %s error: %s\n", name,
			 sqlite3_errmsg (handle));
		return;
	    }
	  xname = gaiaDoubleQuotedSql (name);
	  sql =
	      sqlite3_mprintf
	      ("INSERT INTO \"%s\" (feature_id, layer, label, rotation, geometry) "
	       "VALUES (NULL, ?, ?, ?, ?)", xname);
	  free (xname);
	  ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "CREATE STATEMENT %s error: %s\n", name,
			 sqlite3_errmsg (handle));
		return;
	    }
	  if (hasExtraText)
	    {
		/* creating the Extra Attribute table */
		extra_name = sqlite3_mprintf ("%s_attr", name);
		fk_name = sqlite3_mprintf ("fk_%s", extra_name);
		xextra_name = gaiaDoubleQuotedSql (extra_name);
		xfk_name = gaiaDoubleQuotedSql (fk_name);
		xname = gaiaDoubleQuotedSql (name);
		sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
				       "    attr_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
				       "    feature_id INTEGER NOT NULL,\n"
				       "    attr_key TEXT NOT NULL,\n"
				       "    attr_value TEXT NOT NULL,\n"
				       "    CONSTRAINT \"%s\" FOREIGN KEY (feature_id) "
				       "REFERENCES \"%s\" (feature_id))",
				       xextra_name, xfk_name, xname);
		free (xextra_name);
		free (xfk_name);
		free (xname);
		sqlite3_free (fk_name);
		ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr,
			       "CREATE TABLE %s error: %s\n", extra_name,
			       sqlite3_errmsg (handle));
		      sqlite3_finalize (stmt);
		      return;
		  }
		idx_name = sqlite3_mprintf ("idx_%s", extra_name);
		xidx_name = gaiaDoubleQuotedSql (idx_name);
		xextra_name = gaiaDoubleQuotedSql (extra_name);
		sql =
		    sqlite3_mprintf
		    ("CREATE INDEX \"%s\" ON \"%s\" (feature_id)", xidx_name,
		     xextra_name);
		free (xidx_name);
		free (xextra_name);
		ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr,
			       "CREATE INDEX %s error: %s\n", idx_name,
			       sqlite3_errmsg (handle));
		      sqlite3_finalize (stmt);
		      return;
		  }
		sqlite3_free (idx_name);
		view_name = sqlite3_mprintf ("%s_view", name);
		xview_name = gaiaDoubleQuotedSql (view_name);
		xname = gaiaDoubleQuotedSql (name);
		xextra_name = gaiaDoubleQuotedSql (extra_name);
		sql = sqlite3_mprintf ("CREATE VIEW \"%s\" AS "
				       "SELECT f.feature_id AS feature_id, f.layer AS layer, f.label AS label, "
				       "f.rotation AS rotation, f.geometry AS geometry, "
				       "a.attr_id AS attr_id, a.attr_key AS attr_key, a.attr_value AS attr_value "
				       "FROM \"%s\" AS f "
				       "LEFT JOIN \"%s\" AS a ON (f.feature_id = a.feature_id)",
				       xview_name, xname, xextra_name);
		free (xview_name);
		free (xname);
		free (xextra_name);
		ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr,
			       "CREATE VIEW %s error: %s\n", view_name,
			       sqlite3_errmsg (handle));
		      sqlite3_finalize (stmt);
		      return;
		  }
		sqlite3_free (view_name);
		xextra_name = gaiaDoubleQuotedSql (extra_name);
		sql =
		    sqlite3_mprintf
		    ("INSERT INTO \"%s\" (attr_id, feature_id, attr_key, attr_value) "
		     "VALUES (NULL, ?, ?, ?)", xextra_name);
		free (xextra_name);
		ret =
		    sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_ext,
					NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr,
			       "CREATE STATEMENT %s error: %s\n", extra_name,
			       sqlite3_errmsg (handle));
		      sqlite3_finalize (stmt);
		      return;
		  }
	    }
	  ret = sqlite3_exec (handle, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "BEGIN %s error: %s\n", name,
			 sqlite3_errmsg (handle));
		sqlite3_finalize (stmt);
		if (stmt_ext != NULL)
		    sqlite3_finalize (stmt_ext);
		return;
	    }
	  lyr = dxf->first;
	  while (lyr != NULL)
	    {
		dxfTextPtr txt = lyr->first_text;
		while (txt != NULL)
		  {
		      sqlite3_reset (stmt);
		      sqlite3_clear_bindings (stmt);
		      sqlite3_bind_text (stmt, 1, lyr->layer_name,
					 strlen (lyr->layer_name),
					 SQLITE_STATIC);
		      sqlite3_bind_text (stmt, 2, txt->label,
					 strlen (txt->label), SQLITE_STATIC);
		      sqlite3_bind_double (stmt, 3, txt->angle);
		      if (text3D)
			  geom = gaiaAllocGeomCollXYZ ();
		      else
			  geom = gaiaAllocGeomColl ();
		      geom->Srid = dxf->srid;
		      if (text3D)
			  gaiaAddPointToGeomCollXYZ (geom, txt->x, txt->y,
						     txt->z);
		      else
			  gaiaAddPointToGeomColl (geom, txt->x, txt->y);
		      gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
		      gaiaFreeGeomColl (geom);
		      sqlite3_bind_blob (stmt, 4, blob, blob_size, free);
		      ret = sqlite3_step (stmt);
		      if (ret == SQLITE_DONE || ret == SQLITE_ROW)
			  ;
		      else
			{
			    fprintf (stderr, "INSERT %s error: %s\n", name,
				     sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    if (stmt_ext != NULL)
				sqlite3_finalize (stmt_ext);
			    ret =
				sqlite3_exec (handle, "ROLLBACK", NULL, NULL,
					      NULL);
			    return;
			}
		      if (stmt_ext != NULL)
			{
			    /* inserting all Extra Attributes */
			    sqlite3_int64 feature_id =
				sqlite3_last_insert_rowid (handle);
			    dxfExtraAttrPtr ext = txt->first;
			    while (ext != NULL)
			      {
				  sqlite3_reset (stmt_ext);
				  sqlite3_clear_bindings (stmt_ext);
				  sqlite3_bind_int64 (stmt_ext, 1, feature_id);
				  sqlite3_bind_text (stmt_ext, 2, ext->key,
						     strlen (ext->key),
						     SQLITE_STATIC);
				  sqlite3_bind_text (stmt_ext, 3, ext->value,
						     strlen (ext->value),
						     SQLITE_STATIC);
				  ret = sqlite3_step (stmt_ext);
				  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
				      ;
				  else
				    {
					fprintf (stderr,
						 "INSERT %s error: %s\n",
						 extra_name,
						 sqlite3_errmsg (handle));
					sqlite3_finalize (stmt);
					sqlite3_finalize (stmt_ext);
					ret =
					    sqlite3_exec (handle, "ROLLBACK",
							  NULL, NULL, NULL);
					return;
				    }
				  ext = ext->next;
			      }
			}
		      txt = txt->next;
		  }
		lyr = lyr->next;
	    }
	  sqlite3_free (name);
	  if (extra_name)
	      sqlite3_free (extra_name);
	  sqlite3_finalize (stmt);
	  if (stmt_ext != NULL)
	      sqlite3_finalize (stmt_ext);
	  ret = sqlite3_exec (handle, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "COMMIT text_layer error: %s\n",
			 sqlite3_errmsg (handle));
		return;
	    }
      }

    if (point)
      {
	  /* creating and populating the POINT layer */
	  stmt_ext = NULL;
	  extra_name = NULL;
	  if (dxf->prefix == NULL)
	      name = sqlite3_mprintf ("point_layer");
	  else
	      name = sqlite3_mprintf ("%spoint_layer", dxf->prefix);
	  xname = gaiaDoubleQuotedSql (name);
	  sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
				 "    feature_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
				 "    layer TEXT NOT NULL)", xname);
	  free (xname);
	  ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "CREATE TABLE %s error: %s\n", name,
			 sqlite3_errmsg (handle));
		return;
	    }
	  sql =
	      sqlite3_mprintf
	      ("SELECT AddGeometryColumn(%Q, 'geometry', "
	       "%d, 'POINT', %Q)", name, dxf->srid, point3D ? "XYZ" : "XY");
	  ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "ADD GEOMETRY %s error: %s\n", name,
			 sqlite3_errmsg (handle));
		return;
	    }
	  sql =
	      sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'geometry')",
			       name);
	  ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "CREATE SPATIAL INDEX %s error: %s\n", name,
			 sqlite3_errmsg (handle));
		return;
	    }
	  xname = gaiaDoubleQuotedSql (name);
	  sql =
	      sqlite3_mprintf
	      ("INSERT INTO \"%s\" (feature_id, layer, geometry) "
	       "VALUES (NULL, ?, ?)", xname);
	  free (xname);
	  ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "CREATE STATEMENT %s error: %s\n", name,
			 sqlite3_errmsg (handle));
		return;
	    }
	  if (hasExtraPoint)
	    {
		/* creating the Extra Attribute table */
		extra_name = sqlite3_mprintf ("%s_attr", name);
		fk_name = sqlite3_mprintf ("fk_%s", extra_name);
		xextra_name = gaiaDoubleQuotedSql (extra_name);
		xfk_name = gaiaDoubleQuotedSql (fk_name);
		xname = gaiaDoubleQuotedSql (name);
		sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
				       "    attr_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
				       "    feature_id INTEGER NOT NULL,\n"
				       "    attr_key TEXT NOT NULL,\n"
				       "    attr_value TEXT NOT NULL,\n"
				       "    CONSTRAINT \"%s\" FOREIGN KEY (feature_id) "
				       "REFERENCES \"%s\" (feature_id))",
				       xextra_name, xfk_name, xname);
		free (xfk_name);
		free (xname);
		free (xextra_name);
		sqlite3_free (fk_name);
		ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr,
			       "CREATE TABLE %s error: %s\n", extra_name,
			       sqlite3_errmsg (handle));
		      sqlite3_finalize (stmt);
		      return;
		  }
		idx_name = sqlite3_mprintf ("idx_%s", extra_name);
		xidx_name = gaiaDoubleQuotedSql (idx_name);
		xname = gaiaDoubleQuotedSql (name);
		sql =
		    sqlite3_mprintf
		    ("CREATE INDEX \"%s\" ON \"%s\" (feature_id)", xidx_name,
		     xname);
		free (xidx_name);
		free (xname);
		ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr,
			       "CREATE INDEX %s error: %s\n", idx_name,
			       sqlite3_errmsg (handle));
		      sqlite3_finalize (stmt);
		      return;
		  }
		sqlite3_free (idx_name);
		view_name = sqlite3_mprintf ("%s_view", name);
		xview_name = gaiaDoubleQuotedSql (view_name);
		xname = gaiaDoubleQuotedSql (name);
		xextra_name = gaiaDoubleQuotedSql (extra_name);
		sql = sqlite3_mprintf ("CREATE VIEW \"%s\" AS "
				       "SELECT f.feature_id AS feature_id, f.layer AS layer, f.geometry AS geometry, "
				       "a.attr_id AS attr_id, a.attr_key AS attr_key, a.attr_value AS attr_value "
				       "FROM \"%s\" AS f "
				       "LEFT JOIN \"%s\" AS a ON (f.feature_id = a.feature_id)",
				       xview_name, xname, xextra_name);
		free (xview_name);
		free (xname);
		free (xextra_name);
		ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr,
			       "CREATE VIEW %s error: %s\n", view_name,
			       sqlite3_errmsg (handle));
		      sqlite3_finalize (stmt);
		      return;
		  }
		sqlite3_free (view_name);
		xextra_name = gaiaDoubleQuotedSql (extra_name);
		sql =
		    sqlite3_mprintf
		    ("INSERT INTO \"%s\" (attr_id, feature_id, attr_key, attr_value) "
		     "VALUES (NULL, ?, ?, ?)", xextra_name);
		free (xextra_name);
		ret =
		    sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_ext,
					NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr,
			       "CREATE STATEMENT %s error: %s\n", extra_name,
			       sqlite3_errmsg (handle));
		      sqlite3_finalize (stmt);
		      return;
		  }
	    }
	  ret = sqlite3_exec (handle, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "BEGIN %s error: %s\n", name,
			 sqlite3_errmsg (handle));
		sqlite3_finalize (stmt);
		if (stmt_ext != NULL)
		    sqlite3_finalize (stmt_ext);
		return;
	    }
	  lyr = dxf->first;
	  while (lyr != NULL)
	    {
		dxfPointPtr pt = lyr->first_point;
		while (pt != NULL)
		  {
		      sqlite3_reset (stmt);
		      sqlite3_clear_bindings (stmt);
		      sqlite3_bind_text (stmt, 1, lyr->layer_name,
					 strlen (lyr->layer_name),
					 SQLITE_STATIC);
		      if (point3D)
			  geom = gaiaAllocGeomCollXYZ ();
		      else
			  geom = gaiaAllocGeomColl ();
		      geom->Srid = dxf->srid;
		      if (point3D)
			  gaiaAddPointToGeomCollXYZ (geom, pt->x, pt->y, pt->z);
		      else
			  gaiaAddPointToGeomColl (geom, pt->x, pt->y);
		      gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
		      gaiaFreeGeomColl (geom);
		      sqlite3_bind_blob (stmt, 2, blob, blob_size, free);
		      ret = sqlite3_step (stmt);
		      if (ret == SQLITE_DONE || ret == SQLITE_ROW)
			  ;
		      else
			{
			    fprintf (stderr, "INSERT %s error: %s\n", name,
				     sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    if (stmt_ext != NULL)
				sqlite3_finalize (stmt_ext);
			    ret =
				sqlite3_exec (handle, "ROLLBACK", NULL, NULL,
					      NULL);
			    return;
			}
		      if (stmt_ext != NULL)
			{
			    /* inserting all Extra Attributes */
			    sqlite3_int64 feature_id =
				sqlite3_last_insert_rowid (handle);
			    dxfExtraAttrPtr ext = pt->first;
			    while (ext != NULL)
			      {
				  sqlite3_reset (stmt_ext);
				  sqlite3_clear_bindings (stmt_ext);
				  sqlite3_bind_int64 (stmt_ext, 1, feature_id);
				  sqlite3_bind_text (stmt_ext, 2, ext->key,
						     strlen (ext->key),
						     SQLITE_STATIC);
				  sqlite3_bind_text (stmt_ext, 3, ext->value,
						     strlen (ext->value),
						     SQLITE_STATIC);
				  ret = sqlite3_step (stmt_ext);
				  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
				      ;
				  else
				    {
					fprintf (stderr,
						 "INSERT %s error: %s\n",
						 extra_name,
						 sqlite3_errmsg (handle));
					sqlite3_finalize (stmt);
					sqlite3_finalize (stmt_ext);
					ret =
					    sqlite3_exec (handle, "ROLLBACK",
							  NULL, NULL, NULL);
					return;
				    }
				  ext = ext->next;
			      }
			}
		      pt = pt->next;
		  }
		lyr = lyr->next;
	    }
	  sqlite3_free (name);
	  if (extra_name)
	      sqlite3_free (extra_name);
	  sqlite3_finalize (stmt);
	  if (stmt_ext != NULL)
	      sqlite3_finalize (stmt_ext);
	  ret = sqlite3_exec (handle, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "COMMIT point_layer error: %s\n",
			 sqlite3_errmsg (handle));
		return;
	    }
      }

    if (line)
      {
	  /* creating and populating the LINE layer */
	  stmt_ext = NULL;
	  extra_name = NULL;
	  if (dxf->prefix == NULL)
	      name = sqlite3_mprintf ("line_layer");
	  else
	      name = sqlite3_mprintf ("%sline_layer", dxf->prefix);
	  xname = gaiaDoubleQuotedSql (name);
	  sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
				 "    feature_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
				 "    layer TEXT NOT NULL)", xname);
	  free (xname);
	  ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "CREATE TABLE %s error: %s\n", name,
			 sqlite3_errmsg (handle));
		return;
	    }
	  sql =
	      sqlite3_mprintf
	      ("SELECT AddGeometryColumn(%Q, 'geometry', "
	       "%d, 'LINESTRING', %Q)", name, dxf->srid, line3D ? "XYZ" : "XY");
	  ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "ADD GEOMETRY %s error: %s\n", name,
			 sqlite3_errmsg (handle));
		return;
	    }
	  sql =
	      sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'geometry')",
			       name);
	  ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "CREATE SPATIAL INDEX %s error: %s\n", name,
			 sqlite3_errmsg (handle));
		return;
	    }
	  xname = gaiaDoubleQuotedSql (name);
	  sql =
	      sqlite3_mprintf
	      ("INSERT INTO \"%s\" (feature_id, layer, geometry) "
	       "VALUES (NULL, ?, ?)", xname);
	  free (xname);
	  ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "CREATE STATEMENT %s error: %s\n", name,
			 sqlite3_errmsg (handle));
		return;
	    }
	  if (hasExtraLine)
	    {
		/* creating the Extra Attribute table */
		extra_name = sqlite3_mprintf ("%s_attr", name);
		fk_name = sqlite3_mprintf ("fk_%s", extra_name);
		xextra_name = gaiaDoubleQuotedSql (extra_name);
		xfk_name = gaiaDoubleQuotedSql (fk_name);
		xname = gaiaDoubleQuotedSql (name);
		sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
				       "    attr_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
				       "    feature_id INTEGER NOT NULL,\n"
				       "    attr_key TEXT NOT NULL,\n"
				       "    attr_value TEXT NOT NULL,\n"
				       "    CONSTRAINT \"%s\" FOREIGN KEY (feature_id) "
				       "REFERENCES \"%s\" (feature_id))",
				       xextra_name, xfk_name, xname);
		free (xextra_name);
		free (xfk_name);
		free (xname);
		sqlite3_free (fk_name);
		ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr,
			       "CREATE TABLE %s error: %s\n", extra_name,
			       sqlite3_errmsg (handle));
		      sqlite3_finalize (stmt);
		      return;
		  }
		idx_name = sqlite3_mprintf ("idx_%s", extra_name);
		xidx_name = gaiaDoubleQuotedSql (idx_name);
		xextra_name = gaiaDoubleQuotedSql (extra_name);
		sql =
		    sqlite3_mprintf
		    ("CREATE INDEX \"%s\" ON \"%s\" (feature_id)", xidx_name,
		     xextra_name);
		free (xidx_name);
		free (xextra_name);
		ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr,
			       "CREATE INDEX %s error: %s\n", idx_name,
			       sqlite3_errmsg (handle));
		      sqlite3_finalize (stmt);
		      return;
		  }
		sqlite3_free (idx_name);
		view_name = sqlite3_mprintf ("%s_view", extra_name);
		xview_name = gaiaDoubleQuotedSql (view_name);
		xname = gaiaDoubleQuotedSql (name);
		xextra_name = gaiaDoubleQuotedSql (extra_name);
		sql = sqlite3_mprintf ("CREATE VIEW \"%s\" AS "
				       "SELECT f.feature_id AS feature_id, f.layer AS layer, f.geometry AS geometry, "
				       "a.attr_id AS attr_id, a.attr_key AS attr_key, a.attr_value AS attr_value "
				       "FROM \"%s\" AS f "
				       "LEFT JOIN \"%s\" AS a ON (f.feature_id = a.feature_id)",
				       xview_name, xname, xextra_name);
		free (xview_name);
		free (xname);
		free (xextra_name);
		ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr,
			       "CREATE VIEW %s error: %s\n", view_name,
			       sqlite3_errmsg (handle));
		      sqlite3_finalize (stmt);
		      return;
		  }
		sqlite3_free (view_name);
		xextra_name = gaiaDoubleQuotedSql (extra_name);
		sql =
		    sqlite3_mprintf
		    ("INSERT INTO \"%s\" (attr_id, feature_id, attr_key, attr_value) "
		     "VALUES (NULL, ?, ?, ?)", xextra_name);
		free (xextra_name);
		ret =
		    sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_ext,
					NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr,
			       "CREATE STATEMENT %s error: %s\n", extra_name,
			       sqlite3_errmsg (handle));
		      sqlite3_finalize (stmt);
		      return;
		  }
	    }
	  ret = sqlite3_exec (handle, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "BEGIN %s error: %s\n", name,
			 sqlite3_errmsg (handle));
		sqlite3_finalize (stmt);
		if (stmt_ext != NULL)
		    sqlite3_finalize (stmt_ext);
		return;
	    }
	  lyr = dxf->first;
	  while (lyr != NULL)
	    {
		dxfPolylinePtr ln = lyr->first_line;
		while (ln != NULL)
		  {
		      int iv;
		      gaiaLinestringPtr p_ln;
		      sqlite3_reset (stmt);
		      sqlite3_clear_bindings (stmt);
		      sqlite3_bind_text (stmt, 1, lyr->layer_name,
					 strlen (lyr->layer_name),
					 SQLITE_STATIC);
		      if (line3D)
			  geom = gaiaAllocGeomCollXYZ ();
		      else
			  geom = gaiaAllocGeomColl ();
		      geom->Srid = dxf->srid;
		      gaiaAddLinestringToGeomColl (geom, ln->points);
		      p_ln = geom->FirstLinestring;
		      for (iv = 0; iv < ln->points; iv++)
			{
			    if (line3D)
			      {
				  gaiaSetPointXYZ (p_ln->Coords, iv,
						   *(ln->x + iv), *(ln->y + iv),
						   *(ln->z + iv));
			      }
			    else
			      {
				  gaiaSetPoint (p_ln->Coords, iv, *(ln->x + iv),
						*(ln->y + iv));
			      }
			}
		      gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
		      gaiaFreeGeomColl (geom);
		      sqlite3_bind_blob (stmt, 2, blob, blob_size, free);
		      ret = sqlite3_step (stmt);
		      if (ret == SQLITE_DONE || ret == SQLITE_ROW)
			  ;
		      else
			{
			    fprintf (stderr, "INSERT %s error: %s\n", name,
				     sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    if (stmt_ext != NULL)
				sqlite3_finalize (stmt_ext);
			    ret =
				sqlite3_exec (handle, "ROLLBACK", NULL, NULL,
					      NULL);
			    return;
			}
		      if (stmt_ext != NULL)
			{
			    /* inserting all Extra Attributes */
			    sqlite3_int64 feature_id =
				sqlite3_last_insert_rowid (handle);
			    dxfExtraAttrPtr ext = ln->first;
			    while (ext != NULL)
			      {
				  sqlite3_reset (stmt_ext);
				  sqlite3_clear_bindings (stmt_ext);
				  sqlite3_bind_int64 (stmt_ext, 1, feature_id);
				  sqlite3_bind_text (stmt_ext, 2, ext->key,
						     strlen (ext->key),
						     SQLITE_STATIC);
				  sqlite3_bind_text (stmt_ext, 3, ext->value,
						     strlen (ext->value),
						     SQLITE_STATIC);
				  ret = sqlite3_step (stmt_ext);
				  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
				      ;
				  else
				    {
					fprintf (stderr,
						 "INSERT %s error: %s\n",
						 extra_name,
						 sqlite3_errmsg (handle));
					sqlite3_finalize (stmt);
					sqlite3_finalize (stmt_ext);
					ret =
					    sqlite3_exec (handle, "ROLLBACK",
							  NULL, NULL, NULL);
					return;
				    }
				  ext = ext->next;
			      }
			}
		      ln = ln->next;
		  }
		lyr = lyr->next;
	    }
	  sqlite3_free (name);
	  if (extra_name)
	      sqlite3_free (extra_name);
	  sqlite3_finalize (stmt);
	  if (stmt_ext != NULL)
	      sqlite3_finalize (stmt_ext);
	  ret = sqlite3_exec (handle, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "COMMIT line_layer error: %s\n",
			 sqlite3_errmsg (handle));
		return;
	    }
      }

    if (polyg)
      {
	  /* creating and populating the POLYG layer */
	  stmt_ext = NULL;
	  extra_name = NULL;
	  if (dxf->prefix == NULL)
	      name = sqlite3_mprintf ("polyg_layer");
	  else
	      name = sqlite3_mprintf ("%spolyg_layer", dxf->prefix);
	  xname = gaiaDoubleQuotedSql (name);
	  sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
				 "    feature_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
				 "    layer TEXT NOT NULL)", xname);
	  free (xname);
	  ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "CREATE TABLE %s error: %s\n", name,
			 sqlite3_errmsg (handle));
		return;
	    }
	  sql =
	      sqlite3_mprintf
	      ("SELECT AddGeometryColumn(%Q, 'geometry', "
	       "%d, 'POLYGON', %Q)", name, dxf->srid, polyg3D ? "XYZ" : "XY");
	  ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "ADD GEOMETRY %s error: %s\n", name,
			 sqlite3_errmsg (handle));
		return;
	    }
	  sql =
	      sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'geometry')",
			       name);
	  ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "CREATE SPATIAL INDEX %s error: %s\n", name,
			 sqlite3_errmsg (handle));
		return;
	    }
	  xname = gaiaDoubleQuotedSql (name);
	  sql =
	      sqlite3_mprintf
	      ("INSERT INTO \"%s\" (feature_id, layer, geometry) "
	       "VALUES (NULL, ?, ?)", xname);
	  free (xname);
	  ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "CREATE STATEMENT %s error: %s\n", name,
			 sqlite3_errmsg (handle));
		return;
	    }
	  if (hasExtraPolyg)
	    {
		/* creating the Extra Attribute table */
		extra_name = sqlite3_mprintf ("%s_attr", name);
		fk_name = sqlite3_mprintf ("fk_%s", extra_name);
		xextra_name = gaiaDoubleQuotedSql (extra_name);
		xfk_name = gaiaDoubleQuotedSql (fk_name);
		xname = gaiaDoubleQuotedSql (name);
		sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
				       "    attr_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
				       "    feature_id INTEGER NOT NULL,\n"
				       "    attr_key TEXT NOT NULL,\n"
				       "    attr_value TEXT NOT NULL,\n"
				       "    CONSTRAINT \"%s\" FOREIGN KEY (feature_id) "
				       "REFERENCES \"%s\" (feature_id))",
				       xextra_name, xfk_name, xname);
		free (xextra_name);
		free (xfk_name);
		free (xname);
		sqlite3_free (fk_name);
		ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr,
			       "CREATE TABLE %s error: %s\n", extra_name,
			       sqlite3_errmsg (handle));
		      sqlite3_finalize (stmt);
		      return;
		  }
		idx_name = sqlite3_mprintf ("idx_%s", extra_name);
		xidx_name = gaiaDoubleQuotedSql (idx_name);
		xextra_name = gaiaDoubleQuotedSql (extra_name);
		sql =
		    sqlite3_mprintf
		    ("CREATE INDEX \"%s\" ON \"%s\" (feature_id)", xidx_name,
		     xextra_name);
		free (xidx_name);
		free (xextra_name);
		ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr,
			       "CREATE INDEX %s error: %s\n", idx_name,
			       sqlite3_errmsg (handle));
		      sqlite3_finalize (stmt);
		      return;
		  }
		sqlite3_free (idx_name);
		view_name = sqlite3_mprintf ("%s_view", name);
		xview_name = gaiaDoubleQuotedSql (view_name);
		xname = gaiaDoubleQuotedSql (name);
		xextra_name = gaiaDoubleQuotedSql (extra_name);
		sql = sqlite3_mprintf ("CREATE VIEW \"%s\" AS "
				       "SELECT f.feature_id AS feature_id, f.layer AS layer, f.geometry AS geometry, "
				       "a.attr_id AS attr_id, a.attr_key AS attr_key, a.attr_value AS attr_value "
				       "FROM \"%s\" AS f "
				       "LEFT JOIN \"%s\" AS a ON (f.feature_id = a.feature_id)",
				       xview_name, xname, xextra_name);
		free (xview_name);
		free (xname);
		free (xextra_name);
		ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr,
			       "CREATE VIEW %s error: %s\n", view_name,
			       sqlite3_errmsg (handle));
		      sqlite3_finalize (stmt);
		      return;
		  }
		sqlite3_free (view_name);
		xextra_name = gaiaDoubleQuotedSql (extra_name);
		sql =
		    sqlite3_mprintf
		    ("INSERT INTO \"%s\" (attr_id, feature_id, attr_key, attr_value) "
		     "VALUES (NULL, ?, ?, ?)", xextra_name);
		free (xextra_name);
		ret =
		    sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_ext,
					NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr,
			       "CREATE STATEMENT %s error: %s\n", extra_name,
			       sqlite3_errmsg (handle));
		      sqlite3_finalize (stmt);
		      return;
		  }
	    }
	  ret = sqlite3_exec (handle, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "BEGIN %s error: %s\n", name,
			 sqlite3_errmsg (handle));
		sqlite3_finalize (stmt);
		if (stmt_ext != NULL)
		    sqlite3_finalize (stmt_ext);
		return;
	    }
	  lyr = dxf->first;
	  while (lyr != NULL)
	    {
		dxfPolylinePtr pg = lyr->first_polyg;
		while (pg != NULL)
		  {
		      dxfHolePtr hole;
		      int num_holes;
		      int iv;
		      gaiaPolygonPtr p_pg;
		      gaiaRingPtr p_rng;
		      sqlite3_reset (stmt);
		      sqlite3_clear_bindings (stmt);
		      sqlite3_bind_text (stmt, 1, lyr->layer_name,
					 strlen (lyr->layer_name),
					 SQLITE_STATIC);
		      if (polyg3D)
			  geom = gaiaAllocGeomCollXYZ ();
		      else
			  geom = gaiaAllocGeomColl ();
		      geom->Srid = dxf->srid;
		      num_holes = 0;
		      hole = pg->first_hole;
		      while (hole != NULL)
			{
			    num_holes++;
			    hole = hole->next;
			}
		      gaiaAddPolygonToGeomColl (geom, pg->points, num_holes);
		      p_pg = geom->FirstPolygon;
		      p_rng = p_pg->Exterior;
		      for (iv = 0; iv < pg->points; iv++)
			{
			    if (lyr->is3Dpolyg)
			      {
				  gaiaSetPointXYZ (p_rng->Coords, iv,
						   *(pg->x + iv), *(pg->y + iv),
						   *(pg->z + iv));
			      }
			    else
			      {
				  gaiaSetPoint (p_rng->Coords, iv,
						*(pg->x + iv), *(pg->y + iv));
			      }
			}
		      num_holes = 0;
		      hole = pg->first_hole;
		      while (hole != NULL)
			{
			    p_rng =
				gaiaAddInteriorRing (p_pg, num_holes,
						     hole->points);
			    for (iv = 0; iv < hole->points; iv++)
			      {
				  if (lyr->is3Dpolyg)
				    {
					gaiaSetPointXYZ (p_rng->Coords, iv,
							 *(hole->x + iv),
							 *(hole->y + iv),
							 *(hole->z + iv));
				    }
				  else
				    {
					gaiaSetPoint (p_rng->Coords, iv,
						      *(hole->x + iv),
						      *(hole->y + iv));
				    }
			      }
			    num_holes++;
			    hole = hole->next;
			}
		      gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
		      gaiaFreeGeomColl (geom);
		      sqlite3_bind_blob (stmt, 2, blob, blob_size, free);
		      ret = sqlite3_step (stmt);
		      if (ret == SQLITE_DONE || ret == SQLITE_ROW)
			  ;
		      else
			{
			    fprintf (stderr, "INSERT %s error: %s\n", name,
				     sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    if (stmt_ext != NULL)
				sqlite3_finalize (stmt_ext);
			    ret =
				sqlite3_exec (handle, "ROLLBACK", NULL, NULL,
					      NULL);
			    return;
			}
		      if (stmt_ext != NULL)
			{
			    /* inserting all Extra Attributes */
			    sqlite3_int64 feature_id =
				sqlite3_last_insert_rowid (handle);
			    dxfExtraAttrPtr ext = pg->first;
			    while (ext != NULL)
			      {
				  sqlite3_reset (stmt_ext);
				  sqlite3_clear_bindings (stmt_ext);
				  sqlite3_bind_int64 (stmt_ext, 1, feature_id);
				  sqlite3_bind_text (stmt_ext, 2, ext->key,
						     strlen (ext->key),
						     SQLITE_STATIC);
				  sqlite3_bind_text (stmt_ext, 3, ext->value,
						     strlen (ext->value),
						     SQLITE_STATIC);
				  ret = sqlite3_step (stmt_ext);
				  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
				      ;
				  else
				    {
					fprintf (stderr,
						 "INSERT %s error: %s\n",
						 extra_name,
						 sqlite3_errmsg (handle));
					sqlite3_finalize (stmt);
					if (stmt_ext != NULL)
					    sqlite3_finalize (stmt_ext);
					sqlite3_finalize (stmt);
					if (stmt_ext != NULL)
					    sqlite3_finalize (stmt_ext);
					sqlite3_finalize (stmt_ext);
					ret =
					    sqlite3_exec (handle, "ROLLBACK",
							  NULL, NULL, NULL);
					return;
				    }
				  ext = ext->next;
			      }
			}
		      pg = pg->next;
		  }
		lyr = lyr->next;
	    }
	  sqlite3_free (name);
	  if (extra_name)
	      sqlite3_free (extra_name);
	  sqlite3_finalize (stmt);
	  if (stmt_ext != NULL)
	      sqlite3_finalize (stmt_ext);
	  ret = sqlite3_exec (handle, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "COMMIT polyg_layer error: %s\n",
			 sqlite3_errmsg (handle));
		return;
	    }
      }
}

static void
import_by_layer (sqlite3 * handle, dxfParserPtr dxf)
{
/* populating the target DB - by distinct layers */
    char *sql;
    int ret;
    sqlite3_stmt *stmt;
    sqlite3_stmt *stmt_ext;
    unsigned char *blob;
    int blob_size;
    gaiaGeomCollPtr geom;
    gaiaLinestringPtr p_ln;
    gaiaPolygonPtr p_pg;
    gaiaRingPtr p_rng;
    int iv;
    char *name;
    char *xname;
    char *attr_name;
    char *xattr_name;
    char *fk_name;
    char *xfk_name;
    char *idx_name;
    char *xidx_name;
    char *view_name;
    char *xview_name;
    dxfTextPtr txt;
    dxfPointPtr pt;
    dxfPolylinePtr ln;
    dxfPolylinePtr pg;

    dxfLayerPtr lyr = dxf->first;
    while (lyr != NULL)
      {
	  /* looping on layers */
	  int text = 0;
	  int point = 0;
	  int line = 0;
	  int polyg = 0;
	  int suffix = 0;
	  if (lyr->first_text != NULL)
	      text = 1;
	  if (lyr->first_point != NULL)
	      point = 1;
	  if (lyr->first_line != NULL)
	      line = 1;
	  if (lyr->first_polyg != NULL)
	      polyg = 1;
	  if (text + point + line + polyg > 1)
	      suffix = 1;
	  else
	      suffix = 0;
	  if (text)
	    {
		/* creating and populating the TEXT-layer */
		stmt_ext = NULL;
		attr_name = NULL;
		if (dxf->prefix == NULL)
		  {
		      if (suffix)
			  name = sqlite3_mprintf ("%s_text", lyr->layer_name);
		      else
			  name = sqlite3_mprintf ("%s", lyr->layer_name);
		  }
		else
		  {
		      if (suffix)
			  name =
			      sqlite3_mprintf ("%s%s_text", dxf->prefix,
					       lyr->layer_name);
		      else
			  name =
			      sqlite3_mprintf ("%s%s", dxf->prefix,
					       lyr->layer_name);
		  }
		xname = gaiaDoubleQuotedSql (name);
		sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
				       "    feature_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
				       "    layer TEXT NOT NULL,\n"
				       "    label TEXT NOT NULL,\n"
				       "    rotation DOUBLE NOT NULL)", xname);
		free (xname);
		ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr, "CREATE TABLE %s error: %s\n", name,
			       sqlite3_errmsg (handle));
		      sqlite3_free (name);
		      return;
		  }
		sql =
		    sqlite3_mprintf ("SELECT AddGeometryColumn(%Q, 'geometry', "
				     "%d, 'POINT', %Q)", name, dxf->srid,
				     lyr->is3Dtext ? "XYZ" : "XY");
		ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr, "ADD GEOMETRY %s error: %s\n", name,
			       sqlite3_errmsg (handle));
		      sqlite3_free (name);
		      return;
		  }
		sql =
		    sqlite3_mprintf
		    ("SELECT CreateSpatialIndex(%Q, 'geometry')", name);
		ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr, "CREATE SPATIAL INDEX %s error: %s\n",
			       name, sqlite3_errmsg (handle));
		      sqlite3_free (name);
		      return;
		  }
		sql =
		    sqlite3_mprintf
		    ("INSERT INTO \"%s\" (feature_id, layer, label, rotation, geometry) "
		     "VALUES (NULL, ?, ?, ?, ?)", name);
		ret =
		    sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr, "CREATE STATEMENT %s error: %s\n", name,
			       sqlite3_errmsg (handle));
		      sqlite3_free (name);
		      return;
		  }
		if (lyr->hasExtraText)
		  {
		      /* creating the Extra Attribute table */
		      if (dxf->prefix == NULL)
			{
			    if (suffix)
			      {
				  attr_name =
				      sqlite3_mprintf ("%s_text_attr",
						       lyr->layer_name);
				  fk_name =
				      sqlite3_mprintf ("fk_%s_text_attr",
						       lyr->layer_name);
			      }
			    else
			      {
				  attr_name =
				      sqlite3_mprintf ("%s_attr",
						       lyr->layer_name);
				  fk_name =
				      sqlite3_mprintf ("fk_%s_attr",
						       lyr->layer_name);
			      }
			}
		      else
			{
			    if (suffix)
			      {
				  attr_name =
				      sqlite3_mprintf ("%s%s_text_attr",
						       dxf->prefix,
						       lyr->layer_name);
				  fk_name =
				      sqlite3_mprintf ("fk_%s%s_text_attr",
						       dxf->prefix,
						       lyr->layer_name);
			      }
			    else
			      {
				  attr_name =
				      sqlite3_mprintf ("%s%s_attr", dxf->prefix,
						       lyr->layer_name);
				  fk_name =
				      sqlite3_mprintf ("fk_%s%s_attr",
						       dxf->prefix,
						       lyr->layer_name);
			      }
			}
		      xfk_name = gaiaDoubleQuotedSql (fk_name);
		      xattr_name = gaiaDoubleQuotedSql (attr_name);
		      xname = gaiaDoubleQuotedSql (name);
		      sqlite3_free (fk_name);
		      sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
					     "    attr_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
					     "    feature_id INTEGER NOT NULL,\n"
					     "    attr_key TEXT NOT NULL,\n"
					     "    attr_value TEXT NOT NULL,\n"
					     "    CONSTRAINT \"%s\" FOREIGN KEY (feature_id) "
					     "REFERENCES \"%s\" (feature_id))",
					     xattr_name, xfk_name, xname);
		      free (xfk_name);
		      ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		      sqlite3_free (sql);
		      if (ret != SQLITE_OK)
			{
			    fprintf (stderr,
				     "CREATE TABLE %s error: %s\n", fk_name,
				     sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    return;
			}
		      if (dxf->prefix == NULL)
			{
			    if (suffix)
				idx_name =
				    sqlite3_mprintf ("idx_%s_text_attr",
						     lyr->layer_name);
			    else
				idx_name =
				    sqlite3_mprintf ("idx_%s_attr",
						     lyr->layer_name);
			}
		      else
			{
			    if (suffix)
				idx_name =
				    sqlite3_mprintf ("idx_%s%s_text_attr",
						     dxf->prefix,
						     lyr->layer_name);
			    else
				idx_name =
				    sqlite3_mprintf ("idx_%s%s_attr",
						     dxf->prefix,
						     lyr->layer_name);
			}
		      xidx_name = gaiaDoubleQuotedSql (idx_name);
		      sql =
			  sqlite3_mprintf
			  ("CREATE INDEX \"%s\" ON \"%s\" (feature_id)",
			   xidx_name, xname);
		      free (xidx_name);
		      ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		      sqlite3_free (sql);
		      if (ret != SQLITE_OK)
			{
			    fprintf (stderr,
				     "CREATE INDEX %s error: %s\n", idx_name,
				     sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    return;
			}
		      sqlite3_free (idx_name);
		      if (dxf->prefix == NULL)
			{
			    if (suffix)
				view_name =
				    sqlite3_mprintf ("%s_text_view",
						     lyr->layer_name);
			    else
				view_name =
				    sqlite3_mprintf ("%s_view",
						     lyr->layer_name);
			}
		      else
			{
			    if (suffix)
				view_name =
				    sqlite3_mprintf ("%s%s_text_view",
						     dxf->prefix,
						     lyr->layer_name);
			    else
				view_name =
				    sqlite3_mprintf ("%s%s_view", dxf->prefix,
						     lyr->layer_name);
			}
		      xview_name = gaiaDoubleQuotedSql (view_name);
		      sql = sqlite3_mprintf ("CREATE VIEW \"%s\" AS "
					     "SELECT f.feature_id AS feature_id, f.layer AS layer, f.label AS label, "
					     "f.rotation AS rotation, f.geometry AS geometry, "
					     "a.attr_id AS attr_id, a.attr_key AS attr_key, a.attr_value AS attr_value "
					     "FROM \"%s\" AS f "
					     "LEFT JOIN \"%s\" AS a ON (f.feature_id = a.feature_id)",
					     xview_name, xname, xattr_name);
		      free (xview_name);
		      free (xname);
		      ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		      sqlite3_free (sql);
		      if (ret != SQLITE_OK)
			{
			    fprintf (stderr,
				     "CREATE VIEW %s error: %s\n", view_name,
				     sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    return;
			}
		      sqlite3_free (view_name);
		      sql =
			  sqlite3_mprintf
			  ("INSERT INTO \"%s\" (attr_id, feature_id, attr_key, attr_value) "
			   "VALUES (NULL, ?, ?, ?)", xattr_name);
		      free (xattr_name);
		      ret =
			  sqlite3_prepare_v2 (handle, sql, strlen (sql),
					      &stmt_ext, NULL);
		      sqlite3_free (sql);
		      if (ret != SQLITE_OK)
			{
			    fprintf (stderr,
				     "CREATE STATEMENT %s error: %s\n",
				     attr_name, sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    return;
			}
		  }
		ret = sqlite3_exec (handle, "BEGIN", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr, "BEGIN %s error: %s\n", name,
			       sqlite3_errmsg (handle));
		      sqlite3_finalize (stmt);
		      if (stmt_ext != NULL)
			  sqlite3_finalize (stmt_ext);
		      sqlite3_free (name);
		      if (attr_name)
			  sqlite3_free (attr_name);
		      return;
		  }
		txt = lyr->first_text;
		while (txt != NULL)
		  {
		      sqlite3_reset (stmt);
		      sqlite3_clear_bindings (stmt);
		      sqlite3_bind_text (stmt, 1, lyr->layer_name,
					 strlen (lyr->layer_name),
					 SQLITE_STATIC);
		      sqlite3_bind_text (stmt, 2, txt->label,
					 strlen (txt->label), SQLITE_STATIC);
		      sqlite3_bind_double (stmt, 3, txt->angle);
		      if (lyr->is3Dtext)
			  geom = gaiaAllocGeomCollXYZ ();
		      else
			  geom = gaiaAllocGeomColl ();
		      geom->Srid = dxf->srid;
		      if (lyr->is3Dtext)
			  gaiaAddPointToGeomCollXYZ (geom, txt->x, txt->y,
						     txt->z);
		      else
			  gaiaAddPointToGeomColl (geom, txt->x, txt->y);
		      gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
		      gaiaFreeGeomColl (geom);
		      sqlite3_bind_blob (stmt, 4, blob, blob_size, free);
		      ret = sqlite3_step (stmt);
		      if (ret == SQLITE_DONE || ret == SQLITE_ROW)
			  ;
		      else
			{
			    fprintf (stderr, "INSERT %s error: %s\n", name,
				     sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    if (stmt_ext != NULL)
				sqlite3_finalize (stmt_ext);
			    ret =
				sqlite3_exec (handle, "ROLLBACK", NULL, NULL,
					      NULL);
			    sqlite3_free (name);
			    if (attr_name)
				sqlite3_free (attr_name);
			    return;
			}
		      if (stmt_ext != NULL)
			{
			    /* inserting all Extra Attributes */
			    sqlite3_int64 feature_id =
				sqlite3_last_insert_rowid (handle);
			    dxfExtraAttrPtr ext = txt->first;
			    while (ext != NULL)
			      {
				  sqlite3_reset (stmt_ext);
				  sqlite3_clear_bindings (stmt_ext);
				  sqlite3_bind_int64 (stmt_ext, 1, feature_id);
				  sqlite3_bind_text (stmt_ext, 2, ext->key,
						     strlen (ext->key),
						     SQLITE_STATIC);
				  sqlite3_bind_text (stmt_ext, 3, ext->value,
						     strlen (ext->value),
						     SQLITE_STATIC);
				  ret = sqlite3_step (stmt_ext);
				  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
				      ;
				  else
				    {
					fprintf (stderr,
						 "INSERT %s error: %s\n",
						 attr_name,
						 sqlite3_errmsg (handle));
					sqlite3_finalize (stmt);
					sqlite3_finalize (stmt_ext);
					sqlite3_free (name);
					if (attr_name)
					    sqlite3_free (attr_name);
					ret =
					    sqlite3_exec (handle, "ROLLBACK",
							  NULL, NULL, NULL);
					return;
				    }
				  ext = ext->next;
			      }
			}
		      txt = txt->next;
		  }
		sqlite3_finalize (stmt);
		if (stmt_ext != NULL)
		    sqlite3_finalize (stmt_ext);
		ret = sqlite3_exec (handle, "COMMIT", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr, "COMMIT %s error: %s\n", name,
			       sqlite3_errmsg (handle));
		      sqlite3_free (name);
		      if (attr_name)
			  sqlite3_free (attr_name);
		      return;
		  }
		sqlite3_free (name);
		if (attr_name)
		    sqlite3_free (attr_name);
	    }

	  if (point)
	    {
		/* creating and populating the POINT-layer */
		stmt_ext = NULL;
		attr_name = NULL;
		if (dxf->prefix == NULL)
		  {
		      if (suffix)
			  name = sqlite3_mprintf ("%s_point", lyr->layer_name);
		      else
			  name = sqlite3_mprintf ("%s", lyr->layer_name);
		  }
		else
		  {
		      if (suffix)
			  name =
			      sqlite3_mprintf ("%s%s_point", dxf->prefix,
					       lyr->layer_name);
		      else
			  name =
			      sqlite3_mprintf ("%s%s", dxf->prefix,
					       lyr->layer_name);
		  }
		xname = gaiaDoubleQuotedSql (name);
		sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
				       "    feature_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
				       "    layer TEXT NOT NULL)", xname);
		free (xname);
		ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr, "CREATE TABLE %s error: %s\n", name,
			       sqlite3_errmsg (handle));
		      sqlite3_free (name);
		      return;
		  }
		sql =
		    sqlite3_mprintf ("SELECT AddGeometryColumn(%Q, 'geometry', "
				     "%d, 'POINT', %Q)", name, dxf->srid,
				     lyr->is3Dpoint ? "XYZ" : "XY");
		ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr, "ADD GEOMETRY %s error: %s\n", name,
			       sqlite3_errmsg (handle));
		      sqlite3_free (name);
		      return;
		  }
		sql =
		    sqlite3_mprintf
		    ("SELECT CreateSpatialIndex(%Q, 'geometry')", name);
		ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr, "CREATE SPATIAL INDEX %s error: %s\n",
			       name, sqlite3_errmsg (handle));
		      sqlite3_free (name);
		      return;
		  }
		sql =
		    sqlite3_mprintf
		    ("INSERT INTO \"%s\" (feature_id, layer, geometry) "
		     "VALUES (NULL, ?, ?)", name);
		ret =
		    sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr, "CREATE STATEMENT %s error: %s\n", name,
			       sqlite3_errmsg (handle));
		      sqlite3_free (name);
		      return;
		  }
		if (lyr->hasExtraPoint)
		  {
		      /* creating the Extra Attribute table */
		      if (dxf->prefix == NULL)
			{
			    if (suffix)
			      {
				  attr_name =
				      sqlite3_mprintf ("%s_point_attr",
						       lyr->layer_name);
				  fk_name =
				      sqlite3_mprintf ("fk_%s_point_attr",
						       lyr->layer_name);
			      }
			    else
			      {
				  attr_name =
				      sqlite3_mprintf ("%s_attr",
						       lyr->layer_name);
				  fk_name =
				      sqlite3_mprintf ("fk_%s_attr",
						       lyr->layer_name);
			      }
			}
		      else
			{
			    if (suffix)
			      {
				  attr_name =
				      sqlite3_mprintf ("%s%s_point_attr",
						       dxf->prefix,
						       lyr->layer_name);
				  fk_name =
				      sqlite3_mprintf ("fk_%s%s_point_attr",
						       dxf->prefix,
						       lyr->layer_name);
			      }
			    else
			      {
				  attr_name =
				      sqlite3_mprintf ("%s%s_attr", dxf->prefix,
						       lyr->layer_name);
				  fk_name =
				      sqlite3_mprintf ("fk_%s%s_attr",
						       dxf->prefix,
						       lyr->layer_name);
			      }
			}
		      xfk_name = gaiaDoubleQuotedSql (fk_name);
		      xattr_name = gaiaDoubleQuotedSql (attr_name);
		      xname = gaiaDoubleQuotedSql (name);
		      sqlite3_free (fk_name);
		      sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
					     "    attr_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
					     "    feature_id INTEGER NOT NULL,\n"
					     "    attr_key TEXT NOT NULL,\n"
					     "    attr_value TEXT NOT NULL,\n"
					     "    CONSTRAINT \"%s\" FOREIGN KEY (feature_id) "
					     "REFERENCES \"%s\" (feature_id))",
					     xattr_name, xfk_name, xname);
		      free (xfk_name);
		      ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		      sqlite3_free (sql);
		      if (ret != SQLITE_OK)
			{
			    fprintf (stderr,
				     "CREATE TABLE %s error: %s\n", fk_name,
				     sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    return;
			}
		      if (dxf->prefix == NULL)
			{
			    if (suffix)
				idx_name =
				    sqlite3_mprintf ("idx_%s_poiny_attr",
						     lyr->layer_name);
			    else
				idx_name =
				    sqlite3_mprintf ("idx_%s_attr",
						     lyr->layer_name);
			}
		      else
			{
			    if (suffix)
				idx_name =
				    sqlite3_mprintf ("idx_%s%s_point_attr",
						     dxf->prefix,
						     lyr->layer_name);
			    else
				idx_name =
				    sqlite3_mprintf ("idx_%s%s_attr",
						     dxf->prefix,
						     lyr->layer_name);
			}
		      xidx_name = gaiaDoubleQuotedSql (idx_name);
		      sql =
			  sqlite3_mprintf
			  ("CREATE INDEX \"%s\" ON \"%s\" (feature_id)",
			   xidx_name, xname);
		      free (xidx_name);
		      ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		      sqlite3_free (sql);
		      if (ret != SQLITE_OK)
			{
			    fprintf (stderr,
				     "CREATE INDEX %s error: %s\n", idx_name,
				     sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    return;
			}
		      sqlite3_free (idx_name);
		      if (dxf->prefix == NULL)
			{
			    if (suffix)
				view_name =
				    sqlite3_mprintf ("%s_point_view",
						     lyr->layer_name);
			    else
				view_name =
				    sqlite3_mprintf ("%s_view",
						     lyr->layer_name);
			}
		      else
			{
			    if (suffix)
				view_name =
				    sqlite3_mprintf ("%s%s_point_view",
						     dxf->prefix,
						     lyr->layer_name);
			    else
				view_name =
				    sqlite3_mprintf ("%s%s_view", dxf->prefix,
						     lyr->layer_name);
			}
		      xview_name = gaiaDoubleQuotedSql (view_name);
		      sql = sqlite3_mprintf ("CREATE VIEW \"%s\" AS "
					     "SELECT f.feature_id AS feature_id, f.layer AS layer, f.geometry AS geometry, "
					     "a.attr_id AS attr_id, a.attr_key AS attr_key, a.attr_value AS attr_value "
					     "FROM \"%s\" AS f "
					     "LEFT JOIN \"%s\" AS a ON (f.feature_id = a.feature_id)",
					     xview_name, xname, xattr_name);
		      free (xview_name);
		      free (xname);
		      ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		      sqlite3_free (sql);
		      if (ret != SQLITE_OK)
			{
			    fprintf (stderr,
				     "CREATE VIEW %s error: %s\n", view_name,
				     sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    return;
			}
		      sqlite3_free (view_name);
		      sql =
			  sqlite3_mprintf
			  ("INSERT INTO \"%s\" (attr_id, feature_id, attr_key, attr_value) "
			   "VALUES (NULL, ?, ?, ?)", xattr_name);
		      free (xattr_name);
		      ret =
			  sqlite3_prepare_v2 (handle, sql, strlen (sql),
					      &stmt_ext, NULL);
		      sqlite3_free (sql);
		      if (ret != SQLITE_OK)
			{
			    fprintf (stderr,
				     "CREATE STATEMENT %s error: %s\n",
				     attr_name, sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    return;
			}
		  }
		ret = sqlite3_exec (handle, "BEGIN", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr, "BEGIN %s error: %s\n", name,
			       sqlite3_errmsg (handle));
		      sqlite3_finalize (stmt);
		      if (stmt_ext != NULL)
			  sqlite3_finalize (stmt_ext);
		      sqlite3_free (name);
		      if (attr_name)
			  sqlite3_free (attr_name);
		      return;
		  }
		pt = lyr->first_point;
		while (pt != NULL)
		  {
		      sqlite3_reset (stmt);
		      sqlite3_clear_bindings (stmt);
		      sqlite3_bind_text (stmt, 1, lyr->layer_name,
					 strlen (lyr->layer_name),
					 SQLITE_STATIC);
		      if (lyr->is3Dpoint)
			  geom = gaiaAllocGeomCollXYZ ();
		      else
			  geom = gaiaAllocGeomColl ();
		      geom->Srid = dxf->srid;
		      if (lyr->is3Dpoint)
			  gaiaAddPointToGeomCollXYZ (geom, pt->x, pt->y, pt->z);
		      else
			  gaiaAddPointToGeomColl (geom, pt->x, pt->y);
		      gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
		      gaiaFreeGeomColl (geom);
		      sqlite3_bind_blob (stmt, 2, blob, blob_size, free);
		      ret = sqlite3_step (stmt);
		      if (ret == SQLITE_DONE || ret == SQLITE_ROW)
			  ;
		      else
			{
			    fprintf (stderr, "INSERT %s error: %s\n", name,
				     sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    if (stmt_ext != NULL)
				sqlite3_finalize (stmt_ext);
			    ret =
				sqlite3_exec (handle, "ROLLBACK", NULL, NULL,
					      NULL);
			    sqlite3_free (name);
			    if (attr_name)
				sqlite3_free (attr_name);
			    return;
			}
		      if (stmt_ext != NULL)
			{
			    /* inserting all Extra Attributes */
			    sqlite3_int64 feature_id =
				sqlite3_last_insert_rowid (handle);
			    dxfExtraAttrPtr ext = pt->first;
			    while (ext != NULL)
			      {
				  sqlite3_reset (stmt_ext);
				  sqlite3_clear_bindings (stmt_ext);
				  sqlite3_bind_int64 (stmt_ext, 1, feature_id);
				  sqlite3_bind_text (stmt_ext, 2, ext->key,
						     strlen (ext->key),
						     SQLITE_STATIC);
				  sqlite3_bind_text (stmt_ext, 3, ext->value,
						     strlen (ext->value),
						     SQLITE_STATIC);
				  ret = sqlite3_step (stmt_ext);
				  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
				      ;
				  else
				    {
					fprintf (stderr,
						 "INSERT %s error: %s\n",
						 attr_name,
						 sqlite3_errmsg (handle));
					sqlite3_finalize (stmt);
					sqlite3_finalize (stmt_ext);
					sqlite3_free (name);
					if (attr_name)
					    sqlite3_free (attr_name);
					ret =
					    sqlite3_exec (handle, "ROLLBACK",
							  NULL, NULL, NULL);
					return;
				    }
				  ext = ext->next;
			      }
			}
		      pt = pt->next;
		  }
		sqlite3_finalize (stmt);
		if (stmt_ext != NULL)
		    sqlite3_finalize (stmt_ext);
		ret = sqlite3_exec (handle, "COMMIT", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr, "COMMIT %s error: %s\n", name,
			       sqlite3_errmsg (handle));
		      sqlite3_free (name);
		      if (attr_name)
			  sqlite3_free (attr_name);
		      return;
		  }
		sqlite3_free (name);
		if (attr_name)
		    sqlite3_free (attr_name);
	    }

	  if (line)
	    {
		/* creating and populating the LINE-layer */
		stmt_ext = NULL;
		attr_name = NULL;
		if (dxf->prefix == NULL)
		  {
		      if (suffix)
			  name = sqlite3_mprintf ("%s_line", lyr->layer_name);
		      else
			  name = sqlite3_mprintf ("%s", lyr->layer_name);
		  }
		else
		  {
		      if (suffix)
			  name =
			      sqlite3_mprintf ("%s%s_line", dxf->prefix,
					       lyr->layer_name);
		      else
			  name =
			      sqlite3_mprintf ("%s%s", dxf->prefix,
					       lyr->layer_name);
		  }
		xname = gaiaDoubleQuotedSql (name);
		sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
				       "    feature_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
				       "    layer TEXT NOT NULL)", xname);
		free (xname);
		ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr, "CREATE TABLE %s error: %s\n", name,
			       sqlite3_errmsg (handle));
		      sqlite3_free (name);
		      return;
		  }
		sql =
		    sqlite3_mprintf ("SELECT AddGeometryColumn(%Q, 'geometry', "
				     "%d, 'LINESTRING', %Q)", name, dxf->srid,
				     lyr->is3Dline ? "XYZ" : "XY");
		ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr, "ADD GEOMETRY %s error: %s\n", name,
			       sqlite3_errmsg (handle));
		      sqlite3_free (name);
		      return;
		  }
		sql =
		    sqlite3_mprintf
		    ("SELECT CreateSpatialIndex(%Q, 'geometry')", name);
		ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr, "CREATE SPATIAL INDEX %s error: %s\n",
			       name, sqlite3_errmsg (handle));
		      sqlite3_free (name);
		      return;
		  }
		sql =
		    sqlite3_mprintf
		    ("INSERT INTO \"%s\" (feature_id, layer, geometry) "
		     "VALUES (NULL, ?, ?)", name);
		ret =
		    sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr, "CREATE STATEMENT %s error: %s\n", name,
			       sqlite3_errmsg (handle));
		      sqlite3_free (name);
		      return;
		  }
		if (lyr->hasExtraLine)
		  {
		      /* creating the Extra Attribute table */
		      if (dxf->prefix == NULL)
			{
			    if (suffix)
			      {
				  attr_name =
				      sqlite3_mprintf ("%s_line_attr",
						       lyr->layer_name);
				  fk_name =
				      sqlite3_mprintf ("fk_%s_line_attr",
						       lyr->layer_name);
			      }
			    else
			      {
				  attr_name =
				      sqlite3_mprintf ("%s_attr",
						       lyr->layer_name);
				  fk_name =
				      sqlite3_mprintf ("fk_%s_attr",
						       lyr->layer_name);
			      }
			}
		      else
			{
			    if (suffix)
			      {
				  attr_name =
				      sqlite3_mprintf ("%s%s_line_attr",
						       dxf->prefix,
						       lyr->layer_name);
				  fk_name =
				      sqlite3_mprintf ("fk_%s%s_line_attr",
						       dxf->prefix,
						       lyr->layer_name);
			      }
			    else
			      {
				  attr_name =
				      sqlite3_mprintf ("%s%s_attr", dxf->prefix,
						       lyr->layer_name);
				  fk_name =
				      sqlite3_mprintf ("fk_%s%s_attr",
						       dxf->prefix,
						       lyr->layer_name);
			      }
			}
		      xfk_name = gaiaDoubleQuotedSql (fk_name);
		      xattr_name = gaiaDoubleQuotedSql (attr_name);
		      xname = gaiaDoubleQuotedSql (name);
		      sqlite3_free (fk_name);
		      sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
					     "    attr_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
					     "    feature_id INTEGER NOT NULL,\n"
					     "    attr_key TEXT NOT NULL,\n"
					     "    attr_value TEXT NOT NULL,\n"
					     "    CONSTRAINT \"%s\" FOREIGN KEY (feature_id) "
					     "REFERENCES \"%s\" (feature_id))",
					     xattr_name, xfk_name, xname);
		      free (xfk_name);
		      ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		      sqlite3_free (sql);
		      if (ret != SQLITE_OK)
			{
			    fprintf (stderr,
				     "CREATE TABLE %s error: %s\n", fk_name,
				     sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    return;
			}
		      if (dxf->prefix == NULL)
			{
			    if (suffix)
				idx_name =
				    sqlite3_mprintf ("idx_%s_line_attr",
						     lyr->layer_name);
			    else
				idx_name =
				    sqlite3_mprintf ("idx_%s_attr",
						     lyr->layer_name);
			}
		      else
			{
			    if (suffix)
				idx_name =
				    sqlite3_mprintf ("idx_%s%s_line_attr",
						     dxf->prefix,
						     lyr->layer_name);
			    else
				idx_name =
				    sqlite3_mprintf ("idx_%s%s_attr",
						     dxf->prefix,
						     lyr->layer_name);
			}
		      xidx_name = gaiaDoubleQuotedSql (idx_name);
		      sql =
			  sqlite3_mprintf
			  ("CREATE INDEX \"%s\" ON \"%s\" (feature_id)",
			   xidx_name, xname);
		      free (xidx_name);
		      ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		      sqlite3_free (sql);
		      if (ret != SQLITE_OK)
			{
			    fprintf (stderr,
				     "CREATE INDEX %s error: %s\n", idx_name,
				     sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    return;
			}
		      sqlite3_free (idx_name);
		      if (dxf->prefix == NULL)
			{
			    if (suffix)
				view_name =
				    sqlite3_mprintf ("%s_line_view",
						     lyr->layer_name);
			    else
				view_name =
				    sqlite3_mprintf ("%s_view",
						     lyr->layer_name);
			}
		      else
			{
			    if (suffix)
				view_name =
				    sqlite3_mprintf ("%s%s_line_view",
						     dxf->prefix,
						     lyr->layer_name);
			    else
				view_name =
				    sqlite3_mprintf ("%s%s_view", dxf->prefix,
						     lyr->layer_name);
			}
		      xview_name = gaiaDoubleQuotedSql (view_name);
		      sql = sqlite3_mprintf ("CREATE VIEW \"%s\" AS "
					     "SELECT f.feature_id AS feature_id, f.layer AS layer, f.geometry AS geometry, "
					     "a.attr_id AS attr_id, a.attr_key AS attr_key, a.attr_value AS attr_value "
					     "FROM \"%s\" AS f "
					     "LEFT JOIN \"%s\" AS a ON (f.feature_id = a.feature_id)",
					     xview_name, xname, xattr_name);
		      free (xview_name);
		      free (xname);
		      ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		      sqlite3_free (sql);
		      if (ret != SQLITE_OK)
			{
			    fprintf (stderr,
				     "CREATE VIEW %s error: %s\n", view_name,
				     sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    return;
			}
		      sqlite3_free (view_name);
		      sql =
			  sqlite3_mprintf
			  ("INSERT INTO \"%s\" (attr_id, feature_id, attr_key, attr_value) "
			   "VALUES (NULL, ?, ?, ?)", xattr_name);
		      free (xattr_name);
		      ret =
			  sqlite3_prepare_v2 (handle, sql, strlen (sql),
					      &stmt_ext, NULL);
		      sqlite3_free (sql);
		      if (ret != SQLITE_OK)
			{
			    fprintf (stderr,
				     "CREATE STATEMENT %s error: %s\n",
				     attr_name, sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    return;
			}
		  }
		ret = sqlite3_exec (handle, "BEGIN", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr, "BEGIN %s error: %s\n", name,
			       sqlite3_errmsg (handle));
		      sqlite3_finalize (stmt);
		      if (stmt_ext != NULL)
			  sqlite3_finalize (stmt_ext);
		      sqlite3_free (name);
		      if (attr_name)
			  sqlite3_free (attr_name);
		      return;
		  }
		ln = lyr->first_line;
		while (ln != NULL)
		  {
		      sqlite3_reset (stmt);
		      sqlite3_clear_bindings (stmt);
		      sqlite3_bind_text (stmt, 1, lyr->layer_name,
					 strlen (lyr->layer_name),
					 SQLITE_STATIC);
		      if (lyr->is3Dline)
			  geom = gaiaAllocGeomCollXYZ ();
		      else
			  geom = gaiaAllocGeomColl ();
		      geom->Srid = dxf->srid;
		      gaiaAddLinestringToGeomColl (geom, ln->points);
		      p_ln = geom->FirstLinestring;
		      for (iv = 0; iv < ln->points; iv++)
			{
			    if (lyr->is3Dline)
			      {
				  gaiaSetPointXYZ (p_ln->Coords, iv,
						   *(ln->x + iv), *(ln->y + iv),
						   *(ln->z + iv));
			      }
			    else
			      {
				  gaiaSetPoint (p_ln->Coords, iv, *(ln->x + iv),
						*(ln->y + iv));
			      }
			}
		      gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
		      gaiaFreeGeomColl (geom);
		      sqlite3_bind_blob (stmt, 2, blob, blob_size, free);
		      ret = sqlite3_step (stmt);
		      if (ret == SQLITE_DONE || ret == SQLITE_ROW)
			  ;
		      else
			{
			    fprintf (stderr, "INSERT %s error: %s\n", name,
				     sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    if (stmt_ext != NULL)
				sqlite3_finalize (stmt_ext);
			    ret =
				sqlite3_exec (handle, "ROLLBACK", NULL, NULL,
					      NULL);
			    sqlite3_free (name);
			    if (attr_name)
				sqlite3_free (attr_name);
			    return;
			}
		      if (stmt_ext != NULL)
			{
			    /* inserting all Extra Attributes */
			    sqlite3_int64 feature_id =
				sqlite3_last_insert_rowid (handle);
			    dxfExtraAttrPtr ext = ln->first;
			    while (ext != NULL)
			      {
				  sqlite3_reset (stmt_ext);
				  sqlite3_clear_bindings (stmt_ext);
				  sqlite3_bind_int64 (stmt_ext, 1, feature_id);
				  sqlite3_bind_text (stmt_ext, 2, ext->key,
						     strlen (ext->key),
						     SQLITE_STATIC);
				  sqlite3_bind_text (stmt_ext, 3, ext->value,
						     strlen (ext->value),
						     SQLITE_STATIC);
				  ret = sqlite3_step (stmt_ext);
				  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
				      ;
				  else
				    {
					fprintf (stderr,
						 "INSERT %s error: %s\n",
						 attr_name,
						 sqlite3_errmsg (handle));
					sqlite3_finalize (stmt);
					sqlite3_finalize (stmt_ext);
					sqlite3_free (name);
					if (attr_name)
					    sqlite3_free (attr_name);
					ret =
					    sqlite3_exec (handle, "ROLLBACK",
							  NULL, NULL, NULL);
					return;
				    }
				  ext = ext->next;
			      }
			}
		      ln = ln->next;
		  }
		sqlite3_finalize (stmt);
		if (stmt_ext != NULL)
		    sqlite3_finalize (stmt_ext);
		ret = sqlite3_exec (handle, "COMMIT", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr, "COMMIT %s error: %s\n", name,
			       sqlite3_errmsg (handle));
		      sqlite3_free (name);
		      if (attr_name)
			  sqlite3_free (attr_name);
		      return;
		  }
		sqlite3_free (name);
		if (attr_name)
		    sqlite3_free (attr_name);
	    }

	  if (polyg)
	    {
		/* creating and populating the POLYG-layer */
		stmt_ext = NULL;
		attr_name = NULL;
		if (dxf->prefix == NULL)
		  {
		      if (suffix)
			  name = sqlite3_mprintf ("%s_polyg", lyr->layer_name);
		      else
			  name = sqlite3_mprintf ("%s", lyr->layer_name);
		  }
		else
		  {
		      if (suffix)
			  name =
			      sqlite3_mprintf ("%s%s_polyg", dxf->prefix,
					       lyr->layer_name);
		      else
			  name =
			      sqlite3_mprintf ("%s%s", dxf->prefix,
					       lyr->layer_name);
		  }
		xname = gaiaDoubleQuotedSql (name);
		sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
				       "    feature_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
				       "    layer TEXT NOT NULL)", xname);
		free (xname);
		ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr, "CREATE TABLE %s error: %s\n", name,
			       sqlite3_errmsg (handle));
		      sqlite3_free (name);
		      return;
		  }
		sql =
		    sqlite3_mprintf ("SELECT AddGeometryColumn(%Q, 'geometry', "
				     "%d, 'POLYGON', %Q)", name, dxf->srid,
				     lyr->is3Dpolyg ? "XYZ" : "XY");
		ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr, "ADD GEOMETRY %s error: %s\n", name,
			       sqlite3_errmsg (handle));
		      sqlite3_free (name);
		      return;
		  }
		sql =
		    sqlite3_mprintf
		    ("SELECT CreateSpatialIndex(%Q, 'geometry')", name);
		ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr, "CREATE SPATIAL INDEX %s error: %s\n",
			       name, sqlite3_errmsg (handle));
		      sqlite3_free (name);
		      return;
		  }
		sql =
		    sqlite3_mprintf
		    ("INSERT INTO \"%s\" (feature_id, layer, geometry) "
		     "VALUES (NULL, ?, ?)", name);
		ret =
		    sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr, "CREATE STATEMENT %s error: %s\n", name,
			       sqlite3_errmsg (handle));
		      sqlite3_free (name);
		      return;
		  }
		if (lyr->hasExtraText)
		  {
		      /* creating the Extra Attribute table */
		      if (dxf->prefix == NULL)
			{
			    if (suffix)
			      {
				  attr_name =
				      sqlite3_mprintf ("%s_polyg_attr",
						       lyr->layer_name);
				  fk_name =
				      sqlite3_mprintf ("fk_%s_polyg_attr",
						       lyr->layer_name);
			      }
			    else
			      {
				  attr_name =
				      sqlite3_mprintf ("%s_attr",
						       lyr->layer_name);
				  fk_name =
				      sqlite3_mprintf ("fk_%s_attr",
						       lyr->layer_name);
			      }
			}
		      else
			{
			    if (suffix)
			      {
				  attr_name =
				      sqlite3_mprintf ("%s%s_polyg_attr",
						       dxf->prefix,
						       lyr->layer_name);
				  fk_name =
				      sqlite3_mprintf ("fk_%s%s_polyg_attr",
						       dxf->prefix,
						       lyr->layer_name);
			      }
			    else
			      {
				  attr_name =
				      sqlite3_mprintf ("%s%s_attr", dxf->prefix,
						       lyr->layer_name);
				  fk_name =
				      sqlite3_mprintf ("fk_%s%s_attr",
						       dxf->prefix,
						       lyr->layer_name);
			      }
			}
		      xfk_name = gaiaDoubleQuotedSql (fk_name);
		      xattr_name = gaiaDoubleQuotedSql (attr_name);
		      xname = gaiaDoubleQuotedSql (name);
		      sqlite3_free (fk_name);
		      sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
					     "    attr_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
					     "    feature_id INTEGER NOT NULL,\n"
					     "    attr_key TEXT NOT NULL,\n"
					     "    attr_value TEXT NOT NULL,\n"
					     "    CONSTRAINT \"%s\" FOREIGN KEY (feature_id) "
					     "REFERENCES \"%s\" (feature_id))",
					     xattr_name, xfk_name, xname);
		      free (xfk_name);
		      ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		      sqlite3_free (sql);
		      if (ret != SQLITE_OK)
			{
			    fprintf (stderr,
				     "CREATE TABLE %s error: %s\n", fk_name,
				     sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    return;
			}
		      if (dxf->prefix == NULL)
			{
			    if (suffix)
				idx_name =
				    sqlite3_mprintf ("idx_%s_polyg_attr",
						     lyr->layer_name);
			    else
				idx_name =
				    sqlite3_mprintf ("idx_%s_attr",
						     lyr->layer_name);
			}
		      else
			{
			    if (suffix)
				idx_name =
				    sqlite3_mprintf ("idx_%s%s_polyg_attr",
						     dxf->prefix,
						     lyr->layer_name);
			    else
				idx_name =
				    sqlite3_mprintf ("idx_%s%s_attr",
						     dxf->prefix,
						     lyr->layer_name);
			}
		      xidx_name = gaiaDoubleQuotedSql (idx_name);
		      sql =
			  sqlite3_mprintf
			  ("CREATE INDEX \"%s\" ON \"%s\" (feature_id)",
			   xidx_name, xname);
		      free (xidx_name);
		      ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		      sqlite3_free (sql);
		      if (ret != SQLITE_OK)
			{
			    fprintf (stderr,
				     "CREATE INDEX %s error: %s\n", idx_name,
				     sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    return;
			}
		      sqlite3_free (idx_name);
		      if (dxf->prefix == NULL)
			{
			    if (suffix)
				view_name =
				    sqlite3_mprintf ("%s_polyg_view",
						     lyr->layer_name);
			    else
				view_name =
				    sqlite3_mprintf ("%s_view",
						     lyr->layer_name);
			}
		      else
			{
			    if (suffix)
				view_name =
				    sqlite3_mprintf ("%s%s_polyg_view",
						     dxf->prefix,
						     lyr->layer_name);
			    else
				view_name =
				    sqlite3_mprintf ("%s%s_view", dxf->prefix,
						     lyr->layer_name);
			}
		      xview_name = gaiaDoubleQuotedSql (view_name);
		      sql = sqlite3_mprintf ("CREATE VIEW \"%s\" AS "
					     "SELECT f.feature_id AS feature_id, f.layer AS layer, f.geometry AS geometry, "
					     "a.attr_id AS attr_id, a.attr_key AS attr_key, a.attr_value AS attr_value "
					     "FROM \"%s\" AS f "
					     "LEFT JOIN \"%s\" AS a ON (f.feature_id = a.feature_id)",
					     xview_name, xname, xattr_name);
		      free (xview_name);
		      free (xname);
		      ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
		      sqlite3_free (sql);
		      if (ret != SQLITE_OK)
			{
			    fprintf (stderr,
				     "CREATE VIEW %s error: %s\n", view_name,
				     sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    return;
			}
		      sqlite3_free (view_name);
		      sql =
			  sqlite3_mprintf
			  ("INSERT INTO \"%s\" (attr_id, feature_id, attr_key, attr_value) "
			   "VALUES (NULL, ?, ?, ?)", xattr_name);
		      free (xattr_name);
		      ret =
			  sqlite3_prepare_v2 (handle, sql, strlen (sql),
					      &stmt_ext, NULL);
		      sqlite3_free (sql);
		      if (ret != SQLITE_OK)
			{
			    fprintf (stderr,
				     "CREATE STATEMENT %s error: %s\n",
				     attr_name, sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    return;
			}
		  }
		ret = sqlite3_exec (handle, "BEGIN", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr, "BEGIN %s error: %s\n", name,
			       sqlite3_errmsg (handle));
		      sqlite3_finalize (stmt);
		      if (stmt_ext != NULL)
			  sqlite3_finalize (stmt_ext);
		      sqlite3_free (name);
		      if (attr_name)
			  sqlite3_free (attr_name);
		      return;
		  }
		pg = lyr->first_polyg;
		while (pg != NULL)
		  {
		      int num_holes;
		      dxfHolePtr hole;
		      sqlite3_reset (stmt);
		      sqlite3_clear_bindings (stmt);
		      sqlite3_bind_text (stmt, 1, lyr->layer_name,
					 strlen (lyr->layer_name),
					 SQLITE_STATIC);
		      if (lyr->is3Dpolyg)
			  geom = gaiaAllocGeomCollXYZ ();
		      else
			  geom = gaiaAllocGeomColl ();
		      geom->Srid = dxf->srid;
		      num_holes = 0;
		      hole = pg->first_hole;
		      while (hole != NULL)
			{
			    num_holes++;
			    hole = hole->next;
			}
		      gaiaAddPolygonToGeomColl (geom, pg->points, num_holes);
		      p_pg = geom->FirstPolygon;
		      p_rng = p_pg->Exterior;
		      for (iv = 0; iv < pg->points; iv++)
			{
			    if (lyr->is3Dpolyg)
			      {
				  gaiaSetPointXYZ (p_rng->Coords, iv,
						   *(pg->x + iv), *(pg->y + iv),
						   *(pg->z + iv));
			      }
			    else
			      {
				  gaiaSetPoint (p_rng->Coords, iv,
						*(pg->x + iv), *(pg->y + iv));
			      }
			}
		      num_holes = 0;
		      hole = pg->first_hole;
		      while (hole != NULL)
			{
			    p_rng =
				gaiaAddInteriorRing (p_pg, num_holes,
						     hole->points);
			    for (iv = 0; iv < hole->points; iv++)
			      {
				  if (lyr->is3Dpolyg)
				    {
					gaiaSetPointXYZ (p_rng->Coords, iv,
							 *(hole->x + iv),
							 *(hole->y + iv),
							 *(hole->z + iv));
				    }
				  else
				    {
					gaiaSetPoint (p_rng->Coords, iv,
						      *(hole->x + iv),
						      *(hole->y + iv));
				    }
			      }
			    num_holes++;
			    hole = hole->next;
			}
		      gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
		      gaiaFreeGeomColl (geom);
		      sqlite3_bind_blob (stmt, 2, blob, blob_size, free);
		      ret = sqlite3_step (stmt);
		      if (ret == SQLITE_DONE || ret == SQLITE_ROW)
			  ;
		      else
			{
			    fprintf (stderr, "INSERT %s error: %s\n", name,
				     sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    if (stmt_ext != NULL)
				sqlite3_finalize (stmt_ext);
			    ret =
				sqlite3_exec (handle, "ROLLBACK", NULL, NULL,
					      NULL);
			    sqlite3_free (name);
			    if (attr_name)
				sqlite3_free (attr_name);
			    return;
			}
		      if (stmt_ext != NULL)
			{
			    /* inserting all Extra Attributes */
			    sqlite3_int64 feature_id =
				sqlite3_last_insert_rowid (handle);
			    dxfExtraAttrPtr ext = pg->first;
			    while (ext != NULL)
			      {
				  sqlite3_reset (stmt_ext);
				  sqlite3_clear_bindings (stmt_ext);
				  sqlite3_bind_int64 (stmt_ext, 1, feature_id);
				  sqlite3_bind_text (stmt_ext, 2, ext->key,
						     strlen (ext->key),
						     SQLITE_STATIC);
				  sqlite3_bind_text (stmt_ext, 3, ext->value,
						     strlen (ext->value),
						     SQLITE_STATIC);
				  ret = sqlite3_step (stmt_ext);
				  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
				      ;
				  else
				    {
					fprintf (stderr,
						 "INSERT %s error: %s\n",
						 attr_name,
						 sqlite3_errmsg (handle));
					sqlite3_finalize (stmt);
					sqlite3_finalize (stmt_ext);
					sqlite3_free (name);
					if (attr_name)
					    sqlite3_free (attr_name);
					ret =
					    sqlite3_exec (handle, "ROLLBACK",
							  NULL, NULL, NULL);
					return;
				    }
				  ext = ext->next;
			      }
			}
		      pg = pg->next;
		  }
		sqlite3_finalize (stmt);
		if (stmt_ext != NULL)
		    sqlite3_finalize (stmt_ext);
		ret = sqlite3_exec (handle, "COMMIT", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
		  {
		      fprintf (stderr, "COMMIT %s error: %s\n", name,
			       sqlite3_errmsg (handle));
		      sqlite3_free (name);
		      if (attr_name)
			  sqlite3_free (attr_name);
		      return;
		  }
		sqlite3_free (name);
		if (attr_name)
		    sqlite3_free (attr_name);
	    }
	  lyr = lyr->next;
      }
}

static void
spatialite_autocreate (sqlite3 * db)
{
/* attempting to perform self-initialization for a newly created DB */
    int ret;
    char sql[1024];
    char *err_msg = NULL;
    int count;
    int i;
    char **results;
    int rows;
    int columns;

/* checking if this DB is really empty */
    strcpy (sql, "SELECT Count(*) from sqlite_master");
    ret = sqlite3_get_table (db, sql, &results, &rows, &columns, NULL);
    if (ret != SQLITE_OK)
	return;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	      count = atoi (results[(i * columns) + 0]);
      }
    sqlite3_free_table (results);

    if (count > 0)
	return;

/* all right, it's empty: proceding to initialize */
    strcpy (sql, "SELECT InitSpatialMetadata(1)");
    ret = sqlite3_exec (db, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return;
      }
}

static void
do_help ()
{
/* printing the argument list */
    fprintf (stderr, "\n\nusage: spatialite_load_dxf ARGLIST\n");
    fprintf (stderr,
	     "==============================================================\n");
    fprintf (stderr,
	     "-h or --help                    print this help message\n");
    fprintf (stderr,
	     "-d or --db-path  pathname       the SpatiaLite DB path\n");
    fprintf (stderr, "-x or --dxf-path pathname       the input DXF path\n\n");
    fprintf (stderr, "you can specify the following options as well:\n");
    fprintf (stderr, "----------------------------------------------\n");
    fprintf (stderr,
	     "-s or --srid       num          an explicit SRID value\n");
    fprintf (stderr,
	     "-p or --prefix  layer_prefix    prefix for DB layer names\n");
    fprintf (stderr,
	     "-l or --layer   layer_name      will import a single DXF layer\n");
    fprintf (stderr,
	     "-all or --all-layers            will import all layers (default)\n\n");
    fprintf (stderr,
	     "-distinct or --distinct-layers  respecting individual DXF layers\n");
    fprintf (stderr,
	     "-mixed or --mixed-layers        merging layers altogether by type\n");
    fprintf (stderr,
	     "                                distinct|mixed are mutually\n");
    fprintf (stderr,
	     "                                exclusive; by default: distinct\n\n");
    fprintf (stderr,
	     "-auto or --auto_2d_3d           2D/3D based on input geometries\n");
    fprintf (stderr,
	     "-2d or --force_2d               unconditionally force 2D\n");
    fprintf (stderr,
	     "-3d or --force_3d               unconditionally force 3D\n");
    fprintf (stderr,
	     "                                auto|2d|3d are mutually exclusive\n");
    fprintf (stderr, "                                by default: auto\n\n");
    fprintf (stderr,
	     "-linked or --linked-rings      support linked polygon rings\n");
    fprintf (stderr,
	     "-unlinked or --unlinked-rings  support unlinked polygon rings\n");
    fprintf (stderr,
	     "                               linked|unlinked are mutually exclusive\n");
    fprintf (stderr, "                                by default: none\n\n");
    fprintf (stderr, "--------------------------\n");
    fprintf (stderr,
	     "-m or --in-memory               using IN-MEMORY database\n");
    fprintf (stderr,
	     "-jo or --journal-off            unsafe [but faster] mode\n");
}

int
main (int argc, char *argv[])
{
/* main DXF importer */
    sqlite3 *handle = NULL;
    int ret;
    int i;
    int next_arg = ARG_NONE;
    int error = 0;
    int in_memory = 0;
    int cache_size = 0;
    int journal_off = 0;
    char *db_path = NULL;
    char *dxf_path = NULL;
    char *selected_layer = NULL;
    char *prefix = NULL;
    int linked_rings = 0;
    int unlinked_rings = 0;
    int mode = IMPORT_BY_LAYER;
    int force_dims = AUTO_2D_3D;
    int srid = -1;
    dxfParserPtr dxf = NULL;
    void *cache = spatialite_alloc_connection ();

    for (i = 1; i < argc; i++)
      {
	  /* parsing the invocation arguments */
	  if (next_arg != ARG_NONE)
	    {
		switch (next_arg)
		  {
		  case ARG_DB_PATH:
		      db_path = argv[i];
		      break;
		  case ARG_DXF_PATH:
		      dxf_path = argv[i];
		      break;
		  case ARG_LAYER:
		      selected_layer = argv[i];
		      break;
		  case ARG_PREFIX:
		      prefix = argv[i];
		      break;
		  case ARG_SRID:
		      srid = atoi (argv[i]);
		      break;
		  case ARG_CACHE_SIZE:
		      cache_size = atoi (argv[i]);
		  };
		next_arg = ARG_NONE;
		continue;
	    }
	  if (strcasecmp (argv[i], "--help") == 0
	      || strcmp (argv[i], "-h") == 0)
	    {
		do_help ();
		return -1;
	    }
	  if (strcasecmp (argv[i], "--db-path") == 0)
	    {
		next_arg = ARG_DB_PATH;
		continue;
	    }
	  if (strcmp (argv[i], "-d") == 0)
	    {
		next_arg = ARG_DB_PATH;
		continue;
	    }
	  if (strcasecmp (argv[i], "--dxf-path") == 0)
	    {
		next_arg = ARG_DXF_PATH;
		continue;
	    }
	  if (strcmp (argv[i], "-x") == 0)
	    {
		next_arg = ARG_DXF_PATH;
		continue;
	    }
	  if (strcasecmp (argv[i], "--layer") == 0)
	    {
		next_arg = ARG_LAYER;
		continue;
	    }
	  if (strcmp (argv[i], "-l") == 0)
	    {
		next_arg = ARG_LAYER;
		continue;
	    }
	  if (strcasecmp (argv[i], "--prefix") == 0)
	    {
		next_arg = ARG_PREFIX;
		continue;
	    }
	  if (strcmp (argv[i], "-p") == 0)
	    {
		next_arg = ARG_PREFIX;
		continue;
	    }
	  if (strcasecmp (argv[i], "--srid") == 0)
	    {
		next_arg = ARG_SRID;
		continue;
	    }
	  if (strcmp (argv[i], "-s") == 0)
	    {
		next_arg = ARG_SRID;
		continue;
	    }
	  if (strcasecmp (argv[i], "--all-layers") == 0)
	    {
		selected_layer = NULL;
		continue;
	    }
	  if (strcasecmp (argv[i], "-all") == 0)
	    {
		selected_layer = NULL;
		continue;
	    }
	  if (strcasecmp (argv[i], "--mixed-layers") == 0)
	    {
		mode = IMPORT_MIXED;
		continue;
	    }
	  if (strcasecmp (argv[i], "-mixed") == 0)
	    {
		mode = IMPORT_MIXED;
		continue;
	    }
	  if (strcasecmp (argv[i], "--distinct-layers") == 0)
	    {
		mode = IMPORT_BY_LAYER;
		continue;
	    }
	  if (strcasecmp (argv[i], "-distinct") == 0)
	    {
		mode = IMPORT_BY_LAYER;
		continue;
	    }
	  if (strcasecmp (argv[i], "--auto-2d-3d") == 0)
	    {
		force_dims = AUTO_2D_3D;
		continue;
	    }
	  if (strcasecmp (argv[i], "-auto") == 0)
	    {
		force_dims = AUTO_2D_3D;
		continue;
	    }
	  if (strcasecmp (argv[i], "--force-2d") == 0)
	    {
		force_dims = FORCE_2D;
		continue;
	    }
	  if (strcasecmp (argv[i], "-2d") == 0)
	    {
		force_dims = FORCE_2D;
		continue;
	    }
	  if (strcasecmp (argv[i], "--force-3d") == 0)
	    {
		force_dims = FORCE_3D;
		continue;
	    }
	  if (strcasecmp (argv[i], "-3d") == 0)
	    {
		force_dims = FORCE_3D;
		continue;
	    }
	  if (strcasecmp (argv[i], "--linked-rings") == 0
	      || strcmp (argv[i], "-linked") == 0)
	    {
		linked_rings = 1;
		unlinked_rings = 0;
		continue;
	    }
	  if (strcasecmp (argv[i], "--unlinked-rings") == 0
	      || strcmp (argv[i], "-unlinked") == 0)
	    {
		linked_rings = 0;
		unlinked_rings = 1;
		continue;
	    }
	  if (strcasecmp (argv[i], "-m") == 0)
	    {
		in_memory = 1;
		next_arg = ARG_NONE;
		continue;
	    }
	  if (strcasecmp (argv[i], "--in-memory") == 0)
	    {
		in_memory = 1;
		next_arg = ARG_NONE;
		continue;
	    }
	  if (strcasecmp (argv[i], "-jo") == 0)
	    {
		journal_off = 1;
		next_arg = ARG_NONE;
		continue;
	    }
	  if (strcasecmp (argv[i], "--journal-off") == 0)
	    {
		journal_off = 1;
		next_arg = ARG_NONE;
		continue;
	    }
	  if (strcasecmp (argv[i], "--cache-size") == 0
	      || strcmp (argv[i], "-cs") == 0)
	    {
		next_arg = ARG_CACHE_SIZE;
		continue;
	    }
	  fprintf (stderr, "unknown argument: %s\n", argv[i]);
	  error = 1;
      }
    if (error)
      {
	  do_help ();
	  return -1;
      }
/* checking the arguments */
    if (!db_path)
      {
	  fprintf (stderr, "did you forget setting the --db-path argument ?\n");
	  error = 1;
      }
    if (!dxf_path)
      {
	  fprintf (stderr,
		   "did you forget setting the --dxf-path argument ?\n");
	  error = 1;
      }
    if (error)
      {
	  do_help ();
	  return -1;
      }

/* creating the target DB */
    if (in_memory)
	cache_size = 0;
    ret =
	sqlite3_open_v2 (db_path, &handle,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "cannot open DB: %s\n", sqlite3_errmsg (handle));
	  sqlite3_close (handle);
	  goto stop;
      }
    if (cache_size > 0)
      {
	  /* setting the CACHE-SIZE */
	  char *sql = sqlite3_mprintf ("PRAGMA cache_size=%d", cache_size);
	  sqlite3_exec (handle, sql, NULL, NULL, NULL);
	  sqlite3_free (sql);
      }
    if (journal_off)
      {
	  /* disabling the journal: unsafe but faster */
	  sqlite3_exec (handle, "PRAGMA journal_mode = OFF", NULL, NULL, NULL);
      }

    if (in_memory)
      {
	  /* loading the DB in-memory */
	  sqlite3 *mem_handle;
	  sqlite3_backup *backup;
	  int ret;
	  ret =
	      sqlite3_open_v2 (":memory:", &mem_handle,
			       SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
			       NULL);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "cannot open 'MEMORY-DB': %s\n",
			 sqlite3_errmsg (mem_handle));
		sqlite3_close (handle);
		sqlite3_close (mem_handle);
		return -1;
	    }
	  backup = sqlite3_backup_init (mem_handle, "main", handle, "main");
	  if (!backup)
	    {
		fprintf (stderr, "cannot load 'MEMORY-DB'\n");
		sqlite3_close (handle);
		sqlite3_close (mem_handle);
		return -1;
	    }
	  while (1)
	    {
		ret = sqlite3_backup_step (backup, 1024);
		if (ret == SQLITE_DONE)
		    break;
	    }
	  ret = sqlite3_backup_finish (backup);
	  sqlite3_close (handle);
	  handle = mem_handle;
	  spatialite_init_ex (handle, cache, 0);
	  printf ("\nusing IN-MEMORY database\n");
      }
    else
	spatialite_init_ex (handle, cache, 0);
    spatialite_autocreate (handle);

/* attempting to parse the DXF file */
    dxf =
	alloc_dxf_parser (srid, force_dims, prefix, selected_layer,
			  linked_rings, unlinked_rings);
    if (!parse_dxf_file (dxf, dxf_path))
      {
	  fprintf (stderr, "Unable to parse \"%s\"\n", dxf_path);
	  goto stop;
      }

/* populating the target DB */
    if (mode == IMPORT_MIXED)
	import_mixed (handle, dxf);
    else
	import_by_layer (handle, dxf);

    if (in_memory)
      {
	  /* exporting the in-memory DB to filesystem */
	  sqlite3 *disk_handle;
	  sqlite3_backup *backup;
	  int ret;
	  printf ("\nexporting IN_MEMORY database ... wait please ...\n");
	  ret =
	      sqlite3_open_v2 (db_path, &disk_handle,
			       SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
			       NULL);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "cannot open '%s': %s\n", db_path,
			 sqlite3_errmsg (disk_handle));
		sqlite3_close (disk_handle);
		return -1;
	    }
	  backup = sqlite3_backup_init (disk_handle, "main", handle, "main");
	  if (!backup)
	    {
		fprintf (stderr, "Backup failure: 'MEMORY-DB' wasn't saved\n");
		sqlite3_close (handle);
		sqlite3_close (disk_handle);
		return -1;
	    }
	  while (1)
	    {
		ret = sqlite3_backup_step (backup, 1024);
		if (ret == SQLITE_DONE)
		    break;
	    }
	  ret = sqlite3_backup_finish (backup);
	  sqlite3_close (handle);
	  handle = disk_handle;
	  printf ("\tIN_MEMORY database succesfully exported\n");
      }

/* memory cleanup */
  stop:
    if (handle != NULL)
      {
	  /* closing the DB connection */
	  ret = sqlite3_close (handle);
	  if (ret != SQLITE_OK)
	      fprintf (stderr, "sqlite3_close() error: %s\n",
		       sqlite3_errmsg (handle));
      }
    destroy_dxf_parser (dxf);
    spatialite_cleanup_ex (cache);
    return 0;
}
