/*

 dxf_loader.c -- implements DXF support [loding features into the DB]

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
#include <spatialite/gaiaaux.h>
#include <spatialite/gg_dxf.h>
#include <spatialite.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#define strcasecmp	_stricmp
#endif /* not WIN32 */

static int
import_mixed (sqlite3 * handle, gaiaDxfParserPtr dxf, int append)
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

    gaiaDxfLayerPtr lyr = dxf->first;
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
		return 0;
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
		return 0;
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
		return 0;
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
		return 0;
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
		      return 0;
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
		      return 0;
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
		      return 0;
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
		      return 0;
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
		return 0;
	    }
	  lyr = dxf->first;
	  while (lyr != NULL)
	    {
		gaiaDxfTextPtr txt = lyr->first_text;
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
			    return 0;
			}
		      if (stmt_ext != NULL)
			{
			    /* inserting all Extra Attributes */
			    sqlite3_int64 feature_id =
				sqlite3_last_insert_rowid (handle);
			    gaiaDxfExtraAttrPtr ext = txt->first;
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
					return 0;
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
		return 0;
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
		return 0;
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
		return 0;
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
		return 0;
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
		return 0;
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
		      return 0;
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
		      return 0;
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
		      return 0;
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
		      return 0;
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
		return 0;
	    }
	  lyr = dxf->first;
	  while (lyr != NULL)
	    {
		gaiaDxfPointPtr pt = lyr->first_point;
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
			    return 0;
			}
		      if (stmt_ext != NULL)
			{
			    /* inserting all Extra Attributes */
			    sqlite3_int64 feature_id =
				sqlite3_last_insert_rowid (handle);
			    gaiaDxfExtraAttrPtr ext = pt->first;
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
					return 0;
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
		return 0;
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
		return 0;
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
		return 0;
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
		return 0;
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
		return 0;
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
		      return 0;
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
		      return 0;
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
		      return 0;
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
		      return 0;
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
		return 0;
	    }
	  lyr = dxf->first;
	  while (lyr != NULL)
	    {
		gaiaDxfPolylinePtr ln = lyr->first_line;
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
			    return 0;
			}
		      if (stmt_ext != NULL)
			{
			    /* inserting all Extra Attributes */
			    sqlite3_int64 feature_id =
				sqlite3_last_insert_rowid (handle);
			    gaiaDxfExtraAttrPtr ext = ln->first;
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
					return 0;
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
		return 0;
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
		return 0;
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
		return 0;
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
		return 0;
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
		return 0;
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
		      return 0;
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
		      return 0;
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
		      return 0;
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
		      return 0;
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
		return 0;
	    }
	  lyr = dxf->first;
	  while (lyr != NULL)
	    {
		gaiaDxfPolylinePtr pg = lyr->first_polyg;
		while (pg != NULL)
		  {
		      gaiaDxfHolePtr hole;
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
			    return 0;
			}
		      if (stmt_ext != NULL)
			{
			    /* inserting all Extra Attributes */
			    sqlite3_int64 feature_id =
				sqlite3_last_insert_rowid (handle);
			    gaiaDxfExtraAttrPtr ext = pg->first;
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
					return 0;
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
		return 0;
	    }
      }
    return 1;
}

static int
import_by_layer (sqlite3 * handle, gaiaDxfParserPtr dxf, int append)
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
    gaiaDxfTextPtr txt;
    gaiaDxfPointPtr pt;
    gaiaDxfPolylinePtr ln;
    gaiaDxfPolylinePtr pg;

    gaiaDxfLayerPtr lyr = dxf->first;
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
		      return 0;
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
		      return 0;
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
		      return 0;
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
		      return 0;
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
			    return 0;
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
			    return 0;
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
			    return 0;
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
			    return 0;
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
		      return 0;
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
			    return 0;
			}
		      if (stmt_ext != NULL)
			{
			    /* inserting all Extra Attributes */
			    sqlite3_int64 feature_id =
				sqlite3_last_insert_rowid (handle);
			    gaiaDxfExtraAttrPtr ext = txt->first;
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
					return 0;
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
		      return 0;
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
		      return 0;
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
		      return 0;
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
		      return 0;
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
		      return 0;
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
			    return 0;
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
			    return 0;
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
			    return 0;
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
			    return 0;
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
		      return 0;
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
			    return 0;
			}
		      if (stmt_ext != NULL)
			{
			    /* inserting all Extra Attributes */
			    sqlite3_int64 feature_id =
				sqlite3_last_insert_rowid (handle);
			    gaiaDxfExtraAttrPtr ext = pt->first;
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
					return 0;
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
		      return 0;
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
		      return 0;
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
		      return 0;
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
		      return 0;
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
		      return 0;
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
			    return 0;
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
			    return 0;
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
			    return 0;
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
			    return 0;
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
		      return 0;
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
			    return 0;
			}
		      if (stmt_ext != NULL)
			{
			    /* inserting all Extra Attributes */
			    sqlite3_int64 feature_id =
				sqlite3_last_insert_rowid (handle);
			    gaiaDxfExtraAttrPtr ext = ln->first;
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
					return 0;
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
		      return 0;
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
		      return 0;
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
		      return 0;
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
		      return 0;
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
		      return 0;
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
			    return 0;
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
			    return 0;
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
			    return 0;
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
			    return 0;
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
		      return 0;
		  }
		pg = lyr->first_polyg;
		while (pg != NULL)
		  {
		      int num_holes;
		      gaiaDxfHolePtr hole;
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
			    return 0;
			}
		      if (stmt_ext != NULL)
			{
			    /* inserting all Extra Attributes */
			    sqlite3_int64 feature_id =
				sqlite3_last_insert_rowid (handle);
			    gaiaDxfExtraAttrPtr ext = pg->first;
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
					return 0;
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
		      return 0;
		  }
		sqlite3_free (name);
		if (attr_name)
		    sqlite3_free (attr_name);
	    }
	  lyr = lyr->next;
      }
    return 1;
}

GAIAGEO_DECLARE int
gaiaLoadFromDxfParser (sqlite3 * handle,
		       gaiaDxfParserPtr dxf, int mode, int append)
{
/* populating the target DB */
    int ret;

    if (dxf == NULL)
	return 0;
    if (dxf->first != NULL)
	return 0;

    if (mode == GAIA_DXF_IMPORT_MIXED)
	ret = import_mixed (handle, dxf, append);
    else
	ret = import_by_layer (handle, dxf, append);
    return ret;
}
