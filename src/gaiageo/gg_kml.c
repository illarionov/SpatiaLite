/*

 gg_kml.c -- KML parser/lexer 
  
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

#if defined(_WIN32) || defined(WIN32)
# include <io.h>
#define isatty  _isatty
#define fileno  _fileno
#endif

int kml_parse_error;

#define KML_PARSER_OPEN_NODE		1
#define KML_PARSER_SELF_CLOSED_NODE	2
#define KML_PARSER_CLOSED_NODE		3

#define GAIA_KML_UNKNOWN		0
#define GAIA_KML_POINT			1
#define GAIA_KML_LINESTRING		2
#define GAIA_KML_POLYGON		3
#define GAIA_KML_MULTIGEOMETRY		4

/*
** This is a linked-list struct to store all the values for each token.
*/
typedef struct kmlFlexTokenStruct
{
    char *value;
    struct kmlFlexTokenStruct *Next;
} kmlFlexToken;

typedef struct kml_coord
{
    char *Value;
    struct kml_coord *Next;
} kmlCoord;
typedef kmlCoord *kmlCoordPtr;

typedef struct kml_attr
{
    char *Key;
    char *Value;
    struct kml_attr *Next;
} kmlAttr;
typedef kmlAttr *kmlAttrPtr;

typedef struct kml_node
{
    char *Tag;
    int Type;
    int Error;
    struct kml_attr *Attributes;
    struct kml_coord *Coordinates;
    struct kml_node *Next;
} kmlNode;
typedef kmlNode *kmlNodePtr;

typedef struct kml_dynamic_ring
{
    gaiaDynamicLinePtr ring;
    int interior;
    int has_z;
    struct kml_dynamic_ring *next;
} kmlDynamicRing;
typedef kmlDynamicRing *kmlDynamicRingPtr;

typedef struct kml_dynamic_polygon
{
    struct kml_dynamic_ring *first;
    struct kml_dynamic_ring *last;
} kmlDynamicPolygon;
typedef kmlDynamicPolygon *kmlDynamicPolygonPtr;

static kmlDynamicPolygonPtr
kml_alloc_dyn_polygon (void)
{
/* creating a dynamic polygon (ring collection) */
    kmlDynamicPolygonPtr p = malloc (sizeof (kmlDynamicPolygon));
    p->first = NULL;
    p->last = NULL;
    return p;
}

static void
kml_free_dyn_polygon (kmlDynamicPolygonPtr dyn)
{
/* deleting a dynamic polygon (ring collection) */
    kmlDynamicRingPtr r;
    kmlDynamicRingPtr rn;
    if (!dyn)
	return;
    r = dyn->first;
    while (r)
      {
	  rn = r->next;
	  if (r->ring)
	      gaiaFreeDynamicLine (r->ring);
	  free (r);
	  r = rn;
      }
    free (dyn);
}

static void
kml_add_polygon_ring (kmlDynamicPolygonPtr dyn_pg, gaiaDynamicLinePtr dyn,
		      int interior, int has_z)
{
/* inserting a further ring into the collection (dynamic polygon) */
    kmlDynamicRingPtr p = malloc (sizeof (kmlDynamicRing));
    p->ring = dyn;
    p->interior = interior;
    p->has_z = has_z;
    p->next = NULL;
    if (dyn_pg->first == NULL)
	dyn_pg->first = p;
    if (dyn_pg->last != NULL)
	dyn_pg->last->next = p;
    dyn_pg->last = p;
}

static void
kml_freeString (char **ptr)
{
/* releasing a string from the lexer */
    if (*ptr != NULL)
	free (*ptr);
    *ptr = NULL;
}

static void
kml_saveString (char **ptr, const char *str)
{
/* saving a string from the lexer */
    int len = strlen (str);
    kml_freeString (ptr);
    *ptr = malloc (len + 1);
    strcpy (*ptr, str);
}

static kmlCoordPtr
kml_coord (void *value)
{
/* creating a coord Item */
    int len;
    kmlFlexToken *tok = (kmlFlexToken *) value;
    kmlCoordPtr c = malloc (sizeof (kmlCoord));
    len = strlen (tok->value);
    c->Value = malloc (len + 1);
    strcpy (c->Value, tok->value);
    c->Next = NULL;
    return c;
}

static void
kml_freeCoordinate (kmlCoordPtr c)
{
/* deleting a KML coordinate */
    if (c == NULL)
	return;
    if (c->Value)
	free (c->Value);
    free (c);
}

static kmlAttrPtr
kml_attribute (void *key, void *value)
{
/* creating an attribute */
    int len;
    kmlFlexToken *k_tok = (kmlFlexToken *) key;
    kmlFlexToken *v_tok = (kmlFlexToken *) value;
    kmlAttrPtr a = malloc (sizeof (kmlAttr));
    len = strlen (k_tok->value);
    a->Key = malloc (len + 1);
    strcpy (a->Key, k_tok->value);
    len = strlen (v_tok->value);
/* we need to de-quote the string, removing first and last ".." */
    if (*(v_tok->value + 0) == '"' && *(v_tok->value + len - 1) == '"')
      {
	  a->Value = malloc (len - 1);
	  memcpy (a->Value, v_tok->value + 1, len - 1);
	  *(a->Value + len - 1) = '\0';
      }
    else
      {
	  a->Value = malloc (len + 1);
	  strcpy (a->Value, v_tok->value);
      }
    a->Next = NULL;
    return a;
}

static void
kml_freeAttribute (kmlAttrPtr a)
{
/* deleting a KML attribute */
    if (a == NULL)
	return;
    if (a->Key)
	free (a->Key);
    if (a->Value)
	free (a->Value);
    free (a);
}

static void
kml_freeNode (kmlNodePtr n)
{
/* deleting a KML node */
    kmlAttrPtr a;
    kmlAttrPtr an;
    kmlCoordPtr c;
    kmlCoordPtr cn;
    if (n == NULL)
	return;
    a = n->Attributes;
    while (a)
      {
	  an = a->Next;
	  kml_freeAttribute (a);
	  a = an;
      }
    c = n->Coordinates;
    while (c)
      {
	  cn = c->Next;
	  kml_freeCoordinate (c);
	  c = cn;
      }
    if (n->Tag)
	free (n->Tag);
    free (n);
}

static void
kml_freeTree (kmlNodePtr t)
{
/* deleting a KML tree */
    kmlNodePtr n;
    kmlNodePtr nn;
    n = t;
    while (n)
      {
	  nn = n->Next;
	  kml_freeNode (n);
	  n = nn;
      }
}

static kmlNodePtr
kml_createNode (void *tag, void *attributes, void *coords)
{
/* creating a node */
    int len;
    kmlFlexToken *tok = (kmlFlexToken *) tag;
    kmlNodePtr n = malloc (sizeof (kmlNode));
    len = strlen (tok->value);
    n->Tag = malloc (len + 1);
    strcpy (n->Tag, tok->value);
    n->Type = KML_PARSER_OPEN_NODE;
    n->Error = 0;
    n->Attributes = attributes;
    n->Coordinates = coords;
    n->Next = NULL;
    return n;
}

static kmlNodePtr
kml_createSelfClosedNode (void *tag, void *attributes)
{
/* creating a self-closed node */
    int len;
    kmlFlexToken *tok = (kmlFlexToken *) tag;
    kmlNodePtr n = malloc (sizeof (kmlNode));
    len = strlen (tok->value);
    n->Tag = malloc (len + 1);
    strcpy (n->Tag, tok->value);
    n->Type = KML_PARSER_SELF_CLOSED_NODE;
    n->Error = 0;
    n->Attributes = attributes;
    n->Coordinates = NULL;
    n->Next = NULL;
    return n;
}

static kmlNodePtr
kml_closingNode (void *tag)
{
/* creating a closing node */
    int len;
    kmlFlexToken *tok = (kmlFlexToken *) tag;
    kmlNodePtr n = malloc (sizeof (kmlNode));
    len = strlen (tok->value);
    n->Tag = malloc (len + 1);
    strcpy (n->Tag, tok->value);
    n->Type = KML_PARSER_CLOSED_NODE;
    n->Error = 0;
    n->Attributes = NULL;
    n->Coordinates = NULL;
    n->Next = NULL;
    return n;
}

static int
kml_cleanup (kmlFlexToken * token)
{
    kmlFlexToken *ptok;
    kmlFlexToken *ptok_n;
    if (token == NULL)
	return 0;
    ptok = token;
    while (ptok)
      {
	  ptok_n = ptok->Next;
	  if (ptok->value != NULL)
	      free (ptok->value);
	  free (ptok);
	  ptok = ptok_n;
      }
    return 0;
}

static void
kml_xferString (char **p, const char *str)
{
/* saving some token */
    int len;
    if (str == NULL)
      {
	  *p = NULL;
	  return;
      }
    len = strlen (str);
    *p = malloc (len + 1);
    strcpy (*p, str);
}

static int
guessKmlGeometryType (kmlNodePtr node)
{
/* attempting to guess the Geometry Type for a KML node */
    int type = GAIA_KML_UNKNOWN;
    if (strcmp (node->Tag, "Point") == 0)
	type = GAIA_KML_POINT;
    if (strcmp (node->Tag, "LineString") == 0)
	type = GAIA_KML_LINESTRING;
    if (strcmp (node->Tag, "Polygon") == 0)
	type = GAIA_KML_POLYGON;
    if (strcmp (node->Tag, "MultiGeometry") == 0)
	type = GAIA_KML_MULTIGEOMETRY;
    return type;
}

static int
kml_check_coord (const char *value)
{
/* checking a KML coordinate */
    int decimal = 0;
    const char *p = value;
    if (*p == '+' || *p == '-')
	p++;
    while (*p != '\0')
      {
	  if (*p == '.')
	    {
		if (!decimal)
		    decimal = 1;
		else
		    return 0;
	    }
	  else if (*p >= '0' && *p <= '9')
	      ;
	  else
	      return 0;
	  p++;
      }
    return 1;
}

static int
kml_extract_coords (const char *value, double *x, double *y, double *z,
		    int *count)
{
/* extracting KML coords from a comma-separated string */
    const char *in = value;
    char buf[1024];
    char *out = buf;
    *out = '\0';

    while (*in != '\0')
      {
	  if (*in == ',')
	    {
		*out = '\0';
		if (*buf != '\0')
		  {
		      if (!kml_check_coord (buf))
			  return 0;
		      switch (*count)
			{
			case 0:
			    *x = atof (buf);
			    *count += 1;
			    break;
			case 1:
			    *y = atof (buf);
			    *count += 1;
			    break;
			case 2:
			    *z = atof (buf);
			    *count += 1;
			    break;
			default:
			    *count += 1;
			    break;
			};
		  }
		in++;
		out = buf;
		*out = '\0';
		continue;
	    }
	  *out++ = *in++;
      }
    *out = '\0';
/* parsing the last item */
    if (*buf != '\0')
      {
	  if (!kml_check_coord (buf))
	      return 0;
	  switch (*count)
	    {
	    case 0:
		*x = atof (buf);
		*count += 1;
		break;
	    case 1:
		*y = atof (buf);
		*count += 1;
		break;
	    case 2:
		*z = atof (buf);
		*count += 1;
		break;
	    default:
		*count += 1;
		break;
	    };
      }
    return 1;
}

