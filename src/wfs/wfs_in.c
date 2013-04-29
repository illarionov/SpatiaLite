/*

 wfs_in.c -- implements WFS support [import]

 version 4.1, 2013 April 22

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
 
Portions created by the Initial Developer are Copyright (C) 2008-2012
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
#include <time.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#include <spatialite/sqlite.h>
#include <spatialite/debug.h>

#include <spatialite/gaiaaux.h>
#include <spatialite/gaiageo.h>
#include <spatialite.h>
#include <spatialite_private.h>

#ifdef ENABLE_LIBXML2		/* LIBXML2 enabled: supporting XML documents */

#include <libxml/parser.h>

#ifdef _WIN32
#define atoll	_atoi64
#endif /* not WIN32 */

#ifdef _WIN32
#define strcasecmp	_stricmp
#endif /* not WIN32 */

struct wfs_srid_def
{
/* a WFS supported SRID */
    int srid;
    char *srs_name;
    struct wfs_srid_def *next;
};

struct wfs_keyword
{
/* a WFS keyword */
    char *keyword;
    struct wfs_keyword *next;
};

struct wfs_layer_def
{
/* a WFS layer */
    char *name;
    char *title;
    char *abstract;
    struct wfs_srid_def *first_srid;
    struct wfs_srid_def *last_srid;
    struct wfs_keyword *first_key;
    struct wfs_keyword *last_key;
    struct wfs_layer_def *next;
};

struct wfs_catalog
{
/* a list of WFS layers */
    char *base_url;
    struct wfs_layer_def *first;
    struct wfs_layer_def *last;
};

struct wfs_column_def
{
/* a WFS attribute / column */
    char *name;
    int type;
    int is_nullable;
    char *value;
    struct wfs_column_def *next;
};

struct wfs_layer_schema
{
/* a WFS table / layer schema */
    int error;
    int swap_axes;
    struct wfs_column_def *first;
    struct wfs_column_def *last;
    char *geometry_name;
    int geometry_type;
    int srid;
    int dims;
    int is_nullable;
    char *geometry_value;
    sqlite3_stmt *stmt;
    sqlite3 *sqlite;
};

struct wfs_attribute
{
/* a WFS attribute value */
    struct wfs_column_def *column;
    char *value;
    struct wfs_attribute *next;
};

struct wfs_feature
{
/* a WFS feature */
    struct wfs_attribute *first;
    struct wfs_attribute *last;
    char *geometry_value;
};

static struct wfs_column_def *
alloc_wfs_column (const char *name, int type, int is_nullable)
{
/* allocating a WFS attribute / column definition */
    int len;
    struct wfs_column_def *col = malloc (sizeof (struct wfs_column_def));
    len = strlen (name);
    col->name = malloc (len + 1);
    strcpy (col->name, name);
    col->type = type;
    col->is_nullable = is_nullable;
    col->value = NULL;
    col->next = NULL;
    return col;
}

static void
free_wfs_column (struct wfs_column_def *col)
{
/* memory cleanup: destroying a WFS column definition */
    if (col == NULL)
	return;
    if (col->name != NULL)
	free (col->name);
    if (col->value != NULL)
	free (col->value);
    free (col);
}

static struct wfs_layer_schema *
alloc_wfs_layer_schema ()
{
/* allocating an empty WFS schema descriptor */
    struct wfs_layer_schema *ptr = malloc (sizeof (struct wfs_layer_schema));
    ptr->error = 0;
    ptr->swap_axes = 0;
    ptr->first = NULL;
    ptr->last = NULL;
    ptr->geometry_name = NULL;
    ptr->geometry_type = GAIA_UNKNOWN;
    ptr->srid = -1;
    ptr->dims = 2;
    ptr->geometry_value = NULL;
    ptr->stmt = NULL;
    return ptr;
}

static void
free_wfs_layer_schema (struct wfs_layer_schema *ptr)
{
/* memory cleanup: destroying a WFS schema */
    struct wfs_column_def *col;
    struct wfs_column_def *n_col;
    if (ptr == NULL)
	return;
    col = ptr->first;
    while (col != NULL)
      {
	  n_col = col->next;
	  free_wfs_column (col);
	  col = n_col;
      }
    if (ptr->geometry_name != NULL)
	free (ptr->geometry_name);
    if (ptr->geometry_value != NULL)
	free (ptr->geometry_value);
    if (ptr->stmt != NULL)
	sqlite3_finalize (ptr->stmt);
    free (ptr);
}

static void
reset_wfs_values (struct wfs_layer_schema *ptr)
{
/* memory cleanup: resetting attribute values */
    struct wfs_column_def *col;
    if (ptr == NULL)
	return;
    col = ptr->first;
    while (col != NULL)
      {
	  if (col->value != NULL)
	    {
		free (col->value);
		col->value = NULL;
	    }
	  col = col->next;
      }
    if (ptr->geometry_value != NULL)
      {
	  free (ptr->geometry_value);
	  ptr->geometry_value = NULL;
      }
}

static int
count_wfs_values (struct wfs_layer_schema *ptr)
{
/* counting how many valid values  */
    int count = 0;
    struct wfs_column_def *col;
    if (ptr == NULL)
	return 0;
    col = ptr->first;
    while (col != NULL)
      {
	  if (col->value != NULL)
	      count++;
	  col = col->next;
      }
    if (ptr->geometry_value != NULL)
	count++;
    return count;
}

static void
add_wfs_column_to_schema (struct wfs_layer_schema *ptr, const char *name,
			  int type, int is_nullable)
{
/* adding an attribute / column into a WFS table / schema */
    struct wfs_column_def *col;
    if (ptr == NULL)
	return;
    col = alloc_wfs_column (name, type, is_nullable);
    if (ptr->first == NULL)
	ptr->first = col;
    if (ptr->last != NULL)
	ptr->last->next = col;
    ptr->last = col;
}

static void
set_wfs_geometry (struct wfs_layer_schema *ptr, const char *name, int type,
		  int is_nullable)
{
/* setting the Geometry for a WFS schema */
    int len;
    if (ptr == NULL)
	return;
    if (ptr->geometry_name != NULL)
	free (ptr->geometry_name);
    len = strlen (name);
    ptr->geometry_name = malloc (len + 1);
    strcpy (ptr->geometry_name, name);
    ptr->geometry_type = type;
    ptr->is_nullable = is_nullable;
}

static struct wfs_srid_def *
alloc_wfs_srid (int srid, const char *srs_name)
{
/* allocating a WFS SRID definition */
    int len;
    struct wfs_srid_def *ptr = malloc (sizeof (struct wfs_srid_def));
    ptr->srid = srid;
    len = strlen (srs_name);
    ptr->srs_name = malloc (len + 1);
    strcpy (ptr->srs_name, srs_name);
    ptr->next = NULL;
    return ptr;
}

static struct wfs_keyword *
alloc_wfs_keyword (const char *keyword)
{
/* allocating a WFS Keyword */
    int len;
    struct wfs_keyword *ptr = malloc (sizeof (struct wfs_keyword));
    len = strlen (keyword);
    ptr->keyword = malloc (len + 1);
    strcpy (ptr->keyword, keyword);
    ptr->next = NULL;
    return ptr;
}

static struct wfs_layer_def *
alloc_wfs_layer (const char *name, const char *title, const char *abstract)
{
/* allocating a WFS catalog item / layer definition */
    int len;
    struct wfs_layer_def *lyr = malloc (sizeof (struct wfs_layer_def));
    len = strlen (name);
    lyr->name = malloc (len + 1);
    strcpy (lyr->name, name);
    if (title == NULL)
	lyr->title = NULL;
    else
      {
	  len = strlen (title);
	  lyr->title = malloc (len + 1);
	  strcpy (lyr->title, title);
      }
    if (abstract == NULL)
	lyr->abstract = NULL;
    else
      {
	  len = strlen (abstract);
	  lyr->abstract = malloc (len + 1);
	  strcpy (lyr->abstract, abstract);
      }
    lyr->first_srid = NULL;
    lyr->last_srid = NULL;
    lyr->first_key = NULL;
    lyr->last_key = NULL;
    lyr->next = NULL;
    return lyr;
}

static void
free_wfs_layer (struct wfs_layer_def *lyr)
{
/* memory cleanup: destroying a WFS catalog item definition */
    struct wfs_srid_def *srid;
    struct wfs_srid_def *n_srid;
    struct wfs_keyword *key;
    struct wfs_keyword *n_key;
    if (lyr == NULL)
	return;
    if (lyr->name != NULL)
	free (lyr->name);
    if (lyr->title != NULL)
	free (lyr->title);
    if (lyr->abstract != NULL)
	free (lyr->abstract);
    srid = lyr->first_srid;
    while (srid != NULL)
      {
	  n_srid = srid->next;
	  if (srid->srs_name != NULL)
	      free (srid->srs_name);
	  free (srid);
	  srid = n_srid;
      }
    key = lyr->first_key;
    while (key != NULL)
      {
	  n_key = key->next;
	  free (key->keyword);
	  free (key);
	  key = n_key;
      }
    free (lyr);
}

static struct wfs_catalog *
alloc_wfs_catalog ()
{
/* allocating an empty WFS catalog object */
    struct wfs_catalog *ptr = malloc (sizeof (struct wfs_catalog));
    ptr->first = NULL;
    ptr->last = NULL;
    ptr->base_url = NULL;
    return ptr;
}

static void
free_wfs_catalog (struct wfs_catalog *ptr)
{
/* memory cleanup: destroying a WFS catalog object */
    struct wfs_layer_def *lyr;
    struct wfs_layer_def *n_lyr;
    if (ptr == NULL)
	return;
    lyr = ptr->first;
    while (lyr != NULL)
      {
	  n_lyr = lyr->next;
	  free_wfs_layer (lyr);
	  lyr = n_lyr;
      }
    if (ptr->base_url != NULL)
	free (ptr->base_url);
    free (ptr);
}

static void
add_wfs_layer_to_catalog (struct wfs_catalog *ptr, const char *name,
			  const char *title, const char *abstract)
{
/* adding an item (aka Layer) into a WFS Catalog */
    struct wfs_layer_def *lyr;
    if (ptr == NULL)
	return;
    lyr = alloc_wfs_layer (name, title, abstract);
    if (ptr->first == NULL)
	ptr->first = lyr;
    if (ptr->last != NULL)
	ptr->last->next = lyr;
    ptr->last = lyr;
}

