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
/* memory cleanup: destroying a WFS schema */
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
load_wfs_schema (const char *path_or_url, char **err_msg)
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
    struct _xmlAttr *attr;
    if (node == NULL)
	return;
    attr = node->properties;
    while (attr != NULL)
      {
	  if (attr->name != NULL)
	    {
		if (strcmp ((const char *) (attr->name), "srsName") == 0)
		    schema->srid = parse_srsname (attr->children);
		if (strcmp ((const char *) (attr->name), "dimension") == 0)
		    schema->dims = parse_dimension (attr->children);
	    }
	  attr = attr->next;
      }
    sniff_gml_geometry (node->children, schema);
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
/* invalidating the still pendenting SQL transaction */
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
load_from_wfs (sqlite3 * sqlite, char *path_or_url,
	       char *table, char *pk_column_name, int spatial_index, int *rows,
	       char **err_msg)
{
/* attempting to load data from some WFS source */
    xmlDocPtr xml_doc = NULL;
    xmlNodePtr root;
    struct wfs_layer_schema *schema = NULL;
    int len;
    char *describe_uri = NULL;
    gaiaOutBuffer errBuf;
    int ok = 0;
    int sniffed = 0;
    xmlGenericErrorFunc parsingError = (xmlGenericErrorFunc) wfsParsingError;
    *rows = 0;
    *err_msg = NULL;
    if (path_or_url == NULL)
	return 0;

/* loading the WFS payload from URL (or file) */
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

/* extracting the WFS schema URL (or path) */
    if (get_DescribeFeatureType_uri (xml_doc, &describe_uri) == 0)
      {
	  const char *msg = "Unable to retrieve the DescribeFeatureType URI";
	  len = strlen (msg);
	  *err_msg = malloc (len + 1);
	  strcpy (*err_msg, msg);
	  goto end;
      }

/* loading and parsing the WFS schema */
    schema = load_wfs_schema (describe_uri, err_msg);
    if (schema == NULL)
	goto end;

/* creating the output table */
    root = xmlDocGetRootElement (xml_doc);
    sniffed = 0;
    sniff_geometries (root, schema, &sniffed);
    if (!prepare_sql
	(sqlite, schema, table, pk_column_name, spatial_index, err_msg))
	goto end;

/* parsing the WFS payload */
    root = xmlDocGetRootElement (xml_doc);
    parse_wfs_features (root, schema, rows);
    if (schema->error)
      {
	  *rows = 0;
	  do_rollback (sqlite, schema);
      }
    else
	do_commit (sqlite, schema);
    if (schema->error)
      {
	  *rows = 0;
	  goto end;
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

#else /* LIBXML2 isn't enabled */

SPATIALITE_DECLARE int
load_from_wfs (sqlite3 * sqlite, char *path_or_url,
	       char *table, char *pk_column_name, int spatial_index, int *rows,
	       char **err_msg)
{
/* LIBXML2 isn't enabled: always returning an error */
    int len;
    const char *msg = "Sorry ... libspatialite was built disabling LIBXML2\n"
	"and is thus unable to support LOADWFS";

/* silencing stupid compiler warnings */
    if (sqlite == NULL || path_or_url == NULL || table == NULL
	|| pk_column_name == NULL || spatial_index == NULL || rows == NULL)
	path_or_url = NULL;

    len = strlen (msg);
    *err_msg = malloc (len + 1);
    strcpy (*err_msg, msg);
    return 0;
}

#endif /* end LIBXML2 conditionals */