static int
kml_parse_point_v2 (kmlCoordPtr coord, double *x, double *y, double *z,
		    int *has_z)
{
/* parsing KML <coordinates> [Point] */
    int count = 0;
    kmlCoordPtr c = coord;
    while (c)
      {
	  if (!kml_extract_coords (c->Value, x, y, z, &count))
	      return 0;
	  c = c->Next;
      }
    if (count == 2)
      {
	  *has_z = 0;
	  return 1;
      }
    if (count == 3)
      {
	  *has_z = 1;
	  return 1;
      }
    return 0;
}

static int
kml_parse_point (gaiaGeomCollPtr geom, kmlNodePtr node, kmlNodePtr * next)
{
/* parsing a <Point> */
    double x;
    double y;
    double z;
    int has_z;
    gaiaGeomCollPtr pt;
    gaiaGeomCollPtr last;

    if (strcmp (node->Tag, "coordinates") == 0)
      {
	  /* parsing a KML <Point> */
	  if (!kml_parse_point_v2 (node->Coordinates, &x, &y, &z, &has_z))
	      return 0;
	  node = node->Next;
	  if (node == NULL)
	      return 0;
	  if (strcmp (node->Tag, "coordinates") == 0)
	      ;
	  else
	      return 0;
	  node = node->Next;
	  if (node == NULL)
	      return 0;
	  if (strcmp (node->Tag, "Point") == 0)
	      ;
	  else
	      return 0;
	  *next = node->Next;
	  goto ok;
      }
    return 0;

  ok:
/* ok, KML nodes match as expected */
    if (has_z)
      {
	  pt = gaiaAllocGeomCollXYZ ();
	  gaiaAddPointToGeomCollXYZ (pt, x, y, z);
      }
    else
      {
	  pt = gaiaAllocGeomColl ();
	  gaiaAddPointToGeomColl (pt, x, y);
      }
    last = geom;
    while (1)
      {
	  /* searching the last Geometry within chain */
	  if (last->Next == NULL)
	      break;
	  last = last->Next;
      }
    last->Next = pt;
    return 1;
}

static int
kml_extract_multi_coord (const char *value, double *x, double *y, double *z,
			 int *count, int *follow)
{
/* extracting KML coords from a comma-separated string */
    const char *in = value;
    char buf[1024];
    char *out = buf;
    int last;
    *out = '\0';
    while (*in != '\0')
      {
	  last = *in;
	  if (*in == ',')
	    {
		*out = '\0';
		if (*buf != '\0')
		  {
		      if (!kml_check_coord (buf))
			  return 0;
		      switch (*count)
			{
			case 0:
			    *x = atof (buf);
			    *count += 1;
			    break;
			case 1:
			    *y = atof (buf);
			    *count += 1;
			    break;
			case 2:
			    *z = atof (buf);
			    *count += 1;
			    break;
			default:
			    *count += 1;
			    break;
			};
		  }
		in++;
		out = buf;
		*out = '\0';
		continue;
	    }
	  *out++ = *in++;
      }
    *out = '\0';
/* parsing the last item */
    if (*buf != '\0')
      {
	  if (!kml_check_coord (buf))
	      return 0;
	  switch (*count)
	    {
	    case 0:
		*x = atof (buf);
		*count += 1;
		break;
	    case 1:
		*y = atof (buf);
		*count += 1;
		break;
	    case 2:
		*z = atof (buf);
		*count += 1;
		break;
	    default:
		*count += 1;
		break;
	    };
      }
    if (last == ',')
	*follow = 1;
    else
	*follow = 0;
    return 1;
}

static int
kml_extract_multi_coords (kmlCoordPtr coord, double *x, double *y, double *z,
			  int *count, kmlCoordPtr * next)
{
/* extracting KML coords from a comma-separated string */
    int follow;
    kmlCoordPtr c = coord;
    while (c)
      {
	  if (!kml_extract_multi_coord (c->Value, x, y, z, count, &follow))
	      return 0;
	  if (!follow && c->Next != NULL)
	    {
		if (*(c->Next->Value) == ',')
		    follow = 1;
	    }
	  if (follow)
	      c = c->Next;
	  else
	    {
		*next = c->Next;
		break;
	    }
      }
    return 1;
}

static void
kml_add_point_to_line (gaiaDynamicLinePtr dyn, double x, double y)
{
/* appending a point */
    gaiaAppendPointToDynamicLine (dyn, x, y);
}

static void
kml_add_point_to_lineZ (gaiaDynamicLinePtr dyn, double x, double y, double z)
{
/* appending a point */
    gaiaAppendPointZToDynamicLine (dyn, x, y, z);
}

static int
kml_parse_coordinates (kmlCoordPtr coord, gaiaDynamicLinePtr dyn, int *has_z)
{
/* parsing KML <coordinates> [Linestring or Ring] */
    int count = 0;
    double x;
    double y;
    double z;
    kmlCoordPtr next;
    kmlCoordPtr c = coord;
    while (c)
      {
	  if (!kml_extract_multi_coords (c, &x, &y, &z, &count, &next))
	      return 0;
	  if (count == 2)
	    {
		*has_z = 0;
		kml_add_point_to_line (dyn, x, y);
		count = 0;
	    }
	  else if (count == 3)
	    {
		kml_add_point_to_lineZ (dyn, x, y, z);
		count = 0;
	    }
	  else
	      return 0;
	  c = next;
      }
    return 1;
}

static int
kml_count_dyn_points (gaiaDynamicLinePtr dyn)
{
/* count how many vertices are into sone linestring/ring */
    int iv = 0;
    gaiaPointPtr pt = dyn->First;
    while (pt)
      {
	  iv++;
	  pt = pt->Next;
      }
    return iv;
}

static int
kml_parse_linestring (gaiaGeomCollPtr geom, kmlNodePtr node, kmlNodePtr * next)
{
/* parsing a <LineString> */
    gaiaGeomCollPtr ln;
    gaiaGeomCollPtr last;
    gaiaLinestringPtr new_ln;
    gaiaPointPtr pt;
    gaiaDynamicLinePtr dyn = gaiaAllocDynamicLine ();
    int iv;
    int has_z = 1;
    int points = 0;

    if (strcmp (node->Tag, "coordinates") == 0)
      {
	  /* parsing a KML <LineString> */
	  if (!kml_parse_coordinates (node->Coordinates, dyn, &has_z))
	      goto error;
	  node = node->Next;
	  if (node == NULL)
	      goto error;
	  if (strcmp (node->Tag, "coordinates") == 0)
	      ;
	  else
	      goto error;
	  node = node->Next;
	  if (node == NULL)
	      goto error;
	  if (strcmp (node->Tag, "LineString") == 0)
	      ;
	  else
	      goto error;
	  *next = node->Next;
      }

/* ok, KML nodes match as expected */
    points = kml_count_dyn_points (dyn);
    if (points < 2)
	goto error;
    if (has_z)
      {
	  ln = gaiaAllocGeomCollXYZ ();
	  new_ln = gaiaAddLinestringToGeomColl (ln, points);
	  pt = dyn->First;
	  iv = 0;
	  while (pt)
	    {
		gaiaSetPointXYZ (new_ln->Coords, iv, pt->X, pt->Y, pt->Z);
		iv++;
		pt = pt->Next;
	    }
      }
    else
      {
	  ln = gaiaAllocGeomColl ();
	  new_ln = gaiaAddLinestringToGeomColl (ln, points);
	  pt = dyn->First;
	  iv = 0;
	  while (pt)
	    {
		gaiaSetPoint (new_ln->Coords, iv, pt->X, pt->Y);
		iv++;
		pt = pt->Next;
	    }
      }
    last = geom;
    while (1)
      {
	  /* searching the last Geometry within chain */
	  if (last->Next == NULL)
	      break;
	  last = last->Next;
      }
    last->Next = ln;
    gaiaFreeDynamicLine (dyn);
    return 1;

  error:
    gaiaFreeDynamicLine (dyn);
    return 0;
}

static gaiaDynamicLinePtr
kml_parse_ring (kmlNodePtr node, int *interior, int *has_z, kmlNodePtr * next)
{
/* parsing a generic KML ring */
    gaiaDynamicLinePtr dyn = gaiaAllocDynamicLine ();
    *has_z = 1;

    if (strcmp (node->Tag, "outerBoundaryIs") == 0)
      {
	  /* parsing a KML <outerBoundaryIs> */
	  node = node->Next;
	  if (node == NULL)
	      goto error;
	  if (strcmp (node->Tag, "LinearRing") == 0)
	      ;
	  else
	      goto error;
	  node = node->Next;
	  if (node == NULL)
	      goto error;
	  if (strcmp (node->Tag, "coordinates") == 0)
	    {
		/* parsing a KML <kml:coordinates> */
		if (!kml_parse_coordinates (node->Coordinates, dyn, has_z))
		    goto error;
		node = node->Next;
		if (node == NULL)
		    goto error;
		if (strcmp (node->Tag, "coordinates") == 0)
		    ;
		else
		    goto error;
	    }
	  else
	      goto error;
	  node = node->Next;
	  if (node == NULL)
	      goto error;
	  if (strcmp (node->Tag, "LinearRing") == 0)
	      ;
	  else
	      goto error;
	  node = node->Next;
	  if (node == NULL)
	      goto error;
	  if (strcmp (node->Tag, "outerBoundaryIs") == 0)
	      ;
	  else
	      goto error;
	  *interior = 0;
	  *next = node->Next;
	  return dyn;
      }
    if (strcmp (node->Tag, "innerBoundaryIs") == 0)
      {
	  /* parsing a KML <innerBoundaryIs> */
	  node = node->Next;
	  if (node == NULL)
	      goto error;
	  if (strcmp (node->Tag, "LinearRing") == 0)
	      ;
	  else
	      goto error;
	  node = node->Next;
	  if (node == NULL)
	      goto error;
	  if (strcmp (node->Tag, "coordinates") == 0)
	    {
		/* parsing a KML <coordinates> */
		if (!kml_parse_coordinates (node->Coordinates, dyn, has_z))
		    goto error;
		node = node->Next;
		if (node == NULL)
		    goto error;
		if (strcmp (node->Tag, "coordinates") == 0)
		    ;
		else
		    goto error;
	    }
	  else
	      goto error;
	  node = node->Next;
	  if (node == NULL)
	      goto error;
	  if (strcmp (node->Tag, "LinearRing") == 0)
	      ;
	  else
	      goto error;
	  node = node->Next;
	  if (node == NULL)
	      goto error;
	  if (strcmp (node->Tag, "innerBoundaryIs") == 0)
	      ;
	  else
	      goto error;
	  *interior = 1;
	  *next = node->Next;
	  return dyn;
      }

  error:
    gaiaFreeDynamicLine (dyn);
    return 0;
}

