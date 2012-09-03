/*

 metatables.c -- creating the metadata tables and related triggers

 version 4.0, 2012 September 2

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
 
Portions created by the Initial Developer are Copyright (C) 2008-2012
the Initial Developer. All Rights Reserved.

Contributor(s):
Pepijn Van Eeckhoudt <pepijnvaneeckhoudt@luciad.com>
(implementing Android support)

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

#include "config.h"

#include <spatialite/sqlite.h>
#include <spatialite/debug.h>

#include <spatialite/spatialite.h>
#include <spatialite.h>
#include <spatialite_private.h>
#include <spatialite/gaiaaux.h>
#include <spatialite/gaiageo.h>

struct spatial_index_str
{
/* a struct to implement a linked list of spatial-indexes */
    char ValidRtree;
    char ValidCache;
    char *TableName;
    char *ColumnName;
    struct spatial_index_str *Next;
};

static int
testSpatiaLiteHistory (sqlite3 * sqlite)
{
/* internal utility function:
/
/ checks if the SPATIALITE_HISTORY table already exists
/
*/
    int event_id = 0;
    int table_name = 0;
    int geometry_column = 0;
    int event = 0;
    int timestamp = 0;
    int ver_sqlite = 0;
    int ver_splite = 0;
    char sql[1024];
    int ret;
    const char *name;
    int i;
    char **results;
    int rows;
    int columns;
/* checking the SPATIALITE_HISTORY table */
    strcpy (sql, "PRAGMA table_info(spatialite_history)");
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		name = results[(i * columns) + 1];
		if (strcasecmp (name, "event_id") == 0)
		    event_id = 1;
		if (strcasecmp (name, "table_name") == 0)
		    table_name = 1;
		if (strcasecmp (name, "geometry_column") == 0)
		    geometry_column = 1;
		if (strcasecmp (name, "event") == 0)
		    event = 1;
		if (strcasecmp (name, "timestamp") == 0)
		    timestamp = 1;
		if (strcasecmp (name, "ver_sqlite") == 0)
		    ver_sqlite = 1;
		if (strcasecmp (name, "ver_splite") == 0)
		    ver_splite = 1;
	    }
      }
    sqlite3_free_table (results);
    if (event_id && table_name && geometry_column && event && timestamp
	&& ver_sqlite && ver_splite)
	return 1;
    return 0;
}

static int
checkSpatiaLiteHistory (sqlite3 * sqlite)
{
/* internal utility function:
/
/ checks if the SPATIALITE_HISTORY table already exists
/ if not, such table will then be created
/
*/
    char sql[1024];
    char *errMsg = NULL;
    int ret;

    if (testSpatiaLiteHistory (sqlite))
	return 1;

/* creating the SPATIALITE_HISTORY table */
    strcpy (sql, "CREATE TABLE IF NOT EXISTS ");
    strcat (sql, "spatialite_history (\n");
    strcat (sql, "event_id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,\n");
    strcat (sql, "table_name TEXT NOT NULL,\n");
    strcat (sql, "geometry_column TEXT,\n");
    strcat (sql, "event TEXT NOT NULL,\n");
    strcat (sql, "timestamp TEXT NOT NULL,\n");
    strcat (sql, "ver_sqlite TEXT NOT NULL,\n");
    strcat (sql, "ver_splite TEXT NOT NULL)");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
	return 0;

    if (testSpatiaLiteHistory (sqlite))
	return 1;
    return 0;
}