static struct wfs_feature *
create_feature (struct wfs_layer_schema *schema)
{
/* creating an empty WFS feature object */
    struct wfs_column_def *col;
    struct wfs_feature *feature = malloc (sizeof (struct wfs_feature));
    feature->first = NULL;
    feature->last = NULL;
    feature->geometry_value = NULL;
    col = schema->first;
    while (col != NULL)
      {
	  struct wfs_attribute *attr = malloc (sizeof (struct wfs_attribute));
	  attr->column = col;
	  attr->value = NULL;
	  attr->next = NULL;
	  if (feature->first == NULL)
	      feature->first = attr;
	  if (feature->last != NULL)
	      feature->last->next = attr;
	  feature->last = attr;
	  col = col->next;
      }
    return feature;
}

static void
reset_feature (struct wfs_feature *feature)
{
/* resetting a WFS feature object to its initial empty state */
    struct wfs_attribute *attr = feature->first;
    while (attr != NULL)
      {
	  if (attr->value != NULL)
	      free (attr->value);
	  attr->value = NULL;
	  attr = attr->next;
      }
    if (feature->geometry_value != NULL)
	free (feature->geometry_value);
    feature->geometry_value = NULL;
}

static void
free_feature (struct wfs_feature *feature)
{
/* memory cleanup - freeing a WFS feature object */
    struct wfs_attribute *attr;
    struct wfs_attribute *n_attr;
    reset_feature (feature);
    attr = feature->first;
    while (attr != NULL)
      {
	  n_attr = attr->next;
	  free (attr);
	  attr = n_attr;
      }
    free (feature);
}

static int
compare_features (struct wfs_feature *f1, struct wfs_feature *f2)
{
/* testing if two WFS features are identical */
    struct wfs_attribute *attr1;
    struct wfs_attribute *attr2;
    int cnt1 = 0;
    int cnt2 = 0;
/* counting how many attributes for each feature */
    attr1 = f1->first;
    while (attr1 != NULL)
      {
	  cnt1++;
	  attr1 = attr1->next;
      }
    attr2 = f2->first;
    while (attr2 != NULL)
      {
	  cnt2++;
	  attr2 = attr2->next;
      }
    if (cnt1 != cnt2)
      {
	  /* surely different - mismatching attributes count */
	  return 0;
      }
    if (f1->geometry_value == NULL && f2->geometry_value == NULL)
	;
    else if (f1->geometry_value != NULL && f2->geometry_value != NULL)
      {
	  if (strcmp (f1->geometry_value, f2->geometry_value) != 0)
	    {
		/* surely different - mismatching geometry values */
		return 0;
	    }
      }
    else
      {
	  /* surely different - mismatching geometries */
	  return 0;
      }
    attr1 = f1->first;
    attr2 = f2->first;
    while (attr1 != NULL && attr2 != NULL)
      {
	  if (strcmp (attr1->column->name, attr1->column->name) != 0)
	    {
		/* mismatching attribute name */
		return 0;
	    }
	  if (attr1->value == NULL && attr2->value == NULL)
	      ;
	  else if (attr1->value != NULL && attr2->value != NULL)
	    {
		if (strcmp (attr1->value, attr2->value) != 0)
		  {
		      /* mismatching values */
		      return 0;
		  }
	    }
	  else
	    {
		/* mismatching values */
	    }
	  attr1 = attr1->next;
	  attr2 = attr2->next;
      }
    return 1;
}

static void
set_wfs_catalog_url (struct wfs_catalog *ptr, const char *url)
{
/* setting the base-URL for a WFS catalog */
    int len;
    int i;
    int force_marker = 1;
    if (ptr == NULL)
	return;
    if (ptr->base_url != NULL)
	free (ptr->base_url);
    len = strlen (url);
    ptr->base_url = malloc (len + 2);
    strcpy (ptr->base_url, url);
    for (i = 0; i < (int) strlen (ptr->base_url); i++)
      {
	  if (*(ptr->base_url + i) == '?')
	      force_marker = 0;
      }
    if (force_marker)
	strcat (ptr->base_url, "?");
}

static void
add_wfs_srid_to_layer (struct wfs_layer_def *ptr, int srid,
		       const char *srs_name)
{
/* adding a SRID supported by a WFS Layer */
    struct wfs_srid_def *p_srid;
    if (ptr == NULL)
	return;
    p_srid = alloc_wfs_srid (srid, srs_name);
    if (ptr->first_srid == NULL)
	ptr->first_srid = p_srid;
    if (ptr->last_srid != NULL)
	ptr->last_srid->next = p_srid;
    ptr->last_srid = p_srid;
}

static void
add_wfs_keyword_to_layer (struct wfs_layer_def *ptr, const char *keyword)
{
/* adding a Keyword to a WFS Layer */
    struct wfs_keyword *key;
    if (ptr == NULL)
	return;
    if (keyword == NULL)
	return;
    key = alloc_wfs_keyword (keyword);
    if (ptr->first_key == NULL)
	ptr->first_key = key;
    if (ptr->last_key != NULL)
	ptr->last_key->next = key;
    ptr->last_key = key;
}

static void
wfsParsingError (void *ctx, const char *msg, ...)
{
/* appending to the current Parsing Error buffer */
    gaiaOutBufferPtr buf = ctx;
    char out[65536];
    va_list args;

    if (ctx != NULL)
	ctx = NULL;		/* suppressing stupid compiler warnings (unused args) */

    va_start (args, msg);
    vsnprintf (out, 65536, msg, args);
    gaiaAppendToOutBuffer (buf, out);
    va_end (args);
}

static int
find_describe_uri (xmlNodePtr node, char **describe_uri)
{
/* parsing the "schemaLocation" string */
    if (node != NULL)
      {
	  if (node->type == XML_TEXT_NODE)
	    {
		char *p_base;
		int len = strlen ((const char *) (node->content));
		char *string = malloc (len + 1);
		strcpy (string, (char *) (node->content));
		p_base = string;
		while (1)
		  {
		      char *p = p_base;
		      while (1)
			{
			    if (*p == ' ' || *p == '\0')
			      {
				  int next = 1;
				  if (*p == '\0')
				      next = 0;
				  *p = '\0';
				  if (strstr (p_base, "DescribeFeatureType") !=
				      NULL)
				    {
					len = strlen (p_base);
					*describe_uri = malloc (len + 1);
					strcpy (*describe_uri, p_base);
					free (string);
					return 1;
				    }
				  if (next)
				      p_base = p + 1;
				  else
				      p_base = p;
				  break;
			      }
			    p++;
			}
		      if (*p_base == '\0')
			  break;
		  }
		free (string);
	    }
      }
    return 0;
}

static int
get_DescribeFeatureType_uri (xmlDocPtr xml_doc, char **describe_uri)
{
/*
/ attempting to retrieve the URI identifying the DescribeFeatureType service
*/
    const char *name;
    xmlNodePtr root = xmlDocGetRootElement (xml_doc);
    struct _xmlAttr *attr;
    if (root == NULL)
	return 0;

    name = (const char *) (root->name);
    if (name != NULL)
      {
	  if (strcmp (name, "FeatureCollection") != 0)
	      return 0;		/* for sure, it's not a WFS answer */
      }

    attr = root->properties;
    while (attr != NULL)
      {
	  if (attr->name != NULL)
	    {
		if (strcmp ((const char *) (attr->name), "schemaLocation") == 0)
		    return find_describe_uri (attr->children, describe_uri);
	    }
	  attr = attr->next;
      }

    return 0;
}

static const char *
parse_attribute_name (xmlNodePtr node)
{
/* parsing the element/name string */
    if (node != NULL)
      {
	  if (node->type == XML_TEXT_NODE)
	      return (const char *) (node->content);
      }
    return NULL;
}

static int
parse_attribute_nillable (xmlNodePtr node)
{
/* parsing the element/nillable string */
    if (node != NULL)
      {
	  if (node->type == XML_TEXT_NODE)
	      if (strcmp ((const char *) (node->content), "false") == 0)
		  return 0;
      }
    return 1;
}

static const char *
clean_xml_prefix (const char *value)
{
/* skipping an eventual XML prefix */
    const char *ptr = value;
    while (1)
      {
	  if (*ptr == '\0')
	      break;
	  if (*ptr == ':')
	      return ptr + 1;
	  ptr++;
      }
    return value;
}

static int
parse_attribute_type (xmlNodePtr node, int *is_geom)
{
/* parsing the element/nillable string */
    *is_geom = 0;
    if (node != NULL)
      {
	  if (node->type == XML_TEXT_NODE)
	    {
		const char *clean =
		    clean_xml_prefix ((const char *) (node->content));
		if (strstr (clean, "Geometry") != NULL)
		  {
		      *is_geom = 1;
		      return GAIA_GEOMETRYCOLLECTION;
		  }
		if (strstr (clean, "MultiPoint") != NULL)
		  {
		      *is_geom = 1;
		      return GAIA_MULTIPOINT;
		  }
		if (strstr (clean, "MultiLineString") != NULL)
		  {
		      *is_geom = 1;
		      return GAIA_MULTILINESTRING;
		  }
		if (strstr (clean, "MultiCurve") != NULL)
		  {
		      *is_geom = 1;
		      return GAIA_MULTILINESTRING;
		  }
		if (strstr (clean, "MultiPolygon") != NULL)
		  {
		      *is_geom = 1;
		      return GAIA_MULTIPOLYGON;
		  }
		if (strstr (clean, "MultiSurface") != NULL)
		  {
		      *is_geom = 1;
		      return GAIA_MULTIPOLYGON;
		  }
		if (strstr (clean, "Point") != NULL)
		  {
		      *is_geom = 1;
		      return GAIA_POINT;
		  }
		if (strstr (clean, "LineString") != NULL)
		  {
		      *is_geom = 1;
		      return GAIA_LINESTRING;
		  }
		if (strstr (clean, "Curve") != NULL)
		  {
		      *is_geom = 1;
		      return GAIA_LINESTRING;
		  }
		if (strstr (clean, "Polygon") != NULL)
		  {
		      *is_geom = 1;
		      return GAIA_POLYGON;
		  }
		if (strstr (clean, "Surface") != NULL)
		  {
		      *is_geom = 1;
		      return GAIA_POLYGON;
		  }
		if (strcmp (clean, "unsignedInt") == 0)
		    return SQLITE_INTEGER;
		if (strcmp (clean, "nonNegativeInteger") == 0)
		    return SQLITE_INTEGER;
		if (strcmp (clean, "negativeInteger") == 0)
		    return SQLITE_INTEGER;
		if (strcmp (clean, "nonPositiveInteger") == 0)
		    return SQLITE_INTEGER;
		if (strcmp (clean, "positiveInteger") == 0)
		    return SQLITE_INTEGER;
		if (strcmp (clean, "integer") == 0)
		    return SQLITE_INTEGER;
		if (strcmp (clean, "int") == 0)
		    return SQLITE_INTEGER;
		if (strcmp (clean, "unsignedShort") == 0)
		    return SQLITE_INTEGER;
		if (strcmp (clean, "short") == 0)
		    return SQLITE_INTEGER;
		if (strcmp (clean, "unsignedLong") == 0)
		    return SQLITE_INTEGER;
		if (strcmp (clean, "long") == 0)
		    return SQLITE_INTEGER;
		if (strcmp (clean, "boolean") == 0)
		    return SQLITE_INTEGER;
		if (strcmp (clean, "unsignedByte") == 0)
		    return SQLITE_INTEGER;
		if (strcmp (clean, "byte") == 0)
		    return SQLITE_INTEGER;
		if (strcmp (clean, "decimal") == 0)
		    return SQLITE_FLOAT;
		if (strcmp (clean, "float") == 0)
		    return SQLITE_FLOAT;
		if (strcmp (clean, "double") == 0)
		    return SQLITE_FLOAT;
	    }
      }
    return SQLITE_TEXT;
}