static int
kml_parse_polygon (gaiaGeomCollPtr geom, kmlNodePtr node, kmlNodePtr * next_n)
{
/* parsing a <Polygon> */
    int interior;
    int has_z;
    int inners;
    int outers;
    int points;
    int iv;
    int ib = 0;
    gaiaGeomCollPtr pg;
    gaiaGeomCollPtr last_g;
    gaiaPolygonPtr new_pg;
    gaiaRingPtr ring;
    gaiaDynamicLinePtr dyn;
    gaiaPointPtr pt;
    gaiaDynamicLinePtr exterior_ring;
    kmlNodePtr next;
    kmlDynamicRingPtr dyn_rng;
    kmlDynamicPolygonPtr dyn_pg = kml_alloc_dyn_polygon ();
    kmlNodePtr n = node;
    while (n)
      {
	  /* looping on rings */
	  if (strcmp (n->Tag, "Polygon") == 0)
	    {
		*next_n = n->Next;
		break;
	    }
	  dyn = kml_parse_ring (n, &interior, &has_z, &next);
	  if (dyn == NULL)
	      goto error;
	  if (kml_count_dyn_points (dyn) < 4)
	    {
		/* cannot be a valid ring */
		goto error;
	    }
	  /* checking if the ring is closed */
	  if (has_z)
	    {
		if (dyn->First->X == dyn->Last->X
		    && dyn->First->Y == dyn->Last->Y
		    && dyn->First->Z == dyn->Last->Z)
		    ;
		else
		    goto error;
	    }
	  else
	    {
		if (dyn->First->X == dyn->Last->X
		    && dyn->First->Y == dyn->Last->Y)
		    ;
		else
		    goto error;
	    }
	  kml_add_polygon_ring (dyn_pg, dyn, interior, has_z);
	  n = next;
      }
/* ok, KML nodes match as expected */
    inners = 0;
    outers = 0;
    has_z = 1;
    dyn_rng = dyn_pg->first;
    while (dyn_rng)
      {
	  /* verifying the rings collection */
	  if (dyn_rng->has_z == 0)
	      has_z = 0;
	  if (dyn_rng->interior)
	      inners++;
	  else
	    {
		outers++;
		points = kml_count_dyn_points (dyn_rng->ring);
		exterior_ring = dyn_rng->ring;
	    }
	  dyn_rng = dyn_rng->next;
      }
    if (outers != 1)		/* no exterior ring declared */
	goto error;

    if (has_z)
      {
	  pg = gaiaAllocGeomCollXYZ ();
	  new_pg = gaiaAddPolygonToGeomColl (pg, points, inners);
	  /* initializing the EXTERIOR RING */
	  ring = new_pg->Exterior;
	  pt = exterior_ring->First;
	  iv = 0;
	  while (pt)
	    {
		gaiaSetPointXYZ (ring->Coords, iv, pt->X, pt->Y, pt->Z);
		iv++;
		pt = pt->Next;
	    }
	  dyn_rng = dyn_pg->first;
	  while (dyn_rng)
	    {
		/* initializing any INTERIOR RING */
		if (dyn_rng->interior == 0)
		  {
		      dyn_rng = dyn_rng->next;
		      continue;
		  }
		points = kml_count_dyn_points (dyn_rng->ring);
		ring = gaiaAddInteriorRing (new_pg, ib, points);
		ib++;
		pt = dyn_rng->ring->First;
		iv = 0;
		while (pt)
		  {
		      gaiaSetPointXYZ (ring->Coords, iv, pt->X, pt->Y, pt->Z);
		      iv++;
		      pt = pt->Next;
		  }
		dyn_rng = dyn_rng->next;
	    }
      }
    else
      {
	  pg = gaiaAllocGeomColl ();
	  new_pg = gaiaAddPolygonToGeomColl (pg, points, inners);
	  /* initializing the EXTERIOR RING */
	  ring = new_pg->Exterior;
	  pt = exterior_ring->First;
	  iv = 0;
	  while (pt)
	    {
		gaiaSetPoint (ring->Coords, iv, pt->X, pt->Y);
		iv++;
		pt = pt->Next;
	    }
	  dyn_rng = dyn_pg->first;
	  while (dyn_rng)
	    {
		/* initializing any INTERIOR RING */
		if (dyn_rng->interior == 0)
		  {
		      dyn_rng = dyn_rng->next;
		      continue;
		  }
		points = kml_count_dyn_points (dyn_rng->ring);
		ring = gaiaAddInteriorRing (new_pg, ib, points);
		ib++;
		pt = dyn_rng->ring->First;
		iv = 0;
		while (pt)
		  {
		      gaiaSetPoint (ring->Coords, iv, pt->X, pt->Y);
		      iv++;
		      pt = pt->Next;
		  }
		dyn_rng = dyn_rng->next;
	    }
      }

    last_g = geom;
    while (1)
      {
	  /* searching the last Geometry within chain */
	  if (last_g->Next == NULL)
	      break;
	  last_g = last_g->Next;
      }
    last_g->Next = pg;
    kml_free_dyn_polygon (dyn_pg);
    return 1;

  error:
    kml_free_dyn_polygon (dyn_pg);
    return 0;
}

static int
kml_parse_multi_geometry (gaiaGeomCollPtr geom, kmlNodePtr node)
{
/* parsing a <MultiGeometry> */
    kmlNodePtr next;
    kmlNodePtr n = node;
    while (n)
      {
	  /* looping on Geometry Members */
	  if (n->Next == NULL)
	    {
		/* verifying the last KML node */
		if (strcmp (n->Tag, "MultiGeometry") == 0)
		    break;
		else
		    return 0;
	    }
	  if (strcmp (n->Tag, "Point") == 0)
	    {
		n = n->Next;
		if (n == NULL)
		    return 0;
		if (!kml_parse_point (geom, n, &next))
		    return 0;
		n = next;
		continue;
	    }
	  else if (strcmp (n->Tag, "LineString") == 0)
	    {
		n = n->Next;
		if (n == NULL)
		    return 0;
		if (!kml_parse_linestring (geom, n, &next))
		    return 0;
		n = next;
		continue;
	    }
	  else if (strcmp (n->Tag, "Polygon") == 0)
	    {
		n = n->Next;
		if (n == NULL)
		    return 0;
		if (!kml_parse_polygon (geom, n, &next))
		    return 0;
		n = next;
		continue;
	    }
	  else
	      return 0;
      }
    return 1;
}