SPATIALITE_PRIVATE void
updateSpatiaLiteHistory (void *p_sqlite, const char *table,
			 const char *geom, const char *operation)
{
/* inserting a row in SPATIALITE_HISTORY */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    char sql[2048];
    sqlite3_stmt *stmt = NULL;
    int ret;

    if (checkSpatiaLiteHistory (sqlite) == 0)
	return;

    strcpy (sql, "INSERT INTO spatialite_history ");
    strcat (sql, "(event_id, table_name, geometry_column, event, timestamp, ");
    strcat (sql, "ver_sqlite, ver_splite) VALUES (NULL, ?, ?, ?, ");
    strcat (sql, "strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), ");
    strcat (sql, "sqlite_version(), spatialite_version())");
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n%s\n", sql, sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, table, strlen (table), SQLITE_STATIC);
    if (!geom)
	sqlite3_bind_null (stmt, 2);
    else
	sqlite3_bind_text (stmt, 2, geom, strlen (geom), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 3, operation, strlen (operation), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	goto stop;
    spatialite_e ("SQL error: %s\n", sqlite3_errmsg (sqlite));

  stop:
    if (stmt)
	sqlite3_finalize (stmt);
}

static int
create_views_geometry_columns (sqlite3 * sqlite)
{
    char sql[4186];
    char *errMsg = NULL;
    int ret;
/* creating the VIEWS_GEOMETRY_COLUMNS table */
    strcpy (sql, "CREATE TABLE IF NOT EXISTS ");
    strcat (sql, "views_geometry_columns (\n");
    strcat (sql, "view_name TEXT NOT NULL,\n");
    strcat (sql, "view_geometry TEXT NOT NULL,\n");
    strcat (sql, "view_rowid TEXT NOT NULL,\n");
    strcat (sql, "f_table_name TEXT NOT NULL,\n");
    strcat (sql, "f_geometry_column TEXT NOT NULL,\n");
    strcat (sql, "read_only INTEGER NOT NULL,\n");
    strcat (sql, "CONSTRAINT pk_geom_cols_views PRIMARY KEY ");
    strcat (sql, "(view_name, view_geometry),\n");
    strcat (sql, "CONSTRAINT fk_views_geom_cols FOREIGN KEY ");
    strcat (sql, "(f_table_name, f_geometry_column) ");
    strcat (sql, "REFERENCES geometry_columns ");
    strcat (sql, "(f_table_name, f_geometry_column) ");
    strcat (sql, "ON DELETE CASCADE,\n");
    strcat (sql, "CONSTRAINT ck_vw_rdonly CHECK (read_only IN ");
    strcat (sql, "(0,1)))");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
/* creating an INDEX supporting the GEOMETRY_COLUMNS FK */
    strcpy (sql, "CREATE INDEX IF NOT EXISTS ");
    strcat (sql, "idx_viewsjoin ON views_geometry_columns\n");
    strcat (sql, "(f_table_name, f_geometry_column)");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
/* creating the VIEWS_GEOMETRY_COLUMNS triggers */
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vwgc_view_name_insert\n");
    strcat (sql, "BEFORE INSERT ON 'views_geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns violates constraint: ");
    strcat (sql, "view_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.view_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns violates constraint: ");
    strcat (sql, "view_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.view_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns violates constraint: \n");
    strcat (sql, "view_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.view_name <> lower(NEW.view_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vwgc_view_name_update\n");
    strcat (sql, "BEFORE UPDATE OF 'view_name' ON 'views_geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns violates constraint: ");
    strcat (sql, "view_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.view_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns violates constraint: ");
    strcat (sql, "view_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.view_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns violates constraint: ");
    strcat (sql, "view_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.view_name <> lower(NEW.view_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vwgc_view_geometry_insert\n");
    strcat (sql, "BEFORE INSERT ON 'views_geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns violates constraint: ");
    strcat (sql, "view_geometry value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.view_geometry LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns violates constraint: \n");
    strcat (sql, "view_geometry value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.view_geometry LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns violates constraint: ");
    strcat (sql, "view_geometry value must be lower case')\n");
    strcat (sql, "WHERE NEW.view_geometry <> lower(NEW.view_geometry);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vwgc_view_geometry_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'view_geometry' ON 'views_geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns violates constraint: ");
    strcat (sql, "view_geometry value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.view_geometry LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns violates constraint: \n");
    strcat (sql, "view_geometry value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.view_geometry LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns violates constraint: ");
    strcat (sql, "view_geometry value must be lower case')\n");
    strcat (sql, "WHERE NEW.view_geometry <> lower(NEW.view_geometry);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vwgc_view_rowid_update\n");
    strcat (sql, "BEFORE UPDATE OF 'view_rowid' ON 'views_geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns violates constraint: ");
    strcat (sql, "view_rowid value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns violates constraint: ");
    strcat (sql, "view_rowid value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.view_rowid LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns violates constraint: ");
    strcat (sql, "view_rowid value must be lower case')\n");
    strcat (sql, "WHERE NEW.view_rowid <> lower(NEW.view_rowid);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vwgc_view_rowid_insert\n");
    strcat (sql, "BEFORE INSERT ON 'views_geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns violates constraint: ");
    strcat (sql, "view_rowid value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.view_rowid LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns violates constraint: \n");
    strcat (sql, "view_rowid value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.view_rowid LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns violates constraint: ");
    strcat (sql, "view_rowid value must be lower case')\n");
    strcat (sql, "WHERE NEW.view_rowid <> lower(NEW.view_rowid);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vwgc_f_table_name_insert\n");
    strcat (sql, "BEFORE INSERT ON 'views_geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns violates constraint: ");
    strcat (sql, "f_table_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_table_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns violates constraint: ");
    strcat (sql, "f_table_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.f_table_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns violates constraint: \n");
    strcat (sql, "f_table_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.f_table_name <> lower(NEW.f_table_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vwgc_f_table_name_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'f_table_name' ON 'views_geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns violates constraint: ");
    strcat (sql, "f_table_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_table_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns violates constraint: ");
    strcat (sql, "f_table_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.f_table_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns violates constraint: ");
    strcat (sql, "f_table_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.f_table_name <> lower(NEW.f_table_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql,
	    "CREATE TRIGGER IF NOT EXISTS vwgc_f_geometry_column_insert\n");
    strcat (sql, "BEFORE INSERT ON 'views_geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns violates constraint: ");
    strcat (sql, "f_geometry_column value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns violates constraint: \n");
    strcat (sql, "f_geometry_column value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns violates constraint: ");
    strcat (sql, "f_geometry_column value must be lower case')\n");
    strcat (sql,
	    "WHERE NEW.f_geometry_column <> lower(NEW.f_geometry_column);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql,
	    "CREATE TRIGGER IF NOT EXISTS vwgc_f_geometry_column_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'f_geometry_column' ON 'views_geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns violates constraint: ");
    strcat (sql, "f_geometry_column value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns violates constraint: ");
    strcat (sql, "f_geometry_column value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns violates constraint: ");
    strcat (sql, "f_geometry_column value must be lower case')\n");
    strcat (sql,
	    "WHERE NEW.f_geometry_column <> lower(NEW.f_geometry_column);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    return 1;
}

static int
create_virts_geometry_columns (sqlite3 * sqlite)
{
    char sql[4186];
    char *errMsg = NULL;
    int ret;
/* creating the VIRTS_GEOMETRY_COLUMNS table */
    strcpy (sql, "CREATE TABLE IF NOT EXISTS ");
    strcat (sql, "virts_geometry_columns (\n");
    strcat (sql, "virt_name TEXT NOT NULL,\n");
    strcat (sql, "virt_geometry TEXT NOT NULL,\n");
    strcat (sql, "geometry_type INTEGER NOT NULL,\n");
    strcat (sql, "coord_dimension INTEGER NOT NULL,\n");
    strcat (sql, "srid INTEGER NOT NULL,\n");
    strcat (sql, "CONSTRAINT pk_geom_cols_virts PRIMARY KEY ");
    strcat (sql, "(virt_name, virt_geometry),\n");
    strcat (sql, "CONSTRAINT fk_vgc_srid FOREIGN KEY ");
    strcat (sql, "(srid) REFERENCES spatial_ref_sys (srid))");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
/* creating an INDEX supporting the SPATIAL_REF_SYS FK */
    strcpy (sql, "CREATE INDEX IF NOT EXISTS ");
    strcat (sql, "idx_virtssrid ON virts_geometry_columns\n");
    strcat (sql, "(srid)");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
/* creating the VIRTS_GEOMETRY_COLUMNS triggers */
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vtgc_virt_name_insert\n");
    strcat (sql, "BEFORE INSERT ON 'virts_geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on virts_geometry_columns violates constraint: ");
    strcat (sql, "virt_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.virt_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on virts_geometry_columns violates constraint: ");
    strcat (sql, "virt_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.virt_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on virts_geometry_columns violates constraint: \n");
    strcat (sql, "virt_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.virt_name <> lower(NEW.virt_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vtgc_virt_name_update\n");
    strcat (sql, "BEFORE UPDATE OF 'virt_name' ON 'virts_geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on virts_geometry_columns violates constraint: ");
    strcat (sql, "virt_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.virt_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on virts_geometry_columns violates constraint: ");
    strcat (sql, "virt_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.virt_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on virts_geometry_columns violates constraint: ");
    strcat (sql, "virt_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.virt_name <> lower(NEW.virt_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vtgc_virt_geometry_insert\n");
    strcat (sql, "BEFORE INSERT ON 'virts_geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on virts_geometry_columns violates constraint: ");
    strcat (sql, "virt_geometry value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.virt_geometry LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on virts_geometry_columns violates constraint: \n");
    strcat (sql, "virt_geometry value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.virt_geometry LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on virts_geometry_columns violates constraint: ");
    strcat (sql, "virt_geometry value must be lower case')\n");
    strcat (sql, "WHERE NEW.virt_geometry <> lower(NEW.virt_geometry);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vtgc_virt_geometry_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'virt_geometry' ON 'virts_geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on virts_geometry_columns violates constraint: ");
    strcat (sql, "virt_geometry value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.virt_geometry LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on virts_geometry_columns violates constraint: \n");
    strcat (sql, "virt_geometry value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.virt_geometry LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on virts_geometry_columns violates constraint: ");
    strcat (sql, "virt_geometry value must be lower case')\n");
    strcat (sql, "WHERE NEW.virt_geometry <> lower(NEW.virt_geometry);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vtgc_geometry_type_insert\n");
    strcat (sql, "BEFORE INSERT ON 'virts_geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql, "SELECT RAISE(ABORT,'geometry_type must be one of ");
    strcat (sql, "0,1,2,3,4,5,6,7,");
    strcat (sql, "1000,1001,1002,1003,1004,1005,1006,1007,");
    strcat (sql, "2000,2001,2002,2003,2004,2005,2006,2007,");
    strcat (sql, "3000,3001,3002,3003,3004,3005,3006,3007')\n");
    strcat (sql, "WHERE NOT(NEW.geometry_type IN (0,1,2,3,4,5,6,7,");
    strcat (sql, "1000,1001,1002,1003,1004,1005,1006,1007,");
    strcat (sql, "2000,2001,2002,2003,2004,2005,2006,2007,");
    strcat (sql, "3000,3001,3002,3003,3004,3005,3006,3007));\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vtgc_geometry_type_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'geometry_type' ON 'virts_geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql, "SELECT RAISE(ABORT,'geometry_type must be one of ");
    strcat (sql, "0,1,2,3,4,5,6,7,");
    strcat (sql, "1000,1001,1002,1003,1004,1005,1006,1007,");
    strcat (sql, "2000,2001,2002,2003,2004,2005,2006,2007,");
    strcat (sql, "3000,3001,3002,3003,3004,3005,3006,3007')\n");
    strcat (sql, "WHERE NOT(NEW.geometry_type IN (0,1,2,3,4,5,6,7,");
    strcat (sql, "1000,1001,1002,1003,1004,1005,1006,1007,");
    strcat (sql, "2000,2001,2002,2003,2004,2005,2006,2007,");
    strcat (sql, "3000,3001,3002,3003,3004,3005,3006,3007));\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vtgc_coord_dimension_insert\n");
    strcat (sql, "BEFORE INSERT ON 'virts_geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'coord_dimension must be one of 2,3,4')\n");
    strcat (sql, "WHERE NOT(NEW.coord_dimension IN (2,3,4));\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vtgc_coord_dimension_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'coord_dimension' ON 'virts_geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'coord_dimension must be one of 2,3,4')\n");
    strcat (sql, "WHERE NOT(NEW.coord_dimension IN (2,3,4));\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    return 1;
}

static int
create_geometry_columns_statistics (sqlite3 * sqlite)
{
    char sql[4186];
    char *errMsg = NULL;
    int ret;
/* creating the GEOMETRY_COLUMNS_STATISTICS table */
    strcpy (sql, "CREATE TABLE IF NOT EXISTS ");
    strcat (sql, "geometry_columns_statistics (\n");
    strcat (sql, "f_table_name TEXT NOT NULL,\n");
    strcat (sql, "f_geometry_column TEXT NOT NULL,\n");
    strcat (sql, "last_verified TIMESTAMP,\n");
    strcat (sql, "row_count INTEGER,\n");
    strcat (sql, "extent_min_x DOUBLE,\n");
    strcat (sql, "extent_min_y DOUBLE,\n");
    strcat (sql, "extent_max_x DOUBLE,\n");
    strcat (sql, "extent_max_y DOUBLE,\n");
    strcat (sql, "CONSTRAINT pk_gc_statistics PRIMARY KEY ");
    strcat (sql, "(f_table_name, f_geometry_column),\n");
    strcat (sql, "CONSTRAINT fk_gc_statistics FOREIGN KEY ");
    strcat (sql, "(f_table_name, f_geometry_column) REFERENCES ");
    strcat (sql, "geometry_columns (f_table_name, f_geometry_column) ");
    strcat (sql, "ON DELETE CASCADE)");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
/* creating the GEOMETRY_COLUMNS_STATISTICS triggers */
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS gcs_f_table_name_insert\n");
    strcat (sql, "BEFORE INSERT ON 'geometry_columns_statistics'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns_statistics violates constraint: ");
    strcat (sql, "f_table_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_table_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns_statistics violates constraint: ");
    strcat (sql, "f_table_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.f_table_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns_statistics violates constraint: \n");
    strcat (sql, "f_table_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.f_table_name <> lower(NEW.f_table_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS gcs_f_table_name_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'f_table_name' ON 'geometry_columns_statistics'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns_statistics violates constraint: ");
    strcat (sql, "f_table_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_table_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns_statistics violates constraint: ");
    strcat (sql, "f_table_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.f_table_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns_statistics violates constraint: ");
    strcat (sql, "f_table_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.f_table_name <> lower(NEW.f_table_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS gcs_f_geometry_column_insert\n");
    strcat (sql, "BEFORE INSERT ON 'geometry_columns_statistics'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns_statistics violates constraint: ");
    strcat (sql, "f_geometry_column value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns_statistics violates constraint: \n");
    strcat (sql, "f_geometry_column value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns_statistics violates constraint: ");
    strcat (sql, "f_geometry_column value must be lower case')\n");
    strcat (sql,
	    "WHERE NEW.f_geometry_column <> lower(NEW.f_geometry_column);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS gcs_f_geometry_column_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'f_geometry_column' ON 'geometry_columns_statistics'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns_statistics violates constraint: ");
    strcat (sql, "f_geometry_column value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns_statistics violates constraint: ");
    strcat (sql, "f_geometry_column value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns_statistics violates constraint: ");
    strcat (sql, "f_geometry_column value must be lower case')\n");
    strcat (sql,
	    "WHERE NEW.f_geometry_column <> lower(NEW.f_geometry_column);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    return 1;
}

static int
create_views_geometry_columns_statistics (sqlite3 * sqlite)
{
    char sql[4186];
    char *errMsg = NULL;
    int ret;
/* creating the VIEWS_GEOMETRY_COLUMNS_STATISTICS table */
    strcpy (sql, "CREATE TABLE IF NOT EXISTS ");
    strcat (sql, "views_geometry_columns_statistics (\n");
    strcat (sql, "view_name TEXT NOT NULL,\n");
    strcat (sql, "view_geometry TEXT NOT NULL,\n");
    strcat (sql, "last_verified TIMESTAMP,\n");
    strcat (sql, "row_count INTEGER,\n");
    strcat (sql, "extent_min_x DOUBLE,\n");
    strcat (sql, "extent_min_y DOUBLE,\n");
    strcat (sql, "extent_max_x DOUBLE,\n");
    strcat (sql, "extent_max_y DOUBLE,\n");
    strcat (sql, "CONSTRAINT pk_vwgc_statistics PRIMARY KEY ");
    strcat (sql, "(view_name, view_geometry),\n");
    strcat (sql, "CONSTRAINT fk_vwgc_statistics FOREIGN KEY ");
    strcat (sql, "(view_name, view_geometry) REFERENCES ");
    strcat (sql, "views_geometry_columns (view_name, view_geometry) ");
    strcat (sql, "ON DELETE CASCADE)");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
/* creating the VIEWS_GEOMETRY_COLUMNS_STATISTICS triggers */
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vwgcs_view_name_insert\n");
    strcat (sql, "BEFORE INSERT ON 'views_geometry_columns_statistics'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns_statistics violates constraint: ");
    strcat (sql, "view_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.view_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns_statistics violates constraint: ");
    strcat (sql, "view_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.view_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns_statistics violates constraint: \n");
    strcat (sql, "view_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.view_name <> lower(NEW.view_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vwgcs_view_name_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'view_name' ON 'views_geometry_columns_statistics'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns_statistics violates constraint: ");
    strcat (sql, "view_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.view_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns_statistics violates constraint: ");
    strcat (sql, "view_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.view_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns_statistics violates constraint: ");
    strcat (sql, "view_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.view_name <> lower(NEW.view_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vwgcs_view_geometry_insert\n");
    strcat (sql, "BEFORE INSERT ON 'views_geometry_columns_statistics'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns_statistics violates constraint: ");
    strcat (sql, "view_geometry value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.view_geometry LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns_statistics violates constraint: \n");
    strcat (sql, "view_geometry value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.view_geometry LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns_statistics violates constraint: ");
    strcat (sql, "view_geometry value must be lower case')\n");
    strcat (sql, "WHERE NEW.view_geometry <> lower(NEW.view_geometry);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vwgcs_view_geometry_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'view_geometry' ON 'views_geometry_columns_statistics'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns_statistics violates constraint: ");
    strcat (sql, "view_geometry value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.view_geometry LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns_statistics violates constraint: \n");
    strcat (sql, "view_geometry value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.view_geometry LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns_statistics violates constraint: ");
    strcat (sql, "view_geometry value must be lower case')\n");
    strcat (sql, "WHERE NEW.view_geometry <> lower(NEW.view_geometry);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    return 1;
}

static int
create_virts_geometry_columns_statistics (sqlite3 * sqlite)
{
    char sql[4186];
    char *errMsg = NULL;
    int ret;
/* creating the VIRTS_GEOMETRY_COLUMNS_STATISTICS table */
    strcpy (sql, "CREATE TABLE IF NOT EXISTS ");
    strcat (sql, "virts_geometry_columns_statistics (\n");
    strcat (sql, "virt_name TEXT NOT NULL,\n");
    strcat (sql, "virt_geometry TEXT NOT NULL,\n");
    strcat (sql, "last_verified TIMESTAMP,\n");
    strcat (sql, "row_count INTEGER,\n");
    strcat (sql, "extent_min_x DOUBLE,\n");
    strcat (sql, "extent_min_y DOUBLE,\n");
    strcat (sql, "extent_max_x DOUBLE,\n");
    strcat (sql, "extent_max_y DOUBLE,\n");
    strcat (sql, "CONSTRAINT pk_vrtgc_statistics PRIMARY KEY ");
    strcat (sql, "(virt_name, virt_geometry),\n");
    strcat (sql, "CONSTRAINT fk_vrtgc_statistics FOREIGN KEY ");
    strcat (sql, "(virt_name, virt_geometry) REFERENCES ");
    strcat (sql, "virts_geometry_columns (virt_name, virt_geometry) ");
    strcat (sql, "ON DELETE CASCADE)");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
/* creating the VIRTS_GEOMETRY_COLUMNS_STATISTICS triggers */
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vtgcs_virt_name_insert\n");
    strcat (sql, "BEFORE INSERT ON 'virts_geometry_columns_statistics'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on virts_geometry_columns_statistics violates constraint: ");
    strcat (sql, "virt_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.virt_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on virts_geometry_columns_statistics violates constraint: ");
    strcat (sql, "virt_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.virt_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on virts_geometry_columns_statistics violates constraint: \n");
    strcat (sql, "virt_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.virt_name <> lower(NEW.virt_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vtgcs_virt_name_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'virt_name' ON 'virts_geometry_columns_statistics'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on virts_geometry_columns_statistics violates constraint: ");
    strcat (sql, "virt_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.virt_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on virts_geometry_columns_statistics violates constraint: ");
    strcat (sql, "virt_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.virt_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on virts_geometry_columns_statistics violates constraint: ");
    strcat (sql, "virt_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.virt_name <> lower(NEW.virt_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vtgcs_virt_geometry_insert\n");
    strcat (sql, "BEFORE INSERT ON 'virts_geometry_columns_statistics'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on virts_geometry_columns_statistics violates constraint: ");
    strcat (sql, "virt_geometry value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.virt_geometry LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on virts_geometry_columns_statistics violates constraint: \n");
    strcat (sql, "virt_geometry value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.virt_geometry LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on virts_geometry_columns_statistics violates constraint: ");
    strcat (sql, "virt_geometry value must be lower case')\n");
    strcat (sql, "WHERE NEW.virt_geometry <> lower(NEW.virt_geometry);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vtgcs_virt_geometry_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'virt_geometry' ON 'virts_geometry_columns_statistics'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on virts_geometry_columns_statistics violates constraint: ");
    strcat (sql, "virt_geometry value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.virt_geometry LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on virts_geometry_columns_statistics violates constraint: \n");
    strcat (sql, "virt_geometry value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.virt_geometry LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on virts_geometry_columns_statistics violates constraint: ");
    strcat (sql, "virt_geometry value must be lower case')\n");
    strcat (sql, "WHERE NEW.virt_geometry <> lower(NEW.virt_geometry);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    return 1;
}

static int
create_geometry_columns_field_infos (sqlite3 * sqlite)
{
    char sql[4186];
    char *errMsg = NULL;
    int ret;
/* creating the GEOMETRY_COLUMNS_FIELD_INFOS table */
    strcpy (sql, "CREATE TABLE IF NOT EXISTS ");
    strcat (sql, "geometry_columns_field_infos (\n");
    strcat (sql, "f_table_name TEXT NOT NULL,\n");
    strcat (sql, "f_geometry_column TEXT NOT NULL,\n");
    strcat (sql, "ordinal INTEGER NOT NULL,\n");
    strcat (sql, "column_name TEXT NOT NULL,\n");
    strcat (sql, "null_values INTEGER NOT NULL,\n");
    strcat (sql, "integer_values INTEGER NOT NULL,\n");
    strcat (sql, "double_values INTEGER NOT NULL,\n");
    strcat (sql, "text_values INTEGER NOT NULL,\n");
    strcat (sql, "blob_values INTEGER NOT NULL,\n");
    strcat (sql, "max_size INTEGER,\n");
    strcat (sql, "integer_min INTEGER,\n");
    strcat (sql, "integer_max INTEGER,\n");
    strcat (sql, "double_min DOUBLE,\n");
    strcat (sql, "double_max DOUBLE,\n");
    strcat (sql, "CONSTRAINT pk_gcfld_infos PRIMARY KEY ");
    strcat (sql, "(f_table_name, f_geometry_column, ordinal, column_name),\n");
    strcat (sql, "CONSTRAINT fk_gcfld_infos FOREIGN KEY ");
    strcat (sql, "(f_table_name, f_geometry_column) REFERENCES ");
    strcat (sql, "geometry_columns (f_table_name, f_geometry_column) ");
    strcat (sql, "ON DELETE CASCADE)");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
/* creating the GEOMETRY_COLUMNS_FIELD_INFOS triggers */
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS gcfi_f_table_name_insert\n");
    strcat (sql, "BEFORE INSERT ON 'geometry_columns_field_infos'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns_field_infos violates constraint: ");
    strcat (sql, "f_table_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_table_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns_field_infos violates constraint: ");
    strcat (sql, "f_table_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.f_table_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns_field_infos violates constraint: \n");
    strcat (sql, "f_table_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.f_table_name <> lower(NEW.f_table_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS gcfi_f_table_name_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'f_table_name' ON 'geometry_columns_field_infos'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns_field_infos violates constraint: ");
    strcat (sql, "f_table_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_table_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns_field_infos violates constraint: ");
    strcat (sql, "f_table_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.f_table_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns_field_infos violates constraint: ");
    strcat (sql, "f_table_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.f_table_name <> lower(NEW.f_table_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql,
	    "CREATE TRIGGER IF NOT EXISTS gcfi_f_geometry_column_insert\n");
    strcat (sql, "BEFORE INSERT ON 'geometry_columns_field_infos'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns_field_infos violates constraint: ");
    strcat (sql, "f_geometry_column value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns_field_infos violates constraint: \n");
    strcat (sql, "f_geometry_column value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns_field_infos violates constraint: ");
    strcat (sql, "f_geometry_column value must be lower case')\n");
    strcat (sql,
	    "WHERE NEW.f_geometry_column <> lower(NEW.f_geometry_column);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql,
	    "CREATE TRIGGER IF NOT EXISTS gcfi_f_geometry_column_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'f_geometry_column' ON 'geometry_columns_field_infos'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns_field_infos violates constraint: ");
    strcat (sql, "f_geometry_column value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns_field_infos violates constraint: ");
    strcat (sql, "f_geometry_column value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns_field_infos violates constraint: ");
    strcat (sql, "f_geometry_column value must be lower case')\n");
    strcat (sql,
	    "WHERE NEW.f_geometry_column <> lower(NEW.f_geometry_column);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    return 1;
}

static int
create_views_geometry_columns_field_infos (sqlite3 * sqlite)
{
    char sql[4186];
    char *errMsg = NULL;
    int ret;
/* creating the VIEWS_COLUMNS_FIELD_INFOS table */
    strcpy (sql, "CREATE TABLE IF NOT EXISTS ");
    strcat (sql, "views_geometry_columns_field_infos (\n");
    strcat (sql, "view_name TEXT NOT NULL,\n");
    strcat (sql, "view_geometry TEXT NOT NULL,\n");
    strcat (sql, "ordinal INTEGER NOT NULL,\n");
    strcat (sql, "column_name TEXT NOT NULL,\n");
    strcat (sql, "null_values INTEGER NOT NULL,\n");
    strcat (sql, "integer_values INTEGER NOT NULL,\n");
    strcat (sql, "double_values INTEGER NOT NULL,\n");
    strcat (sql, "text_values INTEGER NOT NULL,\n");
    strcat (sql, "blob_values INTEGER NOT NULL,\n");
    strcat (sql, "max_size INTEGER,\n");
    strcat (sql, "integer_min INTEGER,\n");
    strcat (sql, "integer_max INTEGER,\n");
    strcat (sql, "double_min DOUBLE,\n");
    strcat (sql, "double_max DOUBLE,\n");
    strcat (sql, "CONSTRAINT pk_vwgcfld_infos PRIMARY KEY ");
    strcat (sql, "(view_name, view_geometry, ordinal, column_name),\n");
    strcat (sql, "CONSTRAINT fk_vwgcfld_infos FOREIGN KEY ");
    strcat (sql, "(view_name, view_geometry) REFERENCES ");
    strcat (sql, "views_geometry_columns (view_name, view_geometry) ");
    strcat (sql, "ON DELETE CASCADE)");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
/* creating the VIEWS_COLUMNS_FIELD_INFOS triggers */
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vwgcfi_view_name_insert\n");
    strcat (sql, "BEFORE INSERT ON 'views_geometry_columns_field_infos'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns_field_infos violates constraint: ");
    strcat (sql, "view_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.view_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns_field_infos violates constraint: ");
    strcat (sql, "view_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.view_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns_field_infos violates constraint: \n");
    strcat (sql, "view_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.view_name <> lower(NEW.view_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vwgcfi_view_name_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'view_name' ON 'views_geometry_columns_field_infos'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns_field_infos violates constraint: ");
    strcat (sql, "view_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.view_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns_field_infos violates constraint: ");
    strcat (sql, "view_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.view_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns_field_infos violates constraint: ");
    strcat (sql, "view_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.view_name <> lower(NEW.view_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vwgcfi_view_geometry_insert\n");
    strcat (sql, "BEFORE INSERT ON 'views_geometry_columns_field_infos'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns_field_infos violates constraint: ");
    strcat (sql, "view_geometry value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.view_geometry LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns_field_infos violates constraint: \n");
    strcat (sql, "view_geometry value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.view_geometry LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns_field_infos violates constraint: ");
    strcat (sql, "view_geometry value must be lower case')\n");
    strcat (sql, "WHERE NEW.view_geometry <> lower(NEW.view_geometry);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vwgcfi_view_geometry_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'view_geometry' ON 'views_geometry_columns_field_infos'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns_field_infos violates constraint: ");
    strcat (sql, "view_geometry value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.view_geometry LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns_field_infos violates constraint: \n");
    strcat (sql, "view_geometry value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.view_geometry LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns_field_infos violates constraint: ");
    strcat (sql, "view_geometry value must be lower case')\n");
    strcat (sql, "WHERE NEW.view_geometry <> lower(NEW.view_geometry);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    return 1;
}

static int
create_virts_geometry_columns_field_infos (sqlite3 * sqlite)
{
    char sql[4186];
    char *errMsg = NULL;
    int ret;
/* creating the VIRTS_GEOMETRY_COLUMNS_FIELD_INFOS table */
    strcpy (sql, "CREATE TABLE IF NOT EXISTS ");
    strcat (sql, "virts_geometry_columns_field_infos (\n");
    strcat (sql, "virt_name TEXT NOT NULL,\n");
    strcat (sql, "virt_geometry TEXT NOT NULL,\n");
    strcat (sql, "ordinal INTEGER NOT NULL,\n");
    strcat (sql, "column_name TEXT NOT NULL,\n");
    strcat (sql, "null_values INTEGER NOT NULL,\n");
    strcat (sql, "integer_values INTEGER NOT NULL,\n");
    strcat (sql, "double_values INTEGER NOT NULL,\n");
    strcat (sql, "text_values INTEGER NOT NULL,\n");
    strcat (sql, "blob_values INTEGER NOT NULL,\n");
    strcat (sql, "max_size INTEGER,\n");
    strcat (sql, "integer_min INTEGER,\n");
    strcat (sql, "integer_max INTEGER,\n");
    strcat (sql, "double_min DOUBLE,\n");
    strcat (sql, "double_max DOUBLE,\n");
    strcat (sql, "CONSTRAINT pk_vrtgcfld_infos PRIMARY KEY ");
    strcat (sql, "(virt_name, virt_geometry, ordinal, column_name),\n");
    strcat (sql, "CONSTRAINT fk_vrtgcfld_infos FOREIGN KEY ");
    strcat (sql, "(virt_name, virt_geometry) REFERENCES ");
    strcat (sql, "virts_geometry_columns (virt_name, virt_geometry) ");
    strcat (sql, "ON DELETE CASCADE)");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
/* creating the VIRTS_GEOMETRY_COLUMNS_FIELD_INFOS triggers */
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vtgcfi_virt_name_insert\n");
    strcat (sql, "BEFORE INSERT ON 'virts_geometry_columns_field_infos'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on virts_geometry_columns_field_infos violates constraint: ");
    strcat (sql, "virt_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.virt_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on virts_geometry_columns_field_infos violates constraint: ");
    strcat (sql, "virt_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.virt_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on virts_geometry_columns_field_infos violates constraint: \n");
    strcat (sql, "virt_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.virt_name <> lower(NEW.virt_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vtgcfi_virt_name_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'virt_name' ON 'virts_geometry_columns_field_infos'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on virts_geometry_columns_field_infos violates constraint: ");
    strcat (sql, "virt_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.virt_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on virts_geometry_columns_field_infos violates constraint: ");
    strcat (sql, "virt_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.virt_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on virts_geometry_columns_field_infos violates constraint: ");
    strcat (sql, "virt_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.virt_name <> lower(NEW.virt_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vtgcfi_virt_geometry_insert\n");
    strcat (sql, "BEFORE INSERT ON 'virts_geometry_columns_field_infos'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on virts_geometry_columns_field_infos violates constraint: ");
    strcat (sql, "virt_geometry value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.virt_geometry LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on virts_geometry_columns_field_infos violates constraint: \n");
    strcat (sql, "virt_geometry value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.virt_geometry LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on virts_geometry_columns_field_infos violates constraint: ");
    strcat (sql, "virt_geometry value must be lower case')\n");
    strcat (sql, "WHERE NEW.virt_geometry <> lower(NEW.virt_geometry);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vtgcfi_virt_geometry_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'virt_geometry' ON 'virts_geometry_columns_field_infos'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on virts_geometry_columns_field_infos violates constraint: ");
    strcat (sql, "virt_geometry value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.virt_geometry LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on virts_geometry_columns_field_infos violates constraint: \n");
    strcat (sql, "virt_geometry value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.virt_geometry LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on virts_geometry_columns_field_infos violates constraint: ");
    strcat (sql, "virt_geometry value must be lower case')\n");
    strcat (sql, "WHERE NEW.virt_geometry <> lower(NEW.virt_geometry);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    return 1;
}

static int
create_geometry_columns_times (sqlite3 * sqlite)
{
    char sql[4186];
    char *errMsg = NULL;
    int ret;
/* creating the GEOMETRY_COLUMNS_TIME table */
    strcpy (sql, "CREATE TABLE IF NOT EXISTS ");
    strcat (sql, "geometry_columns_time (\n");
    strcat (sql, "f_table_name TEXT NOT NULL,\n");
    strcat (sql, "f_geometry_column TEXT NOT NULL,\n");
    strcat (sql,
	    "last_insert TIMESTAMP NOT NULL DEFAULT '0000-01-01T00:00:00.000Z',\n");
    strcat (sql,
	    "last_update TIMESTAMP NOT NULL DEFAULT '0000-01-01T00:00:00.000Z',\n");
    strcat (sql,
	    "last_delete TIMESTAMP NOT NULL DEFAULT '0000-01-01T00:00:00.000Z',\n");
    strcat (sql, "CONSTRAINT pk_gc_time PRIMARY KEY ");
    strcat (sql, "(f_table_name, f_geometry_column),\n");
    strcat (sql, "CONSTRAINT fk_gc_time FOREIGN KEY ");
    strcat (sql, "(f_table_name, f_geometry_column) ");
    strcat (sql, "REFERENCES geometry_columns ");
    strcat (sql, "(f_table_name, f_geometry_column) ");
    strcat (sql, "ON DELETE CASCADE)");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
/* creating the GEOMETRY_COLUMNS_TIME triggers */
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS gctm_f_table_name_insert\n");
    strcat (sql, "BEFORE INSERT ON 'geometry_columns_time'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns_time violates constraint: ");
    strcat (sql, "f_table_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_table_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns_time violates constraint: ");
    strcat (sql, "f_table_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.f_table_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns_time violates constraint: \n");
    strcat (sql, "f_table_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.f_table_name <> lower(NEW.f_table_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS gctm_f_table_name_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'f_table_name' ON 'geometry_columns_time'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns_time violates constraint: ");
    strcat (sql, "f_table_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_table_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns_time violates constraint: ");
    strcat (sql, "f_table_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.f_table_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns_time violates constraint: ");
    strcat (sql, "f_table_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.f_table_name <> lower(NEW.f_table_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql,
	    "CREATE TRIGGER IF NOT EXISTS gctm_f_geometry_column_insert\n");
    strcat (sql, "BEFORE INSERT ON 'geometry_columns_time'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns_time violates constraint: ");
    strcat (sql, "f_geometry_column value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns_time violates constraint: \n");
    strcat (sql, "f_geometry_column value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns_time violates constraint: ");
    strcat (sql, "f_geometry_column value must be lower case')\n");
    strcat (sql,
	    "WHERE NEW.f_geometry_column <> lower(NEW.f_geometry_column);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql,
	    "CREATE TRIGGER IF NOT EXISTS gctm_f_geometry_column_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'f_geometry_column' ON 'geometry_columns_time'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns_time violates constraint: ");
    strcat (sql, "f_geometry_column value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns_time violates constraint: ");
    strcat (sql, "f_geometry_column value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns_time violates constraint: ");
    strcat (sql, "f_geometry_column value must be lower case')\n");
    strcat (sql,
	    "WHERE NEW.f_geometry_column <> lower(NEW.f_geometry_column);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    return 1;
}

static int
create_geometry_columns_auth (sqlite3 * sqlite)
{
    char sql[4186];
    char *errMsg = NULL;
    int ret;
/* creating the GEOMETRY_COLUMNS_AUTH table */
    strcpy (sql, "CREATE TABLE IF NOT EXISTS ");
    strcat (sql, "geometry_columns_auth (\n");
    strcat (sql, "f_table_name TEXT NOT NULL,\n");
    strcat (sql, "f_geometry_column TEXT NOT NULL,\n");
    strcat (sql, "read_only INTEGER NOT NULL,\n");
    strcat (sql, "hidden INTEGER NOT NULL,\n");
    strcat (sql, "CONSTRAINT pk_gc_auth PRIMARY KEY ");
    strcat (sql, "(f_table_name, f_geometry_column),\n");
    strcat (sql, "CONSTRAINT fk_gc_auth FOREIGN KEY ");
    strcat (sql, "(f_table_name, f_geometry_column) ");
    strcat (sql, "REFERENCES geometry_columns ");
    strcat (sql, "(f_table_name, f_geometry_column) ");
    strcat (sql, "ON DELETE CASCADE,\n");
    strcat (sql, "CONSTRAINT ck_gc_ronly CHECK (read_only IN ");
    strcat (sql, "(0,1)),\n");
    strcat (sql, "CONSTRAINT ck_gc_hidden CHECK (hidden IN ");
    strcat (sql, "(0,1)))");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
/* creating the GEOMETRY_COLUMNS_AUTH triggers */
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS gcau_f_table_name_insert\n");
    strcat (sql, "BEFORE INSERT ON 'geometry_columns_auth'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns_auth violates constraint: ");
    strcat (sql, "f_table_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_table_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns_auth violates constraint: ");
    strcat (sql, "f_table_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.f_table_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns_auth violates constraint: \n");
    strcat (sql, "f_table_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.f_table_name <> lower(NEW.f_table_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS gcau_f_table_name_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'f_table_name' ON 'geometry_columns_auth'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns_auth violates constraint: ");
    strcat (sql, "f_table_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_table_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns_auth violates constraint: ");
    strcat (sql, "f_table_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.f_table_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns_auth violates constraint: ");
    strcat (sql, "f_table_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.f_table_name <> lower(NEW.f_table_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql,
	    "CREATE TRIGGER IF NOT EXISTS gcau_f_geometry_column_insert\n");
    strcat (sql, "BEFORE INSERT ON 'geometry_columns_auth'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns_auth violates constraint: ");
    strcat (sql, "f_geometry_column value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns_auth violates constraint: \n");
    strcat (sql, "f_geometry_column value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns_auth violates constraint: ");
    strcat (sql, "f_geometry_column value must be lower case')\n");
    strcat (sql,
	    "WHERE NEW.f_geometry_column <> lower(NEW.f_geometry_column);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql,
	    "CREATE TRIGGER IF NOT EXISTS gcau_f_geometry_column_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'f_geometry_column' ON 'geometry_columns_auth'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns_auth violates constraint: ");
    strcat (sql, "f_geometry_column value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns_auth violates constraint: ");
    strcat (sql, "f_geometry_column value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns_auth violates constraint: ");
    strcat (sql, "f_geometry_column value must be lower case')\n");
    strcat (sql,
	    "WHERE NEW.f_geometry_column <> lower(NEW.f_geometry_column);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    return 1;
}

static int
create_views_geometry_columns_auth (sqlite3 * sqlite)
{
    char sql[4186];
    char *errMsg = NULL;
    int ret;
/* creating the VIEWS_GEOMETRY_COLUMNS_AUTH table */
    strcpy (sql, "CREATE TABLE IF NOT EXISTS ");
    strcat (sql, "views_geometry_columns_auth (\n");
    strcat (sql, "view_name TEXT NOT NULL,\n");
    strcat (sql, "view_geometry TEXT NOT NULL,\n");
    strcat (sql, "hidden INTEGER NOT NULL,\n");
    strcat (sql, "CONSTRAINT pk_vwgc_auth PRIMARY KEY ");
    strcat (sql, "(view_name, view_geometry),\n");
    strcat (sql, "CONSTRAINT fk_vwgc_auth FOREIGN KEY ");
    strcat (sql, "(view_name, view_geometry) ");
    strcat (sql, "REFERENCES views_geometry_columns ");
    strcat (sql, "(view_name, view_geometry) ");
    strcat (sql, "ON DELETE CASCADE,\n");
    strcat (sql, "CONSTRAINT ck_vwgc_hidden CHECK (hidden IN ");
    strcat (sql, "(0,1)))");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
/* creating the VIEWS_GEOMETRY_COLUMNS_AUTH triggers */
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vwgcau_view_name_insert\n");
    strcat (sql, "BEFORE INSERT ON 'views_geometry_columns_auth'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns_auth violates constraint: ");
    strcat (sql, "view_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.view_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns_auth violates constraint: ");
    strcat (sql, "view_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.view_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns_auth violates constraint: \n");
    strcat (sql, "view_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.view_name <> lower(NEW.view_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vwgcau_view_name_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'view_name' ON 'views_geometry_columns_auth'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns_auth violates constraint: ");
    strcat (sql, "view_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.view_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns_auth violates constraint: ");
    strcat (sql, "view_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.view_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns_auth violates constraint: ");
    strcat (sql, "view_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.view_name <> lower(NEW.view_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vwgcau_view_geometry_insert\n");
    strcat (sql, "BEFORE INSERT ON 'views_geometry_columns_auth'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns_auth violates constraint: ");
    strcat (sql, "view_geometry value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.view_geometry LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns_auth violates constraint: \n");
    strcat (sql, "view_geometry value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.view_geometry LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on views_geometry_columns_auth violates constraint: ");
    strcat (sql, "view_geometry value must be lower case')\n");
    strcat (sql, "WHERE NEW.view_geometry <> lower(NEW.view_geometry);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vwgcau_view_geometry_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'view_geometry'  ON 'views_geometry_columns_auth'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns_auth violates constraint: ");
    strcat (sql, "view_geometry value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.view_geometry LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns_auth violates constraint: \n");
    strcat (sql, "view_geometry value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.view_geometry LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on views_geometry_columns_auth violates constraint: ");
    strcat (sql, "view_geometry value must be lower case')\n");
    strcat (sql, "WHERE NEW.view_geometry <> lower(NEW.view_geometry);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    return 1;
}

static int
create_virts_geometry_columns_auth (sqlite3 * sqlite)
{
    char sql[4186];
    char *errMsg = NULL;
    int ret;
/* creating the VIRTS_GEOMETRY_COLUMNS_AUTH table */
    strcpy (sql, "CREATE TABLE IF NOT EXISTS ");
    strcat (sql, "virts_geometry_columns_auth (\n");
    strcat (sql, "virt_name TEXT NOT NULL,\n");
    strcat (sql, "virt_geometry TEXT NOT NULL,\n");
    strcat (sql, "hidden INTEGER NOT NULL,\n");
    strcat (sql, "CONSTRAINT pk_vrtgc_auth PRIMARY KEY ");
    strcat (sql, "(virt_name, virt_geometry),\n");
    strcat (sql, "CONSTRAINT fk_vrtgc_auth FOREIGN KEY ");
    strcat (sql, "(virt_name, virt_geometry) ");
    strcat (sql, "REFERENCES virts_geometry_columns ");
    strcat (sql, "(virt_name, virt_geometry) ");
    strcat (sql, "ON DELETE CASCADE,\n");
    strcat (sql, "CONSTRAINT ck_vrtgc_hidden CHECK (hidden IN ");
    strcat (sql, "(0,1)))");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
/* creating the VIRTS_GEOMETRY_COLUMNS_AUTH triggers */
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vtgcau_virt_name_insert\n");
    strcat (sql, "BEFORE INSERT ON 'virts_geometry_columns_auth'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on virts_geometry_columns_auth violates constraint: ");
    strcat (sql, "virt_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.virt_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on virts_geometry_columns_auth violates constraint: ");
    strcat (sql, "virt_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.virt_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on virts_geometry_columns_auth violates constraint: \n");
    strcat (sql, "virt_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.virt_name <> lower(NEW.virt_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vtgcau_virt_name_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'virt_name' ON 'virts_geometry_columns_auth'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on virts_geometry_columns_auth violates constraint: ");
    strcat (sql, "virt_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.virt_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on virts_geometry_columns_auth violates constraint: ");
    strcat (sql, "virt_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.virt_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on virts_geometry_columns_auth violates constraint: ");
    strcat (sql, "virt_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.virt_name <> lower(NEW.virt_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vtgcau_virt_geometry_insert\n");
    strcat (sql, "BEFORE INSERT ON 'virts_geometry_columns_auth'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on virts_geometry_columns_auth violates constraint: ");
    strcat (sql, "virt_geometry value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.virt_geometry LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on virts_geometry_columns_auth violates constraint: \n");
    strcat (sql, "virt_geometry value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.virt_geometry LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on virts_geometry_columns_auth violates constraint: ");
    strcat (sql, "virt_geometry value must be lower case')\n");
    strcat (sql, "WHERE NEW.virt_geometry <> lower(NEW.virt_geometry);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    strcpy (sql, "CREATE TRIGGER IF NOT EXISTS vtgcau_virt_geometry_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'virt_geometry' ON 'virts_geometry_columns_auth'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on virts_geometry_columns_auth violates constraint: ");
    strcat (sql, "virt_geometry value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.virt_geometry LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on virts_geometry_columns_auth violates constraint: \n");
    strcat (sql, "virt_geometry value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.virt_geometry LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on virts_geometry_columns_auth violates constraint: ");
    strcat (sql, "virt_geometry value must be lower case')\n");
    strcat (sql, "WHERE NEW.virt_geometry <> lower(NEW.virt_geometry);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    return 1;
}

static int
create_geometry_columns_views (sqlite3 * sqlite)
{
    char sql[4186];
    char *errMsg = NULL;
    int ret;
/* creating the VECTOR_LAYERS view */
    strcpy (sql, "CREATE VIEW  IF NOT EXISTS ");
    strcat (sql, "vector_layers AS\n");
    strcat (sql, "SELECT 'SpatialTable' AS layer_type, ");
    strcat (sql, "f_table_name AS table_name, ");
    strcat (sql, "f_geometry_column AS geometry_column, ");
    strcat (sql, "geometry_type AS geometry_type, ");
    strcat (sql, "coord_dimension AS coord_dimension, ");
    strcat (sql, "srid AS srid, ");
    strcat (sql, "spatial_index_enabled AS spatial_index_enabled\n");
    strcat (sql, "FROM geometry_columns\n");
    strcat (sql, "UNION\n");
    strcat (sql, "SELECT 'SpatialView' AS layer_type, ");
    strcat (sql, "a.view_name AS table_name, ");
    strcat (sql, "a.view_geometry AS geometry_column, ");
    strcat (sql, "b.geometry_type AS geometry_type, ");
    strcat (sql, "b.coord_dimension AS coord_dimension, ");
    strcat (sql, "b.srid AS srid, ");
    strcat (sql, "b.spatial_index_enabled AS spatial_index_enabled\n");
    strcat (sql, "FROM views_geometry_columns AS a\n");
    strcat (sql, "LEFT JOIN geometry_columns AS b ON (");
    strcat (sql, "Upper(a.f_table_name) = Upper(b.f_table_name) AND ");
    strcat (sql, "Upper(a.f_geometry_column) = Upper(b.f_geometry_column))\n");
    strcat (sql, "UNION\n");
    strcat (sql, "SELECT 'VirtualShape' AS layer_type, ");
    strcat (sql, "virt_name AS table_name, ");
    strcat (sql, "virt_geometry AS geometry_column, ");
    strcat (sql, "geometry_type AS geometry_type, ");
    strcat (sql, "coord_dimension AS coord_dimension, ");
    strcat (sql, "srid AS srid, ");
    strcat (sql, "0 AS spatial_index_enabled\n");
    strcat (sql, "FROM virts_geometry_columns");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
/* creating the VECTOR_LAYERS_AUTH view */
    strcpy (sql, "CREATE VIEW  IF NOT EXISTS ");
    strcat (sql, "vector_layers_auth AS\n");
    strcat (sql, "SELECT 'SpatialTable' AS layer_type, ");
    strcat (sql, "f_table_name AS table_name, ");
    strcat (sql, "f_geometry_column AS geometry_column, ");
    strcat (sql, "read_only AS read_only, ");
    strcat (sql, "hidden AS hidden\n");
    strcat (sql, "FROM geometry_columns_auth\n");
    strcat (sql, "UNION\n");
    strcat (sql, "SELECT 'SpatialView' AS layer_type, ");
    strcat (sql, "a.view_name AS table_name, ");
    strcat (sql, "a.view_geometry AS geometry_column, ");
    strcat (sql, "b.read_only AS read_only, ");
    strcat (sql, "a.hidden AS hidden\n");
    strcat (sql, "FROM views_geometry_columns_auth AS a\n");
    strcat (sql, "JOIN views_geometry_columns AS b ON (");
    strcat (sql, "Upper(a.view_name) = Upper(b.view_name) AND ");
    strcat (sql, "Upper(a.view_geometry) = Upper(b.view_geometry))\n");
    strcat (sql, "UNION\n");
    strcat (sql, "SELECT 'VirtualShape' AS layer_type, ");
    strcat (sql, "virt_name AS table_name, ");
    strcat (sql, "virt_geometry AS geometry_column, ");
    strcat (sql, "1 AS read_only, ");
    strcat (sql, "hidden AS hidden\n");
    strcat (sql, "FROM virts_geometry_columns_auth");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
/* creating the VECTOR_LAYERS_STATISTICS view */
    strcpy (sql, "CREATE VIEW IF NOT EXISTS ");
    strcat (sql, "vector_layers_statistics AS\n");
    strcat (sql, "SELECT 'SpatialTable' AS layer_type, ");
    strcat (sql, "f_table_name AS table_name, ");
    strcat (sql, "f_geometry_column AS geometry_column, ");
    strcat (sql, "last_verified AS last_verified, ");
    strcat (sql, "row_count AS row_count, ");
    strcat (sql, "extent_min_x AS extent_min_x, ");
    strcat (sql, "extent_min_y AS extent_min_y, ");
    strcat (sql, "extent_max_x AS extent_max_x, ");
    strcat (sql, "extent_max_y AS extent_max_y\n");
    strcat (sql, "FROM geometry_columns_statistics\n");
    strcat (sql, "UNION\n");
    strcat (sql, "SELECT 'SpatialView' AS layer_type, ");
    strcat (sql, "view_name AS table_name, ");
    strcat (sql, "view_geometry AS geometry_column, ");
    strcat (sql, "last_verified AS last_verified, ");
    strcat (sql, "row_count AS row_count, ");
    strcat (sql, "extent_min_x AS extent_min_x, ");
    strcat (sql, "extent_min_y AS extent_min_y, ");
    strcat (sql, "extent_max_x AS extent_max_x, ");
    strcat (sql, "extent_max_y AS extent_max_y\n");
    strcat (sql, "FROM views_geometry_columns_statistics\n");
    strcat (sql, "UNION\n");
    strcat (sql, "SELECT 'VirtualShape' AS layer_type, ");
    strcat (sql, "virt_name AS table_name, ");
    strcat (sql, "virt_geometry AS geometry_column, ");
    strcat (sql, "last_verified AS last_verified, ");
    strcat (sql, "row_count AS row_count, ");
    strcat (sql, "extent_min_x AS extent_min_x, ");
    strcat (sql, "extent_min_y AS extent_min_y, ");
    strcat (sql, "extent_max_x AS extent_max_x, ");
    strcat (sql, "extent_max_y AS extent_max_y\n");
    strcat (sql, "FROM virts_geometry_columns_statistics");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
/* creating the VECTOR_LAYERS_FIELD_INFOS view */
    strcpy (sql, "CREATE VIEW IF NOT EXISTS ");
    strcat (sql, "vector_layers_field_infos AS\n");
    strcat (sql, "SELECT 'SpatialTable' AS layer_type, ");
    strcat (sql, "f_table_name AS table_name, ");
    strcat (sql, "f_geometry_column AS geometry_column, ");
    strcat (sql, "ordinal AS ordinal, ");
    strcat (sql, "column_name AS column_name, ");
    strcat (sql, "null_values AS null_values, ");
    strcat (sql, "integer_values AS integer_values, ");
    strcat (sql, "double_values AS double_values, ");
    strcat (sql, "text_values AS text_values, ");
    strcat (sql, "blob_values AS blob_values, ");
    strcat (sql, "max_size AS max_size, ");
    strcat (sql, "integer_min AS integer_min, ");
    strcat (sql, "integer_max AS integer_max, ");
    strcat (sql, "double_min AS double_min, ");
    strcat (sql, "double_max double_max\n");
    strcat (sql, "FROM geometry_columns_field_infos\n");
    strcat (sql, "UNION\n");
    strcat (sql, "SELECT 'SpatialView' AS layer_type, ");
    strcat (sql, "view_name AS table_name, ");
    strcat (sql, "view_geometry AS geometry_column, ");
    strcat (sql, "ordinal AS ordinal, ");
    strcat (sql, "column_name AS column_name, ");
    strcat (sql, "null_values AS null_values, ");
    strcat (sql, "integer_values AS integer_values, ");
    strcat (sql, "double_values AS double_values, ");
    strcat (sql, "text_values AS text_values, ");
    strcat (sql, "blob_values AS blob_values, ");
    strcat (sql, "max_size AS max_size, ");
    strcat (sql, "integer_min AS integer_min, ");
    strcat (sql, "integer_max AS integer_max, ");
    strcat (sql, "double_min AS double_min, ");
    strcat (sql, "double_max double_max\n");
    strcat (sql, "FROM views_geometry_columns_field_infos\n");
    strcat (sql, "UNION\n");
    strcat (sql, "SELECT 'VirtualShape' AS layer_type, ");
    strcat (sql, "virt_name AS table_name, ");
    strcat (sql, "virt_geometry AS geometry_column, ");
    strcat (sql, "ordinal AS ordinal, ");
    strcat (sql, "column_name AS column_name, ");
    strcat (sql, "null_values AS null_values, ");
    strcat (sql, "integer_values AS integer_values, ");
    strcat (sql, "double_values AS double_values, ");
    strcat (sql, "text_values AS text_values, ");
    strcat (sql, "blob_values AS blob_values, ");
    strcat (sql, "max_size AS max_size, ");
    strcat (sql, "integer_min AS integer_min, ");
    strcat (sql, "integer_max AS integer_max, ");
    strcat (sql, "double_min AS double_min, ");
    strcat (sql, "double_max double_max\n");
    strcat (sql, "FROM virts_geometry_columns_field_infos");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    return 1;
}

static int
create_sql_statements_log (sqlite3 * sqlite)
{
    char sql[4186];
    char *errMsg = NULL;
    int ret;
/* creating the SQL_STATEMENTS_LOG table */
    strcpy (sql, "CREATE TABLE  IF NOT EXISTS ");
    strcat (sql, "sql_statements_log (\n");
    strcat (sql, "id INTEGER PRIMARY KEY AUTOINCREMENT,\n");
    strcat (sql,
	    "time_start TIMESTAMP NOT NULL DEFAULT '0000-01-01T00:00:00.000Z',\n");
    strcat (sql,
	    "time_end TIMESTAMP NOT NULL DEFAULT '0000-01-01T00:00:00.000Z',\n");
    strcat (sql, "user_agent TEXT NOT NULL,\n");
    strcat (sql, "sql_statement TEXT NOT NULL,\n");
    strcat (sql, "success INTEGER NOT NULL DEFAULT 0,\n");
    strcat (sql, "error_cause TEXT NOT NULL DEFAULT 'ABORTED',\n");
    strcat (sql, "CONSTRAINT sqllog_success CHECK ");
    strcat (sql, "(success IN (0,1)))");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    return 1;
}

SPATIALITE_PRIVATE int
createAdvancedMetaData (void *p_sqlite)
{
/* creating the advanced MetaData tables */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    if (!create_views_geometry_columns (sqlite))
	return 0;
    if (!create_virts_geometry_columns (sqlite))
	return 0;
    if (!create_geometry_columns_statistics (sqlite))
	return 0;
    if (!create_views_geometry_columns_statistics (sqlite))
	return 0;
    if (!create_virts_geometry_columns_statistics (sqlite))
	return 0;
    if (!create_geometry_columns_field_infos (sqlite))
	return 0;
    if (!create_views_geometry_columns_field_infos (sqlite))
	return 0;
    if (!create_virts_geometry_columns_field_infos (sqlite))
	return 0;
    if (!create_geometry_columns_times (sqlite))
	return 0;
    if (!create_geometry_columns_auth (sqlite))
	return 0;
    if (!create_views_geometry_columns_auth (sqlite))
	return 0;
    if (!create_virts_geometry_columns_auth (sqlite))
	return 0;
    if (!create_geometry_columns_views (sqlite))
	return 0;
    if (!create_sql_statements_log (sqlite))
	return 0;

    return 1;
}

SPATIALITE_PRIVATE int
createGeometryColumns (void *p_sqlite)
{
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    char sql[4186];
    char *errMsg = NULL;
    int ret;
/* creating the GEOMETRY_COLUMNS table */
    strcpy (sql, "CREATE TABLE geometry_columns (\n");
    strcat (sql, "f_table_name TEXT NOT NULL,\n");
    strcat (sql, "f_geometry_column TEXT NOT NULL,\n");
    strcat (sql, "geometry_type INTEGER NOT NULL,\n");
    strcat (sql, "coord_dimension INTEGER NOT NULL,\n");
    strcat (sql, "srid INTEGER NOT NULL,\n");
    strcat (sql, "spatial_index_enabled INTEGER NOT NULL,\n");
    strcat (sql, "CONSTRAINT pk_geom_cols PRIMARY KEY ");
    strcat (sql, "(f_table_name, f_geometry_column),\n");
    strcat (sql, "CONSTRAINT fk_gc_srs FOREIGN KEY ");
    strcat (sql, "(srid) REFERENCES spatial_ref_sys (srid),\n");
    strcat (sql, "CONSTRAINT ck_gc_rtree CHECK ");
    strcat (sql, "(spatial_index_enabled IN (0,1,2)))");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    updateSpatiaLiteHistory (sqlite, "geometry_columns", NULL,
			     "table successfully created");
/* creating an INDEX corresponding to the SRID FK */
    strcpy (sql, "CREATE INDEX idx_srid_geocols ON geometry_columns\n");
    strcat (sql, "(srid) ");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
/* creating the GEOMETRY_COLUMNS triggers */
    strcpy (sql, "CREATE TRIGGER geometry_columns_f_table_name_insert\n");
    strcat (sql, "BEFORE INSERT ON 'geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns violates constraint: ");
    strcat (sql, "f_table_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_table_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns violates constraint: ");
    strcat (sql, "f_table_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.f_table_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns violates constraint: \n");
    strcat (sql, "f_table_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.f_table_name <> lower(NEW.f_table_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    updateSpatiaLiteHistory (sqlite, "geometry_columns", NULL,
			     "trigger 'geometry_columns_f_table_name_insert' successfully created");
    strcpy (sql, "CREATE TRIGGER geometry_columns_f_table_name_update\n");
    strcat (sql, "BEFORE UPDATE OF 'f_table_name' ON 'geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns violates constraint: ");
    strcat (sql, "f_table_name value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_table_name LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns violates constraint: ");
    strcat (sql, "f_table_name value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.f_table_name LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns violates constraint: ");
    strcat (sql, "f_table_name value must be lower case')\n");
    strcat (sql, "WHERE NEW.f_table_name <> lower(NEW.f_table_name);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    updateSpatiaLiteHistory (sqlite, "geometry_columns", NULL,
			     "trigger 'geometry_columns_f_table_name_update' successfully created");
    strcpy (sql, "CREATE TRIGGER geometry_columns_f_geometry_column_insert\n");
    strcat (sql, "BEFORE INSERT ON 'geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns violates constraint: ");
    strcat (sql, "f_geometry_column value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns violates constraint: \n");
    strcat (sql, "f_geometry_column value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'insert on geometry_columns violates constraint: ");
    strcat (sql, "f_geometry_column value must be lower case')\n");
    strcat (sql,
	    "WHERE NEW.f_geometry_column <> lower(NEW.f_geometry_column);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    updateSpatiaLiteHistory (sqlite, "geometry_columns", NULL,
			     "trigger 'geometry_columns_f_geometry_column_insert' successfully created");
    strcpy (sql, "CREATE TRIGGER geometry_columns_f_geometry_column_update\n");
    strcat (sql,
	    "BEFORE UPDATE OF 'f_geometry_column' ON 'geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns violates constraint: ");
    strcat (sql, "f_geometry_column value must not contain a single quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%''%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns violates constraint: ");
    strcat (sql, "f_geometry_column value must not contain a double quote')\n");
    strcat (sql, "WHERE NEW.f_geometry_column LIKE ('%\"%');\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'update on geometry_columns violates constraint: ");
    strcat (sql, "f_geometry_column value must be lower case')\n");
    strcat (sql,
	    "WHERE NEW.f_geometry_column <> lower(NEW.f_geometry_column);\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    updateSpatiaLiteHistory (sqlite, "geometry_columns", NULL,
			     "trigger 'geometry_columns_f_geometry_column_update' successfully created");
    strcpy (sql, "CREATE TRIGGER geometry_columns_geometry_type_insert\n");
    strcat (sql, "BEFORE INSERT ON 'geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql, "SELECT RAISE(ABORT,'geometry_type must be one of ");
    strcat (sql, "0,1,2,3,4,5,6,7,");
    strcat (sql, "1000,1001,1002,1003,1004,1005,1006,1007,");
    strcat (sql, "2000,2001,2002,2003,2004,2005,2006,2007,");
    strcat (sql, "3000,3001,3002,3003,3004,3005,3006,3007')\n");
    strcat (sql, "WHERE NOT(NEW.geometry_type IN (0,1,2,3,4,5,6,7,");
    strcat (sql, "1000,1001,1002,1003,1004,1005,1006,1007,");
    strcat (sql, "2000,2001,2002,2003,2004,2005,2006,2007,");
    strcat (sql, "3000,3001,3002,3003,3004,3005,3006,3007));\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    updateSpatiaLiteHistory (sqlite, "geometry_columns", NULL,
			     "trigger 'geometry_columns_geometry_type_insert' successfully created");
    strcpy (sql, "CREATE TRIGGER geometry_columns_geometry_type_update\n");
    strcat (sql, "BEFORE UPDATE OF 'geometry_type' ON 'geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql, "SELECT RAISE(ABORT,'geometry_type must be one of ");
    strcat (sql, "0,1,2,3,4,5,6,7,");
    strcat (sql, "1000,1001,1002,1003,1004,1005,1006,1007,");
    strcat (sql, "2000,2001,2002,2003,2004,2005,2006,2007,");
    strcat (sql, "3000,3001,3002,3003,3004,3005,3006,3007')\n");
    strcat (sql, "WHERE NOT(NEW.geometry_type IN (0,1,2,3,4,5,6,7,");
    strcat (sql, "1000,1001,1002,1003,1004,1005,1006,1007,");
    strcat (sql, "2000,2001,2002,2003,2004,2005,2006,2007,");
    strcat (sql, "3000,3001,3002,3003,3004,3005,3006,3007));\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    updateSpatiaLiteHistory (sqlite, "geometry_columns", NULL,
			     "trigger 'geometry_columns_geometry_type_update' successfully created");
    strcpy (sql, "CREATE TRIGGER geometry_columns_coord_dimension_insert\n");
    strcat (sql, "BEFORE INSERT ON 'geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'coord_dimension must be one of 2,3,4')\n");
    strcat (sql, "WHERE NOT(NEW.coord_dimension IN (2,3,4));\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    updateSpatiaLiteHistory (sqlite, "geometry_columns", NULL,
			     "trigger 'geometry_columns_coord_dimension_insert' successfully created");
    strcpy (sql, "CREATE TRIGGER geometry_columns_coord_dimension_update\n");
    strcat (sql, "BEFORE UPDATE OF 'coord_dimension' ON 'geometry_columns'\n");
    strcat (sql, "FOR EACH ROW BEGIN\n");
    strcat (sql,
	    "SELECT RAISE(ABORT,'coord_dimension must be one of 2,3,4')\n");
    strcat (sql, "WHERE NOT(NEW.coord_dimension IN (2,3,4));\n");
    strcat (sql, "END");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s: %s\n", sql, errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    updateSpatiaLiteHistory (sqlite, "geometry_columns", NULL,
			     "trigger 'geometry_columns_coord_dimension_update' successfully created");
    return 1;
}

SPATIALITE_PRIVATE void
updateGeometryTriggers (void *p_sqlite, const char *table, const char *column)
{
/* updates triggers for some Spatial Column */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    int col_index;
    const char *col_dims;
    int index;
    int cached;
    int dims;
    char *txt_dims;
    int len;
    char *errMsg = NULL;
    char *sql_statement;
    char *raw;
    char *quoted_trigger;
    char *quoted_rtree;
    char *quoted_table;
    char *quoted_column;
    char *p_table = NULL;
    char *p_column = NULL;
    sqlite3_stmt *stmt;
    struct spatial_index_str *first_idx = NULL;
    struct spatial_index_str *last_idx = NULL;
    struct spatial_index_str *curr_idx;
    struct spatial_index_str *next_idx;
    int metadata_version = checkSpatialMetaData (sqlite);

    if (!getRealSQLnames (sqlite, table, column, &p_table, &p_column))
      {
	  spatialite_e
	      ("updateTableTriggers() error: not existing Table or Column\n");
	  return;
      }
    if (metadata_version == 3)
      {
	  /* current metadata style >= v.4.0.0 */
	  sql_statement = sqlite3_mprintf ("SELECT spatial_index_enabled "
					   "FROM geometry_columns WHERE Lower(f_table_name) = Lower(?) "
					   "AND Lower(f_geometry_column) = Lower(?)");
      }
    else
      {
	  /* legacy metadata style <= v.3.1.0 */
	  sql_statement =
	      sqlite3_mprintf ("SELECT spatial_index_enabled, coord_dimension "
			       "FROM geometry_columns WHERE Lower(f_table_name) = Lower(?) "
			       "AND Lower(f_geometry_column) = Lower(?)");
      }
/* compiling SQL prepared statement */
    ret =
	sqlite3_prepare_v2 (sqlite, sql_statement, strlen (sql_statement),
			    &stmt, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("updateTableTriggers: error %d \"%s\"\n",
			sqlite3_errcode (sqlite), sqlite3_errmsg (sqlite));
	  return;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, table, strlen (table), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, column, strlen (column), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		col_index = sqlite3_column_int (stmt, 0);
		if (metadata_version == 1)
		  {
		      /* legacy metadata style <= v.3.1.0 */
		      col_dims = (const char *) sqlite3_column_text (stmt, 1);
		      dims = GAIA_XY;
		      if (strcasecmp (col_dims, "XYZ") == 0)
			  dims = GAIA_XY_Z;
		      if (strcasecmp (col_dims, "XYM") == 0)
			  dims = GAIA_XY_M;
		      if (strcasecmp (col_dims, "XYZM") == 0)
			  dims = GAIA_XY_Z_M;
		      switch (dims)
			{
			case GAIA_XY_Z:
			    txt_dims = "XYZ";
			    break;
			case GAIA_XY_M:
			    txt_dims = "XYM";
			    break;
			case GAIA_XY_Z_M:
			    txt_dims = "XYZM";
			    break;
			default:
			    txt_dims = "XY";
			    break;
			};
		  }
		index = 0;
		cached = 0;
		if (col_index == 1)
		    index = 1;
		if (col_index == 2)
		    cached = 1;

		/* trying to delete old versions [v2.0, v2.2] triggers[if any] */
		raw = sqlite3_mprintf ("gti_%s_%s", p_table, p_column);
		quoted_trigger = gaiaDoubleQuotedSql (raw);
		sqlite3_free (raw);
		sql_statement =
		    sqlite3_mprintf ("DROP TRIGGER IF EXISTS \"%s\"",
				     quoted_trigger);
		free (quoted_trigger);
		ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
		sqlite3_free (sql_statement);
		if (ret != SQLITE_OK)
		    goto error;
		raw = sqlite3_mprintf ("gtu_%s_%s", p_table, p_column);
		quoted_trigger = gaiaDoubleQuotedSql (raw);
		sqlite3_free (raw);
		sql_statement =
		    sqlite3_mprintf ("DROP TRIGGER IF EXISTS \"%s\"",
				     quoted_trigger);
		free (quoted_trigger);
		ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
		sqlite3_free (sql_statement);
		if (ret != SQLITE_OK)
		    goto error;
		raw = sqlite3_mprintf ("gsi_%s_%s", p_table, p_column);
		quoted_trigger = gaiaDoubleQuotedSql (raw);
		sqlite3_free (raw);
		sql_statement =
		    sqlite3_mprintf ("DROP TRIGGER IF EXISTS \"%s\"",
				     quoted_trigger);
		free (quoted_trigger);
		ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
		sqlite3_free (sql_statement);
		if (ret != SQLITE_OK)
		    goto error;
		raw = sqlite3_mprintf ("gsu_%s_%s", p_table, p_column);
		quoted_trigger = gaiaDoubleQuotedSql (raw);
		sqlite3_free (raw);
		sql_statement =
		    sqlite3_mprintf ("DROP TRIGGER IF EXISTS \"%s\"",
				     quoted_trigger);
		free (quoted_trigger);
		ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
		sqlite3_free (sql_statement);
		if (ret != SQLITE_OK)
		    goto error;
		/* end deletion old versions [v2.0, v2.2] triggers[if any] */

		/* deleting the old INSERT trigger TYPE [if any] */
		raw = sqlite3_mprintf ("ggi_%s_%s", p_table, p_column);
		quoted_trigger = gaiaDoubleQuotedSql (raw);
		sqlite3_free (raw);
		sql_statement =
		    sqlite3_mprintf ("DROP TRIGGER IF EXISTS \"%s\"",
				     quoted_trigger);
		free (quoted_trigger);
		ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
		sqlite3_free (sql_statement);
		if (ret != SQLITE_OK)
		    goto error;

		/* inserting the new INSERT trigger TYPE */
		raw = sqlite3_mprintf ("ggi_%s_%s", p_table, p_column);
		quoted_trigger = gaiaDoubleQuotedSql (raw);
		sqlite3_free (raw);
		quoted_table = gaiaDoubleQuotedSql (p_table);
		quoted_column = gaiaDoubleQuotedSql (p_column);
		if (metadata_version == 3)
		  {
		      /* current metadata style >= v.4.0.0 */
		      sql_statement =
			  sqlite3_mprintf
			  ("CREATE TRIGGER \"%s\" BEFORE INSERT ON \"%s\"\n"
			   "FOR EACH ROW BEGIN\n"
			   "SELECT RAISE(ROLLBACK, '%q.%q violates Geometry constraint [geom-type or SRID not allowed]')\n"
			   "WHERE (SELECT geometry_type FROM geometry_columns\n"
			   "WHERE Lower(f_table_name) = Lower(%Q) AND "
			   "Lower(f_geometry_column) = Lower(%Q)\n"
			   "AND GeometryConstraints(NEW.\"%s\", geometry_type, srid) = 1) IS NULL;\nEND",
			   quoted_trigger, quoted_table, p_table, p_column,
			   p_table, p_column, quoted_column);
		      free (quoted_trigger);
		      free (quoted_table);
		      free (quoted_column);
		  }
		else
		  {
		      /* legacy metadata style <= v.3.1.0 */
		      sql_statement =
			  sqlite3_mprintf
			  ("CREATE TRIGGER \"%s\" BEFORE INSERT ON \"%s\"\n"
			   "FOR EACH ROW BEGIN\n"
			   "SELECT RAISE(ROLLBACK, '%q.%q violates Geometry constraint [geom-type or SRID not allowed]')\n"
			   "WHERE (SELECT type FROM geometry_columns\n"
			   "WHERE f_table_name = %Q AND f_geometry_column = %Q\n"
			   "AND GeometryConstraints(NEW.\"%s\", type, srid, %Q) = 1) IS NULL;\nEND",
			   quoted_trigger, quoted_table, p_table, p_column,
			   p_table, p_column, quoted_column, txt_dims);
		      free (quoted_trigger);
		      free (quoted_table);
		      free (quoted_column);
		  }
		ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
		sqlite3_free (sql_statement);
		if (ret != SQLITE_OK)
		    goto error;
		/* deleting the old UPDATE trigger TYPE [if any] */
		raw = sqlite3_mprintf ("ggu_%s_%s", p_table, p_column);
		quoted_trigger = gaiaDoubleQuotedSql (raw);
		sqlite3_free (raw);
		sql_statement =
		    sqlite3_mprintf ("DROP TRIGGER IF EXISTS \"%s\"",
				     quoted_trigger);
		free (quoted_trigger);
		ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
		sqlite3_free (sql_statement);
		if (ret != SQLITE_OK)
		    goto error;

		/* inserting the new UPDATE trigger TYPE */
		raw = sqlite3_mprintf ("ggu_%s_%s", p_table, p_column);
		quoted_trigger = gaiaDoubleQuotedSql (raw);
		sqlite3_free (raw);
		quoted_table = gaiaDoubleQuotedSql (p_table);
		quoted_column = gaiaDoubleQuotedSql (p_column);
		if (metadata_version == 3)
		  {
		      /* current metadata style >= v.4.0.0 */
		      sql_statement =
			  sqlite3_mprintf
			  ("CREATE TRIGGER \"%s\" BEFORE UPDATE ON \"%s\"\n"
			   "FOR EACH ROW BEGIN\n"
			   "SELECT RAISE(ROLLBACK, '%q.%q violates Geometry constraint [geom-type or SRID not allowed]')\n"
			   "WHERE (SELECT geometry_type FROM geometry_columns\n"
			   "WHERE Lower(f_table_name) = Lower(%Q) AND "
			   "Lower(f_geometry_column) = Lower(%Q)\n"
			   "AND GeometryConstraints(NEW.\"%s\", geometry_type, srid) = 1) IS NULL;\nEND",
			   quoted_trigger, quoted_table, p_table, p_column,
			   p_table, p_column, quoted_column);
		      free (quoted_trigger);
		      free (quoted_table);
		      free (quoted_column);
		  }
		else
		  {
		      /* legacy metadata style <= v.3.1.0 */
		      sql_statement =
			  sqlite3_mprintf
			  ("CREATE TRIGGER \"%s\" BEFORE UPDATE ON \"%s\"\n"
			   "FOR EACH ROW BEGIN\n"
			   "SELECT RAISE(ROLLBACK, '%q.%q violates Geometry constraint [geom-type or SRID not allowed]')\n"
			   "WHERE (SELECT type FROM geometry_columns\n"
			   "WHERE f_table_name = %Q AND f_geometry_column = %Q\n"
			   "AND GeometryConstraints(NEW.\"%s\", type, srid, %Q) = 1) IS NULL;\nEND",
			   quoted_trigger, quoted_table, p_table, p_column,
			   p_table, p_column, quoted_column, txt_dims);
		      free (quoted_trigger);
		      free (quoted_table);
		      free (quoted_column);
		  }
		ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
		sqlite3_free (sql_statement);
		if (ret != SQLITE_OK)
		    goto error;

		/* inserting SpatialIndex information into the linked list */
		curr_idx = malloc (sizeof (struct spatial_index_str));
		len = strlen (p_table);
		curr_idx->TableName = malloc (len + 1);
		strcpy (curr_idx->TableName, p_table);
		len = strlen (p_column);
		curr_idx->ColumnName = malloc (len + 1);
		strcpy (curr_idx->ColumnName, p_column);
		curr_idx->ValidRtree = (char) index;
		curr_idx->ValidCache = (char) cached;
		curr_idx->Next = NULL;
		if (!first_idx)
		    first_idx = curr_idx;
		if (last_idx)
		    last_idx->Next = curr_idx;
		last_idx = curr_idx;

		/* deleting the old INSERT trigger SPATIAL_INDEX [if any] */
		raw = sqlite3_mprintf ("gii_%s_%s", p_table, p_column);
		quoted_trigger = gaiaDoubleQuotedSql (raw);
		sqlite3_free (raw);
		sql_statement =
		    sqlite3_mprintf ("DROP TRIGGER IF EXISTS \"%s\"",
				     quoted_trigger);
		free (quoted_trigger);
		ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
		sqlite3_free (sql_statement);
		if (ret != SQLITE_OK)
		    goto error;

		if (metadata_version == 3)
		  {
		      /* deleting the old UPDATE (timestamp) trigger [if any] */
		      raw = sqlite3_mprintf ("tmu_%s_%s", p_table, p_column);
		      quoted_trigger = gaiaDoubleQuotedSql (raw);
		      sqlite3_free (raw);
		      sql_statement =
			  sqlite3_mprintf ("DROP TRIGGER IF EXISTS \"%s\"",
					   quoted_trigger);
		      free (quoted_trigger);
		      ret =
			  sqlite3_exec (sqlite, sql_statement, NULL, NULL,
					&errMsg);
		      sqlite3_free (sql_statement);
		      if (ret != SQLITE_OK)
			  goto error;
		      /* deleting the old INSERT (timestamp) trigger [if any] */
		      raw = sqlite3_mprintf ("tmi_%s_%s", p_table, p_column);
		      quoted_trigger = gaiaDoubleQuotedSql (raw);
		      sqlite3_free (raw);
		      sql_statement =
			  sqlite3_mprintf ("DROP TRIGGER IF EXISTS \"%s\"",
					   quoted_trigger);
		      free (quoted_trigger);
		      ret =
			  sqlite3_exec (sqlite, sql_statement, NULL, NULL,
					&errMsg);
		      sqlite3_free (sql_statement);
		      if (ret != SQLITE_OK)
			  goto error;
		      /* deleting the old DELETE (timestamp) trigger [if any] */
		      raw = sqlite3_mprintf ("tmd_%s_%s", p_table, p_column);
		      quoted_trigger = gaiaDoubleQuotedSql (raw);
		      sqlite3_free (raw);
		      sql_statement =
			  sqlite3_mprintf ("DROP TRIGGER IF EXISTS \"%s\"",
					   quoted_trigger);
		      free (quoted_trigger);
		      ret =
			  sqlite3_exec (sqlite, sql_statement, NULL, NULL,
					&errMsg);
		      sqlite3_free (sql_statement);
		      if (ret != SQLITE_OK)
			  goto error;
		  }

		if (index == 0 && cached == 0)
		  {

		      if (metadata_version == 3)
			{
			    /* current metadata style >= v.4.0.0 */

			    /* inserting the new UPDATE (timestamp) trigger */
			    raw =
				sqlite3_mprintf ("tmu_%s_%s", p_table,
						 p_column);
			    quoted_trigger = gaiaDoubleQuotedSql (raw);
			    sqlite3_free (raw);
			    quoted_table = gaiaDoubleQuotedSql (p_table);
			    sql_statement =
				sqlite3_mprintf
				("CREATE TRIGGER \"%s\" AFTER UPDATE ON \"%s\"\n"
				 "FOR EACH ROW BEGIN\n"
				 "UPDATE geometry_columns_time SET last_update = strftime('%%Y-%%m-%%dT%%H:%%M:%%fZ', 'now')\n"
				 "WHERE Lower(f_table_name) = Lower(%Q) AND "
				 "Lower(f_geometry_column) = Lower(%Q);\nEND",
				 quoted_trigger, quoted_table, p_table,
				 p_column);
			    free (quoted_trigger);
			    free (quoted_table);
			    ret =
				sqlite3_exec (sqlite, sql_statement, NULL, NULL,
					      &errMsg);
			    sqlite3_free (sql_statement);
			    if (ret != SQLITE_OK)
				goto error;

			    /* inserting the new INSERT (timestamp) trigger */
			    raw =
				sqlite3_mprintf ("tmi_%s_%s", p_table,
						 p_column);
			    quoted_trigger = gaiaDoubleQuotedSql (raw);
			    sqlite3_free (raw);
			    quoted_table = gaiaDoubleQuotedSql (p_table);
			    sql_statement =
				sqlite3_mprintf
				("CREATE TRIGGER \"%s\" AFTER INSERT ON \"%s\"\n"
				 "FOR EACH ROW BEGIN\n"
				 "UPDATE geometry_columns_time SET last_insert = strftime('%%Y-%%m-%%dT%%H:%%M:%%fZ', 'now')\n"
				 "WHERE Lower(f_table_name) = Lower(%Q) AND "
				 "Lower(f_geometry_column) = Lower(%Q);\nEND",
				 quoted_trigger, quoted_table, p_table,
				 p_column);
			    free (quoted_trigger);
			    free (quoted_table);
			    ret =
				sqlite3_exec (sqlite, sql_statement, NULL, NULL,
					      &errMsg);
			    sqlite3_free (sql_statement);
			    if (ret != SQLITE_OK)
				goto error;

			    /* inserting the new DELETE (timestamp) trigger */
			    raw =
				sqlite3_mprintf ("tmd_%s_%s", p_table,
						 p_column);
			    quoted_trigger = gaiaDoubleQuotedSql (raw);
			    sqlite3_free (raw);
			    quoted_table = gaiaDoubleQuotedSql (p_table);
			    sql_statement =
				sqlite3_mprintf
				("CREATE TRIGGER \"%s\" AFTER DELETE ON \"%s\"\n"
				 "FOR EACH ROW BEGIN\n"
				 "UPDATE geometry_columns_time SET last_delete = strftime('%%Y-%%m-%%dT%%H:%%M:%%fZ', 'now')\n"
				 "WHERE Lower(f_table_name) = Lower(%Q) AND "
				 "Lower(f_geometry_column) = Lower(%Q);\nEND",
				 quoted_trigger, quoted_table, p_table,
				 p_column);
			    free (quoted_trigger);
			    free (quoted_table);
			    ret =
				sqlite3_exec (sqlite, sql_statement, NULL, NULL,
					      &errMsg);
			    sqlite3_free (sql_statement);
			    if (ret != SQLITE_OK)
				goto error;
			}
		  }

		/* deleting the old INSERT trigger SPATIAL_INDEX [if any] */
		raw = sqlite3_mprintf ("gid_%s_%s", p_table, p_column);
		quoted_trigger = gaiaDoubleQuotedSql (raw);
		sqlite3_free (raw);
		sql_statement =
		    sqlite3_mprintf ("DROP TRIGGER IF EXISTS \"%s\"",
				     quoted_trigger);
		free (quoted_trigger);
		ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
		sqlite3_free (sql_statement);
		if (ret != SQLITE_OK)
		    goto error;

		/* deleting the old UPDATE trigger SPATIAL_INDEX [if any] */
		raw = sqlite3_mprintf ("giu_%s_%s", p_table, p_column);
		quoted_trigger = gaiaDoubleQuotedSql (raw);
		sqlite3_free (raw);
		sql_statement =
		    sqlite3_mprintf ("DROP TRIGGER IF EXISTS \"%s\"",
				     quoted_trigger);
		free (quoted_trigger);
		ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
		sqlite3_free (sql_statement);
		if (ret != SQLITE_OK)
		    goto error;

		/* deleting the old DELETE trigger SPATIAL_INDEX [if any] */
		raw = sqlite3_mprintf ("gid_%s_%s", p_table, p_column);
		quoted_trigger = gaiaDoubleQuotedSql (raw);
		sqlite3_free (raw);
		sql_statement =
		    sqlite3_mprintf ("DROP TRIGGER IF EXISTS \"%s\"",
				     quoted_trigger);
		free (quoted_trigger);
		ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
		sqlite3_free (sql_statement);
		if (ret != SQLITE_OK)
		    goto error;

		/* deleting the old INSERT trigger MBR_CACHE [if any] */
		raw = sqlite3_mprintf ("gci_%s_%s", p_table, p_column);
		quoted_trigger = gaiaDoubleQuotedSql (raw);
		sqlite3_free (raw);
		sql_statement =
		    sqlite3_mprintf ("DROP TRIGGER IF EXISTS \"%s\"",
				     quoted_trigger);
		free (quoted_trigger);
		ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
		sqlite3_free (sql_statement);
		if (ret != SQLITE_OK)
		    goto error;

		/* deleting the old UPDATE trigger MBR_CACHE [if any] */
		raw = sqlite3_mprintf ("gcu_%s_%s", p_table, p_column);
		quoted_trigger = gaiaDoubleQuotedSql (raw);
		sqlite3_free (raw);
		sql_statement =
		    sqlite3_mprintf ("DROP TRIGGER IF EXISTS \"%s\"",
				     quoted_trigger);
		free (quoted_trigger);
		ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
		sqlite3_free (sql_statement);
		if (ret != SQLITE_OK)
		    goto error;

		/* deleting the old UPDATE trigger MBR_CACHE [if any] */
		raw = sqlite3_mprintf ("gcd_%s_%s", p_table, p_column);
		quoted_trigger = gaiaDoubleQuotedSql (raw);
		sqlite3_free (raw);
		sql_statement =
		    sqlite3_mprintf ("DROP TRIGGER IF EXISTS \"%s\"",
				     quoted_trigger);
		free (quoted_trigger);
		ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
		sqlite3_free (sql_statement);
		if (ret != SQLITE_OK)
		    goto error;

		if (index)
		  {
		      /* inserting the new INSERT trigger RTree */
		      if (metadata_version == 3)
			{
			    /* current metadata style >= v.4.0.0 */
			    raw =
				sqlite3_mprintf ("gii_%s_%s", p_table,
						 p_column);
			    quoted_trigger = gaiaDoubleQuotedSql (raw);
			    sqlite3_free (raw);
			    raw =
				sqlite3_mprintf ("idx_%s_%s", p_table,
						 p_column);
			    quoted_rtree = gaiaDoubleQuotedSql (raw);
			    quoted_table = gaiaDoubleQuotedSql (p_table);
			    quoted_column = gaiaDoubleQuotedSql (p_column);
			    sql_statement =
				sqlite3_mprintf
				("CREATE TRIGGER \"%s\" AFTER INSERT ON \"%s\"\n"
				 "FOR EACH ROW BEGIN\n"
				 "UPDATE geometry_columns_time SET last_insert = strftime('%%Y-%%m-%%dT%%H:%%M:%%fZ', 'now')\n"
				 "WHERE Lower(f_table_name) = Lower(%Q) AND "
				 "Lower(f_geometry_column) = Lower(%Q);\n"
				 "DELETE FROM \"%s\" WHERE pkid=NEW.ROWID;\n"
				 "SELECT RTreeAlign(%Q, NEW.ROWID, NEW.\"%s\");\nEND",
				 quoted_trigger, quoted_table, p_table,
				 p_column, quoted_rtree, raw, quoted_column);
			    sqlite3_free (raw);
			    free (quoted_trigger);
			    free (quoted_rtree);
			    free (quoted_table);
			    free (quoted_column);
			}
		      else
			{
			    /* legacy metadata style <= v.3.1.0 */
			    raw =
				sqlite3_mprintf ("gii_%s_%s", p_table,
						 p_column);
			    quoted_trigger = gaiaDoubleQuotedSql (raw);
			    sqlite3_free (raw);
			    raw =
				sqlite3_mprintf ("idx_%s_%s", p_table,
						 p_column);
			    quoted_rtree = gaiaDoubleQuotedSql (raw);
			    quoted_table = gaiaDoubleQuotedSql (p_table);
			    quoted_column = gaiaDoubleQuotedSql (p_column);
			    sql_statement =
				sqlite3_mprintf
				("CREATE TRIGGER \"%s\" AFTER INSERT ON \"%s\"\n"
				 "FOR EACH ROW BEGIN\n"
				 "DELETE FROM \"%s\" WHERE pkid=NEW.ROWID;\n"
				 "SELECT RTreeAlign(%Q, NEW.ROWID, NEW.\"%s\");\nEND",
				 quoted_trigger, quoted_table, quoted_rtree,
				 raw, quoted_column);
			    sqlite3_free (raw);
			    free (quoted_trigger);
			    free (quoted_rtree);
			    free (quoted_table);
			    free (quoted_column);
			}
		      ret =
			  sqlite3_exec (sqlite, sql_statement, NULL, NULL,
					&errMsg);
		      sqlite3_free (sql_statement);
		      if (ret != SQLITE_OK)
			  goto error;

		      /* inserting the new UPDATE trigger RTree */
		      if (metadata_version == 3)
			{
			    /* current metadata style >= v.4.0.0 */
			    raw =
				sqlite3_mprintf ("giu_%s_%s", p_table,
						 p_column);
			    quoted_trigger = gaiaDoubleQuotedSql (raw);
			    sqlite3_free (raw);
			    raw =
				sqlite3_mprintf ("idx_%s_%s", p_table,
						 p_column);
			    quoted_rtree = gaiaDoubleQuotedSql (raw);
			    quoted_table = gaiaDoubleQuotedSql (p_table);
			    quoted_column = gaiaDoubleQuotedSql (p_column);
			    sql_statement =
				sqlite3_mprintf
				("CREATE TRIGGER \"%s\" AFTER UPDATE ON \"%s\"\n"
				 "FOR EACH ROW BEGIN\n"
				 "UPDATE geometry_columns_time SET last_update = strftime('%%Y-%%m-%%dT%%H:%%M:%%fZ', 'now')\n"
				 "WHERE Lower(f_table_name) = Lower(%Q) AND "
				 "Lower(f_geometry_column) = Lower(%Q);\n"
				 "DELETE FROM \"%s\" WHERE pkid=NEW.ROWID;\n"
				 "SELECT RTreeAlign(%Q, NEW.ROWID, NEW.\"%s\");\nEND",
				 quoted_trigger, quoted_table, p_table,
				 p_column, quoted_rtree, raw, quoted_column);
			    sqlite3_free (raw);
			    free (quoted_trigger);
			    free (quoted_rtree);
			    free (quoted_table);
			    free (quoted_column);
			}
		      else
			{
			    /* legacy metadata style <= v.3.1.0 */
			    raw =
				sqlite3_mprintf ("giu_%s_%s", p_table,
						 p_column);
			    quoted_trigger = gaiaDoubleQuotedSql (raw);
			    sqlite3_free (raw);
			    raw =
				sqlite3_mprintf ("idx_%s_%s", p_table,
						 p_column);
			    quoted_rtree = gaiaDoubleQuotedSql (raw);
			    quoted_table = gaiaDoubleQuotedSql (p_table);
			    quoted_column = gaiaDoubleQuotedSql (p_column);
			    sql_statement =
				sqlite3_mprintf
				("CREATE TRIGGER \"%s\" AFTER UPDATE ON \"%s\"\n"
				 "FOR EACH ROW BEGIN\n"
				 "DELETE FROM \"%s\" WHERE pkid=NEW.ROWID;\n"
				 "SELECT RTreeAlign(%Q, NEW.ROWID, NEW.\"%s\");\nEND",
				 quoted_trigger, quoted_table, quoted_rtree,
				 raw, quoted_column);
			    sqlite3_free (raw);
			    free (quoted_trigger);
			    free (quoted_rtree);
			    free (quoted_table);
			    free (quoted_column);
			}
		      ret =
			  sqlite3_exec (sqlite, sql_statement, NULL, NULL,
					&errMsg);
		      sqlite3_free (sql_statement);
		      if (ret != SQLITE_OK)
			  goto error;

		      /* inserting the new DELETE trigger RTree */
		      if (metadata_version == 3)
			{
			    /* current metadata style >= v.4.0.0 */
			    raw =
				sqlite3_mprintf ("gid_%s_%s", p_table,
						 p_column);
			    quoted_trigger = gaiaDoubleQuotedSql (raw);
			    sqlite3_free (raw);
			    raw =
				sqlite3_mprintf ("idx_%s_%s", p_table,
						 p_column);
			    quoted_rtree = gaiaDoubleQuotedSql (raw);
			    sqlite3_free (raw);
			    quoted_table = gaiaDoubleQuotedSql (p_table);
			    quoted_column = gaiaDoubleQuotedSql (p_column);
			    sql_statement =
				sqlite3_mprintf
				("CREATE TRIGGER \"%s\" AFTER DELETE ON \"%s\"\n"
				 "FOR EACH ROW BEGIN\n"
				 "UPDATE geometry_columns_time SET last_delete = strftime('%%Y-%%m-%%dT%%H:%%M:%%fZ', 'now')\n"
				 "WHERE Lower(f_table_name) = Lower(%Q) AND "
				 "Lower(f_geometry_column) = Lower(%Q);\n"
				 "DELETE FROM \"%s\" WHERE pkid=OLD.ROWID;\nEND",
				 quoted_trigger, quoted_table, p_table,
				 p_column, quoted_rtree);
			    free (quoted_trigger);
			    free (quoted_rtree);
			    free (quoted_table);
			    free (quoted_column);
			}
		      else
			{
			    /* legacy metadata style <= v.3.1.0 */
			    raw =
				sqlite3_mprintf ("gid_%s_%s", p_table,
						 p_column);
			    quoted_trigger = gaiaDoubleQuotedSql (raw);
			    sqlite3_free (raw);
			    raw =
				sqlite3_mprintf ("idx_%s_%s", p_table,
						 p_column);
			    quoted_rtree = gaiaDoubleQuotedSql (raw);
			    sqlite3_free (raw);
			    quoted_table = gaiaDoubleQuotedSql (p_table);
			    quoted_column = gaiaDoubleQuotedSql (p_column);
			    sql_statement =
				sqlite3_mprintf
				("CREATE TRIGGER \"%s\" AFTER DELETE ON \"%s\"\n"
				 "FOR EACH ROW BEGIN\n"
				 "DELETE FROM \"%s\" WHERE pkid=OLD.ROWID;\nEND",
				 quoted_trigger, quoted_table, quoted_rtree);
			    free (quoted_trigger);
			    free (quoted_rtree);
			    free (quoted_table);
			    free (quoted_column);
			}
		      ret =
			  sqlite3_exec (sqlite, sql_statement, NULL, NULL,
					&errMsg);
		      sqlite3_free (sql_statement);
		      if (ret != SQLITE_OK)
			  goto error;
		  }

		if (cached)
		  {
		      /* inserting the new INSERT trigger MBRcache */
		      if (metadata_version == 3)
			{
			    /* current metadata style >= v.4.0.0 */
			    raw =
				sqlite3_mprintf ("gci_%s_%s", p_table,
						 p_column);
			    quoted_trigger = gaiaDoubleQuotedSql (raw);
			    sqlite3_free (raw);
			    raw =
				sqlite3_mprintf ("cache_%s_%s", p_table,
						 p_column);
			    quoted_rtree = gaiaDoubleQuotedSql (raw);
			    sqlite3_free (raw);
			    quoted_table = gaiaDoubleQuotedSql (p_table);
			    quoted_column = gaiaDoubleQuotedSql (p_column);
			    sql_statement =
				sqlite3_mprintf
				("CREATE TRIGGER \"%s\" AFTER INSERT ON \"%s\"\n"
				 "FOR EACH ROW BEGIN\n"
				 "UPDATE geometry_columns_time SET last_insert = strftime('%%Y-%%m-%%dT%%H:%%M:%%fZ', 'now')\n"
				 "WHERE Lower(f_table_name) = Lower(%Q) AND "
				 "Lower(f_geometry_column) = Lower(%Q);\n"
				 "INSERT INTO \"%s\" (rowid, mbr) VALUES (NEW.ROWID,\nBuildMbrFilter("
				 "MbrMinX(NEW.\"%s\"), MbrMinY(NEW.\"%s\"), MbrMaxX(NEW.\"%s\"), MbrMaxY(NEW.\"%s\")));\nEND",
				 quoted_trigger, quoted_table, p_table,
				 p_column, quoted_rtree, quoted_column,
				 quoted_column, quoted_column, quoted_column);
			    free (quoted_trigger);
			    free (quoted_rtree);
			    free (quoted_table);
			    free (quoted_column);
			}
		      else
			{
			    /* legacy metadata style <= v.3.1.0 */
			    raw =
				sqlite3_mprintf ("gci_%s_%s", p_table,
						 p_column);
			    quoted_trigger = gaiaDoubleQuotedSql (raw);
			    sqlite3_free (raw);
			    raw =
				sqlite3_mprintf ("cache_%s_%s", p_table,
						 p_column);
			    quoted_rtree = gaiaDoubleQuotedSql (raw);
			    sqlite3_free (raw);
			    quoted_table = gaiaDoubleQuotedSql (p_table);
			    quoted_column = gaiaDoubleQuotedSql (p_column);
			    sql_statement =
				sqlite3_mprintf
				("CREATE TRIGGER \"%s\" AFTER INSERT ON \"%s\"\n"
				 "FOR EACH ROW BEGIN\n"
				 "INSERT INTO \"%s\" (rowid, mbr) VALUES (NEW.ROWID,\nBuildMbrFilter("
				 "MbrMinX(NEW.\"%s\"), MbrMinY(NEW.\"%s\"), MbrMaxX(NEW.\"%s\"), MbrMaxY(NEW.\"%s\")));\nEND",
				 quoted_trigger, quoted_table, quoted_rtree,
				 quoted_column, quoted_column, quoted_column,
				 quoted_column);
			    free (quoted_trigger);
			    free (quoted_rtree);
			    free (quoted_table);
			    free (quoted_column);
			}
		      ret =
			  sqlite3_exec (sqlite, sql_statement, NULL, NULL,
					&errMsg);
		      sqlite3_free (sql_statement);
		      if (ret != SQLITE_OK)
			  goto error;

		      /* inserting the new UPDATE trigger MBRcache */
		      if (metadata_version == 3)
			{
			    /* current metadata style >= v.4.0.0 */
			    raw =
				sqlite3_mprintf ("gcu_%s_%s", p_table,
						 p_column);
			    quoted_trigger = gaiaDoubleQuotedSql (raw);
			    sqlite3_free (raw);
			    raw =
				sqlite3_mprintf ("cache_%s_%s", p_table,
						 p_column);
			    quoted_rtree = gaiaDoubleQuotedSql (raw);
			    sqlite3_free (raw);
			    quoted_table = gaiaDoubleQuotedSql (p_table);
			    quoted_column = gaiaDoubleQuotedSql (p_column);
			    sql_statement =
				sqlite3_mprintf
				("CREATE TRIGGER \"%s\" AFTER UPDATE ON \"%s\"\n"
				 "FOR EACH ROW BEGIN\n"
				 "UPDATE geometry_columns_time SET last_update = strftime('%%Y-%%m-%%dT%%H:%%M:%%fZ', 'now')\n"
				 "WHERE Lower(f_table_name) = Lower(%Q) AND "
				 "Lower(f_geometry_column) = Lower(%Q);\n"
				 "UPDATE \"%s\" SET mbr = BuildMbrFilter("
				 "MbrMinX(NEW.\"%s\"), MbrMinY(NEW.\"%s\"), MbrMaxX(NEW.\"%s\"), MbrMaxY(NEW.\"%s\"))\n"
				 "WHERE rowid = NEW.ROWID;\nEND",
				 quoted_trigger, quoted_table, p_table,
				 p_column, quoted_rtree,
				 quoted_column, quoted_column, quoted_column,
				 quoted_column);
			    free (quoted_trigger);
			    free (quoted_rtree);
			    free (quoted_table);
			    free (quoted_column);
			}
		      else
			{
			    /* legacy metadata style <= v.3.1.0 */
			    raw =
				sqlite3_mprintf ("gcu_%s_%s", p_table,
						 p_column);
			    quoted_trigger = gaiaDoubleQuotedSql (raw);
			    sqlite3_free (raw);
			    raw =
				sqlite3_mprintf ("cache_%s_%s", p_table,
						 p_column);
			    quoted_rtree = gaiaDoubleQuotedSql (raw);
			    sqlite3_free (raw);
			    quoted_table = gaiaDoubleQuotedSql (p_table);
			    quoted_column = gaiaDoubleQuotedSql (p_column);
			    sql_statement =
				sqlite3_mprintf
				("CREATE TRIGGER \"%s\" AFTER UPDATE ON \"%s\"\n"
				 "FOR EACH ROW BEGIN\n"
				 "UPDATE \"%s\" SET mbr = BuildMbrFilter("
				 "MbrMinX(NEW.\"%s\"), MbrMinY(NEW.\"%s\"), MbrMaxX(NEW.\"%s\"), MbrMaxY(NEW.\"%s\"))\n"
				 "WHERE rowid = NEW.ROWID;\nEND",
				 quoted_trigger, quoted_table, quoted_rtree,
				 quoted_column, quoted_column, quoted_column,
				 quoted_column);
			    free (quoted_trigger);
			    free (quoted_rtree);
			    free (quoted_table);
			    free (quoted_column);
			}
		      ret =
			  sqlite3_exec (sqlite, sql_statement, NULL, NULL,
					&errMsg);
		      sqlite3_free (sql_statement);
		      if (ret != SQLITE_OK)
			  goto error;

		      /* inserting the new DELETE trigger MBRcache */
		      if (metadata_version == 3)
			{
			    /* current metadata style >= v.4.0.0 */
			    raw =
				sqlite3_mprintf ("gcd_%s_%s", p_table,
						 p_column);
			    quoted_trigger = gaiaDoubleQuotedSql (raw);
			    sqlite3_free (raw);
			    raw =
				sqlite3_mprintf ("cache_%s_%s", p_table,
						 p_column);
			    quoted_rtree = gaiaDoubleQuotedSql (raw);
			    sqlite3_free (raw);
			    quoted_table = gaiaDoubleQuotedSql (p_table);
			    sql_statement =
				sqlite3_mprintf
				("CREATE TRIGGER \"%s\" AFTER DELETE ON \"%s\"\n"
				 "FOR EACH ROW BEGIN\n"
				 "UPDATE geometry_columns_time SET last_delete = strftime('%%Y-%%m-%%dT%%H:%%M:%%fZ', 'now')\n"
				 "WHERE Lower(f_table_name) = Lower(%Q) AND "
				 "Lower(f_geometry_column) = Lower(%Q);\n"
				 "DELETE FROM \"%s\" WHERE rowid = OLD.ROWID;\nEND",
				 quoted_trigger, quoted_table, p_table,
				 p_column, quoted_rtree);
			    free (quoted_trigger);
			    free (quoted_rtree);
			    free (quoted_table);
			}
		      else
			{
			    /* legacy metadata style <= v.3.1.0 */
			    raw =
				sqlite3_mprintf ("gcd_%s_%s", p_table,
						 p_column);
			    quoted_trigger = gaiaDoubleQuotedSql (raw);
			    sqlite3_free (raw);
			    raw =
				sqlite3_mprintf ("cache_%s_%s", p_table,
						 p_column);
			    quoted_rtree = gaiaDoubleQuotedSql (raw);
			    sqlite3_free (raw);
			    quoted_table = gaiaDoubleQuotedSql (p_table);
			    quoted_column = gaiaDoubleQuotedSql (p_column);
			    sql_statement =
				sqlite3_mprintf
				("CREATE TRIGGER \"%s\" AFTER DELETE ON \"%s\"\n"
				 "FOR EACH ROW BEGIN\n"
				 "DELETE FROM \"%s\" WHERE rowid = OLD.ROWID;\nEND",
				 quoted_trigger, quoted_table, quoted_rtree);
			    free (quoted_trigger);
			    free (quoted_rtree);
			    free (quoted_table);
			}
		      ret =
			  sqlite3_exec (sqlite, sql_statement, NULL, NULL,
					&errMsg);
		      sqlite3_free (sql_statement);
		      if (ret != SQLITE_OK)
			  goto error;
		  }

	    }
      }
    ret = sqlite3_finalize (stmt);
/* now we'll adjust any related SpatialIndex as required */
    curr_idx = first_idx;
    while (curr_idx)
      {
	  if (curr_idx->ValidRtree)
	    {
		/* building RTree SpatialIndex */
		raw = sqlite3_mprintf ("idx_%s_%s", curr_idx->TableName,
				       curr_idx->ColumnName);
		quoted_rtree = gaiaDoubleQuotedSql (raw);
		sqlite3_free (raw);
		sql_statement = sqlite3_mprintf ("CREATE VIRTUAL TABLE \"%s\" "
						 "USING rtree(pkid, xmin, xmax, ymin, ymax)",
						 quoted_rtree);
		free (quoted_rtree);
		ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
		sqlite3_free (sql_statement);
		if (ret != SQLITE_OK)
		    goto error;
		buildSpatialIndex (sqlite,
				   (unsigned char *) (curr_idx->TableName),
				   curr_idx->ColumnName);
	    }
	  if (curr_idx->ValidCache)
	    {
		/* building MbrCache SpatialIndex */
		raw = sqlite3_mprintf ("cache_%s_%s", curr_idx->TableName,
				       curr_idx->ColumnName);
		quoted_rtree = gaiaDoubleQuotedSql (raw);
		sqlite3_free (raw);
		quoted_table = gaiaDoubleQuotedSql (curr_idx->TableName);
		quoted_column = gaiaDoubleQuotedSql (curr_idx->ColumnName);
		sql_statement = sqlite3_mprintf ("CREATE VIRTUAL TABLE \"%s\" "
						 "USING MbrCache(\"%s\", \"%s\")",
						 quoted_rtree, quoted_table,
						 quoted_column);
		free (quoted_rtree);
		free (quoted_table);
		free (quoted_column);
		ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
		sqlite3_free (sql_statement);
		if (ret != SQLITE_OK)
		    goto error;
	    }
	  curr_idx = curr_idx->Next;
      }
    goto index_cleanup;
  error:
    spatialite_e ("updateTableTriggers: \"%s\"\n", errMsg);
    sqlite3_free (errMsg);
  index_cleanup:
    curr_idx = first_idx;
    while (curr_idx)
      {
	  next_idx = curr_idx->Next;
	  if (curr_idx->TableName)
	      free (curr_idx->TableName);
	  if (curr_idx->ColumnName)
	      free (curr_idx->ColumnName);
	  free (curr_idx);
	  curr_idx = next_idx;
      }
    if (p_table)
	free (p_table);
    if (p_column)
	free (p_column);
}

SPATIALITE_PRIVATE void
buildSpatialIndex (void *p_sqlite, const unsigned char *table,
		   const char *column)
{
/* loading a SpatialIndex [RTree] */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    char *raw;
    char *quoted_rtree;
    char *quoted_table;
    char *quoted_column;
    char *sql_statement;
    char *errMsg = NULL;
    int ret;

    raw = sqlite3_mprintf ("idx_%s_%s", table, column);
    quoted_rtree = gaiaDoubleQuotedSql (raw);
    sqlite3_free (raw);
    quoted_table = gaiaDoubleQuotedSql ((const char *) table);
    quoted_column = gaiaDoubleQuotedSql (column);
    sql_statement = sqlite3_mprintf ("INSERT INTO \"%s\" "
				     "(pkid, xmin, xmax, ymin, ymax) "
				     "SELECT ROWID, MbrMinX(\"%s\"), MbrMaxX(\"%s\"), MbrMinY(\"%s\"), MbrMaxY(\"%s\") "
				     "FROM \"%s\" WHERE MbrMinX(\"%s\") IS NOT NULL",
				     quoted_rtree, quoted_column, quoted_column,
				     quoted_column, quoted_column, quoted_table,
				     quoted_column);
    free (quoted_rtree);
    free (quoted_table);
    free (quoted_column);
    ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("buildSpatialIndex error: \"%s\"\n", errMsg);
	  sqlite3_free (errMsg);
      }
}

SPATIALITE_PRIVATE int
getRealSQLnames (void *p_sqlite, const char *table, const char *column,
		 char **real_table, char **real_column)
{
/* attempting to retrieve the "real" table and column names (upper/lowercase) */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    char *p_table = NULL;
    char *p_column = NULL;
    char *sql_statement;
    char *quoted;
    const char *name;
    int len;
    sqlite3_stmt *stmt;
    int ret;

    sql_statement = sqlite3_mprintf ("SELECT name "
				     "FROM sqlite_master WHERE type = 'table' "
				     "AND Lower(name) = Lower(?)");
/* compiling SQL prepared statement */
    ret =
	sqlite3_prepare_v2 (sqlite, sql_statement, strlen (sql_statement),
			    &stmt, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("real_names: error %d \"%s\"\n",
			sqlite3_errcode (sqlite), sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, table, strlen (table), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		name = (const char *) sqlite3_column_text (stmt, 0);
		len = sqlite3_column_bytes (stmt, 0);
		if (p_table)
		    free (p_table);
		p_table = malloc (len + 1);
		strcpy (p_table, name);
	    }
      }
    sqlite3_finalize (stmt);

    if (p_table == NULL)
	return 0;

    quoted = gaiaDoubleQuotedSql (p_table);
    sql_statement = sqlite3_mprintf ("PRAGMA table_info(\"%s\")", quoted);
    free (quoted);
/* compiling SQL prepared statement */
    ret =
	sqlite3_prepare_v2 (sqlite, sql_statement, strlen (sql_statement),
			    &stmt, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("real_names: error %d \"%s\"\n",
			sqlite3_errcode (sqlite), sqlite3_errmsg (sqlite));
	  free (p_table);
	  return 0;
      }
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		name = (const char *) sqlite3_column_text (stmt, 1);
		len = sqlite3_column_bytes (stmt, 1);
		if (strcasecmp (name, column) == 0)
		  {
		      if (p_column)
			  free (p_column);
		      p_column = malloc (len + 1);
		      strcpy (p_column, name);
		  }
	    }
      }
    sqlite3_finalize (stmt);

    if (p_column == NULL)
      {
	  free (p_table);
	  return 0;
      }

    *real_table = p_table;
    *real_column = p_column;
    return 1;
}