static void
parse_wfs_schema_element (struct _xmlAttr *attr,
			  struct wfs_layer_schema *schema)
{
/* parsing a WFS attribute / column definition */
    const char *name = NULL;
    int type = SQLITE_NULL;
    int is_nullable = 1;
    int is_geom = 0;

    while (attr != NULL)
      {
	  if (attr->name != NULL)
	    {
		if (strcmp ((const char *) (attr->name), "name") == 0)
		    name = parse_attribute_name (attr->children);
		if (strcmp ((const char *) (attr->name), "nillable") == 0)
		    is_nullable = parse_attribute_nillable (attr->children);
		if (strcmp ((const char *) (attr->name), "type") == 0)
		  {
		      type = parse_attribute_type (attr->children, &is_geom);
		  }
	    }
	  attr = attr->next;
      }

    if (name == NULL || (is_geom == 0 && type == SQLITE_NULL)
	|| (is_geom != 0 && type == GAIA_UNKNOWN))
	return;

    if (is_geom)
	set_wfs_geometry (schema, name, type, is_nullable);
    else
	add_wfs_column_to_schema (schema, name, type, is_nullable);
}

static void
parse_wfs_schema (xmlNodePtr node, struct wfs_layer_schema *schema,
		  int *sequence)
{
/* recursively parsing the WFS layer schema */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (cur_node->name);
		if (name != NULL)
		  {
		      if (strcmp (name, "element") == 0 && *sequence)
			  parse_wfs_schema_element (cur_node->properties,
						    schema);
		      else
			{
			    if (strcmp (name, "sequence") == 0)
				*sequence = 1;
			    parse_wfs_schema (cur_node->children, schema,
					      sequence);
			    if (strcmp (name, "sequence") == 0)
				*sequence = 0;
			}
		  }
	    }
      }
}

static struct wfs_layer_schema *
load_wfs_schema (const char *path_or_url, int swap_axes, char **err_msg)
{
/* attempting to retrieve the WFS layer schema */
    xmlDocPtr xml_doc = NULL;
    xmlNodePtr root;
    int len;
    int sequence = 0;
    struct wfs_layer_schema *schema = NULL;
    gaiaOutBuffer errBuf;
    xmlGenericErrorFunc parsingError = (xmlGenericErrorFunc) wfsParsingError;

    gaiaOutBufferInitialize (&errBuf);
    xmlSetGenericErrorFunc (&errBuf, parsingError);
    xml_doc = xmlReadFile (path_or_url, NULL, 0);
    if (xml_doc == NULL)
      {
	  /* parsing error; not a well-formed XML */
	  if (errBuf.Buffer != NULL)
	    {
		len = strlen (errBuf.Buffer);
		*err_msg = malloc (len + 1);
		strcpy (*err_msg, errBuf.Buffer);
	    }
	  goto end;
      }

    schema = alloc_wfs_layer_schema ();
    schema->swap_axes = swap_axes;
    root = xmlDocGetRootElement (xml_doc);
    parse_wfs_schema (root, schema, &sequence);
    if (schema->first == NULL && schema->geometry_name == NULL)
      {
	  const char *msg = "Unable to identify a valid WFS layer schema";
	  len = strlen (msg);
	  *err_msg = malloc (len + 1);
	  strcpy (*err_msg, msg);
	  free_wfs_layer_schema (schema);
	  schema = NULL;
      }

  end:
    gaiaOutBufferReset (&errBuf);
    xmlSetGenericErrorFunc ((void *) stderr, NULL);
    if (xml_doc != NULL)
	xmlFreeDoc (xml_doc);
    return schema;
}

static void
gml_out (gaiaOutBufferPtr buf, const xmlChar * str)
{
/* clean GML output */
    const xmlChar *p = str;
    while (*p != '\0')
      {
	  if (*p == '>')
	      gaiaAppendToOutBuffer (buf, "&gt;");
	  else if (*p == '<')
	      gaiaAppendToOutBuffer (buf, "&lt;");
	  else if (*p == '&')
	      gaiaAppendToOutBuffer (buf, "&amp;");
	  else if (*p == '"')
	      gaiaAppendToOutBuffer (buf, "&quot;");
	  else if (*p == '\'')
	      gaiaAppendToOutBuffer (buf, "&apos;");
	  else
	    {
		char xx[2];
		xx[0] = *p;
		xx[1] = '\0';
		gaiaAppendToOutBuffer (buf, xx);
	    }
	  p++;
      }
}

static void
reassemble_gml (xmlNodePtr node, gaiaOutBufferPtr buf)
{
/* recursively printing the XML-DOM nodes */
    struct _xmlAttr *attr;
    xmlNodePtr child;
    xmlNs *ns;
    const xmlChar *namespace;
    int has_children;
    int has_text;

    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		gaiaAppendToOutBuffer (buf, "<");
		ns = node->ns;
		namespace = NULL;
		if (ns != NULL)
		    namespace = ns->prefix;
		if (namespace)
		  {
		      gml_out (buf, namespace);
		      gaiaAppendToOutBuffer (buf, ":");
		  }
		gml_out (buf, node->name);
		attr = node->properties;
		while (attr != NULL)
		  {
		      /* attributes */
		      if (attr->type == XML_ATTRIBUTE_NODE)
			{
			    xmlNodePtr text = attr->children;
			    gaiaAppendToOutBuffer (buf, " ");
			    ns = attr->ns;
			    namespace = NULL;
			    if (ns != NULL)
				namespace = ns->prefix;
			    if (namespace)
			      {
				  gml_out (buf, namespace);
				  gaiaAppendToOutBuffer (buf, ":");
			      }
			    gml_out (buf, attr->name);
			    gaiaAppendToOutBuffer (buf, "=\"");
			    if (text != NULL)
			      {
				  if (text->type == XML_TEXT_NODE)
				      gml_out (buf, text->content);
			      }
			    gaiaAppendToOutBuffer (buf, "\"");
			}
		      attr = attr->next;
		  }
		has_children = 0;
		has_text = 0;
		child = node->children;
		while (child)
		  {
		      if (child->type == XML_ELEMENT_NODE)
			  has_children = 1;
		      if (child->type == XML_TEXT_NODE)
			  has_text++;
		      child = child->next;
		  }
		if (has_children)
		    has_text = 0;

		if (!has_text && !has_children)
		    gaiaAppendToOutBuffer (buf, " />");

		if (has_text)
		  {
		      child = node->children;
		      if (child->type == XML_TEXT_NODE)
			{
			    /* text node */
			    gaiaAppendToOutBuffer (buf, ">");
			    gml_out (buf, child->content);
			    gaiaAppendToOutBuffer (buf, "</");
			    ns = node->ns;
			    namespace = NULL;
			    if (ns != NULL)
				namespace = ns->prefix;
			    if (namespace)
			      {
				  gml_out (buf, namespace);
				  gaiaAppendToOutBuffer (buf, ":");
			      }
			    gml_out (buf, node->name);
			    gaiaAppendToOutBuffer (buf, ">");
			}
		  }
		if (has_children)
		  {
		      /* recursively expanding all children */
		      gaiaAppendToOutBuffer (buf, ">");
		      reassemble_gml (node->children, buf);
		      gaiaAppendToOutBuffer (buf, "</");
		      ns = node->ns;
		      namespace = NULL;
		      if (ns != NULL)
			  namespace = ns->prefix;
		      if (namespace)
			{
			    gml_out (buf, namespace);
			    gaiaAppendToOutBuffer (buf, ":");
			}
		      gml_out (buf, node->name);
		      gaiaAppendToOutBuffer (buf, ">");
		  }
	    }
	  node = node->next;
      }
}

static void
set_feature_geom (xmlNodePtr node, struct wfs_layer_schema *schema)
{
/* saving the feature's geometry value */
    gaiaOutBuffer gml;
    gaiaOutBufferInitialize (&gml);

    /* reassembling the GML expression */
    reassemble_gml (node, &gml);
    if (gml.Buffer != NULL)
      {
	  if (schema->geometry_value != NULL)
	      free (schema->geometry_value);
	  schema->geometry_value = gml.Buffer;
      }
}

static void
set_feature_value (xmlNodePtr node, struct wfs_column_def *col)
{
/* saving an attribute value */
    if (node->type == XML_TEXT_NODE)
      {
	  int len = strlen ((const char *) node->content);
	  char *buf = malloc (len + 1);
	  strcpy (buf, (const char *) node->content);
	  if (col->value != NULL)
	      free (col->value);
	  col->value = buf;
      }
}

