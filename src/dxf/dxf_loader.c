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
#include <spatialite_private.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#define strcasecmp	_stricmp
#endif /* not WIN32 */

static int
create_text_stmt (sqlite3 * handle, const char *name, sqlite3_stmt ** xstmt)
{
/* creating the "Text" insert statement */
    char *sql;
    int ret;
    sqlite3_stmt *stmt;
    char *xname;
    *xstmt = NULL;

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
	  spatialite_e ("CREATE STATEMENT %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    *xstmt = stmt;
    return 1;
}

static int
create_point_stmt (sqlite3 * handle, const char *name, sqlite3_stmt ** xstmt)
{
/* creating the "Point" insert statement */
    char *sql;
    int ret;
    sqlite3_stmt *stmt;
    char *xname;
    *xstmt = NULL;

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
	  spatialite_e ("CREATE STATEMENT %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    *xstmt = stmt;
    return 1;
}

static int
create_line_stmt (sqlite3 * handle, const char *name, sqlite3_stmt ** xstmt)
{
/* creating the "Line" insert statement */
    char *sql;
    int ret;
    sqlite3_stmt *stmt;
    char *xname;
    *xstmt = NULL;

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
	  spatialite_e ("CREATE STATEMENT %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    *xstmt = stmt;
    return 1;
}

static int
create_polyg_stmt (sqlite3 * handle, const char *name, sqlite3_stmt ** xstmt)
{
/* creating the "Polyg" insert statement */
    char *sql;
    int ret;
    sqlite3_stmt *stmt;
    char *xname;
    *xstmt = NULL;

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
	  spatialite_e ("CREATE STATEMENT %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    *xstmt = stmt;
    return 1;
}

static int
create_hatch_stmt (sqlite3 * handle, const char *name, sqlite3_stmt ** xstmt)
{
/* creating the "Hatch" insert statement */
    char *sql;
    int ret;
    sqlite3_stmt *stmt;
    char *xname;
    *xstmt = NULL;

    xname = gaiaDoubleQuotedSql (name);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (feature_id, layer, boundary_geom, pattern_geom) "
	 "VALUES (NULL, ?, ?, ?)", xname);
    free (xname);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE STATEMENT %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    *xstmt = stmt;
    return 1;
}

static int
create_extra_stmt (sqlite3 * handle, const char *extra_name,
		   sqlite3_stmt ** xstmt)
{
/* creating the Extra Attributes insert statement */
    char *sql;
    int ret;
    sqlite3_stmt *stmt;
    char *xextra_name;
    *xstmt = NULL;

    xextra_name = gaiaDoubleQuotedSql (extra_name);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (attr_id, feature_id, attr_key, attr_value) "
	 "VALUES (NULL, ?, ?, ?)", xextra_name);
    free (xextra_name);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE STATEMENT %s error: %s\n", extra_name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    *xstmt = stmt;
    return 1;
}

static int
create_mixed_text_table (sqlite3 * handle, const char *name, int srid,
			 int text3D, sqlite3_stmt ** xstmt)
{
/* attempting to create the "Text-mixed" table */
    char *sql;
    int ret;
    sqlite3_stmt *stmt;
    char *xname;
    *xstmt = NULL;

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
	  spatialite_e ("CREATE TABLE %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(%Q, 'geometry', "
	 "%d, 'POINT', %Q)", name, srid, text3D ? "XYZ" : "XY");
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("ADD GEOMETRY %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'geometry')", name);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE SPATIAL INDEX %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    if (!create_text_stmt (handle, name, &stmt))
	return 0;

    *xstmt = stmt;
    return 1;
}

static int
create_mixed_point_table (sqlite3 * handle, const char *name, int srid,
			  int point3D, sqlite3_stmt ** xstmt)
{
/* attempting to create the "Point-mixed" table */
    char *sql;
    int ret;
    sqlite3_stmt *stmt;
    char *xname;
    *xstmt = NULL;

    xname = gaiaDoubleQuotedSql (name);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
			   "    feature_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "    layer TEXT NOT NULL)", xname);
    free (xname);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(%Q, 'geometry', "
	 "%d, 'POINT', %Q)", name, srid, point3D ? "XYZ" : "XY");
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("ADD GEOMETRY %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'geometry')", name);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE SPATIAL INDEX %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    if (!create_point_stmt (handle, name, &stmt))
	return 0;

    *xstmt = stmt;
    return 1;
}

static int
create_mixed_line_table (sqlite3 * handle, const char *name, int srid,
			 int line3D, sqlite3_stmt ** xstmt)
{
/* attempting to create the "Line-mixed" table */
    char *sql;
    int ret;
    sqlite3_stmt *stmt;
    char *xname;
    *xstmt = NULL;

    xname = gaiaDoubleQuotedSql (name);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
			   "    feature_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "    layer TEXT NOT NULL)", xname);
    free (xname);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(%Q, 'geometry', "
	 "%d, 'LINESTRING', %Q)", name, srid, line3D ? "XYZ" : "XY");
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("ADD GEOMETRY %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'geometry')", name);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE SPATIAL INDEX %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    if (!create_line_stmt (handle, name, &stmt))
	return 0;

    *xstmt = stmt;
    return 1;
}

static int
create_mixed_polyg_table (sqlite3 * handle, const char *name, int srid,
			  int polyg3D, sqlite3_stmt ** xstmt)
{
/* attempting to create the "Polyg-mixed" table */
    char *sql;
    int ret;
    sqlite3_stmt *stmt;
    char *xname;
    *xstmt = NULL;

    xname = gaiaDoubleQuotedSql (name);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
			   "    feature_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "    layer TEXT NOT NULL)", xname);
    free (xname);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(%Q, 'geometry', "
	 "%d, 'POLYGON', %Q)", name, srid, polyg3D ? "XYZ" : "XY");
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("ADD GEOMETRY %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'geometry')", name);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE SPATIAL INDEX %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    if (!create_polyg_stmt (handle, name, &stmt))
	return 0;

    *xstmt = stmt;
    return 1;
}

static int
create_mixed_hatch_table (sqlite3 * handle, const char *name, int srid,
			  sqlite3_stmt ** xstmt)
{
/* attempting to create the "Hatch-mixed" table */
    char *sql;
    int ret;
    sqlite3_stmt *stmt;
    char *xname;
    *xstmt = NULL;

    xname = gaiaDoubleQuotedSql (name);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
			   "    feature_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "    layer TEXT NOT NULL)", xname);
    free (xname);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(%Q, 'boundary_geom', "
	 "%d, 'MULTIPOLYGON', 'XY')", name, srid);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("ADD GEOMETRY %s (boundary) error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sql =
	sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'boundary_geom')",
			 name);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE SPATIAL INDEX %s (boundary) error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(%Q, 'pattern_geom', "
	 "%d, 'MULTILINESTRING', 'XY')", name, srid);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("ADD GEOMETRY %s (pattern) error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sql =
	sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'pattern_geom')", name);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE SPATIAL INDEX %s (pattern) error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    if (!create_hatch_stmt (handle, name, &stmt))
	return 0;

    *xstmt = stmt;
    return 1;
}

static char *
create_extra_attr_table_name (const char *name)
{
/* creating the Extra Attributes table name [Mixed Layers] */
    return sqlite3_mprintf ("%s_attr", name);
}