static gaiaGeomCollPtr
kml_validate_geometry (gaiaGeomCollPtr chain)
{
    int xy = 0;
    int xyz = 0;
    int pts = 0;
    int lns = 0;
    int pgs = 0;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    gaiaPointPtr save_pt;
    gaiaLinestringPtr save_ln;
    gaiaPolygonPtr save_pg;
    gaiaRingPtr i_ring;
    gaiaRingPtr o_ring;
    int ib;
    gaiaGeomCollPtr g;
    gaiaGeomCollPtr geom;

    g = chain;
    while (g)
      {
	  if (g != chain)
	    {
		if (g->DimensionModel == GAIA_XY)
		    xy++;
		if (g->DimensionModel == GAIA_XY_Z)
		    xyz++;
	    }
	  pt = g->FirstPoint;
	  while (pt)
	    {
		pts++;
		save_pt = pt;
		pt = pt->Next;
	    }
	  ln = g->FirstLinestring;
	  while (ln)
	    {
		lns++;
		save_ln = ln;
		ln = ln->Next;
	    }
	  pg = g->FirstPolygon;
	  while (pg)
	    {
		pgs++;
		save_pg = pg;
		pg = pg->Next;
	    }
	  g = g->Next;
      }
    if (pts == 1 && lns == 0 && pgs == 0)
      {
	  /* POINT */
	  if (xy > 0)
	    {
		/* 2D [XY] */
		geom = gaiaAllocGeomColl ();
		if (chain->DeclaredType == GAIA_GEOMETRYCOLLECTION)
		    geom->DeclaredType = GAIA_MULTIPOINT;
		else
		    geom->DeclaredType = GAIA_POINT;
		gaiaAddPointToGeomColl (geom, save_pt->X, save_pt->Y);
		return geom;
	    }
	  else
	    {
		/* 3D [XYZ] */
		geom = gaiaAllocGeomCollXYZ ();
		if (chain->DeclaredType == GAIA_GEOMETRYCOLLECTION)
		    geom->DeclaredType = GAIA_MULTIPOINT;
		else
		    geom->DeclaredType = GAIA_POINT;
		gaiaAddPointToGeomCollXYZ (geom, save_pt->X, save_pt->Y,
					   save_pt->Z);
		return geom;
	    }
      }
    if (pts == 0 && lns == 1 && pgs == 0)
      {
	  /* LINESTRING */
	  if (xy > 0)
	    {
		/* 2D [XY] */
		geom = gaiaAllocGeomColl ();
	    }
	  else
	    {
		/* 3D [XYZ] */
		geom = gaiaAllocGeomCollXYZ ();
	    }
	  if (chain->DeclaredType == GAIA_GEOMETRYCOLLECTION)
	      geom->DeclaredType = GAIA_MULTILINESTRING;
	  else
	      geom->DeclaredType = GAIA_LINESTRING;
	  ln = gaiaAddLinestringToGeomColl (geom, save_ln->Points);
	  gaiaCopyLinestringCoords (ln, save_ln);
	  return geom;
      }
    if (pts == 0 && lns == 0 && pgs == 1)
      {
	  /* POLYGON */
	  if (xy > 0)
	    {
		/* 2D [XY] */
		geom = gaiaAllocGeomColl ();
	    }
	  else
	    {
		/* 3D [XYZ] */
		geom = gaiaAllocGeomCollXYZ ();
	    }
	  if (chain->DeclaredType == GAIA_GEOMETRYCOLLECTION)
	      geom->DeclaredType = GAIA_MULTIPOLYGON;
	  else
	      geom->DeclaredType = GAIA_POLYGON;
	  i_ring = save_pg->Exterior;
	  pg = gaiaAddPolygonToGeomColl (geom, i_ring->Points,
					 save_pg->NumInteriors);
	  o_ring = pg->Exterior;
	  gaiaCopyRingCoords (o_ring, i_ring);
	  for (ib = 0; ib < save_pg->NumInteriors; ib++)
	    {
		i_ring = save_pg->Interiors + ib;
		o_ring = gaiaAddInteriorRing (pg, ib, i_ring->Points);
		gaiaCopyRingCoords (o_ring, i_ring);
	    }
	  return geom;
      }
    if (pts >= 1 && lns == 0 && pgs == 0)
      {
	  /* MULTIPOINT */
	  if (xy > 0)
	    {
		/* 2D [XY] */
		geom = gaiaAllocGeomColl ();
		geom->DeclaredType = GAIA_MULTIPOINT;
		g = chain;
		while (g)
		  {
		      pt = g->FirstPoint;
		      while (pt)
			{
			    gaiaAddPointToGeomColl (geom, pt->X, pt->Y);
			    pt = pt->Next;
			}
		      g = g->Next;
		  }
		return geom;
	    }
	  else
	    {
		/* 3D [XYZ] */
		geom = gaiaAllocGeomCollXYZ ();
		geom->DeclaredType = GAIA_MULTIPOINT;
		g = chain;
		while (g)
		  {
		      pt = g->FirstPoint;
		      while (pt)
			{
			    gaiaAddPointToGeomCollXYZ (geom, pt->X, pt->Y,
						       pt->Z);
			    pt = pt->Next;
			}
		      g = g->Next;
		  }
		return geom;
	    }
      }
    if (pts == 0 && lns >= 1 && pgs == 0)
      {
	  /* MULTILINESTRING */
	  if (xy > 0)
	    {
		/* 2D [XY] */
		geom = gaiaAllocGeomColl ();
		geom->DeclaredType = GAIA_MULTILINESTRING;
		g = chain;
		while (g)
		  {
		      ln = g->FirstLinestring;
		      while (ln)
			{
			    save_ln =
				gaiaAddLinestringToGeomColl (geom, ln->Points);
			    gaiaCopyLinestringCoords (save_ln, ln);
			    ln = ln->Next;
			}
		      g = g->Next;
		  }
		return geom;
	    }
	  else
	    {
		/* 3D [XYZ] */
		geom = gaiaAllocGeomCollXYZ ();
		geom->DeclaredType = GAIA_MULTILINESTRING;
		g = chain;
		while (g)
		  {
		      ln = g->FirstLinestring;
		      while (ln)
			{
			    save_ln =
				gaiaAddLinestringToGeomColl (geom, ln->Points);
			    gaiaCopyLinestringCoords (save_ln, ln);
			    ln = ln->Next;
			}
		      g = g->Next;
		  }
		return geom;
	    }
      }
    if (pts == 0 && lns == 0 && pgs >= 1)
      {
	  /* MULTIPOLYGON */
	  if (xy > 0)
	    {
		/* 2D [XY] */
		geom = gaiaAllocGeomColl ();
		geom->DeclaredType = GAIA_MULTIPOLYGON;
		g = chain;
		while (g)
		  {
		      pg = g->FirstPolygon;
		      while (pg)
			{
			    i_ring = pg->Exterior;
			    save_pg =
				gaiaAddPolygonToGeomColl (geom, i_ring->Points,
							  pg->NumInteriors);
			    o_ring = save_pg->Exterior;
			    gaiaCopyRingCoords (o_ring, i_ring);
			    for (ib = 0; ib < pg->NumInteriors; ib++)
			      {
				  i_ring = pg->Interiors + ib;
				  o_ring =
				      gaiaAddInteriorRing (save_pg, ib,
							   i_ring->Points);
				  gaiaCopyRingCoords (o_ring, i_ring);
			      }
			    pg = pg->Next;
			}
		      g = g->Next;
		  }
		return geom;
	    }
	  else
	    {
		/* 3D [XYZ] */
		geom = gaiaAllocGeomCollXYZ ();
		geom->DeclaredType = GAIA_MULTIPOLYGON;
		g = chain;
		while (g)
		  {
		      pg = g->FirstPolygon;
		      while (pg)
			{
			    i_ring = pg->Exterior;
			    save_pg =
				gaiaAddPolygonToGeomColl (geom, i_ring->Points,
							  pg->NumInteriors);
			    o_ring = save_pg->Exterior;
			    gaiaCopyRingCoords (o_ring, i_ring);
			    for (ib = 0; ib < pg->NumInteriors; ib++)
			      {
				  i_ring = pg->Interiors + ib;
				  o_ring =
				      gaiaAddInteriorRing (save_pg, ib,
							   i_ring->Points);
				  gaiaCopyRingCoords (o_ring, i_ring);
			      }
			    pg = pg->Next;
			}
		      g = g->Next;
		  }
		return geom;
	    }
      }
    if ((pts + lns + pgs) > 0)
      {
	  /* GEOMETRYCOLLECTION */
	  if (xy > 0)
	    {
		/* 2D [XY] */
		geom = gaiaAllocGeomColl ();
		geom->DeclaredType = GAIA_GEOMETRYCOLLECTION;
		g = chain;
		while (g)
		  {
		      pt = g->FirstPoint;
		      while (pt)
			{
			    gaiaAddPointToGeomColl (geom, pt->X, pt->Y);
			    pt = pt->Next;
			}
		      ln = g->FirstLinestring;
		      while (ln)
			{
			    save_ln =
				gaiaAddLinestringToGeomColl (geom, ln->Points);
			    gaiaCopyLinestringCoords (save_ln, ln);
			    ln = ln->Next;
			}
		      pg = g->FirstPolygon;
		      while (pg)
			{
			    i_ring = pg->Exterior;
			    save_pg =
				gaiaAddPolygonToGeomColl (geom, i_ring->Points,
							  pg->NumInteriors);
			    o_ring = save_pg->Exterior;
			    gaiaCopyRingCoords (o_ring, i_ring);
			    for (ib = 0; ib < pg->NumInteriors; ib++)
			      {
				  i_ring = pg->Interiors + ib;
				  o_ring =
				      gaiaAddInteriorRing (save_pg, ib,
							   i_ring->Points);
				  gaiaCopyRingCoords (o_ring, i_ring);
			      }
			    pg = pg->Next;
			}
		      g = g->Next;
		  }
		return geom;
	    }
	  else
	    {
		/* 3D [XYZ] */
		geom = gaiaAllocGeomCollXYZ ();
		geom->DeclaredType = GAIA_GEOMETRYCOLLECTION;
		g = chain;
		while (g)
		  {
		      pt = g->FirstPoint;
		      while (pt)
			{
			    gaiaAddPointToGeomCollXYZ (geom, pt->X, pt->Y,
						       pt->Z);
			    pt = pt->Next;
			}
		      ln = g->FirstLinestring;
		      while (ln)
			{
			    save_ln =
				gaiaAddLinestringToGeomColl (geom, ln->Points);
			    gaiaCopyLinestringCoords (save_ln, ln);
			    ln = ln->Next;
			}
		      pg = g->FirstPolygon;
		      while (pg)
			{
			    i_ring = pg->Exterior;
			    save_pg =
				gaiaAddPolygonToGeomColl (geom, i_ring->Points,
							  pg->NumInteriors);
			    o_ring = save_pg->Exterior;
			    gaiaCopyRingCoords (o_ring, i_ring);
			    for (ib = 0; ib < pg->NumInteriors; ib++)
			      {
				  i_ring = pg->Interiors + ib;
				  o_ring =
				      gaiaAddInteriorRing (save_pg, ib,
							   i_ring->Points);
				  gaiaCopyRingCoords (o_ring, i_ring);
			      }
			    pg = pg->Next;
			}
		      g = g->Next;
		  }
		return geom;
	    }
      }
    return NULL;
}

static void
kml_free_geom_chain (gaiaGeomCollPtr geom)
{
/* deleting a chain of preliminary geometries */
    gaiaGeomCollPtr gn;
    while (geom)
      {
	  gn = geom->Next;
	  gaiaFreeGeomColl (geom);
	  geom = gn;
      }
}

static gaiaGeomCollPtr
kml_build_geometry (kmlNodePtr tree)
{
/* attempting to build a geometry from KML nodes */
    gaiaGeomCollPtr geom;
    gaiaGeomCollPtr result;
    int geom_type;
    kmlNodePtr next;

    if (tree == NULL)
	return NULL;
    geom_type = guessKmlGeometryType (tree);
    if (geom_type == GAIA_KML_UNKNOWN)
      {
	  /* unsupported main geometry type */
	  return NULL;
      }
/* creating the main geometry */
    geom = gaiaAllocGeomColl ();

    switch (geom_type)
      {
	  /* parsing KML nodes accordingly with declared KML type */
      case GAIA_KML_POINT:
	  geom->DeclaredType = GAIA_POINT;
	  if (!kml_parse_point (geom, tree->Next, &next))
	      goto error;
	  break;
      case GAIA_KML_LINESTRING:
	  geom->DeclaredType = GAIA_LINESTRING;
	  if (!kml_parse_linestring (geom, tree->Next, &next))
	      goto error;
	  break;
      case GAIA_KML_POLYGON:
	  geom->DeclaredType = GAIA_POLYGON;
	  if (!kml_parse_polygon (geom, tree->Next, &next))
	      goto error;
	  if (next != NULL)
	      goto error;
	  break;
      case GAIA_KML_MULTIGEOMETRY:
	  geom->DeclaredType = GAIA_GEOMETRYCOLLECTION;
	  if (!kml_parse_multi_geometry (geom, tree->Next))
	      goto error;
	  break;
      };

/* attempting to build the final geometry */
    result = kml_validate_geometry (geom);
    if (result == NULL)
	goto error;
    kml_free_geom_chain (geom);
    return result;

  error:
    kml_free_geom_chain (geom);
    return NULL;
}



/*
** CAVEAT: we must redefine any Lemon/Flex own macro
*/
#define YYMINORTYPE		KML_MINORTYPE
#define YY_CHAR			KML_YY_CHAR
#define	input			kml_input
#define ParseAlloc		kmlParseAlloc
#define ParseFree		kmlParseFree
#define ParseStackPeak		kmlParseStackPeak
#define Parse			kmlParse
#define yyStackEntry		kml_yyStackEntry
#define yyzerominor		kml_yyzerominor
#define yy_accept		kml_yy_accept
#define yy_action		kml_yy_action
#define yy_base			kml_yy_base
#define yy_buffer_stack		kml_yy_buffer_stack
#define yy_buffer_stack_max	kml_yy_buffer_stack_max
#define yy_buffer_stack_top	kml_yy_buffer_stack_top
#define yy_c_buf_p		kml_yy_c_buf_p
#define yy_chk			kml_yy_chk
#define yy_def			kml_yy_def
#define yy_default		kml_yy_default
#define yy_destructor		kml_yy_destructor
#define yy_ec			kml_yy_ec
#define yy_fatal_error		kml_yy_fatal_error
#define yy_find_reduce_action	kml_yy_find_reduce_action
#define yy_find_shift_action	kml_yy_find_shift_action
#define yy_get_next_buffer	kml_yy_get_next_buffer
#define yy_get_previous_state	kml_yy_get_previous_state
#define yy_init			kml_yy_init
#define yy_init_globals		kml_yy_init_globals
#define yy_lookahead		kml_yy_lookahead
#define yy_meta			kml_yy_meta
#define yy_nxt			kml_yy_nxt
#define yy_parse_failed		kml_yy_parse_failed
#define yy_pop_parser_stack	kml_yy_pop_parser_stack
#define yy_reduce		kml_yy_reduce
#define yy_reduce_ofst		kml_yy_reduce_ofst
#define yy_shift		kml_yy_shift
#define yy_shift_ofst		kml_yy_shift_ofst
#define yy_start		kml_yy_start
#define yy_state_type		kml_yy_state_type
#define yy_syntax_error		kml_yy_syntax_error
#define yy_trans_info		kml_yy_trans_info
#define yy_try_NUL_trans	kml_yy_try_NUL_trans
#define yyParser		kml_yyParser
#define yyStackEntry		kml_yyStackEntry
#define yyStackOverflow		kml_yyStackOverflow
#define yyRuleInfo		kml_yyRuleInfo
#define yyunput			kml_yyunput
#define yyzerominor		kml_yyzerominor
#define yyTraceFILE		kml_yyTraceFILE
#define yyTracePrompt		kml_yyTracePrompt
#define yyTokenName		kml_yyTokenName
#define yyRuleName		kml_yyRuleName
#define ParseTrace		kml_ParseTrace