static void
check_feature_value (xmlNodePtr node, struct wfs_layer_schema *schema)
{
/* attempting to extract an attribute value */
    struct wfs_column_def *col;
    if (strcmp ((const char *) (node->name), schema->geometry_name) == 0)
      {
	  set_feature_geom (node->children, schema);
	  return;
      }
    col = schema->first;
    while (col != NULL)
      {
	  if (strcmp ((const char *) (node->name), col->name) == 0)
	    {
		set_feature_value (node->children, col);
		return;
	    }
	  col = col->next;
      }
}

static int
parse_wfs_single_feature (xmlNodePtr node, struct wfs_layer_schema *schema)
{
/* attempting to extract data corresponding to a single feature */
    xmlNodePtr cur_node = NULL;
    int cnt = 0;

    reset_wfs_values (schema);
    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	      check_feature_value (cur_node, schema);
      }
    cnt = count_wfs_values (schema);
    return cnt;
}

static int
do_insert (struct wfs_layer_schema *schema)
{
/* inserting a row into the target table */
    int ret;
    int ind = 1;
    sqlite3_stmt *stmt = schema->stmt;
    struct wfs_column_def *col;

    if (stmt == NULL || schema->error)
      {
	  schema->error = 1;
	  return 0;
      }

/* binding */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    col = schema->first;
    while (col != NULL)
      {
	  if (col->value == NULL)
	      sqlite3_bind_null (stmt, ind);
	  else
	    {
		if (col->type == SQLITE_INTEGER)
		  {
		      sqlite3_int64 val = atoll (col->value);
		      sqlite3_bind_int64 (stmt, ind, val);
		  }
		else if (col->type == SQLITE_FLOAT)
		  {
		      double val = atof (col->value);
		      sqlite3_bind_double (stmt, ind, val);
		  }
		else
		    sqlite3_bind_text (stmt, ind, col->value,
				       strlen (col->value), SQLITE_STATIC);
	    }
	  ind++;
	  col = col->next;
      }
    if (schema->geometry_name != NULL)
      {
	  /* we have a Geometry column */
	  if (schema->geometry_value != NULL)
	    {
		/* preparing the Geometry value */
		gaiaGeomCollPtr geom =
		    gaiaParseGml ((unsigned char *) (schema->geometry_value),
				  schema->sqlite);
		if (geom == NULL)
		    sqlite3_bind_null (stmt, ind);
		else
		  {
		      unsigned char *blob;
		      int blob_size;
		      geom->Srid = schema->srid;
		      if (schema->swap_axes != 0)
			  gaiaSwapCoords (geom);
		      gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
		      sqlite3_bind_blob (stmt, ind, blob, blob_size, free);
		      gaiaFreeGeomColl (geom);
		  }
	    }
	  else
	      sqlite3_bind_null (stmt, ind);
      }

/* inserting */
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	return 1;
    spatialite_e ("loadwfs INSERT error: <%s>\n",
		  sqlite3_errmsg (schema->sqlite));
    schema->error = 1;
    return 0;
}

static void
save_attribute (struct wfs_feature *feature, struct wfs_column_def *col)
{
/* saving an attribute value */
    struct wfs_attribute *attr = feature->first;
    while (attr != NULL)
      {
	  if (attr->column == col)
	    {
		if (attr->value != NULL)
		    free (attr->value);
		attr->value = NULL;
		if (col->value != NULL)
		  {
		      int len = strlen (col->value);
		      attr->value = malloc (len + 1);
		      strcpy (attr->value, col->value);
		  }
		return;
	    }
	  attr = attr->next;
      }
}

static int
do_save_feature (struct wfs_layer_schema *schema, struct wfs_feature *feature)
{
/* saving the current feature data */
    int ret;
    struct wfs_column_def *col;

    if (schema->error)
      {
	  schema->error = 1;
	  return 0;
      }

    reset_feature (feature);
    col = schema->first;
    while (col != NULL)
      {
	  save_attribute (feature, col);
	  col = col->next;
      }
    if (schema->geometry_name != NULL)
      {
	  /* we have a Geometry column */
	  if (schema->geometry_value != NULL)
	    {
		int len = strlen (schema->geometry_value);
		feature->geometry_value = malloc (len + 1);
		strcpy (feature->geometry_value, schema->geometry_value);
	    }
      }
    return 1;
}

static void
parse_wfs_features (xmlNodePtr node, struct wfs_layer_schema *schema, int *rows)
{
/* recursively parsing the GML payload */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (parse_wfs_single_feature (cur_node, schema))
		  {
		      if (schema->error == 0)
			{
			    if (do_insert (schema))
				*rows += 1;
			}
		      return;
		  }
		else
		    parse_wfs_features (cur_node->children, schema, rows);
	    }
      }
}

static void
parse_wfs_last_feature (xmlNodePtr node, struct wfs_layer_schema *schema,
			struct wfs_feature *feature, int *rows)
{
/* recursively parsing the GML payload */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (parse_wfs_single_feature (cur_node, schema))
		  {
		      if (schema->error == 0)
			{
			    if (do_save_feature (schema, feature))
				*rows += 1;
			}
		      return;
		  }
		else
		    parse_wfs_last_feature (cur_node->children, schema, feature,
					    rows);
	    }
      }
}

static int
sniff_feature_value (xmlNodePtr node, struct wfs_layer_schema *schema,
		     xmlNodePtr * geom)
{
/* sniffing attribute values */
    struct wfs_column_def *col;
    if (strcmp ((const char *) (node->name), schema->geometry_name) == 0)
      {
	  *geom = node->children;
	  return 1;
      }
    col = schema->first;
    while (col != NULL)
      {
	  if (strcmp ((const char *) (node->name), col->name) == 0)
	      return 1;
	  col = col->next;
      }
    return 0;
}

static int
parse_srsname (xmlNodePtr node)
{
/* parsing the srsName string */
    if (node != NULL)
      {
	  if (node->type == XML_TEXT_NODE)
	    {
		int len = strlen ((const char *) (node->content));
		const char *end = (const char *) (node->content) + len;
		const char *p = end;
		if (len > 0)
		  {
		      p--;
		      while (p >= (char *) (node->content))
			{
			    if (*p >= '0' && *p <= '9')
			      {
				  p--;
				  continue;
			      }
			    if (p + 1 < end)
				return atoi (p + 1);
			}
		  }
	    }
      }
    return -1;
}

static int
parse_dimension (xmlNodePtr node)
{
/* parsing the dimension string */
    if (node != NULL)
      {
	  if (node->type == XML_TEXT_NODE)
	      return atoi ((const char *) (node->content));
      }
    return 2;
}

static void
sniff_gml_geometry (xmlNodePtr node, struct wfs_layer_schema *schema)
{
/* attempting to identify the Srid and dimension from a GML geometry */
    xmlNodePtr cur_node = NULL;
    if (node == NULL)
	return;
    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		struct _xmlAttr *attr;
		attr = cur_node->properties;
		while (attr != NULL)
		  {
		      if (attr->name != NULL)
			{
			    if (strcmp ((const char *) (attr->name), "srsName")
				== 0)
				schema->srid = parse_srsname (attr->children);
			    if (strcmp
				((const char *) (attr->name), "dimension") == 0)
				schema->dims = parse_dimension (attr->children);
			}
		      attr = attr->next;
		  }
		sniff_gml_geometry (cur_node->children, schema);
	    }
      }
}

static int
sniff_wfs_single_feature (xmlNodePtr node, struct wfs_layer_schema *schema)
{
/* attempting to sniff data corresponding to a single feature */
    xmlNodePtr cur_node = NULL;
    int cnt = 0;
    xmlNodePtr geom = NULL;

    reset_wfs_values (schema);
    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	      cnt += sniff_feature_value (cur_node, schema, &geom);
      }
    if (cnt > 0 && geom != NULL)
      {
	  sniff_gml_geometry (geom, schema);
	  return 1;
      }
    return 0;
}

static void
sniff_geometries (xmlNodePtr node, struct wfs_layer_schema *schema,
		  int *sniffed)
{
/* recursively parsing the GML payload so to sniff the first geometry */
    xmlNodePtr cur_node = NULL;
    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (*sniffed)
		    return;
		if (sniff_wfs_single_feature (cur_node, schema))
		  {
		      *sniffed = 1;
		      return;
		  }
		else
		    sniff_geometries (cur_node->children, schema, sniffed);
	    }
      }
}

static int
check_pk_name (struct wfs_layer_schema *schema, const char *pk_column_name,
	       char *auto_pk_name)
{
/* handling the PK name */
    int num = 0;
    char pk_candidate[1024];
    struct wfs_column_def *col;
    if (pk_column_name == NULL)
	goto check_auto;
    col = schema->first;
    while (col != NULL)
      {
	  /* checking if the required PK does really exists */
	  if (strcasecmp (col->name, pk_column_name) == 0)
	      return 1;
	  col = col->next;
      }
  check_auto:
    strcpy (pk_candidate, auto_pk_name);
    while (1)
      {
	  /* ensuring to return a not ambiguous PK name */
	  int ok = 1;
	  col = schema->first;
	  while (col != NULL)
	    {
		/* checking if the PK candidate isn't already defined */
		if (strcasecmp (col->name, pk_candidate) == 0)
		  {
		      ok = 0;
		      break;
		  }
		col = col->next;
	    }
	  if (ok)
	    {
		strcpy (auto_pk_name, pk_candidate);
		break;
	    }
	  sprintf (pk_candidate, "%s_%d", auto_pk_name, num);
	  num++;
      }
    return 0;
}