static int
create_mixed_text_extra_attr_table (sqlite3 * handle, const char *name,
				    char *extra_name, sqlite3_stmt ** xstmt_ext)
{
/* attempting to create the "Text-mixed-extra-attr" table */
    char *sql;
    int ret;
    sqlite3_stmt *stmt_ext;
    char *xname;
    char *xextra_name;
    char *fk_name;
    char *xfk_name;
    char *idx_name;
    char *xidx_name;
    char *view_name;
    char *xview_name;
    *xstmt_ext = NULL;

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
	  spatialite_e ("CREATE TABLE %s error: %s\n", extra_name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    idx_name = sqlite3_mprintf ("idx_%s", extra_name);
    xidx_name = gaiaDoubleQuotedSql (idx_name);
    xextra_name = gaiaDoubleQuotedSql (extra_name);
    sql =
	sqlite3_mprintf
	("CREATE INDEX \"%s\" ON \"%s\" (feature_id)", xidx_name, xextra_name);
    free (xidx_name);
    free (xextra_name);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX %s error: %s\n", idx_name,
			sqlite3_errmsg (handle));
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
	  spatialite_e ("CREATE VIEW %s error: %s\n", view_name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sqlite3_free (view_name);
    if (!create_extra_stmt (handle, extra_name, &stmt_ext))
	return 0;

    *xstmt_ext = stmt_ext;
    return 1;
}

static int
create_mixed_point_extra_attr_table (sqlite3 * handle, const char *name,
				     char *extra_name,
				     sqlite3_stmt ** xstmt_ext)
{
/* attempting to create the "Point-mixed-extra-attr" table */
    char *sql;
    int ret;
    sqlite3_stmt *stmt_ext;
    char *xname;
    char *xextra_name;
    char *fk_name;
    char *xfk_name;
    char *idx_name;
    char *xidx_name;
    char *view_name;
    char *xview_name;
    *xstmt_ext = NULL;

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
	  spatialite_e ("CREATE TABLE %s error: %s\n", extra_name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    idx_name = sqlite3_mprintf ("idx_%s", extra_name);
    xidx_name = gaiaDoubleQuotedSql (idx_name);
    xname = gaiaDoubleQuotedSql (name);
    sql =
	sqlite3_mprintf
	("CREATE INDEX \"%s\" ON \"%s\" (feature_id)", xidx_name, xname);
    free (xidx_name);
    free (xname);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX %s error: %s\n", idx_name,
			sqlite3_errmsg (handle));
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
	  spatialite_e ("CREATE VIEW %s error: %s\n", view_name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sqlite3_free (view_name);
    if (!create_extra_stmt (handle, extra_name, &stmt_ext))
	return 0;

    *xstmt_ext = stmt_ext;
    return 1;
}

static int
create_mixed_line_extra_attr_table (sqlite3 * handle, const char *name,
				    char *extra_name, sqlite3_stmt ** xstmt_ext)
{
/* attempting to create the "Line-mixed-extra-attr" table */
    char *sql;
    int ret;
    sqlite3_stmt *stmt_ext;
    char *xname;
    char *xextra_name;
    char *fk_name;
    char *xfk_name;
    char *idx_name;
    char *xidx_name;
    char *view_name;
    char *xview_name;
    *xstmt_ext = NULL;

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
	  spatialite_e ("CREATE TABLE %s error: %s\n", extra_name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    idx_name = sqlite3_mprintf ("idx_%s", extra_name);
    xidx_name = gaiaDoubleQuotedSql (idx_name);
    xextra_name = gaiaDoubleQuotedSql (extra_name);
    sql =
	sqlite3_mprintf
	("CREATE INDEX \"%s\" ON \"%s\" (feature_id)", xidx_name, xextra_name);
    free (xidx_name);
    free (xextra_name);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX %s error: %s\n", idx_name,
			sqlite3_errmsg (handle));
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
	  spatialite_e ("CREATE VIEW %s error: %s\n", view_name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sqlite3_free (view_name);
    if (!create_extra_stmt (handle, extra_name, &stmt_ext))
	return 0;

    *xstmt_ext = stmt_ext;
    return 1;
}

static int
create_mixed_polyg_extra_attr_table (sqlite3 * handle, const char *name,
				     char *extra_name,
				     sqlite3_stmt ** xstmt_ext)
{
/* attempting to create the "Polyg-mixed-extra-attr" table */
    char *sql;
    int ret;
    sqlite3_stmt *stmt_ext;
    char *xname;
    char *xextra_name;
    char *fk_name;
    char *xfk_name;
    char *idx_name;
    char *xidx_name;
    char *view_name;
    char *xview_name;
    *xstmt_ext = NULL;

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
	  spatialite_e ("CREATE TABLE %s error: %s\n", extra_name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    idx_name = sqlite3_mprintf ("idx_%s", extra_name);
    xidx_name = gaiaDoubleQuotedSql (idx_name);
    xextra_name = gaiaDoubleQuotedSql (extra_name);
    sql =
	sqlite3_mprintf
	("CREATE INDEX \"%s\" ON \"%s\" (feature_id)", xidx_name, xextra_name);
    free (xidx_name);
    free (xextra_name);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX %s error: %s\n", idx_name,
			sqlite3_errmsg (handle));
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
	  spatialite_e ("CREATE VIEW %s error: %s\n", view_name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sqlite3_free (view_name);
    if (!create_extra_stmt (handle, extra_name, &stmt_ext))
	return 0;

    *xstmt_ext = stmt_ext;
    return 1;
}

static int
check_text_table (sqlite3 * handle, const char *name, int srid, int is3D)
{
/* checking if a Text table already exists */
    char *sql;
    int ok_geom = 0;
    int ok_data = 0;
    int ret;
    int i;
    char *xname;
    char **results;
    int n_rows;
    int n_columns;
    int metadata_version = checkSpatialMetaData (handle);

    if (metadata_version == 1)
      {
	  /* legacy metadata style <= v.3.1.0 */
	  int ok_srid = 0;
	  int ok_type = 0;
	  int dims2d = 0;
	  int dims3d = 0;
	  sql = sqlite3_mprintf ("SELECT srid, type, coord_dimension "
				 "FROM geometry_columns "
				 "WHERE Lower(f_table_name) = Lower(%Q) AND "
				 "Lower(f_geometry_column) = Lower(%Q)", name,
				 "geometry");
	  ret =
	      sqlite3_get_table (handle, sql, &results, &n_rows, &n_columns,
				 NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	      return 0;
	  if (n_rows > 0)
	    {
		for (i = 1; i <= n_rows; i++)
		  {
		      if (atoi (results[(i * n_columns) + 0]) == srid)
			  ok_srid = 1;
		      if (strcmp ("POINT", results[(i * n_columns) + 1]) == 0)
			  ok_type = 1;
		      if (strcmp ("XY", results[(i * n_columns) + 2]) == 0)
			  dims2d = 1;
		      if (strcmp ("XYZ", results[(i * n_columns) + 2]) == 0)
			  dims3d = 1;
		  }
	    }
	  sqlite3_free_table (results);
	  if (ok_srid && ok_type)
	    {
		if (is3D && dims3d)
		    ok_geom = 1;
		if (!is3D && dims2d)
		    ok_geom = 1;
	    }
      }
    else
      {
	  /* current metadata style >= v.4.0.0 */
	  int ok_srid = 0;
	  int ok_type = 0;
	  sql = sqlite3_mprintf ("SELECT srid, geometry_type "
				 "FROM geometry_columns "
				 "WHERE Lower(f_table_name) = Lower(%Q) AND "
				 "Lower(f_geometry_column) = Lower(%Q)", name,
				 "geometry");
	  ret =
	      sqlite3_get_table (handle, sql, &results, &n_rows, &n_columns,
				 NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	      return 0;
	  if (n_rows > 0)
	    {
		for (i = 1; i <= n_rows; i++)
		  {
		      if (atoi (results[(i * n_columns) + 0]) == srid)
			  ok_srid = 1;
		      if (atoi (results[(i * n_columns) + 1]) == 1 && !is3D)
			  ok_type = 1;
		      if (atoi (results[(i * n_columns) + 1]) == 1001 && is3D)
			  ok_type = 1;
		  }
	    }
	  sqlite3_free_table (results);
	  if (ok_srid && ok_type)
	    {
		ok_geom = 1;
	    }
      }

    xname = gaiaDoubleQuotedSql (name);
    sql = sqlite3_mprintf ("PRAGMA table_info(\"%s\")", xname);
    free (xname);
    ret = sqlite3_get_table (handle, sql, &results, &n_rows, &n_columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (n_rows > 0)
      {
	  int ok_feature_id = 0;
	  int ok_layer = 0;
	  int ok_label = 0;
	  int ok_rotation = 0;
	  for (i = 1; i <= n_rows; i++)
	    {
		if (strcasecmp ("feature_id", results[(i * n_columns) + 1]) ==
		    0)
		    ok_feature_id = 1;
		if (strcasecmp ("layer", results[(i * n_columns) + 1]) == 0)
		    ok_layer = 1;
		if (strcasecmp ("label", results[(i * n_columns) + 1]) == 0)
		    ok_label = 1;
		if (strcasecmp ("rotation", results[(i * n_columns) + 1]) == 0)
		    ok_rotation = 1;
	    }
	  if (ok_feature_id && ok_layer && ok_label && ok_rotation)
	      ok_data = 1;
      }
    sqlite3_free_table (results);

    if (ok_geom && ok_data)
	return 1;
    return 0;
}

static int
check_point_table (sqlite3 * handle, const char *name, int srid, int is3D)
{
/* checking if a Point table already exists */
    char *sql;
    int ok_geom = 0;
    int ok_data = 0;
    int ret;
    int i;
    char *xname;
    char **results;
    int n_rows;
    int n_columns;
    int metadata_version = checkSpatialMetaData (handle);

    if (metadata_version == 1)
      {
	  /* legacy metadata style <= v.3.1.0 */
	  int ok_srid = 0;
	  int ok_type = 0;
	  int dims2d = 0;
	  int dims3d = 0;
	  sql = sqlite3_mprintf ("SELECT srid, type, coord_dimension "
				 "FROM geometry_columns "
				 "WHERE Lower(f_table_name) = Lower(%Q) AND "
				 "Lower(f_geometry_column) = Lower(%Q)", name,
				 "geometry");
	  ret =
	      sqlite3_get_table (handle, sql, &results, &n_rows, &n_columns,
				 NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	      return 0;
	  if (n_rows > 0)
	    {
		for (i = 1; i <= n_rows; i++)
		  {
		      if (atoi (results[(i * n_columns) + 0]) == srid)
			  ok_srid = 1;
		      if (strcmp ("POINT", results[(i * n_columns) + 1]) == 0)
			  ok_type = 1;
		      if (strcmp ("XY", results[(i * n_columns) + 2]) == 0)
			  dims2d = 1;
		      if (strcmp ("XYZ", results[(i * n_columns) + 2]) == 0)
			  dims3d = 1;
		  }
	    }
	  sqlite3_free_table (results);
	  if (ok_srid && ok_type)
	    {
		if (is3D && dims3d)
		    ok_geom = 1;
		if (!is3D && dims2d)
		    ok_geom = 1;
	    }
      }
    else
      {
	  /* current metadata style >= v.4.0.0 */
	  int ok_srid = 0;
	  int ok_type = 0;
	  sql = sqlite3_mprintf ("SELECT srid, geometry_type "
				 "FROM geometry_columns "
				 "WHERE Lower(f_table_name) = Lower(%Q) AND "
				 "Lower(f_geometry_column) = Lower(%Q)", name,
				 "geometry");
	  ret =
	      sqlite3_get_table (handle, sql, &results, &n_rows, &n_columns,
				 NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	      return 0;
	  if (n_rows > 0)
	    {
		for (i = 1; i <= n_rows; i++)
		  {
		      if (atoi (results[(i * n_columns) + 0]) == srid)
			  ok_srid = 1;
		      if (atoi (results[(i * n_columns) + 1]) == 1 && !is3D)
			  ok_type = 1;
		      if (atoi (results[(i * n_columns) + 1]) == 1001 && is3D)
			  ok_type = 1;
		  }
	    }
	  sqlite3_free_table (results);
	  if (ok_srid && ok_type)
	    {
		ok_geom = 1;
	    }
      }

    xname = gaiaDoubleQuotedSql (name);
    sql = sqlite3_mprintf ("PRAGMA table_info(\"%s\")", xname);
    free (xname);
    ret = sqlite3_get_table (handle, sql, &results, &n_rows, &n_columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (n_rows > 0)
      {
	  int ok_feature_id = 0;
	  int ok_layer = 0;
	  for (i = 1; i <= n_rows; i++)
	    {
		if (strcasecmp ("feature_id", results[(i * n_columns) + 1]) ==
		    0)
		    ok_feature_id = 1;
		if (strcasecmp ("layer", results[(i * n_columns) + 1]) == 0)
		    ok_layer = 1;
	    }
	  if (ok_feature_id && ok_layer)
	      ok_data = 1;
      }
    sqlite3_free_table (results);

    if (ok_geom && ok_data)
	return 1;
    return 0;
}

static int
check_line_table (sqlite3 * handle, const char *name, int srid, int is3D)
{
/* checking if a Line table already exists */
    char *sql;
    int ok_geom = 0;
    int ok_data = 0;
    int ret;
    int i;
    char *xname;
    char **results;
    int n_rows;
    int n_columns;
    int metadata_version = checkSpatialMetaData (handle);

    if (metadata_version == 1)
      {
	  /* legacy metadata style <= v.3.1.0 */
	  int ok_srid = 0;
	  int ok_type = 0;
	  int dims2d = 0;
	  int dims3d = 0;
	  sql = sqlite3_mprintf ("SELECT srid, type, coord_dimension "
				 "FROM geometry_columns "
				 "WHERE Lower(f_table_name) = Lower(%Q) AND "
				 "Lower(f_geometry_column) = Lower(%Q)", name,
				 "geometry");
	  ret =
	      sqlite3_get_table (handle, sql, &results, &n_rows, &n_columns,
				 NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	      return 0;
	  if (n_rows > 0)
	    {
		for (i = 1; i <= n_rows; i++)
		  {
		      if (atoi (results[(i * n_columns) + 0]) == srid)
			  ok_srid = 1;
		      if (strcmp ("LINESTRING", results[(i * n_columns) + 1]) ==
			  0)
			  ok_type = 1;
		      if (strcmp ("XY", results[(i * n_columns) + 2]) == 0)
			  dims2d = 1;
		      if (strcmp ("XYZ", results[(i * n_columns) + 2]) == 0)
			  dims3d = 1;
		  }
	    }
	  sqlite3_free_table (results);
	  if (ok_srid && ok_type)
	    {
		if (is3D && dims3d)
		    ok_geom = 1;
		if (!is3D && dims2d)
		    ok_geom = 1;
	    }
      }
    else
      {
	  /* current metadata style >= v.4.0.0 */
	  int ok_srid = 0;
	  int ok_type = 0;
	  sql = sqlite3_mprintf ("SELECT srid, geometry_type "
				 "FROM geometry_columns "
				 "WHERE Lower(f_table_name) = Lower(%Q) AND "
				 "Lower(f_geometry_column) = Lower(%Q)", name,
				 "geometry");
	  ret =
	      sqlite3_get_table (handle, sql, &results, &n_rows, &n_columns,
				 NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	      return 0;
	  if (n_rows > 0)
	    {
		for (i = 1; i <= n_rows; i++)
		  {
		      if (atoi (results[(i * n_columns) + 0]) == srid)
			  ok_srid = 1;
		      if (atoi (results[(i * n_columns) + 1]) == 2 && !is3D)
			  ok_type = 1;
		      if (atoi (results[(i * n_columns) + 1]) == 1002 && is3D)
			  ok_type = 1;
		  }
	    }
	  sqlite3_free_table (results);
	  if (ok_srid && ok_type)
	    {
		ok_geom = 1;
	    }
      }

    xname = gaiaDoubleQuotedSql (name);
    sql = sqlite3_mprintf ("PRAGMA table_info(\"%s\")", xname);
    free (xname);
    ret = sqlite3_get_table (handle, sql, &results, &n_rows, &n_columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (n_rows > 0)
      {
	  int ok_feature_id = 0;
	  int ok_layer = 0;
	  for (i = 1; i <= n_rows; i++)
	    {
		if (strcasecmp ("feature_id", results[(i * n_columns) + 1]) ==
		    0)
		    ok_feature_id = 1;
		if (strcasecmp ("layer", results[(i * n_columns) + 1]) == 0)
		    ok_layer = 1;
	    }
	  if (ok_feature_id && ok_layer)
	      ok_data = 1;
      }
    sqlite3_free_table (results);

    if (ok_geom && ok_data)
	return 1;
    return 0;
}

static int
check_polyg_table (sqlite3 * handle, const char *name, int srid, int is3D)
{
/* checking if a Polygon table already exists */
    char *sql;
    int ok_geom = 0;
    int ok_data = 0;
    int ret;
    int i;
    char *xname;
    char **results;
    int n_rows;
    int n_columns;
    int metadata_version = checkSpatialMetaData (handle);

    if (metadata_version == 1)
      {
	  /* legacy metadata style <= v.3.1.0 */
	  int ok_srid = 0;
	  int ok_type = 0;
	  int dims2d = 0;
	  int dims3d = 0;
	  sql = sqlite3_mprintf ("SELECT srid, type, coord_dimension "
				 "FROM geometry_columns "
				 "WHERE Lower(f_table_name) = Lower(%Q) AND "
				 "Lower(f_geometry_column) = Lower(%Q)", name,
				 "geometry");
	  ret =
	      sqlite3_get_table (handle, sql, &results, &n_rows, &n_columns,
				 NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	      return 0;
	  if (n_rows > 0)
	    {
		for (i = 1; i <= n_rows; i++)
		  {
		      if (atoi (results[(i * n_columns) + 0]) == srid)
			  ok_srid = 1;
		      if (strcmp ("POLYGON", results[(i * n_columns) + 1]) == 0)
			  ok_type = 1;
		      if (strcmp ("XY", results[(i * n_columns) + 2]) == 0)
			  dims2d = 1;
		      if (strcmp ("XYZ", results[(i * n_columns) + 2]) == 0)
			  dims3d = 1;
		  }
	    }
	  sqlite3_free_table (results);
	  if (ok_srid && ok_type)
	    {
		if (is3D && dims3d)
		    ok_geom = 1;
		if (!is3D && dims2d)
		    ok_geom = 1;
	    }
      }
    else
      {
	  /* current metadata style >= v.4.0.0 */
	  int ok_srid = 0;
	  int ok_type = 0;
	  sql = sqlite3_mprintf ("SELECT srid, geometry_type "
				 "FROM geometry_columns "
				 "WHERE Lower(f_table_name) = Lower(%Q) AND "
				 "Lower(f_geometry_column) = Lower(%Q)", name,
				 "geometry");
	  ret =
	      sqlite3_get_table (handle, sql, &results, &n_rows, &n_columns,
				 NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	      return 0;
	  if (n_rows > 0)
	    {
		for (i = 1; i <= n_rows; i++)
		  {
		      if (atoi (results[(i * n_columns) + 0]) == srid)
			  ok_srid = 1;
		      if (atoi (results[(i * n_columns) + 1]) == 3 && !is3D)
			  ok_type = 1;
		      if (atoi (results[(i * n_columns) + 1]) == 1003 && is3D)
			  ok_type = 1;
		  }
	    }
	  sqlite3_free_table (results);
	  if (ok_srid && ok_type)
	    {
		ok_geom = 1;
	    }
      }

    xname = gaiaDoubleQuotedSql (name);
    sql = sqlite3_mprintf ("PRAGMA table_info(\"%s\")", xname);
    free (xname);
    ret = sqlite3_get_table (handle, sql, &results, &n_rows, &n_columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (n_rows > 0)
      {
	  int ok_feature_id = 0;
	  int ok_layer = 0;
	  for (i = 1; i <= n_rows; i++)
	    {
		if (strcasecmp ("feature_id", results[(i * n_columns) + 1]) ==
		    0)
		    ok_feature_id = 1;
		if (strcasecmp ("layer", results[(i * n_columns) + 1]) == 0)
		    ok_layer = 1;
	    }
	  if (ok_feature_id && ok_layer)
	      ok_data = 1;
      }
    sqlite3_free_table (results);

    if (ok_geom && ok_data)
	return 1;
    return 0;
}

static int
check_hatch_table (sqlite3 * handle, const char *name, int srid)
{
/* checking if a Hatch table already exists */
    char *sql;
    int ok_geom = 0;
    int ok_data = 0;
    int ret;
    int i;
    char *xname;
    char **results;
    int n_rows;
    int n_columns;
    int metadata_version = checkSpatialMetaData (handle);

    if (metadata_version == 1)
      {
	  /* legacy metadata style <= v.3.1.0 */
	  int ok_bsrid = 0;
	  int ok_btype = 0;
	  int bdims2d = 0;
	  int ok_psrid = 0;
	  int ok_ptype = 0;
	  int pdims2d = 0;
	  sql =
	      sqlite3_mprintf
	      ("SELECT f_geometry_column, srid, type, coord_dimension "
	       "FROM geometry_columns "
	       "WHERE Lower(f_table_name) = Lower(%Q) AND "
	       "(Lower(f_geometry_column) = Lower(%Q) OR "
	       "Lower(f_geometry_column) = Lower(%Q))", name, "boundary_geom",
	       "pattern_geom");
	  ret =
	      sqlite3_get_table (handle, sql, &results, &n_rows, &n_columns,
				 NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	      return 0;
	  if (n_rows > 0)
	    {
		for (i = 1; i <= n_rows; i++)
		  {
		      if (strcasecmp
			  ("boundary_geom", results[(i * n_columns) + 0]) == 0)
			{
			    if (atoi (results[(i * n_columns) + 1]) == srid)
				ok_bsrid = 1;
			    if (strcmp
				("MULTIPOLYGON",
				 results[(i * n_columns) + 2]) == 0)
				ok_btype = 1;
			    if (strcmp ("XY", results[(i * n_columns) + 3]) ==
				0)
				bdims2d = 1;
			}
		      if (strcasecmp
			  ("pattern_geom", results[(i * n_columns) + 0]) == 0)
			{
			    if (atoi (results[(i * n_columns) + 1]) == srid)
				ok_psrid = 1;
			    if (strcmp
				("MULTILINESTRING",
				 results[(i * n_columns) + 2]) == 0)
				ok_ptype = 1;
			    if (strcmp ("XY", results[(i * n_columns) + 3]) ==
				0)
				pdims2d = 1;
			}
		  }
	    }
	  sqlite3_free_table (results);
	  if (ok_bsrid && ok_btype && bdims2d && ok_psrid && ok_ptype
	      && pdims2d)
	      ok_geom = 1;
      }
    else
      {
	  /* current metadata style >= v.4.0.0 */
	  int ok_psrid = 0;
	  int ok_ptype = 0;
	  int ok_bsrid = 0;
	  int ok_btype = 0;
	  sql =
	      sqlite3_mprintf ("SELECT f_geometry_column, srid, geometry_type "
			       "FROM geometry_columns "
			       "WHERE Lower(f_table_name) = Lower(%Q) AND "
			       "(Lower(f_geometry_column) = Lower(%Q) OR "
			       "Lower(f_geometry_column) = Lower(%Q))", name,
			       "boundary_geom", "pattern_geom");
	  ret =
	      sqlite3_get_table (handle, sql, &results, &n_rows, &n_columns,
				 NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	      return 0;
	  if (n_rows > 0)
	    {
		for (i = 1; i <= n_rows; i++)
		  {
		      if (strcasecmp
			  ("boundary_geom", results[(i * n_columns) + 0]) == 0)
			{
			    if (atoi (results[(i * n_columns) + 1]) == srid)
				ok_bsrid = 1;
			    if (atoi (results[(i * n_columns) + 2]) == 6)
				ok_btype = 1;
			}
		      if (strcasecmp
			  ("pattern_geom", results[(i * n_columns) + 0]) == 0)
			{
			    if (atoi (results[(i * n_columns) + 1]) == srid)
				ok_psrid = 1;
			    if (atoi (results[(i * n_columns) + 2]) == 5)
				ok_ptype = 1;
			}
		  }
	    }
	  sqlite3_free_table (results);
	  if (ok_bsrid && ok_btype && ok_psrid && ok_ptype)
	      ok_geom = 1;
      }

    xname = gaiaDoubleQuotedSql (name);
    sql = sqlite3_mprintf ("PRAGMA table_info(\"%s\")", xname);
    free (xname);
    ret = sqlite3_get_table (handle, sql, &results, &n_rows, &n_columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (n_rows > 0)
      {
	  int ok_feature_id = 0;
	  int ok_layer = 0;
	  for (i = 1; i <= n_rows; i++)
	    {
		if (strcasecmp ("feature_id", results[(i * n_columns) + 1]) ==
		    0)
		    ok_feature_id = 1;
		if (strcasecmp ("layer", results[(i * n_columns) + 1]) == 0)
		    ok_layer = 1;
	    }
	  if (ok_feature_id && ok_layer)
	      ok_data = 1;
      }
    sqlite3_free_table (results);

    if (ok_geom && ok_data)
	return 1;
    return 0;
}

static int
check_extra_attr_table (sqlite3 * handle, const char *name)
{
/* checking if an Extra Attributes table already exists */
    char *sql;
    int ok_data = 0;
    int ret;
    int i;
    char *xname;
    char **results;
    int n_rows;
    int n_columns;

    xname = gaiaDoubleQuotedSql (name);
    sql = sqlite3_mprintf ("PRAGMA table_info(\"%s\")", xname);
    free (xname);
    ret = sqlite3_get_table (handle, sql, &results, &n_rows, &n_columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (n_rows > 0)
      {
	  int ok_attr_id = 0;
	  int ok_feature_id = 0;
	  int ok_attr_key = 0;
	  int ok_attr_value = 0;
	  for (i = 1; i <= n_rows; i++)
	    {
		if (strcasecmp ("attr_id", results[(i * n_columns) + 1]) == 0)
		    ok_attr_id = 1;
		if (strcasecmp ("feature_id", results[(i * n_columns) + 1]) ==
		    0)
		    ok_feature_id = 1;
		if (strcasecmp ("attr_key", results[(i * n_columns) + 1]) == 0)
		    ok_attr_key = 1;
		if (strcasecmp ("attr_value", results[(i * n_columns) + 1]) ==
		    0)
		    ok_attr_value = 1;
	    }
	  if (ok_attr_id && ok_feature_id && ok_attr_key && ok_attr_value)
	      ok_data = 1;
      }
    sqlite3_free_table (results);

    if (ok_data)
	return 1;
    return 0;
}

static int
import_mixed (sqlite3 * handle, gaiaDxfParserPtr dxf, int append)
{
/* populating the target DB - all layers mixed altogether */
    int text = 0;
    int point = 0;
    int line = 0;
    int polyg = 0;
    int hatch = 0;
    int hasExtraText = 0;
    int hasExtraPoint = 0;
    int hasExtraLine = 0;
    int hasExtraPolyg = 0;
    int text3D = 0;
    int point3D = 0;
    int line3D = 0;
    int polyg3D = 0;
    int ret;
    sqlite3_stmt *stmt;
    sqlite3_stmt *stmt_ext;
    unsigned char *blob;
    int blob_size;
    unsigned char *blob2;
    int blob_size2;
    gaiaGeomCollPtr geom;
    char *name;
    char *extra_name;

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
	  if (lyr->first_hatch != NULL)
	      hatch = 1;
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
	  if (append && check_text_table (handle, name, dxf->srid, text3D))
	    {
		/* appending into the already existing table */
		if (!create_text_stmt (handle, name, &stmt))
		    return 0;
	    }
	  else
	    {
		/* creating a new table */
		if (!create_mixed_text_table
		    (handle, name, dxf->srid, text3D, &stmt))
		    return 0;
	    }
	  if (hasExtraText)
	    {
		extra_name = create_extra_attr_table_name (name);
		if (append && check_extra_attr_table (handle, extra_name))
		  {
		      /* appending into the already existing table */
		      if (!create_extra_stmt (handle, extra_name, &stmt_ext))
			  return 0;
		  }
		else
		  {
		      /* creating the Extra Attribute table */
		      if (!create_mixed_text_extra_attr_table
			  (handle, name, extra_name, &stmt_ext))
			{
			    sqlite3_finalize (stmt);
			    return 0;
			}
		  }
	    }
	  ret = sqlite3_exec (handle, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("BEGIN %s error: %s\n", name,
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
			    spatialite_e ("INSERT %s error: %s\n", name,
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
					spatialite_e ("INSERT %s error: %s\n",
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
		spatialite_e ("COMMIT text_layer error: %s\n",
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
	  if (append && check_point_table (handle, name, dxf->srid, point3D))
	    {
		/* appending into the already existing table */
		if (!create_point_stmt (handle, name, &stmt))
		    return 0;
	    }
	  else
	    {
		/* creating a new table */
		if (!create_mixed_point_table
		    (handle, name, dxf->srid, point3D, &stmt))
		    return 0;
	    }
	  if (hasExtraPoint)
	    {
		extra_name = create_extra_attr_table_name (name);
		if (append && check_extra_attr_table (handle, extra_name))
		  {
		      /* appending into the already existing table */
		      if (!create_extra_stmt (handle, extra_name, &stmt_ext))
			  return 0;
		  }
		else
		  {
		      /* creating the Extra Attribute table */
		      if (!create_mixed_point_extra_attr_table
			  (handle, name, extra_name, &stmt_ext))
			{
			    sqlite3_finalize (stmt);
			    return 0;
			}
		  }
	    }
	  ret = sqlite3_exec (handle, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("BEGIN %s error: %s\n", name,
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
			    spatialite_e ("INSERT %s error: %s\n", name,
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
					spatialite_e ("INSERT %s error: %s\n",
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
		spatialite_e ("COMMIT point_layer error: %s\n",
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
	  if (append && check_line_table (handle, name, dxf->srid, line3D))
	    {
		/* appending into the already existing table */
		if (!create_line_stmt (handle, name, &stmt))
		    return 0;
	    }
	  else
	    {
		/* creating a new table */
		if (!create_mixed_line_table
		    (handle, name, dxf->srid, line3D, &stmt))
		    return 0;
	    }
	  if (hasExtraLine)
	    {
		extra_name = create_extra_attr_table_name (name);
		if (append && check_extra_attr_table (handle, extra_name))
		  {
		      /* appending into the already existing table */
		      if (!create_extra_stmt (handle, extra_name, &stmt_ext))
			  return 0;
		  }
		else
		  {
		      /* creating the Extra Attribute table */
		      if (!create_mixed_line_extra_attr_table
			  (handle, name, extra_name, &stmt_ext))
			{
			    sqlite3_finalize (stmt);
			    return 0;
			}
		  }
	    }
	  ret = sqlite3_exec (handle, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("BEGIN %s error: %s\n", name,
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
			    spatialite_e ("INSERT %s error: %s\n", name,
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
					spatialite_e ("INSERT %s error: %s\n",
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
		spatialite_e ("COMMIT line_layer error: %s\n",
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
	  if (append && check_polyg_table (handle, name, dxf->srid, polyg3D))
	    {
		/* appending into the already existing table */
		if (!create_polyg_stmt (handle, name, &stmt))
		    return 0;
	    }
	  else
	    {
		/* creating a new table */
		if (!create_mixed_polyg_table
		    (handle, name, dxf->srid, polyg3D, &stmt))
		    return 0;
	    }
	  if (hasExtraPolyg)
	    {
		extra_name = create_extra_attr_table_name (name);
		if (append && check_extra_attr_table (handle, extra_name))
		  {
		      /* appending into the already existing table */
		      if (!create_extra_stmt (handle, extra_name, &stmt_ext))
			  return 0;
		  }
		else
		  {
		      /* creating the Extra Attribute table */
		      if (!create_mixed_polyg_extra_attr_table
			  (handle, name, extra_name, &stmt_ext))
			{
			    sqlite3_finalize (stmt);
			    return 0;
			}
		  }
	    }
	  ret = sqlite3_exec (handle, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("BEGIN %s error: %s\n", name,
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
			    spatialite_e ("INSERT %s error: %s\n", name,
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
					spatialite_e ("INSERT %s error: %s\n",
						      extra_name,
						      sqlite3_errmsg (handle));
					sqlite3_finalize (stmt);
					if (stmt_ext != NULL)
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
		spatialite_e ("COMMIT polyg_layer error: %s\n",
			      sqlite3_errmsg (handle));
		return 0;
	    }
      }

    if (hatch)
      {
	  /* creating and populating the HATCH layer */
	  if (dxf->prefix == NULL)
	      name = sqlite3_mprintf ("hatch_layer");
	  else
	      name = sqlite3_mprintf ("%shatch_layer", dxf->prefix);
	  if (append && check_hatch_table (handle, name, dxf->srid))
	    {
		/* appending into the already existing table */
		if (!create_hatch_stmt (handle, name, &stmt))
		    return 0;
	    }
	  else
	    {
		/* creating a new table */
		if (!create_mixed_hatch_table (handle, name, dxf->srid, &stmt))
		    return 0;
	    }
	  ret = sqlite3_exec (handle, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("BEGIN %s error: %s\n", name,
			      sqlite3_errmsg (handle));
		sqlite3_finalize (stmt);
		if (stmt_ext != NULL)
		    sqlite3_finalize (stmt_ext);
		return 0;
	    }
	  lyr = dxf->first;
	  while (lyr != NULL)
	    {
		gaiaDxfHatchPtr hatch = lyr->first_hatch;
		while (hatch != NULL)
		  {
		      gaiaDxfHatchSegmPtr segm;
		      sqlite3_reset (stmt);
		      sqlite3_clear_bindings (stmt);
		      sqlite3_bind_text (stmt, 1, lyr->layer_name,
					 strlen (lyr->layer_name),
					 SQLITE_STATIC);
		      if (hatch->boundary == NULL)
			  sqlite3_bind_null (stmt, 2);
		      else
			{
			    /* saving the Boundary Geometry */
			    gaiaToSpatiaLiteBlobWkb (hatch->boundary, &blob,
						     &blob_size);
			    sqlite3_bind_blob (stmt, 2, blob, blob_size, free);
			}
		      if (hatch->first_out == NULL)
			  sqlite3_bind_null (stmt, 3);
		      else
			{
			    /* saving the Pattern Geometry */
			    geom = gaiaAllocGeomColl ();
			    geom->Srid = dxf->srid;
			    geom->DeclaredType = GAIA_MULTILINESTRING;
			    segm = hatch->first_out;
			    while (segm != NULL)
			      {
				  gaiaLinestringPtr p_ln =
				      gaiaAddLinestringToGeomColl (geom, 2);
				  gaiaSetPoint (p_ln->Coords, 0, segm->x0,
						segm->y0);
				  gaiaSetPoint (p_ln->Coords, 1, segm->x1,
						segm->y1);
				  segm = segm->next;
			      }
			    gaiaToSpatiaLiteBlobWkb (geom, &blob2, &blob_size2);
			    gaiaFreeGeomColl (geom);
			    sqlite3_bind_blob (stmt, 3, blob2, blob_size2,
					       free);
			}
		      ret = sqlite3_step (stmt);
		      if (ret == SQLITE_DONE || ret == SQLITE_ROW)
			  ;
		      else
			{
			    spatialite_e ("INSERT %s error: %s\n", name,
					  sqlite3_errmsg (handle));
			    sqlite3_finalize (stmt);
			    if (stmt_ext != NULL)
				sqlite3_finalize (stmt_ext);
			    ret =
				sqlite3_exec (handle, "ROLLBACK", NULL, NULL,
					      NULL);
			    return 0;
			}
		      hatch = hatch->next;
		  }
		lyr = lyr->next;
	    }
	  sqlite3_free (name);
	  sqlite3_finalize (stmt);
	  ret = sqlite3_exec (handle, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("COMMIT hatch_layer error: %s\n",
			      sqlite3_errmsg (handle));
		return 0;
	    }
      }
    return 1;
}

static int
create_layer_text_table (sqlite3 * handle, const char *name, int srid,
			 int text3D, sqlite3_stmt ** xstmt)
{
/* attempting to create the "Text-layer" table */
    char *sql;
    int ret;
    sqlite3_stmt *stmt;
    char *xname;
    *xstmt = NULL;

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
	  spatialite_e ("CREATE TABLE %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sql =
	sqlite3_mprintf ("SELECT AddGeometryColumn(%Q, 'geometry', "
			 "%d, 'POINT', %Q)", name, srid, text3D ? "XYZ" : "XY");
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("ADD GEOMETRY %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'geometry')", name);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE SPATIAL INDEX %s error: %s\n",
			name, sqlite3_errmsg (handle));
	  return 0;
      }
    if (!create_text_stmt (handle, name, &stmt))
	return 0;

    *xstmt = stmt;
    return 1;
}

static int
create_layer_point_table (sqlite3 * handle, const char *name, int srid,
			  int point3D, sqlite3_stmt ** xstmt)
{
/* attempting to create the "Point-layer" table */
    char *sql;
    int ret;
    sqlite3_stmt *stmt;
    char *xname;
    *xstmt = NULL;

    xname = gaiaDoubleQuotedSql (name);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
			   "    feature_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "    layer TEXT NOT NULL)", xname);
    free (xname);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sql =
	sqlite3_mprintf ("SELECT AddGeometryColumn(%Q, 'geometry', "
			 "%d, 'POINT', %Q)", name, srid,
			 point3D ? "XYZ" : "XY");
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("ADD GEOMETRY %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'geometry')", name);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE SPATIAL INDEX %s error: %s\n",
			name, sqlite3_errmsg (handle));
	  return 0;
      }
    if (!create_point_stmt (handle, name, &stmt))
	return 0;

    *xstmt = stmt;
    return 1;
}

static int
create_layer_line_table (sqlite3 * handle, const char *name, int srid,
			 int line3D, sqlite3_stmt ** xstmt)
{
/* attempting to create the "Line-layer" table */
    char *sql;
    int ret;
    sqlite3_stmt *stmt;
    char *xname;
    *xstmt = NULL;

    xname = gaiaDoubleQuotedSql (name);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
			   "    feature_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "    layer TEXT NOT NULL)", xname);
    free (xname);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sql =
	sqlite3_mprintf ("SELECT AddGeometryColumn(%Q, 'geometry', "
			 "%d, 'LINESTRING', %Q)", name, srid,
			 line3D ? "XYZ" : "XY");
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("ADD GEOMETRY %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'geometry')", name);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE SPATIAL INDEX %s error: %s\n",
			name, sqlite3_errmsg (handle));
	  return 0;
      }
    if (!create_line_stmt (handle, name, &stmt))
	return 0;

    *xstmt = stmt;
    return 1;
}

static int
create_layer_polyg_table (sqlite3 * handle, const char *name, int srid,
			  int polyg3D, sqlite3_stmt ** xstmt)
{
/* attempting to create the "Polyg-layer" table */
    char *sql;
    int ret;
    sqlite3_stmt *stmt;
    char *xname;
    *xstmt = NULL;

    xname = gaiaDoubleQuotedSql (name);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
			   "    feature_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "    layer TEXT NOT NULL)", xname);
    free (xname);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sql =
	sqlite3_mprintf ("SELECT AddGeometryColumn(%Q, 'geometry', "
			 "%d, 'POLYGON', %Q)", name, srid,
			 polyg3D ? "XYZ" : "XY");
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("ADD GEOMETRY %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'geometry')", name);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE SPATIAL INDEX %s error: %s\n",
			name, sqlite3_errmsg (handle));
	  return 0;
      }
    if (!create_polyg_stmt (handle, name, &stmt))
	return 0;

    *xstmt = stmt;
    return 1;
}

static int
create_layer_hatch_table (sqlite3 * handle, const char *name, int srid,
			  sqlite3_stmt ** xstmt)
{
/* attempting to create the "Hatch-layer" table */
    char *sql;
    int ret;
    sqlite3_stmt *stmt;
    char *xname;
    *xstmt = NULL;

    xname = gaiaDoubleQuotedSql (name);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
			   "    feature_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "    layer TEXT NOT NULL)", xname);
    free (xname);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE %s error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sql =
	sqlite3_mprintf ("SELECT AddGeometryColumn(%Q, 'boundary_geom', "
			 "%d, 'MULTIPOLYGON', 'XY')", name, srid);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("ADD GEOMETRY %s (boundary) error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sql =
	sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'boundary_geom')",
			 name);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE SPATIAL INDEX %s (boundary) error: %s\n",
			name, sqlite3_errmsg (handle));
	  return 0;
      }
    sql =
	sqlite3_mprintf ("SELECT AddGeometryColumn(%Q, 'pattern_geom', "
			 "%d, 'MULTILINESTRING', 'XY')", name, srid);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("ADD GEOMETRY %s (pattern) error: %s\n", name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sql =
	sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'pattern_geom')", name);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE SPATIAL INDEX %s (pattern) error: %s\n",
			name, sqlite3_errmsg (handle));
	  return 0;
      }
    if (!create_hatch_stmt (handle, name, &stmt))
	return 0;

    *xstmt = stmt;
    return 1;
}

static char *
create_layer_extra_attr_table_name (const char *prefix, int suffix,
				    const char *layer_name, const char *type)
{
/* preparing the Extra Attributes table name */
    if (prefix == NULL)
      {
	  if (suffix)
	      return sqlite3_mprintf ("%s_%s_attr", layer_name, type);
	  else
	      return sqlite3_mprintf ("%s_attr", layer_name);
      }
    else
      {
	  if (suffix)
	      return sqlite3_mprintf ("%s%s_%s_attr", prefix, layer_name, type);
	  else
	      return sqlite3_mprintf ("%s%s_attr", prefix, layer_name);
      }
}

static int
create_layer_text_extra_attr_table (sqlite3 * handle, const char *name,
				    const char *prefix, int suffix,
				    const char *layer_name, char *attr_name,
				    sqlite3_stmt ** xstmt_ext)
{
/* attempting to create the "Text-layer-extra-attr" table */
    char *sql;
    int ret;
    sqlite3_stmt *stmt_ext;
    char *xname;
    char *xattr_name;
    char *fk_name;
    char *xfk_name;
    char *idx_name;
    char *xidx_name;
    char *view_name;
    char *xview_name;
    *xstmt_ext = NULL;

    if (prefix == NULL)
      {
	  if (suffix)
	      fk_name = sqlite3_mprintf ("fk_%s_text_attr", layer_name);
	  else
	      fk_name = sqlite3_mprintf ("fk_%s_attr", layer_name);
      }
    else
      {
	  if (suffix)
	      fk_name =
		  sqlite3_mprintf ("fk_%s%s_text_attr", prefix, layer_name);
	  else
	      fk_name = sqlite3_mprintf ("fk_%s%s_attr", prefix, layer_name);
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
	  spatialite_e ("CREATE TABLE %s error: %s\n", attr_name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    if (prefix == NULL)
      {
	  if (suffix)
	      idx_name = sqlite3_mprintf ("idx_%s_text_attr", layer_name);
	  else
	      idx_name = sqlite3_mprintf ("idx_%s_attr", layer_name);
      }
    else
      {
	  if (suffix)
	      idx_name =
		  sqlite3_mprintf ("idx_%s%s_text_attr", prefix, layer_name);
	  else
	      idx_name = sqlite3_mprintf ("idx_%s%s_attr", prefix, layer_name);
      }
    xidx_name = gaiaDoubleQuotedSql (idx_name);
    sql =
	sqlite3_mprintf
	("CREATE INDEX \"%s\" ON \"%s\" (feature_id)", xidx_name, xname);
    free (xidx_name);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX %s error: %s\n", idx_name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sqlite3_free (idx_name);
    if (prefix == NULL)
      {
	  if (suffix)
	      view_name = sqlite3_mprintf ("%s_text_view", layer_name);
	  else
	      view_name = sqlite3_mprintf ("%s_view", layer_name);
      }
    else
      {
	  if (suffix)
	      view_name =
		  sqlite3_mprintf ("%s%s_text_view", prefix, layer_name);
	  else
	      view_name = sqlite3_mprintf ("%s%s_view", prefix, layer_name);
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
    free (xattr_name);
    free (xname);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE VIEW %s error: %s\n", view_name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sqlite3_free (view_name);
    if (!create_extra_stmt (handle, attr_name, &stmt_ext))
	return 0;

    *xstmt_ext = stmt_ext;
    return 1;
}

static int
create_layer_point_extra_attr_table (sqlite3 * handle, const char *name,
				     const char *prefix, int suffix,
				     const char *layer_name, char *attr_name,
				     sqlite3_stmt ** xstmt_ext)
{
/* attempting to create the "Point-layer-extra-attr" table */
    char *sql;
    int ret;
    sqlite3_stmt *stmt_ext;
    char *xname;
    char *xattr_name;
    char *fk_name;
    char *xfk_name;
    char *idx_name;
    char *xidx_name;
    char *view_name;
    char *xview_name;
    *xstmt_ext = NULL;

    if (prefix == NULL)
      {
	  if (suffix)
	      fk_name = sqlite3_mprintf ("fk_%s_point_attr", layer_name);
	  else
	      fk_name = sqlite3_mprintf ("fk_%s_attr", layer_name);
      }
    else
      {
	  if (suffix)
	      fk_name =
		  sqlite3_mprintf ("fk_%s%s_point_attr", prefix, layer_name);
	  else
	      fk_name = sqlite3_mprintf ("fk_%s%s_attr", prefix, layer_name);
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
	  spatialite_e ("CREATE TABLE %s error: %s\n", attr_name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    if (prefix == NULL)
      {
	  if (suffix)
	      idx_name = sqlite3_mprintf ("idx_%s_poiny_attr", layer_name);
	  else
	      idx_name = sqlite3_mprintf ("idx_%s_attr", layer_name);
      }
    else
      {
	  if (suffix)
	      idx_name =
		  sqlite3_mprintf ("idx_%s%s_point_attr", prefix, layer_name);
	  else
	      idx_name = sqlite3_mprintf ("idx_%s%s_attr", prefix, layer_name);
      }
    xidx_name = gaiaDoubleQuotedSql (idx_name);
    sql =
	sqlite3_mprintf
	("CREATE INDEX \"%s\" ON \"%s\" (feature_id)", xidx_name, xname);
    free (xidx_name);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX %s error: %s\n", idx_name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sqlite3_free (idx_name);
    if (prefix == NULL)
      {
	  if (suffix)
	      view_name = sqlite3_mprintf ("%s_point_view", layer_name);
	  else
	      view_name = sqlite3_mprintf ("%s_view", layer_name);
      }
    else
      {
	  if (suffix)
	      view_name =
		  sqlite3_mprintf ("%s%s_point_view", prefix, layer_name);
	  else
	      view_name = sqlite3_mprintf ("%s%s_view", prefix, layer_name);
      }
    xview_name = gaiaDoubleQuotedSql (view_name);
    sql = sqlite3_mprintf ("CREATE VIEW \"%s\" AS "
			   "SELECT f.feature_id AS feature_id, f.layer AS layer, f.geometry AS geometry, "
			   "a.attr_id AS attr_id, a.attr_key AS attr_key, a.attr_value AS attr_value "
			   "FROM \"%s\" AS f "
			   "LEFT JOIN \"%s\" AS a ON (f.feature_id = a.feature_id)",
			   xview_name, xname, xattr_name);
    free (xview_name);
    free (xattr_name);
    free (xname);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE VIEW %s error: %s\n", view_name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sqlite3_free (view_name);
    if (!create_extra_stmt (handle, attr_name, &stmt_ext))
	return 0;

    *xstmt_ext = stmt_ext;
    return 1;
}

static int
create_layer_line_extra_attr_table (sqlite3 * handle, const char *name,
				    const char *prefix, int suffix,
				    const char *layer_name, char *attr_name,
				    sqlite3_stmt ** xstmt_ext)
{
/* attempting to create the "Line-layer-extra-attr" table */
    char *sql;
    int ret;
    sqlite3_stmt *stmt_ext;
    char *xname;
    char *xattr_name;
    char *fk_name;
    char *xfk_name;
    char *idx_name;
    char *xidx_name;
    char *view_name;
    char *xview_name;
    *xstmt_ext = NULL;

    if (prefix == NULL)
      {
	  if (suffix)
	      fk_name = sqlite3_mprintf ("fk_%s_line_attr", layer_name);
	  else
	      fk_name = sqlite3_mprintf ("fk_%s_attr", layer_name);
      }
    else
      {
	  if (suffix)
	      fk_name =
		  sqlite3_mprintf ("fk_%s%s_line_attr", prefix, layer_name);
	  else
	      fk_name = sqlite3_mprintf ("fk_%s%s_attr", prefix, layer_name);
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
	  spatialite_e ("CREATE TABLE %s error: %s\n", attr_name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    if (prefix == NULL)
      {
	  if (suffix)
	      idx_name = sqlite3_mprintf ("idx_%s_line_attr", layer_name);
	  else
	      idx_name = sqlite3_mprintf ("idx_%s_attr", layer_name);
      }
    else
      {
	  if (suffix)
	      idx_name =
		  sqlite3_mprintf ("idx_%s%s_line_attr", prefix, layer_name);
	  else
	      idx_name = sqlite3_mprintf ("idx_%s%s_attr", prefix, layer_name);
      }
    xidx_name = gaiaDoubleQuotedSql (idx_name);
    sql =
	sqlite3_mprintf
	("CREATE INDEX \"%s\" ON \"%s\" (feature_id)", xidx_name, xname);
    free (xidx_name);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX %s error: %s\n", idx_name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sqlite3_free (idx_name);
    if (prefix == NULL)
      {
	  if (suffix)
	      view_name = sqlite3_mprintf ("%s_line_view", layer_name);
	  else
	      view_name = sqlite3_mprintf ("%s_view", layer_name);
      }
    else
      {
	  if (suffix)
	      view_name =
		  sqlite3_mprintf ("%s%s_line_view", prefix, layer_name);
	  else
	      view_name = sqlite3_mprintf ("%s%s_view", prefix, layer_name);
      }
    xview_name = gaiaDoubleQuotedSql (view_name);
    sql = sqlite3_mprintf ("CREATE VIEW \"%s\" AS "
			   "SELECT f.feature_id AS feature_id, f.layer AS layer, f.geometry AS geometry, "
			   "a.attr_id AS attr_id, a.attr_key AS attr_key, a.attr_value AS attr_value "
			   "FROM \"%s\" AS f "
			   "LEFT JOIN \"%s\" AS a ON (f.feature_id = a.feature_id)",
			   xview_name, xname, xattr_name);
    free (xview_name);
    free (xattr_name);
    free (xname);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE VIEW %s error: %s\n", view_name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sqlite3_free (view_name);
    if (!create_extra_stmt (handle, attr_name, &stmt_ext))
	return 0;

    *xstmt_ext = stmt_ext;
    return 1;
}

static int
create_layer_polyg_extra_attr_table (sqlite3 * handle, const char *name,
				     const char *prefix, int suffix,
				     const char *layer_name, char *attr_name,
				     sqlite3_stmt ** xstmt_ext)
{
/* attempting to create the "Polyg-layer-extra-attr" table */
    char *sql;
    int ret;
    sqlite3_stmt *stmt_ext;
    char *xname;
    char *xattr_name;
    char *fk_name;
    char *xfk_name;
    char *idx_name;
    char *xidx_name;
    char *view_name;
    char *xview_name;
    *xstmt_ext = NULL;

    if (prefix == NULL)
      {
	  if (suffix)
	      fk_name = sqlite3_mprintf ("fk_%s_polyg_attr", layer_name);
	  else
	      fk_name = sqlite3_mprintf ("fk_%s_attr", layer_name);
      }
    else
      {
	  if (suffix)
	      fk_name =
		  sqlite3_mprintf ("fk_%s%s_polyg_attr", prefix, layer_name);
	  else
	      fk_name = sqlite3_mprintf ("fk_%s%s_attr", prefix, layer_name);
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
	  spatialite_e ("CREATE TABLE %s error: %s\n", attr_name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    if (prefix == NULL)
      {
	  if (suffix)
	      idx_name = sqlite3_mprintf ("idx_%s_polyg_attr", layer_name);
	  else
	      idx_name = sqlite3_mprintf ("idx_%s_attr", layer_name);
      }
    else
      {
	  if (suffix)
	      idx_name =
		  sqlite3_mprintf ("idx_%s%s_polyg_attr", prefix, layer_name);
	  else
	      idx_name = sqlite3_mprintf ("idx_%s%s_attr", prefix, layer_name);
      }
    xidx_name = gaiaDoubleQuotedSql (idx_name);
    sql =
	sqlite3_mprintf
	("CREATE INDEX \"%s\" ON \"%s\" (feature_id)", xidx_name, xname);
    free (xidx_name);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX %s error: %s\n", idx_name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sqlite3_free (idx_name);
    if (prefix == NULL)
      {
	  if (suffix)
	      view_name = sqlite3_mprintf ("%s_polyg_view", layer_name);
	  else
	      view_name = sqlite3_mprintf ("%s_view", layer_name);
      }
    else
      {
	  if (suffix)
	      view_name =
		  sqlite3_mprintf ("%s%s_polyg_view", prefix, layer_name);
	  else
	      view_name = sqlite3_mprintf ("%s%s_view", prefix, layer_name);
      }
    xview_name = gaiaDoubleQuotedSql (view_name);
    sql = sqlite3_mprintf ("CREATE VIEW \"%s\" AS "
			   "SELECT f.feature_id AS feature_id, f.layer AS layer, f.geometry AS geometry, "
			   "a.attr_id AS attr_id, a.attr_key AS attr_key, a.attr_value AS attr_value "
			   "FROM \"%s\" AS f "
			   "LEFT JOIN \"%s\" AS a ON (f.feature_id = a.feature_id)",
			   xview_name, xname, xattr_name);
    free (xview_name);
    free (xattr_name);
    free (xname);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE VIEW %s error: %s\n", view_name,
			sqlite3_errmsg (handle));
	  return 0;
      }
    sqlite3_free (view_name);
    if (!create_extra_stmt (handle, attr_name, &stmt_ext))
	return 0;

    *xstmt_ext = stmt_ext;
    return 1;
}

static int
import_by_layer (sqlite3 * handle, gaiaDxfParserPtr dxf, int append)
{
/* populating the target DB - by distinct layers */
    int ret;
    sqlite3_stmt *stmt;
    sqlite3_stmt *stmt_ext;
    unsigned char *blob;
    int blob_size;
    unsigned char *blob2;
    int blob_size2;
    gaiaGeomCollPtr geom;
    gaiaLinestringPtr p_ln;
    gaiaPolygonPtr p_pg;
    gaiaRingPtr p_rng;
    int iv;
    char *name;
    char *attr_name;
    gaiaDxfTextPtr txt;
    gaiaDxfPointPtr pt;
    gaiaDxfPolylinePtr ln;
    gaiaDxfPolylinePtr pg;
    gaiaDxfHatchPtr p_hatch;

    gaiaDxfLayerPtr lyr = dxf->first;
    while (lyr != NULL)
      {
	  /* looping on layers */
	  int text = 0;
	  int point = 0;
	  int line = 0;
	  int polyg = 0;
	  int hatch = 0;
	  int suffix = 0;
	  if (lyr->first_text != NULL)
	      text = 1;
	  if (lyr->first_point != NULL)
	      point = 1;
	  if (lyr->first_line != NULL)
	      line = 1;
	  if (lyr->first_polyg != NULL)
	      polyg = 1;
	  if (lyr->first_hatch != NULL)
	      hatch = 1;
	  if (text + point + line + polyg + hatch > 1)
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
		if (append
		    && check_text_table (handle, name, dxf->srid,
					 lyr->is3Dtext))
		  {
		      /* appending into the already existing table */
		      if (!create_text_stmt (handle, name, &stmt))
			  return 0;
		  }
		else
		  {
		      /* creating a new table */
		      if (!create_layer_text_table
			  (handle, name, dxf->srid, lyr->is3Dtext, &stmt))
			{

			    sqlite3_free (name);
			    return 0;
			}
		  }
		if (lyr->hasExtraText)
		  {
		      attr_name =
			  create_layer_extra_attr_table_name (dxf->prefix,
							      suffix,
							      lyr->layer_name,
							      "text");
		      if (append && check_extra_attr_table (handle, attr_name))
			{
			    /* appending into the already existing table */
			    if (!create_extra_stmt
				(handle, attr_name, &stmt_ext))
				return 0;
			}
		      else
			{
			    /* creating the Extra Attribute table */
			    if (!create_layer_text_extra_attr_table
				(handle, name, dxf->prefix, suffix,
				 lyr->layer_name, attr_name, &stmt_ext))
			      {
				  sqlite3_finalize (stmt);
				  return 0;
			      }
			}
		  }
		ret = sqlite3_exec (handle, "BEGIN", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
		  {
		      spatialite_e ("BEGIN %s error: %s\n", name,
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
			    spatialite_e ("INSERT %s error: %s\n", name,
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
					spatialite_e ("INSERT %s error: %s\n",
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
		      spatialite_e ("COMMIT %s error: %s\n", name,
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
		if (append
		    && check_point_table (handle, name, dxf->srid,
					  lyr->is3Dpoint))
		  {
		      /* appending into the already existing table */
		      if (!create_point_stmt (handle, name, &stmt))
			  return 0;
		  }
		else
		  {
		      /* creating a new table */
		      if (!create_layer_point_table
			  (handle, name, dxf->srid, lyr->is3Dpoint, &stmt))
			{

			    sqlite3_free (name);
			    return 0;
			}
		  }
		if (lyr->hasExtraPoint)
		  {
		      attr_name =
			  create_layer_extra_attr_table_name (dxf->prefix,
							      suffix,
							      lyr->layer_name,
							      "point");
		      if (append && check_extra_attr_table (handle, attr_name))
			{
			    /* appending into the already existing table */
			    if (!create_extra_stmt
				(handle, attr_name, &stmt_ext))
				return 0;
			}
		      else
			{
			    /* creating the Extra Attribute table */
			    if (!create_layer_point_extra_attr_table
				(handle, name, dxf->prefix, suffix,
				 lyr->layer_name, attr_name, &stmt_ext))
			      {
				  sqlite3_finalize (stmt);
				  return 0;
			      }
			}
		  }
		ret = sqlite3_exec (handle, "BEGIN", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
		  {
		      spatialite_e ("BEGIN %s error: %s\n", name,
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
			    spatialite_e ("INSERT %s error: %s\n", name,
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
					spatialite_e ("INSERT %s error: %s\n",
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
		      spatialite_e ("COMMIT %s error: %s\n", name,
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
		if (append
		    && check_line_table (handle, name, dxf->srid,
					 lyr->is3Dline))
		  {
		      /* appending into the already existing table */
		      if (!create_line_stmt (handle, name, &stmt))
			  return 0;
		  }
		else
		  {
		      /* creating a new table */
		      if (!create_layer_line_table
			  (handle, name, dxf->srid, lyr->is3Dline, &stmt))
			{

			    sqlite3_free (name);
			    return 0;
			}
		  }
		if (lyr->hasExtraLine)
		  {
		      attr_name =
			  create_layer_extra_attr_table_name (dxf->prefix,
							      suffix,
							      lyr->layer_name,
							      "line");
		      if (append && check_extra_attr_table (handle, attr_name))
			{
			    /* appending into the already existing table */
			    if (!create_extra_stmt
				(handle, attr_name, &stmt_ext))
				return 0;
			}
		      else
			{
			    /* creating the Extra Attribute table */
			    if (!create_layer_line_extra_attr_table
				(handle, name, dxf->prefix, suffix,
				 lyr->layer_name, attr_name, &stmt_ext))
			      {
				  sqlite3_finalize (stmt);
				  return 0;
			      }
			}
		  }
		ret = sqlite3_exec (handle, "BEGIN", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
		  {
		      spatialite_e ("BEGIN %s error: %s\n", name,
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
			    spatialite_e ("INSERT %s error: %s\n", name,
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
					spatialite_e ("INSERT %s error: %s\n",
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
		      spatialite_e ("COMMIT %s error: %s\n", name,
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
		if (append
		    && check_polyg_table (handle, name, dxf->srid,
					  lyr->is3Dpolyg))
		  {
		      /* appending into the already existing table */
		      if (!create_polyg_stmt (handle, name, &stmt))
			  return 0;
		  }
		else
		  {
		      /* creating a new table */
		      if (!create_layer_polyg_table
			  (handle, name, dxf->srid, lyr->is3Dpolyg, &stmt))
			{

			    sqlite3_free (name);
			    return 0;
			}
		  }
		if (lyr->hasExtraPolyg)
		  {
		      attr_name =
			  create_layer_extra_attr_table_name (dxf->prefix,
							      suffix,
							      lyr->layer_name,
							      "polyg");
		      if (append && check_extra_attr_table (handle, attr_name))
			{
			    /* appending into the already existing table */
			    if (!create_extra_stmt
				(handle, attr_name, &stmt_ext))
				return 0;
			}
		      else
			{
			    /* creating the Extra Attribute table */
			    if (!create_layer_polyg_extra_attr_table
				(handle, name, dxf->prefix, suffix,
				 lyr->layer_name, attr_name, &stmt_ext))
			      {
				  sqlite3_finalize (stmt);
				  return 0;
			      }
			}
		  }
		ret = sqlite3_exec (handle, "BEGIN", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
		  {
		      spatialite_e ("BEGIN %s error: %s\n", name,
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
			    spatialite_e ("INSERT %s error: %s\n", name,
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
					spatialite_e ("INSERT %s error: %s\n",
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
		      spatialite_e ("COMMIT %s error: %s\n", name,
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

	  if (hatch)
	    {
		/* creating and populating the HATCH-layer */
		if (dxf->prefix == NULL)
		  {
		      if (suffix)
			  name = sqlite3_mprintf ("%s_hatch", lyr->layer_name);
		      else
			  name = sqlite3_mprintf ("%s", lyr->layer_name);
		  }
		else
		  {
		      if (suffix)
			  name =
			      sqlite3_mprintf ("%s%s_hatch", dxf->prefix,
					       lyr->layer_name);
		      else
			  name =
			      sqlite3_mprintf ("%s%s", dxf->prefix,
					       lyr->layer_name);
		  }
		if (append && check_hatch_table (handle, name, dxf->srid))
		  {
		      /* appending into the already existing table */
		      if (!create_hatch_stmt (handle, name, &stmt))
			  return 0;
		  }
		else
		  {
		      /* creating a new table */
		      if (!create_layer_hatch_table
			  (handle, name, dxf->srid, &stmt))
			{

			    sqlite3_free (name);
			    return 0;
			}
		  }
		ret = sqlite3_exec (handle, "BEGIN", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
		  {
		      spatialite_e ("BEGIN %s error: %s\n", name,
				    sqlite3_errmsg (handle));
		      sqlite3_finalize (stmt);
		      if (stmt_ext != NULL)
			  sqlite3_finalize (stmt_ext);
		      sqlite3_free (name);
		      if (attr_name)
			  sqlite3_free (attr_name);
		      return 0;
		  }
		p_hatch = lyr->first_hatch;
		while (p_hatch != NULL)
		  {
		      gaiaDxfHatchSegmPtr segm;
		      sqlite3_reset (stmt);
		      sqlite3_clear_bindings (stmt);
		      sqlite3_bind_text (stmt, 1, lyr->layer_name,
					 strlen (lyr->layer_name),
					 SQLITE_STATIC);
		      if (p_hatch->boundary == NULL)
			  sqlite3_bind_null (stmt, 2);
		      else
			{
			    /* saving the Boundary Geometry */
			    gaiaToSpatiaLiteBlobWkb (p_hatch->boundary, &blob,
						     &blob_size);
			    sqlite3_bind_blob (stmt, 2, blob, blob_size, free);
			}
		      if (p_hatch->first_out == NULL)
			  sqlite3_bind_null (stmt, 3);
		      else
			{
			    /* saving the Pattern Geometry */
			    geom = gaiaAllocGeomColl ();
			    geom->Srid = dxf->srid;
			    geom->DeclaredType = GAIA_MULTILINESTRING;
			    segm = p_hatch->first_out;
			    while (segm != NULL)
			      {
				  gaiaLinestringPtr p_ln =
				      gaiaAddLinestringToGeomColl (geom, 2);
				  gaiaSetPoint (p_ln->Coords, 0, segm->x0,
						segm->y0);
				  gaiaSetPoint (p_ln->Coords, 1, segm->x1,
						segm->y1);
				  segm = segm->next;
			      }
			    gaiaToSpatiaLiteBlobWkb (geom, &blob2, &blob_size2);
			    gaiaFreeGeomColl (geom);
			    sqlite3_bind_blob (stmt, 3, blob2, blob_size2,
					       free);
			}
		      ret = sqlite3_step (stmt);
		      if (ret == SQLITE_DONE || ret == SQLITE_ROW)
			  ;
		      else
			{
			    spatialite_e ("INSERT %s error: %s\n", name,
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
		      p_hatch = p_hatch->next;
		  }
		sqlite3_finalize (stmt);
		if (stmt_ext != NULL)
		    sqlite3_finalize (stmt_ext);
		ret = sqlite3_exec (handle, "COMMIT", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
		  {
		      spatialite_e ("COMMIT %s error: %s\n", name,
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
    if (dxf->first == NULL)
	return 0;

    if (mode == GAIA_DXF_IMPORT_MIXED)
	ret = import_mixed (handle, dxf, append);
    else
	ret = import_by_layer (handle, dxf, append);
    return ret;
}