/*
 KML_LEMON_H_START - LEMON generated header starts here 
*/
#define KML_NEWLINE                     1
#define KML_END                         2
#define KML_CLOSE                       3
#define KML_OPEN                        4
#define KML_KEYWORD                     5
#define KML_EQ                          6
#define KML_VALUE                       7
#define KML_COORD                       8
/*
 KML_LEMON_H_END - LEMON generated header ends here 
*/


typedef union
{
    char *pval;
    struct symtab *symp;
} kml_yystype;
#define YYSTYPE kml_yystype


/* extern YYSTYPE yylval; */
YYSTYPE KmlLval;



/*
 KML_LEMON_START - LEMON generated code starts here 
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
#define YYNOCODE 28
#define YYACTIONTYPE unsigned char
#define ParseTOKENTYPE void *
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 1000000
#endif
#define ParseARG_SDECL  kmlNodePtr *result ;
#define ParseARG_PDECL , kmlNodePtr *result 
#define ParseARG_FETCH  kmlNodePtr *result  = yypParser->result 
#define ParseARG_STORE yypParser->result  = result 
#define YYNSTATE 49
#define YYNRULE 34
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
 /*     0 */    20,   28,   29,    4,   48,    5,    3,    3,    5,    5,
 /*    10 */    42,   84,    1,   42,   42,   47,   46,    2,   10,    5,
 /*    20 */    21,   12,   32,   23,   42,   38,   22,    6,   49,   23,
 /*    30 */    13,   19,   14,   15,   35,    8,    8,   10,   25,   11,
 /*    40 */    18,   34,   33,   45,   37,   16,   40,   17,   41,   14,
 /*    50 */     9,   23,   43,    7,   45,   27,   30,   26,   31,   36,
 /*    60 */    39,   44,   24,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    12,   13,   14,   15,   16,   17,   15,   15,   17,   17,
 /*    10 */    22,   10,   11,   22,   22,   24,   24,   15,   18,   17,
 /*    20 */     2,    3,    8,    5,   22,   25,    2,    3,    0,    5,
 /*    30 */    18,   19,    4,   20,   21,   20,   20,   18,    2,    3,
 /*    40 */     2,   26,   26,    5,   25,   20,   21,   20,   21,    4,
 /*    50 */    18,    5,   23,   20,    5,    1,    3,   23,    3,    7,
 /*    60 */     3,    3,    6,
};
#define YY_SHIFT_USE_DFLT (-1)
#define YY_SHIFT_MAX 26
static const signed char yy_shift_ofst[] = {
 /*     0 */    -1,   28,   45,   45,   45,   18,   14,   14,   14,   46,
 /*    10 */    46,   14,   14,   24,   38,   14,   14,   14,   49,   36,
 /*    20 */    54,   53,   55,   56,   52,   57,   58,
};
#define YY_REDUCE_USE_DFLT (-13)
#define YY_REDUCE_MAX 18
static const signed char yy_reduce_ofst[] = {
 /*     0 */     1,  -12,   -9,   -8,    2,   12,   13,   15,   16,    0,
 /*    10 */    19,   25,   27,   32,   29,   33,   33,   33,   34,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */    50,   83,   72,   72,   54,   83,   60,   80,   80,   76,
 /*    10 */    76,   61,   59,   83,   83,   64,   66,   62,   83,   83,
 /*    20 */    83,   83,   83,   83,   83,   83,   83,   51,   52,   53,
 /*    30 */    56,   57,   79,   81,   82,   65,   75,   77,   78,   58,
 /*    40 */    67,   63,   68,   69,   70,   71,   73,   74,   55,
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
  "$",             "KML_NEWLINE",   "KML_END",       "KML_CLOSE",   
  "KML_OPEN",      "KML_KEYWORD",   "KML_EQ",        "KML_VALUE",   
  "KML_COORD",     "error",         "main",          "in",          
  "state",         "program",       "kml_tree",      "node",        
  "node_chain",    "open_tag",      "attr",          "attributes",  
  "coord",         "coord_chain",   "close_tag",     "keyword",     
  "extra_nodes",   "extra_attr",    "extra_coord", 
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "main ::= in",
 /*   1 */ "in ::=",
 /*   2 */ "in ::= in state KML_NEWLINE",
 /*   3 */ "state ::= program",
 /*   4 */ "program ::= kml_tree",
 /*   5 */ "kml_tree ::= node",
 /*   6 */ "kml_tree ::= node_chain",
 /*   7 */ "node ::= open_tag KML_END KML_CLOSE",
 /*   8 */ "node ::= open_tag attr KML_END KML_CLOSE",
 /*   9 */ "node ::= open_tag attributes KML_END KML_CLOSE",
 /*  10 */ "node ::= open_tag KML_CLOSE",
 /*  11 */ "node ::= open_tag attr KML_CLOSE",
 /*  12 */ "node ::= open_tag attributes KML_CLOSE",
 /*  13 */ "node ::= open_tag KML_CLOSE coord",
 /*  14 */ "node ::= open_tag KML_CLOSE coord_chain",
 /*  15 */ "node ::= open_tag attr KML_CLOSE coord",
 /*  16 */ "node ::= open_tag attr KML_CLOSE coord_chain",
 /*  17 */ "node ::= open_tag attributes KML_CLOSE coord",
 /*  18 */ "node ::= open_tag attributes KML_CLOSE coord_chain",
 /*  19 */ "node ::= close_tag",
 /*  20 */ "open_tag ::= KML_OPEN keyword",
 /*  21 */ "close_tag ::= KML_OPEN KML_END keyword KML_CLOSE",
 /*  22 */ "keyword ::= KML_KEYWORD",
 /*  23 */ "extra_nodes ::=",
 /*  24 */ "extra_nodes ::= node extra_nodes",
 /*  25 */ "node_chain ::= node node extra_nodes",
 /*  26 */ "attr ::= KML_KEYWORD KML_EQ KML_VALUE",
 /*  27 */ "extra_attr ::=",
 /*  28 */ "extra_attr ::= attr extra_attr",
 /*  29 */ "attributes ::= attr attr extra_attr",
 /*  30 */ "coord ::= KML_COORD",
 /*  31 */ "extra_coord ::=",
 /*  32 */ "extra_coord ::= coord extra_coord",
 /*  33 */ "coord_chain ::= coord coord extra_coord",
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
  { 10, 1 },
  { 11, 0 },
  { 11, 3 },
  { 12, 1 },
  { 13, 1 },
  { 14, 1 },
  { 14, 1 },
  { 15, 3 },
  { 15, 4 },
  { 15, 4 },
  { 15, 2 },
  { 15, 3 },
  { 15, 3 },
  { 15, 3 },
  { 15, 3 },
  { 15, 4 },
  { 15, 4 },
  { 15, 4 },
  { 15, 4 },
  { 15, 1 },
  { 17, 2 },
  { 22, 4 },
  { 23, 1 },
  { 24, 0 },
  { 24, 2 },
  { 16, 3 },
  { 18, 3 },
  { 25, 0 },
  { 25, 2 },
  { 19, 3 },
  { 20, 1 },
  { 26, 0 },
  { 26, 2 },
  { 21, 3 },
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
      case 5: /* kml_tree ::= node */
      case 6: /* kml_tree ::= node_chain */ yytestcase(yyruleno==6);
{ *result = yymsp[0].minor.yy0; }
        break;
      case 7: /* node ::= open_tag KML_END KML_CLOSE */
{ yygotominor.yy0 = kml_createSelfClosedNode((void *)yymsp[-2].minor.yy0, NULL); }
        break;
      case 8: /* node ::= open_tag attr KML_END KML_CLOSE */
      case 9: /* node ::= open_tag attributes KML_END KML_CLOSE */ yytestcase(yyruleno==9);
{ yygotominor.yy0 = kml_createSelfClosedNode((void *)yymsp[-3].minor.yy0, (void *)yymsp[-2].minor.yy0); }
        break;
      case 10: /* node ::= open_tag KML_CLOSE */
{ yygotominor.yy0 = kml_createNode((void *)yymsp[-1].minor.yy0, NULL, NULL); }
        break;
      case 11: /* node ::= open_tag attr KML_CLOSE */
      case 12: /* node ::= open_tag attributes KML_CLOSE */ yytestcase(yyruleno==12);
{ yygotominor.yy0 = kml_createNode((void *)yymsp[-2].minor.yy0, (void *)yymsp[-1].minor.yy0, NULL); }
        break;
      case 13: /* node ::= open_tag KML_CLOSE coord */
      case 14: /* node ::= open_tag KML_CLOSE coord_chain */ yytestcase(yyruleno==14);
{ yygotominor.yy0 = kml_createNode((void *)yymsp[-2].minor.yy0, NULL, (void *)yymsp[0].minor.yy0); }
        break;
      case 15: /* node ::= open_tag attr KML_CLOSE coord */
      case 16: /* node ::= open_tag attr KML_CLOSE coord_chain */ yytestcase(yyruleno==16);
      case 17: /* node ::= open_tag attributes KML_CLOSE coord */ yytestcase(yyruleno==17);
      case 18: /* node ::= open_tag attributes KML_CLOSE coord_chain */ yytestcase(yyruleno==18);
{ yygotominor.yy0 = kml_createNode((void *)yymsp[-3].minor.yy0, (void *)yymsp[-2].minor.yy0, (void *)yymsp[0].minor.yy0); }
        break;
      case 19: /* node ::= close_tag */
{ yygotominor.yy0 = kml_closingNode((void *)yymsp[0].minor.yy0); }
        break;
      case 20: /* open_tag ::= KML_OPEN keyword */
      case 22: /* keyword ::= KML_KEYWORD */ yytestcase(yyruleno==22);
{ yygotominor.yy0 = yymsp[0].minor.yy0; }
        break;
      case 21: /* close_tag ::= KML_OPEN KML_END keyword KML_CLOSE */
{ yygotominor.yy0 = yymsp[-1].minor.yy0; }
        break;
      case 23: /* extra_nodes ::= */
      case 27: /* extra_attr ::= */ yytestcase(yyruleno==27);
      case 31: /* extra_coord ::= */ yytestcase(yyruleno==31);
{ yygotominor.yy0 = NULL; }
        break;
      case 24: /* extra_nodes ::= node extra_nodes */
{ ((kmlNodePtr)yymsp[-1].minor.yy0)->Next = (kmlNodePtr)yymsp[0].minor.yy0;  yygotominor.yy0 = yymsp[-1].minor.yy0; }
        break;
      case 25: /* node_chain ::= node node extra_nodes */
{ 
	   ((kmlNodePtr)yymsp[-1].minor.yy0)->Next = (kmlNodePtr)yymsp[0].minor.yy0; 
	   ((kmlNodePtr)yymsp[-2].minor.yy0)->Next = (kmlNodePtr)yymsp[-1].minor.yy0;
	   yygotominor.yy0 = yymsp[-2].minor.yy0;
	}
        break;
      case 26: /* attr ::= KML_KEYWORD KML_EQ KML_VALUE */
{ yygotominor.yy0 = kml_attribute((void *)yymsp[-2].minor.yy0, (void *)yymsp[0].minor.yy0); }
        break;
      case 28: /* extra_attr ::= attr extra_attr */
{ ((kmlAttrPtr)yymsp[-1].minor.yy0)->Next = (kmlAttrPtr)yymsp[0].minor.yy0;  yygotominor.yy0 = yymsp[-1].minor.yy0; }
        break;
      case 29: /* attributes ::= attr attr extra_attr */
{ 
	   ((kmlAttrPtr)yymsp[-1].minor.yy0)->Next = (kmlAttrPtr)yymsp[0].minor.yy0; 
	   ((kmlAttrPtr)yymsp[-2].minor.yy0)->Next = (kmlAttrPtr)yymsp[-1].minor.yy0;
	   yygotominor.yy0 = yymsp[-2].minor.yy0;
	}
        break;
      case 30: /* coord ::= KML_COORD */
{ yygotominor.yy0 = kml_coord((void *)yymsp[0].minor.yy0); }
        break;
      case 32: /* extra_coord ::= coord extra_coord */
{ ((kmlCoordPtr)yymsp[-1].minor.yy0)->Next = (kmlCoordPtr)yymsp[0].minor.yy0;  yygotominor.yy0 = yymsp[-1].minor.yy0; }
        break;
      case 33: /* coord_chain ::= coord coord extra_coord */
{ 
	   ((kmlCoordPtr)yymsp[-1].minor.yy0)->Next = (kmlCoordPtr)yymsp[0].minor.yy0; 
	   ((kmlCoordPtr)yymsp[-2].minor.yy0)->Next = (kmlCoordPtr)yymsp[-1].minor.yy0;
	   yygotominor.yy0 = yymsp[-2].minor.yy0;
	}
        break;
      default:
      /* (0) main ::= in */ yytestcase(yyruleno==0);
      /* (1) in ::= */ yytestcase(yyruleno==1);
      /* (2) in ::= in state KML_NEWLINE */ yytestcase(yyruleno==2);
      /* (3) state ::= program */ yytestcase(yyruleno==3);
      /* (4) program ::= kml_tree */ yytestcase(yyruleno==4);
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
	kml_parse_error = 1;
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
 KML_LEMON_END - LEMON generated code ends here 
*/

















/*
** CAVEAT: there is an incompatibility between LEMON and FLEX
** this macro resolves the issue
*/
#undef yy_accept
#define yy_accept	yy_kml_flex_accept



/*
 KML_FLEX_START - FLEX generated code starts here 
*/

#line 3 "lex.Kml.c"

#define  YY_INT_ALIGNED short int

/* A lexical scanner generated by flex */

#define yy_create_buffer Kml_create_buffer
#define yy_delete_buffer Kml_delete_buffer
#define yy_flex_debug Kml_flex_debug
#define yy_init_buffer Kml_init_buffer
#define yy_flush_buffer Kml_flush_buffer
#define yy_load_buffer_state Kml_load_buffer_state
#define yy_switch_to_buffer Kml_switch_to_buffer
#define yyin Kmlin
#define yyleng Kmlleng
#define yylex Kmllex
#define yylineno Kmllineno
#define yyout Kmlout
#define yyrestart Kmlrestart
#define yytext Kmltext
#define yywrap Kmlwrap
#define yyalloc Kmlalloc
#define yyrealloc Kmlrealloc
#define yyfree Kmlfree

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
#define YY_NEW_FILE Kmlrestart(Kmlin  )

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

extern int Kmlleng;

extern FILE *Kmlin, *Kmlout;

#define EOB_ACT_CONTINUE_SCAN 0
#define EOB_ACT_END_OF_FILE 1
#define EOB_ACT_LAST_MATCH 2

    #define YY_LESS_LINENO(n)
    
/* Return all but the first "n" matched characters back to the input stream. */
#define yyless(n) \
	do \
		{ \
		/* Undo effects of setting up Kmltext. */ \
        int yyless_macro_arg = (n); \
        YY_LESS_LINENO(yyless_macro_arg);\
		*yy_cp = (yy_hold_char); \
		YY_RESTORE_YY_MORE_OFFSET \
		(yy_c_buf_p) = yy_cp = yy_bp + yyless_macro_arg - YY_MORE_ADJ; \
		YY_DO_BEFORE_ACTION; /* set up Kmltext again */ \
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
	 * (via Kmlrestart()), so that the user can continue scanning by
	 * just pointing Kmlin at a new input file.
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

/* yy_hold_char holds the character lost when Kmltext is formed. */
static char yy_hold_char;
static int yy_n_chars;		/* number of characters read into yy_ch_buf */
int Kmlleng;

/* Points to current character in buffer. */
static char *yy_c_buf_p = (char *) 0;
static int yy_init = 0;		/* whether we need to initialize */
static int yy_start = 0;	/* start state number */

/* Flag which is used to allow Kmlwrap()'s to do buffer switches
 * instead of setting up a fresh Kmlin.  A bit of a hack ...
 */
static int yy_did_buffer_switch_on_eof;

void Kmlrestart (FILE *input_file  );
void Kml_switch_to_buffer (YY_BUFFER_STATE new_buffer  );
YY_BUFFER_STATE Kml_create_buffer (FILE *file,int size  );
void Kml_delete_buffer (YY_BUFFER_STATE b  );
void Kml_flush_buffer (YY_BUFFER_STATE b  );
void Kmlpush_buffer_state (YY_BUFFER_STATE new_buffer  );
void Kmlpop_buffer_state (void );

static void Kmlensure_buffer_stack (void );
static void Kml_load_buffer_state (void );
static void Kml_init_buffer (YY_BUFFER_STATE b,FILE *file  );

#define YY_FLUSH_BUFFER Kml_flush_buffer(YY_CURRENT_BUFFER )

YY_BUFFER_STATE Kml_scan_buffer (char *base,yy_size_t size  );
YY_BUFFER_STATE Kml_scan_string (yyconst char *yy_str  );
YY_BUFFER_STATE Kml_scan_bytes (yyconst char *bytes,int len  );

void *Kmlalloc (yy_size_t  );
void *Kmlrealloc (void *,yy_size_t  );
void Kmlfree (void *  );

#define yy_new_buffer Kml_create_buffer

#define yy_set_interactive(is_interactive) \
	{ \
	if ( ! YY_CURRENT_BUFFER ){ \
        Kmlensure_buffer_stack (); \
		YY_CURRENT_BUFFER_LVALUE =    \
            Kml_create_buffer(Kmlin,YY_BUF_SIZE ); \
	} \
	YY_CURRENT_BUFFER_LVALUE->yy_is_interactive = is_interactive; \
	}

#define yy_set_bol(at_bol) \
	{ \
	if ( ! YY_CURRENT_BUFFER ){\
        Kmlensure_buffer_stack (); \
		YY_CURRENT_BUFFER_LVALUE =    \
            Kml_create_buffer(Kmlin,YY_BUF_SIZE ); \
	} \
	YY_CURRENT_BUFFER_LVALUE->yy_at_bol = at_bol; \
	}

#define YY_AT_BOL() (YY_CURRENT_BUFFER_LVALUE->yy_at_bol)

/* Begin user sect3 */

typedef unsigned char YY_CHAR;

FILE *Kmlin = (FILE *) 0, *Kmlout = (FILE *) 0;

typedef int yy_state_type;

extern int Kmllineno;

int Kmllineno = 1;

extern char *Kmltext;
#define yytext_ptr Kmltext

static yy_state_type yy_get_previous_state (void );
static yy_state_type yy_try_NUL_trans (yy_state_type current_state  );
static int yy_get_next_buffer (void );
static void yy_fatal_error (yyconst char msg[]  );

/* Done after the current pattern has been matched and before the
 * corresponding action - sets up Kmltext.
 */
#define YY_DO_BEFORE_ACTION \
	(yytext_ptr) = yy_bp; \
	Kmlleng = (size_t) (yy_cp - yy_bp); \
	(yy_hold_char) = *yy_cp; \
	*yy_cp = '\0'; \
	(yy_c_buf_p) = yy_cp;

#define YY_NUM_RULES 11
#define YY_END_OF_BUFFER 12
/* This struct is not used in this scanner,
   but its presence is necessary. */
struct yy_trans_info
	{
	flex_int32_t yy_verify;
	flex_int32_t yy_nxt;
	};
static yyconst flex_int16_t yy_accept[19] =
    {   0,
        5,    5,   12,   10,    8,    9,   10,    5,    1,    3,
        2,    4,    7,    0,    6,    5,    7,    0
    } ;

static yyconst flex_int32_t yy_ec[256] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    2,    3,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    2,    1,    4,    1,    1,    1,    1,    1,    1,
        1,    1,    5,    5,    5,    5,    6,    7,    7,    7,
        7,    7,    7,    7,    7,    7,    7,    8,    1,    9,
       10,   11,    1,    1,   12,   12,   12,   12,   12,   12,
       12,   12,   12,   12,   12,   12,   12,   12,   12,   12,
       12,   12,   12,   12,   12,   12,   12,   12,   12,   12,
        1,    1,    1,    1,   12,    1,   12,   12,   12,   12,

       12,   12,   12,   12,   12,   12,   12,   12,   12,   12,
       12,   12,   12,   12,   12,   12,   12,   12,   12,   12,
       12,   12,    1,    1,    1,    1,    1,    1,    1,    1,
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

static yyconst flex_int32_t yy_meta[13] =
    {   0,
        1,    1,    1,    1,    2,    1,    3,    4,    5,    1,
        5,    4
    } ;

static yyconst flex_int16_t yy_base[22] =
    {   0,
        0,    0,   23,   24,   24,   24,   18,    0,   24,   24,
       24,   24,    0,   17,   24,    0,    0,   24,   12,   15,
       16
    } ;

static yyconst flex_int16_t yy_def[22] =
    {   0,
       18,    1,   18,   18,   18,   18,   19,   20,   18,   18,
       18,   18,   21,   19,   18,   20,   21,    0,   18,   18,
       18
    } ;

static yyconst flex_int16_t yy_nxt[37] =
    {   0,
        4,    5,    6,    7,    8,    9,    8,    4,   10,   11,
       12,   13,   14,   14,   14,   14,   16,   16,   17,   17,
       15,   15,   18,    3,   18,   18,   18,   18,   18,   18,
       18,   18,   18,   18,   18,   18
    } ;

static yyconst flex_int16_t yy_chk[37] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,   19,   19,   19,   19,   20,   20,   21,   21,
       14,    7,    3,   18,   18,   18,   18,   18,   18,   18,
       18,   18,   18,   18,   18,   18
    } ;

static yy_state_type yy_last_accepting_state;
static char *yy_last_accepting_cpos;

extern int Kml_flex_debug;
int Kml_flex_debug = 0;

/* The intent behind this definition is that it'll catch
 * any uses of REJECT which flex missed.
 */
#define REJECT reject_used_but_not_detected
#define yymore() yymore_used_but_not_detected
#define YY_MORE_ADJ 0
#define YY_RESTORE_YY_MORE_OFFSET
char *Kmltext;
/* 
 kmlLexer.l -- KML parser - FLEX config
  
 version 2.4, 2011 June 14

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
int kml_line = 1, kml_col = 1;

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

int Kmllex_destroy (void );

int Kmlget_debug (void );

void Kmlset_debug (int debug_flag  );

YY_EXTRA_TYPE Kmlget_extra (void );

void Kmlset_extra (YY_EXTRA_TYPE user_defined  );

FILE *Kmlget_in (void );

void Kmlset_in  (FILE * in_str  );

FILE *Kmlget_out (void );

void Kmlset_out  (FILE * out_str  );

int Kmlget_leng (void );

char *Kmlget_text (void );

int Kmlget_lineno (void );

void Kmlset_lineno (int line_number  );

/* Macros after this point can all be overridden by user definitions in
 * section 1.
 */

#ifndef YY_SKIP_YYWRAP
#ifdef __cplusplus
extern "C" int Kmlwrap (void );
#else
extern int Kmlwrap (void );
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
#define ECHO do { if (fwrite( Kmltext, Kmlleng, 1, Kmlout )) {} } while (0)
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
			     (c = getc( Kmlin )) != EOF && c != '\n'; ++n ) \
			buf[n] = (char) c; \
		if ( c == '\n' ) \
			buf[n++] = (char) c; \
		if ( c == EOF && ferror( Kmlin ) ) \
			YY_FATAL_ERROR( "input in flex scanner failed" ); \
		result = n; \
		} \
	else \
		{ \
		errno=0; \
		while ( (result = fread(buf, 1, max_size, Kmlin))==0 && ferror(Kmlin)) \
			{ \
			if( errno != EINTR) \
				{ \
				YY_FATAL_ERROR( "input in flex scanner failed" ); \
				break; \
				} \
			errno=0; \
			clearerr(Kmlin); \
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

extern int Kmllex (void);

#define YY_DECL int Kmllex (void)
#endif /* !YY_DECL */

/* Code executed at the beginning of each rule, after Kmltext and Kmlleng
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

		if ( ! Kmlin )
			Kmlin = stdin;

		if ( ! Kmlout )
			Kmlout = stdout;

		if ( ! YY_CURRENT_BUFFER ) {
			Kmlensure_buffer_stack ();
			YY_CURRENT_BUFFER_LVALUE =
				Kml_create_buffer(Kmlin,YY_BUF_SIZE );
		}

		Kml_load_buffer_state( );
		}

	while ( 1 )		/* loops until end-of-file is reached */
		{
		yy_cp = (yy_c_buf_p);

		/* Support of Kmltext. */
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
				if ( yy_current_state >= 19 )
					yy_c = yy_meta[(unsigned int) yy_c];
				}
			yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
			++yy_cp;
			}
		while ( yy_base[yy_current_state] != 24 );

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
{ kml_freeString(&(KmlLval.pval)); return KML_END; }
	YY_BREAK
case 2:
YY_RULE_SETUP
{ kml_freeString(&(KmlLval.pval)); return KML_EQ; }
	YY_BREAK
case 3:
YY_RULE_SETUP
{ kml_freeString(&(KmlLval.pval)); return KML_OPEN; }
	YY_BREAK
case 4:
YY_RULE_SETUP
{ kml_freeString(&(KmlLval.pval)); return KML_CLOSE; }
	YY_BREAK
case 5:
YY_RULE_SETUP
{ kml_saveString(&(KmlLval.pval), Kmltext); return KML_COORD; }
	YY_BREAK
case 6:
/* rule 6 can match eol */
YY_RULE_SETUP
{ kml_saveString(&(KmlLval.pval), Kmltext); return KML_VALUE; }
	YY_BREAK
case 7:
YY_RULE_SETUP
{ kml_saveString(&(KmlLval.pval), Kmltext); return KML_KEYWORD; }
	YY_BREAK
case 8:
YY_RULE_SETUP
{ kml_freeString(&(KmlLval.pval)); kml_col += (int) strlen(Kmltext); }               /* ignore but count white space */
	YY_BREAK
case 9:
/* rule 9 can match eol */
YY_RULE_SETUP
{ kml_freeString(&(KmlLval.pval)); kml_col = 0; ++kml_line; }
	YY_BREAK
case 10:
YY_RULE_SETUP
{ kml_freeString(&(KmlLval.pval)); kml_col += (int) strlen(Kmltext); return -1; }
	YY_BREAK
case 11:
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
			 * just pointed Kmlin at a new source and called
			 * Kmllex().  If so, then we have to assure
			 * consistency between YY_CURRENT_BUFFER and our
			 * globals.  Here is the right place to do so, because
			 * this is the first action (other than possibly a
			 * back-up) that will match for the new input source.
			 */
			(yy_n_chars) = YY_CURRENT_BUFFER_LVALUE->yy_n_chars;
			YY_CURRENT_BUFFER_LVALUE->yy_input_file = Kmlin;
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

				if ( Kmlwrap( ) )
					{
					/* Note: because we've taken care in
					 * yy_get_next_buffer() to have set up
					 * Kmltext, we can now set up
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
} /* end of Kmllex */

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
					Kmlrealloc((void *) b->yy_ch_buf,b->yy_buf_size + 2  );
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
			Kmlrestart(Kmlin  );
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
		YY_CURRENT_BUFFER_LVALUE->yy_ch_buf = (char *) Kmlrealloc((void *) YY_CURRENT_BUFFER_LVALUE->yy_ch_buf,new_size  );
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
			if ( yy_current_state >= 19 )
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
		if ( yy_current_state >= 19 )
			yy_c = yy_meta[(unsigned int) yy_c];
		}
	yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
	yy_is_jam = (yy_current_state == 18);

	return yy_is_jam ? 0 : yy_current_state;
}

    static void yyunput (int c, register char * yy_bp )
{
	register char *yy_cp;
    
    yy_cp = (yy_c_buf_p);

	/* undo effects of setting up Kmltext */
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
					Kmlrestart(Kmlin );

					/*FALLTHROUGH*/

				case EOB_ACT_END_OF_FILE:
					{
					if ( Kmlwrap( ) )
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
	*(yy_c_buf_p) = '\0';	/* preserve Kmltext */
	(yy_hold_char) = *++(yy_c_buf_p);

	return c;
}
#endif	/* ifndef YY_NO_INPUT */

/** Immediately switch to a different input stream.
 * @param input_file A readable stream.
 * 
 * @note This function does not reset the start condition to @c INITIAL .
 */
    void Kmlrestart  (FILE * input_file )
{
    
	if ( ! YY_CURRENT_BUFFER ){
        Kmlensure_buffer_stack ();
		YY_CURRENT_BUFFER_LVALUE =
            Kml_create_buffer(Kmlin,YY_BUF_SIZE );
	}

	Kml_init_buffer(YY_CURRENT_BUFFER,input_file );
	Kml_load_buffer_state( );
}

/** Switch to a different input buffer.
 * @param new_buffer The new input buffer.
 * 
 */
    void Kml_switch_to_buffer  (YY_BUFFER_STATE  new_buffer )
{
    
	/* TODO. We should be able to replace this entire function body
	 * with
	 *		Kmlpop_buffer_state();
	 *		Kmlpush_buffer_state(new_buffer);
     */
	Kmlensure_buffer_stack ();
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
	Kml_load_buffer_state( );

	/* We don't actually know whether we did this switch during
	 * EOF (Kmlwrap()) processing, but the only time this flag
	 * is looked at is after Kmlwrap() is called, so it's safe
	 * to go ahead and always set it.
	 */
	(yy_did_buffer_switch_on_eof) = 1;
}

static void Kml_load_buffer_state  (void)
{
    	(yy_n_chars) = YY_CURRENT_BUFFER_LVALUE->yy_n_chars;
	(yytext_ptr) = (yy_c_buf_p) = YY_CURRENT_BUFFER_LVALUE->yy_buf_pos;
	Kmlin = YY_CURRENT_BUFFER_LVALUE->yy_input_file;
	(yy_hold_char) = *(yy_c_buf_p);
}

/** Allocate and initialize an input buffer state.
 * @param file A readable stream.
 * @param size The character buffer size in bytes. When in doubt, use @c YY_BUF_SIZE.
 * 
 * @return the allocated buffer state.
 */
    YY_BUFFER_STATE Kml_create_buffer  (FILE * file, int  size )
{
	YY_BUFFER_STATE b;
    
	b = (YY_BUFFER_STATE) Kmlalloc(sizeof( struct yy_buffer_state )  );
	if ( ! b )
		YY_FATAL_ERROR( "out of dynamic memory in Kml_create_buffer()" );

	b->yy_buf_size = size;

	/* yy_ch_buf has to be 2 characters longer than the size given because
	 * we need to put in 2 end-of-buffer characters.
	 */
	b->yy_ch_buf = (char *) Kmlalloc(b->yy_buf_size + 2  );
	if ( ! b->yy_ch_buf )
		YY_FATAL_ERROR( "out of dynamic memory in Kml_create_buffer()" );

	b->yy_is_our_buffer = 1;

	Kml_init_buffer(b,file );

	return b;
}

/** Destroy the buffer.
 * @param b a buffer created with Kml_create_buffer()
 * 
 */
    void Kml_delete_buffer (YY_BUFFER_STATE  b )
{
    
	if ( ! b )
		return;

	if ( b == YY_CURRENT_BUFFER ) /* Not sure if we should pop here. */
		YY_CURRENT_BUFFER_LVALUE = (YY_BUFFER_STATE) 0;

	if ( b->yy_is_our_buffer )
		Kmlfree((void *) b->yy_ch_buf  );

	Kmlfree((void *) b  );
}

#ifndef __cplusplus
extern int isatty (int );
#endif /* __cplusplus */
    
/* Initializes or reinitializes a buffer.
 * This function is sometimes called more than once on the same buffer,
 * such as during a Kmlrestart() or at EOF.
 */
    static void Kml_init_buffer  (YY_BUFFER_STATE  b, FILE * file )

{
	int oerrno = errno;
    
	Kml_flush_buffer(b );

	b->yy_input_file = file;
	b->yy_fill_buffer = 1;

    /* If b is the current buffer, then Kml_init_buffer was _probably_
     * called from Kmlrestart() or through yy_get_next_buffer.
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
    void Kml_flush_buffer (YY_BUFFER_STATE  b )
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
		Kml_load_buffer_state( );
}

/** Pushes the new state onto the stack. The new state becomes
 *  the current state. This function will allocate the stack
 *  if necessary.
 *  @param new_buffer The new state.
 *  
 */
void Kmlpush_buffer_state (YY_BUFFER_STATE new_buffer )
{
    	if (new_buffer == NULL)
		return;

	Kmlensure_buffer_stack();

	/* This block is copied from Kml_switch_to_buffer. */
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

	/* copied from Kml_switch_to_buffer. */
	Kml_load_buffer_state( );
	(yy_did_buffer_switch_on_eof) = 1;
}

/** Removes and deletes the top of the stack, if present.
 *  The next element becomes the new top.
 *  
 */
void Kmlpop_buffer_state (void)
{
    	if (!YY_CURRENT_BUFFER)
		return;

	Kml_delete_buffer(YY_CURRENT_BUFFER );
	YY_CURRENT_BUFFER_LVALUE = NULL;
	if ((yy_buffer_stack_top) > 0)
		--(yy_buffer_stack_top);

	if (YY_CURRENT_BUFFER) {
		Kml_load_buffer_state( );
		(yy_did_buffer_switch_on_eof) = 1;
	}
}

/* Allocates the stack if it does not exist.
 *  Guarantees space for at least one push.
 */
static void Kmlensure_buffer_stack (void)
{
	int num_to_alloc;
    
	if (!(yy_buffer_stack)) {

		/* First allocation is just for 2 elements, since we don't know if this
		 * scanner will even need a stack. We use 2 instead of 1 to avoid an
		 * immediate realloc on the next call.
         */
		num_to_alloc = 1;
		(yy_buffer_stack) = (struct yy_buffer_state**)Kmlalloc
								(num_to_alloc * sizeof(struct yy_buffer_state*)
								);
		if ( ! (yy_buffer_stack) )
			YY_FATAL_ERROR( "out of dynamic memory in Kmlensure_buffer_stack()" );
								  
		memset((yy_buffer_stack), 0, num_to_alloc * sizeof(struct yy_buffer_state*));
				
		(yy_buffer_stack_max) = num_to_alloc;
		(yy_buffer_stack_top) = 0;
		return;
	}

	if ((yy_buffer_stack_top) >= ((yy_buffer_stack_max)) - 1){

		/* Increase the buffer to prepare for a possible push. */
		int grow_size = 8 /* arbitrary grow size */;

		num_to_alloc = (yy_buffer_stack_max) + grow_size;
		(yy_buffer_stack) = (struct yy_buffer_state**)Kmlrealloc
								((yy_buffer_stack),
								num_to_alloc * sizeof(struct yy_buffer_state*)
								);
		if ( ! (yy_buffer_stack) )
			YY_FATAL_ERROR( "out of dynamic memory in Kmlensure_buffer_stack()" );

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
YY_BUFFER_STATE Kml_scan_buffer  (char * base, yy_size_t  size )
{
	YY_BUFFER_STATE b;
    
	if ( size < 2 ||
	     base[size-2] != YY_END_OF_BUFFER_CHAR ||
	     base[size-1] != YY_END_OF_BUFFER_CHAR )
		/* They forgot to leave room for the EOB's. */
		return 0;

	b = (YY_BUFFER_STATE) Kmlalloc(sizeof( struct yy_buffer_state )  );
	if ( ! b )
		YY_FATAL_ERROR( "out of dynamic memory in Kml_scan_buffer()" );

	b->yy_buf_size = size - 2;	/* "- 2" to take care of EOB's */
	b->yy_buf_pos = b->yy_ch_buf = base;
	b->yy_is_our_buffer = 0;
	b->yy_input_file = 0;
	b->yy_n_chars = b->yy_buf_size;
	b->yy_is_interactive = 0;
	b->yy_at_bol = 1;
	b->yy_fill_buffer = 0;
	b->yy_buffer_status = YY_BUFFER_NEW;

	Kml_switch_to_buffer(b  );

	return b;
}

/** Setup the input buffer state to scan a string. The next call to Kmllex() will
 * scan from a @e copy of @a str.
 * @param yystr a NUL-terminated string to scan
 * 
 * @return the newly allocated buffer state object.
 * @note If you want to scan bytes that may contain NUL values, then use
 *       Kml_scan_bytes() instead.
 */
YY_BUFFER_STATE Kml_scan_string (yyconst char * yystr )
{
    
	return Kml_scan_bytes(yystr,strlen(yystr) );
}

/** Setup the input buffer state to scan the given bytes. The next call to Kmllex() will
 * scan from a @e copy of @a bytes.
 * @param yybytes the byte buffer to scan
 * @param _yybytes_len the number of bytes in the buffer pointed to by @a bytes.
 * 
 * @return the newly allocated buffer state object.
 */
YY_BUFFER_STATE Kml_scan_bytes  (yyconst char * yybytes, int  _yybytes_len )
{
	YY_BUFFER_STATE b;
	char *buf;
	yy_size_t n;
	int i;
    
	/* Get memory for full buffer, including space for trailing EOB's. */
	n = _yybytes_len + 2;
	buf = (char *) Kmlalloc(n  );
	if ( ! buf )
		YY_FATAL_ERROR( "out of dynamic memory in Kml_scan_bytes()" );

	for ( i = 0; i < _yybytes_len; ++i )
		buf[i] = yybytes[i];

	buf[_yybytes_len] = buf[_yybytes_len+1] = YY_END_OF_BUFFER_CHAR;

	b = Kml_scan_buffer(buf,n );
	if ( ! b )
		YY_FATAL_ERROR( "bad buffer in Kml_scan_bytes()" );

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
		/* Undo effects of setting up Kmltext. */ \
        int yyless_macro_arg = (n); \
        YY_LESS_LINENO(yyless_macro_arg);\
		Kmltext[Kmlleng] = (yy_hold_char); \
		(yy_c_buf_p) = Kmltext + yyless_macro_arg; \
		(yy_hold_char) = *(yy_c_buf_p); \
		*(yy_c_buf_p) = '\0'; \
		Kmlleng = yyless_macro_arg; \
		} \
	while ( 0 )

/* Accessor  methods (get/set functions) to struct members. */

/** Get the current line number.
 * 
 */
int Kmlget_lineno  (void)
{
        
    return Kmllineno;
}

/** Get the input stream.
 * 
 */
FILE *Kmlget_in  (void)
{
        return Kmlin;
}

/** Get the output stream.
 * 
 */
FILE *Kmlget_out  (void)
{
        return Kmlout;
}

/** Get the length of the current token.
 * 
 */
int Kmlget_leng  (void)
{
        return Kmlleng;
}

/** Get the current token.
 * 
 */

char *Kmlget_text  (void)
{
        return Kmltext;
}

/** Set the current line number.
 * @param line_number
 * 
 */
void Kmlset_lineno (int  line_number )
{
    
    Kmllineno = line_number;
}

/** Set the input stream. This does not discard the current
 * input buffer.
 * @param in_str A readable stream.
 * 
 * @see Kml_switch_to_buffer
 */
void Kmlset_in (FILE *  in_str )
{
        Kmlin = in_str ;
}

void Kmlset_out (FILE *  out_str )
{
        Kmlout = out_str ;
}

int Kmlget_debug  (void)
{
        return Kml_flex_debug;
}

void Kmlset_debug (int  bdebug )
{
        Kml_flex_debug = bdebug ;
}

static int yy_init_globals (void)
{
        /* Initialization is the same as for the non-reentrant scanner.
     * This function is called from Kmllex_destroy(), so don't allocate here.
     */

    (yy_buffer_stack) = 0;
    (yy_buffer_stack_top) = 0;
    (yy_buffer_stack_max) = 0;
    (yy_c_buf_p) = (char *) 0;
    (yy_init) = 0;
    (yy_start) = 0;

/* Defined in main.c */
#ifdef YY_STDINIT
    Kmlin = stdin;
    Kmlout = stdout;
#else
    Kmlin = (FILE *) 0;
    Kmlout = (FILE *) 0;
#endif

    /* For future reference: Set errno on error, since we are called by
     * Kmllex_init()
     */
    return 0;
}

/* Kmllex_destroy is for both reentrant and non-reentrant scanners. */
int Kmllex_destroy  (void)
{
    
    /* Pop the buffer stack, destroying each element. */
	while(YY_CURRENT_BUFFER){
		Kml_delete_buffer(YY_CURRENT_BUFFER  );
		YY_CURRENT_BUFFER_LVALUE = NULL;
		Kmlpop_buffer_state();
	}

	/* Destroy the stack itself. */
	Kmlfree((yy_buffer_stack) );
	(yy_buffer_stack) = NULL;

    /* Reset the globals. This is important in a non-reentrant scanner so the next time
     * Kmllex() is called, initialization will occur. */
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

void *Kmlalloc (yy_size_t  size )
{
	return (void *) malloc( size );
}

void *Kmlrealloc  (void * ptr, yy_size_t  size )
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

void Kmlfree (void * ptr )
{
	free( (char *) ptr );	/* see Kmlrealloc() for (char *) cast */
}

#define YYTABLES_NAME "yytables"

/**
 * reset the line and column count
 *
 *
 */
void kml_reset_lexer(void)
{

  kml_line = 1;
  kml_col  = 1;

}

/**
 * kmlError() is invoked when the lexer or the parser encounter
 * an error. The error message is passed via *s
 *
 *
 */
void KmlError(char *s)
{
  printf("error: %s at line: %d col: %d\n",s,kml_line,kml_col);

}

int Kmlwrap(void)
{
  return 1;
}


/* 
 KML_FLEX_END - FLEX generated code ends here 
*/



gaiaGeomCollPtr
gaiaParseKml (const unsigned char *dirty_buffer)
{
    void *pParser = ParseAlloc (malloc);
    /* Linked-list of token values */
    kmlFlexToken *tokens = malloc (sizeof (kmlFlexToken));
    /* Pointer to the head of the list */
    kmlFlexToken *head = tokens;
    int yv;
    kmlNodePtr result = NULL;
    gaiaGeomCollPtr geom = NULL;

    KmlLval.pval = NULL;
    tokens->value = NULL;
    tokens->Next = NULL;
    kml_parse_error = 0;
    Kml_scan_string ((char *) dirty_buffer);

    /*
       / Keep tokenizing until we reach the end
       / yylex() will return the next matching Token for us.
     */
    while ((yv = yylex ()) != 0)
      {
	  if (yv == -1)
	    {
		kml_parse_error = 1;
		break;
	    }
	  tokens->Next = malloc (sizeof (kmlFlexToken));
	  tokens->Next->Next = NULL;
	  /*
	     /KmlLval is a global variable from FLEX.
	     /KmlLval is defined in kmlLexglobal.h
	   */
	  kml_xferString (&(tokens->Next->value), KmlLval.pval);
	  /* Pass the token to the wkt parser created from lemon */
	  Parse (pParser, yv, &(tokens->Next->value), &result);
	  tokens = tokens->Next;
      }
    /* This denotes the end of a line as well as the end of the parser */
    Parse (pParser, KML_NEWLINE, 0, &result);
    ParseFree (pParser, free);
    Kmllex_destroy ();

    /* Assigning the token as the end to avoid seg faults while cleaning */
    tokens->Next = NULL;
    kml_cleanup (head);
    kml_freeString (&(KmlLval.pval));

    if (kml_parse_error)
      {
	  if (result)
	      kml_freeTree (result);
	  return NULL;
      }

    /* attempting to build a geometry from KML */
    geom = kml_build_geometry (result);
    geom->Srid = 4326;
    kml_freeTree (result);
    return geom;
}


/*
** CAVEAT: we must now undefine any Lemon/Flex own macro
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