static int
prepare_sql (sqlite3 * sqlite, struct wfs_layer_schema *schema,
	     const char *table, const char *pk_column_name, int spatial_index,
	     char **err_msg)
{
/* creating the output table and preparing the insert statement */
    int len;
    int ret;
    char *errMsg = NULL;
    gaiaOutBuffer sql;
    char *sql2;
    char *quoted;
    struct wfs_column_def *col;
    char auto_pk_name[1024];
    int is_auto_pk = 0;
    sqlite3_stmt *stmt = NULL;

/* attempting to create the SQL Table */
    gaiaOutBufferInitialize (&sql);
    quoted = gaiaDoubleQuotedSql (table);
    sql2 = sqlite3_mprintf ("CREATE TABLE \"%s\" (\n", quoted);
    free (quoted);
    gaiaAppendToOutBuffer (&sql, sql2);
    sqlite3_free (sql2);
    strcpy (auto_pk_name, "pk_uid");
    if (!check_pk_name (schema, pk_column_name, auto_pk_name))
      {
	  /* defining a default Primary Key */
	  is_auto_pk = 1;
	  quoted = gaiaDoubleQuotedSql (auto_pk_name);
	  sql2 =
	      sqlite3_mprintf ("\t\"%s\" INTEGER PRIMARY KEY AUTOINCREMENT,\n",
			       quoted);
	  free (quoted);
	  gaiaAppendToOutBuffer (&sql, sql2);
	  sqlite3_free (sql2);
      }
    col = schema->first;
    while (col != NULL)
      {
	  const char *type = "TEXT";
	  if (col->type == SQLITE_INTEGER)
	      type = "INTEGER";
	  if (col->type == SQLITE_FLOAT)
	      type = "DOUBLE";
	  quoted = gaiaDoubleQuotedSql (col->name);
	  if (!is_auto_pk)
	    {
		/* there is an explicitly defined PK */
		if (strcasecmp (col->name, pk_column_name) == 0)
		  {
		      /* ok, we've found the PK column */
		      if (col == schema->last)
			  sql2 =
			      sqlite3_mprintf ("\t\"%s\" %s PRIMARY KEY)",
					       quoted, type);
		      else
			  sql2 =
			      sqlite3_mprintf ("\t\"%s\" %s PRIMARY KEY,\n",
					       quoted, type);
		      free (quoted);
		      gaiaAppendToOutBuffer (&sql, sql2);
		      sqlite3_free (sql2);
		      col = col->next;
		      continue;
		  }
	    }
	  if (col == schema->last)
	    {
		if (col->is_nullable)
		    sql2 = sqlite3_mprintf ("\t\"%s\" %s)", quoted, type);
		else
		    sql2 =
			sqlite3_mprintf ("\t\"%s\" %s NOT NULL)", quoted, type);
	    }
	  else
	    {
		if (col->is_nullable)
		    sql2 = sqlite3_mprintf ("\t\"%s\" %s,\n", quoted, type);
		else
		    sql2 =
			sqlite3_mprintf ("\t\"%s\" %s NOT NULL,\n", quoted,
					 type);
	    }
	  free (quoted);
	  gaiaAppendToOutBuffer (&sql, sql2);
	  sqlite3_free (sql2);
	  col = col->next;
      }
    ret = sqlite3_exec (sqlite, sql.Buffer, NULL, NULL, &errMsg);
    gaiaOutBufferReset (&sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("loadwfs: CREATE TABLE '%s' error: %s\n", table,
			errMsg);
	  len = strlen (errMsg);
	  *err_msg = malloc (len + 1);
	  strcpy (*err_msg, errMsg);
	  sqlite3_free (errMsg);
	  schema->error = 1;
	  return 0;
      }

    if (schema->geometry_name != NULL)
      {
	  /* creating the Geometry column */
	  const char *gType = "GEOMETRY";
	  const char *gDims = "XY";
	  switch (schema->geometry_type)
	    {
	    case GAIA_POINT:
		gType = "POINT";
		break;
	    case GAIA_LINESTRING:
		gType = "LINESTRING";
		break;
	    case GAIA_POLYGON:
		gType = "POLYGON";
		break;
	    case GAIA_MULTIPOINT:
		gType = "MULTIPOINT";
		break;
	    case GAIA_MULTILINESTRING:
		gType = "MULTILINESTRING";
		break;
	    case GAIA_MULTIPOLYGON:
		gType = "MULTIPOLYGON";
		break;
	    };
	  if (schema->dims == 3)
	      gDims = "XYZ";
	  sql2 =
	      sqlite3_mprintf ("SELECT AddGeometryColumn(%Q, %Q, %d, %Q, %Q)",
			       table, schema->geometry_name, schema->srid,
			       gType, gDims);
	  gaiaAppendToOutBuffer (&sql, sql2);
	  sqlite3_free (sql2);
	  ret = sqlite3_exec (sqlite, sql.Buffer, NULL, NULL, &errMsg);
	  gaiaOutBufferReset (&sql);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("loadwfs: AddGeometryColumn error: %s\n", errMsg);
		len = strlen (errMsg);
		*err_msg = malloc (len + 1);
		strcpy (*err_msg, errMsg);
		sqlite3_free (errMsg);
		schema->error = 1;
		return 0;
	    }
	  if (spatial_index)
	    {
		/* creating the Spatial Index */
		sql2 = sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, %Q)",
					table, schema->geometry_name);
		gaiaAppendToOutBuffer (&sql, sql2);
		sqlite3_free (sql2);
		ret = sqlite3_exec (sqlite, sql.Buffer, NULL, NULL, &errMsg);
		gaiaOutBufferReset (&sql);
		if (ret != SQLITE_OK)
		  {
		      spatialite_e ("loadwfs: CreateSpatialIndex error: %s\n",
				    errMsg);
		      len = strlen (errMsg);
		      *err_msg = malloc (len + 1);
		      strcpy (*err_msg, errMsg);
		      sqlite3_free (errMsg);
		      schema->error = 1;
		      return 0;
		  }
	    }
      }

/* creating the INSERT statement */
    quoted = gaiaDoubleQuotedSql (table);
    sql2 = sqlite3_mprintf ("INSERT INTO \"%s\" (\n", quoted);
    free (quoted);
    gaiaAppendToOutBuffer (&sql, sql2);
    sqlite3_free (sql2);
    if (is_auto_pk)
      {
	  /* the auto-generated PK column */
	  quoted = gaiaDoubleQuotedSql (auto_pk_name);
	  sql2 = sqlite3_mprintf ("\"%s\", ", quoted);
	  free (quoted);
	  gaiaAppendToOutBuffer (&sql, sql2);
	  sqlite3_free (sql2);
      }
    col = schema->first;
    while (col != NULL)
      {
	  /* column names */
	  quoted = gaiaDoubleQuotedSql (col->name);
	  if (col == schema->last && schema->geometry_name == NULL)
	      sql2 = sqlite3_mprintf ("\"%s\") VALUES (", quoted);
	  else
	      sql2 = sqlite3_mprintf ("\"%s\", ", quoted);
	  free (quoted);
	  gaiaAppendToOutBuffer (&sql, sql2);
	  sqlite3_free (sql2);
	  col = col->next;
      }
    if (schema->geometry_name != NULL)
      {
	  /* the geometry column name */
	  quoted = gaiaDoubleQuotedSql (schema->geometry_name);
	  sql2 = sqlite3_mprintf ("\"%s\") VALUES (", quoted);
	  free (quoted);
	  gaiaAppendToOutBuffer (&sql, sql2);
	  sqlite3_free (sql2);
      }
    if (is_auto_pk)
      {
	  /* there is an AUTOINCREMENT PK */
	  gaiaAppendToOutBuffer (&sql, "NULL, ");
      }
    col = schema->first;
    while (col != NULL)
      {
	  if (col == schema->last && schema->geometry_name == NULL)
	      gaiaAppendToOutBuffer (&sql, "?)");
	  else
	      gaiaAppendToOutBuffer (&sql, "?, ");
	  col = col->next;
      }
    if (schema->geometry_name != NULL)
	gaiaAppendToOutBuffer (&sql, "?)");
    ret =
	sqlite3_prepare_v2 (sqlite, sql.Buffer, strlen (sql.Buffer), &stmt,
			    NULL);
    gaiaOutBufferReset (&sql);
    if (ret != SQLITE_OK)
      {
	  errMsg = (char *) sqlite3_errmsg (sqlite);
	  spatialite_e ("loadwfs: \"%s\"\n", errMsg);
	  len = strlen (errMsg);
	  *err_msg = malloc (len + 1);
	  strcpy (*err_msg, errMsg);
	  schema->error = 1;
	  return 0;
      }
    schema->stmt = stmt;
    schema->sqlite = sqlite;

/* starting an SQL Transaction */
    if (sqlite3_exec (sqlite, "BEGIN", NULL, NULL, &errMsg) != SQLITE_OK)
      {
	  spatialite_e ("loadwfs: BEGIN error:\"%s\"\n", errMsg);
	  len = strlen (errMsg);
	  *err_msg = malloc (len + 1);
	  strcpy (*err_msg, errMsg);
	  sqlite3_free (errMsg);
	  schema->error = 1;
      }
    if (schema->error)
	return 0;
    return 1;
}

static void
restart_transaction (sqlite3 * sqlite)
{
/* confirming the still pendenting SQL transaction */
    char *errMsg = NULL;
    if (sqlite3_exec (sqlite, "COMMIT", NULL, NULL, &errMsg) != SQLITE_OK)
      {
	  spatialite_e ("loadwfs: COMMIT error:\"%s\"\n", errMsg);
	  sqlite3_free (errMsg);
      }
/* restarting a further transaction */
    if (sqlite3_exec (sqlite, "BEGIN", NULL, NULL, &errMsg) != SQLITE_OK)
      {
	  spatialite_e ("loadwfs: BEGIN error:\"%s\"\n", errMsg);
	  sqlite3_free (errMsg);
      }
}

static void
do_commit (sqlite3 * sqlite, struct wfs_layer_schema *schema)
{
/* confirming the still pendenting SQL transaction */
    char *errMsg = NULL;
    sqlite3_finalize (schema->stmt);
    schema->stmt = NULL;
    if (sqlite3_exec (sqlite, "COMMIT", NULL, NULL, &errMsg) != SQLITE_OK)
      {
	  spatialite_e ("loadwfs: COMMIT error:\"%s\"\n", errMsg);
	  sqlite3_free (errMsg);
	  schema->error = 1;
      }
}

static void
do_rollback (sqlite3 * sqlite, struct wfs_layer_schema *schema)
{
/* invalidating the still pending SQL transaction */
    char *errMsg = NULL;
    sqlite3_finalize (schema->stmt);
    schema->stmt = NULL;
    if (sqlite3_exec (sqlite, "ROLLBACK", NULL, NULL, &errMsg) != SQLITE_OK)
      {
	  spatialite_e ("loadwfs: ROLLBACK error:\"%s\"\n", errMsg);
	  sqlite3_free (errMsg);
	  schema->error = 1;
      }
}

SPATIALITE_DECLARE int
load_from_wfs (sqlite3 * sqlite, char *path_or_url, int swap_axes,
	       char *table, char *pk_column_name, int spatial_index, int *rows,
	       char **err_msg, void (*progress_callback) (int))
{
/* attempting to load data from some WFS source [not-paged] */
    return load_from_wfs_paged (sqlite, path_or_url, swap_axes, table,
				pk_column_name, spatial_index, -1, rows,
				err_msg, progress_callback);
}

static int
test_wfs_paging (char *path_or_url, int page_size, xmlNodePtr node,
		 struct wfs_layer_schema *schema, int *shift_index)
{
/* testing if the server does actually supports STARTINDEX */
    xmlDocPtr xml_doc = NULL;
    xmlNodePtr root;
    char *page_url;
    int nRows = 0;
    struct wfs_feature *feature_1 = create_feature (schema);
    struct wfs_feature *feature_2 = create_feature (schema);
    *shift_index = 0;
    parse_wfs_last_feature (node, schema, feature_1, &nRows);

/* loading the feature to be tested */
    page_url = sqlite3_mprintf ("%s&maxFeatures=1&startIndex=%d",
				path_or_url, page_size - 1);
    xml_doc = xmlReadFile (page_url, NULL, 0);
    sqlite3_free (page_url);
    if (xml_doc == NULL)
	goto error;
    /* parsing the WFS payload */
    root = xmlDocGetRootElement (xml_doc);
    nRows = 0;
    parse_wfs_last_feature (root, schema, feature_2, &nRows);
    if (!compare_features (feature_1, feature_2))
      {
	  reset_feature (feature_2);
	  if (xml_doc != NULL)
	      xmlFreeDoc (xml_doc);
	  goto second_chance;
      }
    free_feature (feature_1);
    free_feature (feature_2);
    if (xml_doc != NULL)
	xmlFreeDoc (xml_doc);
    return 1;

/* for some MapServer version the first Index is 1, not 0 */
  second_chance:
    page_url = sqlite3_mprintf ("%s&maxFeatures=1&startIndex=%d",
				path_or_url, page_size);
    xml_doc = xmlReadFile (page_url, NULL, 0);
    sqlite3_free (page_url);
    if (xml_doc == NULL)
	goto error;
    /* parsing the WFS payload */
    root = xmlDocGetRootElement (xml_doc);
    nRows = 0;
    parse_wfs_last_feature (root, schema, feature_2, &nRows);
    if (!compare_features (feature_1, feature_2))
	goto error;
    free_feature (feature_1);
    free_feature (feature_2);
    if (xml_doc != NULL)
	xmlFreeDoc (xml_doc);
    *shift_index = 1;
    return 1;
  error:
    free_feature (feature_1);
    free_feature (feature_2);
    if (xml_doc != NULL)
	xmlFreeDoc (xml_doc);
    return 0;
}

SPATIALITE_DECLARE int
load_from_wfs_paged (sqlite3 * sqlite, char *path_or_url, int swap_axes,
		     char *table, char *pk_column_name, int spatial_index,
		     int page_size, int *rows, char **err_msg,
		     void (*progress_callback) (int))
{
/* attempting to load data from some WFS source [paged]*/
    xmlDocPtr xml_doc = NULL;
    xmlNodePtr root;
    struct wfs_layer_schema *schema = NULL;
    int len;
    char *describe_uri = NULL;
    gaiaOutBuffer errBuf;
    int ok = 0;
    int sniffed = 0;
    int pageNo = 0;
    int startIdx = 0;
    int nRows;
    char *page_url;
    int shift_index;
    xmlGenericErrorFunc parsingError = (xmlGenericErrorFunc) wfsParsingError;
    *rows = 0;
    *err_msg = NULL;
    if (path_or_url == NULL)
	return 0;

    while (1)
      {
	  if (page_size <= 0)
	      page_url = path_or_url;
	  else
	      page_url =
		  sqlite3_mprintf ("%s&maxFeatures=%d&startIndex=%d",
				   path_or_url, page_size, startIdx);

	  /* loading the WFS payload from URL (or file) */
	  gaiaOutBufferInitialize (&errBuf);
	  xmlSetGenericErrorFunc (&errBuf, parsingError);
	  xml_doc = xmlReadFile (page_url, NULL, 0);
	  if (page_size > 0)
	      sqlite3_free (page_url);
	  if (xml_doc == NULL)
	    {
		/* parsing error; not a well-formed XML */
		if (errBuf.Buffer != NULL)
		  {
		      len = strlen (errBuf.Buffer);
		      *err_msg = malloc (len + 1);
		      strcpy (*err_msg, errBuf.Buffer);
		  }
		goto end;
	    }

	  if (pageNo == 0)
	    {
		/* extracting the WFS schema URL (or path) */
		if (get_DescribeFeatureType_uri (xml_doc, &describe_uri) == 0)
		  {
		      const char *msg =
			  "Unable to retrieve the DescribeFeatureType URI";
		      len = strlen (msg);
		      *err_msg = malloc (len + 1);
		      strcpy (*err_msg, msg);
		      goto end;
		  }

		/* loading and parsing the WFS schema */
		schema = load_wfs_schema (describe_uri, swap_axes, err_msg);
		if (schema == NULL)
		    goto end;

		/* creating the output table */
		root = xmlDocGetRootElement (xml_doc);
		sniffed = 0;
		sniff_geometries (root, schema, &sniffed);

		if (page_size > 0)
		  {
		      /* testing if the server does actually support STARTINDEX */
		      root = xmlDocGetRootElement (xml_doc);
		      if (!test_wfs_paging
			  (path_or_url, page_size, root, schema, &shift_index))
			{
			    const char *err =
				"loawfs: the WFS server doesn't seem to support STARTINDEX\n"
				"and consequently WFS paging is not available";
			    len = strlen (err);
			    *err_msg = malloc (len + 1);
			    strcpy (*err_msg, err);
			    goto end;
			}
		      startIdx += shift_index;
		  }

		if (!prepare_sql
		    (sqlite, schema, table, pk_column_name, spatial_index,
		     err_msg))
		    goto end;
	    }

	  /* parsing the WFS payload */
	  root = xmlDocGetRootElement (xml_doc);
	  nRows = 0;
	  parse_wfs_features (root, schema, &nRows);
	  *rows += nRows;
	  if (progress_callback != NULL)
	    {
		/* invoking the progress callback */
		int ext_rows = *rows;
		progress_callback (ext_rows);
	    }

	  if (schema->error)
	    {
		*rows = 0;
		do_rollback (sqlite, schema);
	    }
	  else
	    {
		if (page_size > 0 && nRows >= page_size)
		    restart_transaction (sqlite);
		else
		    do_commit (sqlite, schema);
	    }
	  if (schema->error)
	    {
		*rows = 0;
		goto end;
	    }
	  if (page_size <= 0)
	      break;
	  if (nRows < page_size)
	      break;

	  if (xml_doc != NULL)
	      xmlFreeDoc (xml_doc);
	  xml_doc = NULL;
	  pageNo++;
	  startIdx += nRows;
      }

    ok = 1;
  end:
    if (schema != NULL)
	free_wfs_layer_schema (schema);
    if (describe_uri != NULL)
	free (describe_uri);
    gaiaOutBufferReset (&errBuf);
    xmlSetGenericErrorFunc ((void *) stderr, NULL);
    if (xml_doc != NULL)
	xmlFreeDoc (xml_doc);
    return ok;
}

static void
parse_keyword (xmlNodePtr node, struct wfs_catalog *catalog)
{
/* parsing WFS <Keyword> values */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) cur_node->name, "Keyword") == 0)
		  {
		      xmlNodePtr child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
			      {
				  struct wfs_layer_def *lyr = catalog->last;
				  add_wfs_keyword_to_layer (lyr,
							    (const char
							     *)
							    (child_node->
							     content));
			      }
			}
		  }
	    }
      }
}

static void
parse_keyword_string (char *in, struct wfs_catalog *catalog)
{
/* parsing WFS <Keyword> from within a comma delimited string */
    struct wfs_layer_def *lyr = catalog->last;
    int len = strlen (in);
    char *end = in + len;
    char *base = in;
    while (base < end)
      {
	  char *p = base;
	  while (p <= end)
	    {
		if (*p == ',' || *p == '\0')
		  {
		      char *start;
		      *p = '\0';
		      start = base;
		      while (*start == ' ' || *start == '\t' || *start == '\n'
			     || *start == '\r')
			  start++;
		      add_wfs_keyword_to_layer (lyr, start);
		      p++;
		      base = p;
		      break;
		  }
		p++;
	    }
      }
}

static void
parse_keywords (xmlNodePtr node, struct wfs_catalog *catalog)
{
/* parsing the WFS Keywords */
    if (node != NULL)
      {
	  parse_keyword (node, catalog);
	  if (node->type == XML_TEXT_NODE)
	    {
		int len = strlen ((const char *) (node->content));
		char *string = malloc (len + 1);
		strcpy (string, (const char *) (node->content));
		parse_keyword_string (string, catalog);
		free (string);
	    }
      }
}

static void
parse_wfs_layer (xmlNodePtr node, struct wfs_catalog *catalog)
{
/* parsing a WFS layer definition (GetCapabilities/FeatureType) */
    xmlNodePtr cur_node = NULL;
    xmlNodePtr child_node = NULL;
    const char *name = NULL;
    const char *title = NULL;
    const char *abstract = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "Name") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				name = (const char *) (child_node->content);
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Title") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				title = (const char *) (child_node->content);
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Abstract") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				abstract = (const char *) (child_node->content);
			}
		  }
	    }
      }

    if (name != NULL)
      {
	  add_wfs_layer_to_catalog (catalog, name, title, abstract);
	  for (cur_node = node; cur_node; cur_node = cur_node->next)
	    {
		if (cur_node->type == XML_ELEMENT_NODE)
		  {
		      if (strcmp ((const char *) (cur_node->name), "SRS") == 0
			  || strcmp ((const char *) (cur_node->name),
				     "DefaultSRS") == 0
			  || strcmp ((const char *) (cur_node->name),
				     "OtherSRS") == 0)
			{
			    int srid = parse_srsname (cur_node->children);
			    if (srid > 0)
			      {
				  struct wfs_layer_def *lyr = catalog->last;
				  add_wfs_srid_to_layer (lyr, srid,
							 cur_node->
							 children->content);
			      }
			}
		      if (strcmp ((const char *) (cur_node->name), "Keywords")
			  == 0)
			  parse_keywords (cur_node->children, catalog);
		  }
	    }
      }
}

static void
parse_wfs_get_100 (xmlNodePtr node, struct wfs_catalog *catalog)
{
/* attempting to find the GetFeature base-URL (WFS 1.0.0) <Get> */
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  if (attr->name != NULL)
	    {
		if (strcmp ((const char *) (attr->name), "onlineResource") == 0)
		  {
		      xmlNodePtr text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
				set_wfs_catalog_url (catalog,
						     (const char
						      *) (text->content));
			}
		  }
	    }
	  attr = attr->next;
      }
}

static void
parse_wfs_http_100 (xmlNodePtr node, struct wfs_catalog *catalog)
{
/* attempting to find the GetFeature base-URL (WFS 1.0.0) <HTTP> */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "Get") == 0)
		    parse_wfs_get_100 (node, catalog);
	    }
      }
}

static void
parse_wfs_dcptype_100 (xmlNodePtr node, struct wfs_catalog *catalog)
{
/* attempting to find the GetFeature base-URL (WFS 1.0.0) <DCPType> */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "HTTP") == 0)
		    parse_wfs_http_100 (node->children, catalog);
	    }
      }
}

static void
parse_wfs_getfeature_100 (xmlNodePtr node, struct wfs_catalog *catalog)
{
/* attempting to find the GetFeature base-URL (WFS 1.0.0) <GetFeature> */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "DCPType") == 0)
		    parse_wfs_dcptype_100 (node->children, catalog);
	    }
      }
}

static void
parse_wfs_request_100 (xmlNodePtr node, struct wfs_catalog *catalog)
{
/* attempting to find the GetFeature base-URL (WFS 1.0.0) <Request> */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "GetFeature") == 0)
		    parse_wfs_getfeature_100 (node->children, catalog);
	    }
      }
}

static void
parse_wfs_base_url_100 (xmlNodePtr node, struct wfs_catalog *catalog)
{
/* attempting to find the GetFeature base-URL (WFS 1.0.0) <Capability> */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "Request") == 0)
		    parse_wfs_request_100 (node->children, catalog);
	    }
      }
}

static void
parse_wfs_get_110 (xmlNodePtr node, struct wfs_catalog *catalog)
{
/* attempting to find the GetFeature base-URL (WFS 1.1.0) <Get> */
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  if (attr->name != NULL)
	    {
		if (strcmp ((const char *) (attr->name), "href") == 0)
		  {
		      xmlNodePtr text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
				set_wfs_catalog_url (catalog,
						     (const char
						      *) (text->content));
			}
		  }
	    }
	  attr = attr->next;
      }
}

static void
parse_wfs_http_110 (xmlNodePtr node, struct wfs_catalog *catalog)
{
/* attempting to find the GetFeature base-URL (WFS 1.1.0) <HTTP> */
    xmlNodePtr cur_node = NULL;
    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "Get") == 0)
		    parse_wfs_get_110 (cur_node, catalog);
	    }
      }
}

static void
parse_wfs_dcp_110 (xmlNodePtr node, struct wfs_catalog *catalog)
{
/* attempting to find the GetFeature base-URL (WFS 1.1.0) <DCP> */
    xmlNodePtr cur_node = NULL;
    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "HTTP") == 0)
		    parse_wfs_http_110 (cur_node->children, catalog);
	    }
      }
}

static void
parse_wfs_getfeature_110 (xmlNodePtr node, struct wfs_catalog *catalog)
{
/* attempting to find the GetFeature base-URL (WFS 1.1.0) <Operation name="GetFeature"> */
    xmlNodePtr cur_node = NULL;
    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "DCP") == 0)
		    parse_wfs_dcp_110 (cur_node->children, catalog);
	    }
      }
}

static void
parse_wfs_operation_110 (xmlNodePtr node, struct wfs_catalog *catalog)
{
/* attempting to find the GetFeature base-URL (WFS 1.1.0) <Operation name="GetFeature"> */
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  if (attr->name != NULL)
	    {
		if (strcmp ((const char *) (attr->name), "name") == 0)
		  {
		      xmlNodePtr text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  if (strcmp
				      ((const char *) (text->content),
				       "GetFeature") == 0)
				      parse_wfs_getfeature_110 (node->children,
								catalog);
			      }
			}
		  }
	    }
	  attr = attr->next;
      }
}

static void
parse_wfs_base_url_110 (xmlNodePtr node, struct wfs_catalog *catalog)
{
/* attempting to find the GetFeature base-URL (WFS 1.1.0) <OperationsMetadata> */
    xmlNodePtr cur_node = NULL;
    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "Operation") == 0)
		    parse_wfs_operation_110 (cur_node, catalog);
	    }
      }
}

static void
parse_wfs_catalog (xmlNodePtr node, struct wfs_catalog *catalog,
		   int *capabilities, int *list)
{
/* recursively parsing the GetCapabilities payload */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "WFS_Capabilities")
		    == 0)
		    *capabilities = 1;
		if (*capabilities != 0
		    && strcmp ((const char *) (cur_node->name),
			       "FeatureTypeList") == 0)
		    *list = 1;
		if (*capabilities != 0 && *list == 0
		    && strcmp ((const char *) (cur_node->name),
			       "Capability") == 0)
		    parse_wfs_base_url_100 (cur_node->children, catalog);
		if (*capabilities != 0 && *list == 0
		    && strcmp ((const char *) (cur_node->name),
			       "OperationsMetadata") == 0)
		    parse_wfs_base_url_110 (cur_node->children, catalog);
		if (*list != 0
		    && strcmp ((const char *) (cur_node->name),
			       "FeatureType") == 0)
		    parse_wfs_layer (cur_node->children, catalog);
		else
		    parse_wfs_catalog (cur_node->children, catalog,
				       capabilities, list);
		if (*capabilities != 0
		    && strcmp ((const char *) (cur_node->name),
			       "FeatureTypeList") == 0)
		    *list = 0;
		if (strcmp ((const char *) (cur_node->name), "WFS_Capabilities")
		    == 0)
		    *capabilities = 0;
	    }
      }
}

SPATIALITE_DECLARE gaiaWFScatalogPtr
create_wfs_catalog (char *path_or_url, char **err_msg)
{
/* attempting to get and parse a WFS GetCapabilities request */
    xmlDocPtr xml_doc = NULL;
    xmlNodePtr root;
    struct wfs_catalog *catalog = NULL;
    int len;
    int capabilities = 0;
    int list = 0;
    gaiaOutBuffer errBuf;
    xmlGenericErrorFunc parsingError = (xmlGenericErrorFunc) wfsParsingError;
    *err_msg = NULL;
    if (path_or_url == NULL)
	return 0;

/* loading the WFS GetCapabilities from URL (or file) */
    gaiaOutBufferInitialize (&errBuf);
    xmlSetGenericErrorFunc (&errBuf, parsingError);
    xml_doc = xmlReadFile (path_or_url, NULL, 0);
    if (xml_doc == NULL)
      {
	  /* parsing error; not a well-formed XML */
	  if (errBuf.Buffer != NULL)
	    {
		len = strlen (errBuf.Buffer);
		*err_msg = malloc (len + 1);
		strcpy (*err_msg, errBuf.Buffer);
	    }
	  goto end;
      }

/* parsing the WFS payload */
    catalog = alloc_wfs_catalog ();
    root = xmlDocGetRootElement (xml_doc);
    parse_wfs_catalog (root, catalog, &capabilities, &list);
    if (get_wfs_catalog_count ((gaiaWFScatalogPtr) catalog) <= 0)
      {
	  free_wfs_catalog (catalog);
	  catalog = NULL;
	  goto end;
      }

  end:
    gaiaOutBufferReset (&errBuf);
    xmlSetGenericErrorFunc ((void *) stderr, NULL);
    if (xml_doc != NULL)
	xmlFreeDoc (xml_doc);
    return (gaiaWFScatalogPtr) catalog;
}

SPATIALITE_DECLARE void
destroy_wfs_catalog (gaiaWFScatalogPtr handle)
{
/* memory cleanup: freeing a WFS-Catalog object */
    struct wfs_catalog *ptr = (struct wfs_catalog *) handle;
    if (ptr == NULL)
	return;
    free_wfs_catalog (ptr);
}

SPATIALITE_DECLARE const char *
get_wfs_base_url (gaiaWFScatalogPtr handle)
{
/* return the base URL for any WFS-GetFeature call */
    struct wfs_catalog *ptr = (struct wfs_catalog *) handle;
    if (ptr == NULL)
	return NULL;
    return ptr->base_url;
}

SPATIALITE_DECLARE int
get_wfs_catalog_count (gaiaWFScatalogPtr handle)
{
/* counting how many layers are defined within a WFS-Catalog */
    int count = 0;
    struct wfs_layer_def *lyr;
    struct wfs_catalog *ptr = (struct wfs_catalog *) handle;
    if (ptr == NULL)
	return -1;

    lyr = ptr->first;
    while (lyr != NULL)
      {
	  count++;
	  lyr = lyr->next;
      }
    return count;
}

SPATIALITE_DECLARE gaiaWFSitemPtr
get_wfs_catalog_item (gaiaWFScatalogPtr handle, int index)
{
/* attempting to get a reference to some WFS-Layer object */
    int count = 0;
    struct wfs_layer_def *lyr;
    struct wfs_catalog *ptr = (struct wfs_catalog *) handle;
    if (ptr == NULL)
	return NULL;

    lyr = ptr->first;
    while (lyr != NULL)
      {
	  if (count == index)
	      return (gaiaWFSitemPtr) lyr;
	  count++;
	  lyr = lyr->next;
      }
    return NULL;
}

SPATIALITE_DECLARE const char *
get_wfs_item_name (gaiaWFSitemPtr handle)
{
/* return the name corresponding to a WFS-Layer object */
    struct wfs_layer_def *ptr = (struct wfs_layer_def *) handle;
    if (ptr == NULL)
	return NULL;
    return ptr->name;
}

SPATIALITE_DECLARE const char *
get_wfs_item_title (gaiaWFSitemPtr handle)
{
/* return the title corresponding to a WFS-Layer object */
    struct wfs_layer_def *ptr = (struct wfs_layer_def *) handle;
    if (ptr == NULL)
	return NULL;
    return ptr->title;
}

SPATIALITE_DECLARE const char *
get_wfs_item_abstract (gaiaWFSitemPtr handle)
{
/* return the abstract corresponding to a WFS-Layer object */
    struct wfs_layer_def *ptr = (struct wfs_layer_def *) handle;
    if (ptr == NULL)
	return NULL;
    return ptr->abstract;
}

SPATIALITE_DECLARE int
get_wfs_layer_srid_count (gaiaWFSitemPtr handle)
{
/* counting how many SRIDs are supported by a WFS-Layer */
    int count = 0;
    struct wfs_srid_def *srid;
    struct wfs_layer_def *ptr = (struct wfs_layer_def *) handle;
    if (ptr == NULL)
	return -1;

    srid = ptr->first_srid;
    while (srid != NULL)
      {
	  count++;
	  srid = srid->next;
      }
    return count;
}

SPATIALITE_DECLARE int
get_wfs_layer_srid (gaiaWFSitemPtr handle, int index)
{
/* attempting to get the Nth SRID supported by some WFS-Layer object */
    int count = 0;
    struct wfs_srid_def *srid;
    struct wfs_layer_def *ptr = (struct wfs_layer_def *) handle;
    if (ptr == NULL)
	return -1;

    srid = ptr->first_srid;
    while (srid != NULL)
      {
	  if (count == index)
	      return srid->srid;
	  count++;
	  srid = srid->next;
      }
    return -1;
}

SPATIALITE_DECLARE int
get_wfs_keyword_count (gaiaWFSitemPtr handle)
{
/* counting how many Keywords are supported by a WFS-Layer */
    int count = 0;
    struct wfs_keyword *key;
    struct wfs_layer_def *ptr = (struct wfs_layer_def *) handle;
    if (ptr == NULL)
	return -1;

    key = ptr->first_key;
    while (key != NULL)
      {
	  count++;
	  key = key->next;
      }
    return count;
}

SPATIALITE_DECLARE const char *
get_wfs_keyword (gaiaWFSitemPtr handle, int index)
{
/* attempting to get the Nth Keyword supported by some WFS-Layer object */
    int count = 0;
    struct wfs_keyword *key;
    struct wfs_layer_def *ptr = (struct wfs_layer_def *) handle;
    if (ptr == NULL)
	return NULL;

    key = ptr->first_key;
    while (key != NULL)
      {
	  if (count == index)
	      return key->keyword;
	  count++;
	  key = key->next;
      }
    return NULL;
}

static char *
build_url (struct wfs_catalog *ptr, struct wfs_layer_def *lyr,
	   const char *version, int srid, int max_features)
{
/* creating the request URL */
    char *url;
    char *url2;
    const char *srs_name = NULL;
    int len;
    const char *ver = "1.1.0";
    if (ptr->base_url == NULL)
	return NULL;
    if (version != NULL)
      {
	  if (strcmp (version, "1.0.0") == 0)
	      ver = "1.0.0";
      }
    if (srid > 0)
      {
	  struct wfs_srid_def *srs = lyr->first_srid;
	  while (srs != NULL)
	    {
		if (srs->srid == srid)
		  {
		      /* found the required SRS definition */
		      srs_name = srs->srs_name;
		      break;
		  }
		srs = srs->next;
	    }
      }
    if (max_features <= 0)
      {
	  if (srs_name == NULL)
	      url =
		  sqlite3_mprintf
		  ("%sservice=WFS&version=%s&request=GetFeature&typeName=%s",
		   ptr->base_url, ver, lyr->name);
	  else
	      url =
		  sqlite3_mprintf
		  ("%sservice=WFS&version=%s&request=GetFeature&typeName=%s&srsName=%s",
		   ptr->base_url, ver, lyr->name, srs_name);
      }
    else
      {
	  if (srs_name == NULL)
	      url =
		  sqlite3_mprintf
		  ("%sservice=WFS&version=%s&request=GetFeature&typeName=%s&maxFeatures=%d",
		   ptr->base_url, ver, lyr->name, max_features);
	  else
	      url =
		  sqlite3_mprintf
		  ("%sservice=WFS&version=%s&request=GetFeature&typeName=%s&srsName=%s&maxFeatures=%d",
		   ptr->base_url, ver, lyr->name, srs_name, max_features);
      }
    len = strlen (url);
    url2 = malloc (len + 1);
    strcpy (url2, url);
    sqlite3_free (url);
    return url2;
}

SPATIALITE_DECLARE char *
get_wfs_request_url (gaiaWFScatalogPtr handle, const char *name,
		     const char *version, int srid, int max_features)
{
/* attempting to format a GetFeature (GET) URL */
    struct wfs_layer_def *lyr;
    struct wfs_catalog *ptr = (struct wfs_catalog *) handle;
    if (ptr == NULL)
	return NULL;

    lyr = ptr->first;
    while (lyr != NULL)
      {
	  if (strcmp (lyr->name, name) == 0)
	      return build_url (ptr, lyr, version, srid, max_features);
	  lyr = lyr->next;
      }
    return NULL;
}

#else /* LIBXML2 isn't enabled */

SPATIALITE_DECLARE int
load_from_wfs (sqlite3 * sqlite, char *path_or_url, int swap_axes,
	       char *table, char *pk_column_name, int spatial_index, int *rows,
	       char **err_msg, void (*progress_callback) (int))
{
/* LIBXML2 isn't enabled: always returning an error */
    return load_from_wfs_page (sqlite, path_or_url, swap_axes, table,
			       pk_column_name, spatial_index, -1, rows,
			       err_msg, progress_callback);
}

SPATIALITE_DECLARE int
load_from_wfs_paged (sqlite3 * sqlite, char *path_or_url, int swap_axes,
		     char *table, char *pk_column_name, int spatial_index,
		     int page_size, int *rows, char **err_msg,
		     void (*progress_callback) (int))
{
/* LIBXML2 isn't enabled: always returning an error */
    int len;
    const char *msg = "Sorry ... libspatialite was built disabling LIBXML2\n"
	"and is thus unable to support LOADWFS";

/* silencing stupid compiler warnings */
    if (sqlite == NULL || path_or_url == NULL || swap_axes == 0 || table == NULL
	|| pk_column_name == NULL || spatial_index == NULL || ||page_size <= 0
	|| rows == NULL || progress_callback == NULL)
	path_or_url = NULL;

    len = strlen (msg);
    *err_msg = malloc (len + 1);
    strcpy (*err_msg, msg);
    return 0;
}

SPATIALITE_DECLARE gaiaWFScatalogPtr
create_wfs_catalog (char *path_or_url, char **err_msg)
{
/* LIBXML2 isn't enabled: always returning an error */
    int len;
    const char *msg = "Sorry ... libspatialite was built disabling LIBXML2\n"
	"and is thus unable to support WFSCATALOG";

/* silencing stupid compiler warnings */
    if (path_or_url != NULL)
	path_or_url = NULL;

    len = strlen (msg);
    *err_msg = malloc (len + 1);
    strcpy (*err_msg, msg);
    return NULL;
}

SPATIALITE_DECLARE void
destroy_wfs_catalog (gaiaWFScatalogPtr handle)
{
/* LIBXML2 isn't enabled: does absolutely nothing */

/* silencing stupid compiler warnings */
    if (handle != NULL)
	handle = NULL;
    return;
}

SPATIALITE_DECLARE const char *
get_wfs_base_url (gaiaWFScatalogPtr handle)
{
/* LIBXML2 isn't enabled: does absolutely nothing */

/* silencing stupid compiler warnings */
    if (handle != NULL)
	handle = NULL;
    return NULL;
}

SPATIALITE_DECLARE int
get_wfs_catalog_count (gaiaWFScatalogPtr handle)
{
/* LIBXML2 isn't enabled: does absolutely nothing */

/* silencing stupid compiler warnings */
    if (handle != NULL)
	handle = NULL;
    return -1;
}

SPATIALITE_DECLARE gaiaWFSitemPtr
get_wfs_catalog_item (gaiaWFScatalogPtr handle, int index)
{
/* LIBXML2 isn't enabled: does absolutely nothing */

/* silencing stupid compiler warnings */
    if (handle != NULL || index < 0)
	handle = NULL;
    return NULL;
}

SPATIALITE_DECLARE const char *
get_wfs_item_name (gaiaWFSitemPtr handle)
{
/* LIBXML2 isn't enabled: does absolutely nothing */

/* silencing stupid compiler warnings */
    if (handle != NULL)
	handle = NULL;
    return NULL;
}

SPATIALITE_DECLARE const char *
get_wfs_item_title (gaiaWFSitemPtr handle)
{
/* LIBXML2 isn't enabled: does absolutely nothing */

/* silencing stupid compiler warnings */
    if (handle != NULL)
	handle = NULL;
    return NULL;
}

SPATIALITE_DECLARE const char *
get_wfs_item_abstract (gaiaWFSitemPtr handle)
{
/* LIBXML2 isn't enabled: does absolutely nothing */

/* silencing stupid compiler warnings */
    if (handle != NULL)
	handle = NULL;
    return NULL;
}

SPATIALITE_DECLARE int
get_wfs_layer_srid_count (gaiaWFSitemPtr handle)
{
/* LIBXML2 isn't enabled: does absolutely nothing */

/* silencing stupid compiler warnings */
    if (handle != NULL)
	handle = NULL;
    return -1;
}

SPATIALITE_DECLARE int
get_wfs_layer_srid (gaiaWFSitemPtr handle, int index)
{
/* LIBXML2 isn't enabled: does absolutely nothing */

/* silencing stupid compiler warnings */
    if (handle != NULL || index < 0)
	handle = NULL;
    return -1;
}

SPATIALITE_DECLARE int
get_wfs_keyword_count (gaiaWFSitemPtr handle)
{
/* LIBXML2 isn't enabled: does absolutely nothing */

/* silencing stupid compiler warnings */
    if (handle != NULL)
	handle = NULL;
    return -1;
}

SPATIALITE_DECLARE const char *
get_wfs_keyword (gaiaWFSitemPtr handle, int index)
{
/* LIBXML2 isn't enabled: does absolutely nothing */

/* silencing stupid compiler warnings */
    if (handle != NULL || index < 0)
	handle = NULL;
    return NULL;
}

SPATIALITE_DECLARE const char *
get_wfs_request_url (gaiaWFScatalogPtr handle, const char *name,
		     const char *version, int srid)
{
/* LIBXML2 isn't enabled: does absolutely nothing */

/* silencing stupid compiler warnings */
    if (handle != NULL || name == NULL || version == NULL || srid < 0)
	handle = NULL;
    return NULL;
}

#endif /* end LIBXML2 conditionals */
