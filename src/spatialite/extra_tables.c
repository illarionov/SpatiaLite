/*

 extra_tables.c -- Creating all SLD/SE and ISO Metadata extra tables

 version 4.0, 2013 February 16

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

/*
 
CREDITS:

this module has been partly funded by:
Regione Toscana - Settore Sistema Informativo Territoriale ed Ambientale
(implementing XML support - ISO Metadata and SLD/SE Styles) 

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

#include <spatialite.h>
#include <spatialite_private.h>

#ifdef ENABLE_LIBXML2		/* including LIBXML2 */

static int
check_styling_table (sqlite3 * sqlite, const char *table, int is_view)
{
/* checking if some SLD/SE Styling-related table/view already exists */
    int exists = 0;
    char *sql_statement;
    char *errMsg = NULL;
    int ret;
    char **results;
    int rows;
    int columns;
    int i;
    sql_statement =
	sqlite3_mprintf ("SELECT name FROM sqlite_master WHERE type = '%s'"
			 "AND Upper(name) = Upper(%Q)",
			 (!is_view) ? "table" : "view", table);
    ret =
	sqlite3_get_table (sqlite, sql_statement, &results, &rows, &columns,
			   &errMsg);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
	exists = 1;
    sqlite3_free_table (results);
    return exists;
}

static int
create_external_graphics (sqlite3 * sqlite)
{
/* creating the SE_external_graphics table */
    char *sql;
    int ret;
    char *err_msg = NULL;
    sql = "CREATE TABLE SE_external_graphics (\n"
	"xlink_href TEXT NOT NULL PRIMARY KEY,\n"
	"title TEXT NOT NULL DEFAULT '*** undefined ***',\n"
	"abstract TEXT NOT NULL DEFAULT '*** undefined ***',\n"
	"resource BLOB NOT NULL,\n"
	"file_name TEXT NOT NULL DEFAULT '*** undefined ***')";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE 'SE_external_graphics' error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* creating the SE_external_graphics triggers */
    sql = "CREATE TRIGGER sextgr_mime_type_insert\n"
	"BEFORE INSERT ON 'SE_external_graphics'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on SE_external_graphics violates constraint: "
	"GetMimeType(resource) must must be one of ''image/gif'', ''image/png'', "
	"''image/jpeg'', ''image/svg+xml''')\n"
	"WHERE GetMimeType(NEW.resource) NOT IN ('image/gif', 'image/png', "
	"'image/jpeg', 'image/svg+xml');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER sextgr_mime_type_update\n"
	"BEFORE UPDATE OF 'mime_type' ON 'SE_external_graphics'"
	"\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on SE_external_graphics violates constraint: "
	"GetMimeType(resource) must must be one of ''image/gif'', ''image/png'', "
	"''image/jpeg'', ''image/svg+xml''')\n"
	"WHERE GetMimeType(NEW.resource) NOT IN ('image/gif', 'image/png', "
	"'image/jpeg', 'image/svg+xml');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

static int
create_vector_styled_layers (sqlite3 * sqlite, int relaxed)
{
/* creating the SE_vector_styled_layers table */
    char *sql;
    int ret;
    char *err_msg = NULL;
    sql = "CREATE TABLE SE_vector_styled_layers (\n"
	"f_table_name TEXT NOT NULL,\n"
	"f_geometry_column TEXT NOT NULL,\n"
	"style_id INTEGER NOT NULL,\n"
	"style BLOB NOT NULL,\n"
	"CONSTRAINT pk_sevstl PRIMARY KEY "
	"(f_table_name, f_geometry_column, style_id))";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE 'SE_vector_styled_layers' error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* creating the SE_vector_styled_layers triggers */
    sql = "CREATE TRIGGER sevstl_f_table_name_insert\n"
	"BEFORE INSERT ON 'SE_vector_styled_layers'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on SE_vector_styled_layers violates constraint: "
	"f_table_name value must not contain a single quote')\n"
	"WHERE NEW.f_table_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'insert on SE_vector_styled_layers violates constraint: "
	"f_table_name value must not contain a double quote')\n"
	"WHERE NEW.f_table_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'insert on SE_vector_styled_layers violates constraint: "
	"f_table_name value must be lower case')\n"
	"WHERE NEW.f_table_name <> lower(NEW.f_table_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER sevstl_f_table_name_update\n"
	"BEFORE UPDATE OF 'f_table_name' ON 'SE_vector_styled_layers'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on SE_vector_styled_layers violates constraint: "
	"f_table_name value must not contain a single quote')\n"
	"WHERE NEW.f_table_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'update on SE_vector_styled_layers violates constraint: "
	"f_table_name value must not contain a double quote')\n"
	"WHERE NEW.f_table_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'update on SE_vector_styled_layers violates constraint: "
	"f_table_name value must be lower case')\n"
	"WHERE NEW.f_table_name <> lower(NEW.f_table_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER sevstl_f_geometry_column_insert\n"
	"BEFORE INSERT ON 'SE_vector_styled_layers'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on SE_vector_styled_layers violates constraint: "
	"f_geometry_column value must not contain a single quote')\n"
	"WHERE NEW.f_geometry_column LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'insert on SE_vector_styled_layers violates constraint: "
	"f_geometry_column value must not contain a double quote')\n"
	"WHERE NEW.f_geometry_column LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'insert on SE_vector_styled_layers violates constraint: "
	"f_geometry_column value must be lower case')\n"
	"WHERE NEW.f_geometry_column <> lower(NEW.f_geometry_column);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER sevstl_f_geometry_column_update\n"
	"BEFORE UPDATE OF 'f_geometry_column' ON 'SE_vector_styled_layers'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on SE_vector_styled_layers violates constraint: "
	"f_geometry_column value must not contain a single quote')\n"
	"WHERE NEW.f_geometry_column LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'update on SE_vector_styled_layers violates constraint: "
	"f_geometry_column value must not contain a double quote')\n"
	"WHERE NEW.f_geometry_column LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'update on SE_vector_styled_layers violates constraint: "
	"f_geometry_column value must be lower case')\n"
	"WHERE NEW.f_geometry_column <> lower(NEW.f_geometry_column);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    if (relaxed == 0)
      {
	  /* strong trigger - imposing XML schema validation */
	  sql = "CREATE TRIGGER sevstl_style_insert\n"
	      "BEFORE INSERT ON 'SE_vector_styled_layers'\nFOR EACH ROW BEGIN\n"
	      "SELECT RAISE(ABORT,'insert on SE_vector_styled_layers violates constraint: "
	      "not a valid SLD/SE Vector Style')\n"
	      "WHERE XB_IsSldSeVectorStyle(NEW.style) <> 1;\n"
	      "SELECT RAISE(ABORT,'insert on SE_vector_styled_layers violates constraint: "
	      "not an XML Schema Validated SLD/SE Vector Style')\n"
	      "WHERE XB_IsSchemaValidated(NEW.style) <> 1;\nEND";
      }
    else
      {
	  /* relaxed trigger - not imposing XML schema validation */
	  sql = "CREATE TRIGGER sevstl_style_insert\n"
	      "BEFORE INSERT ON 'SE_vector_styled_layers'\nFOR EACH ROW BEGIN\n"
	      "SELECT RAISE(ABORT,'insert on SE_vector_styled_layers violates constraint: "
	      "not a valid SLD/SE Vector Style')\n"
	      "WHERE XB_IsSldSeVectorStyle(NEW.style) <> 1;\nEND";
      }
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    if (relaxed == 0)
      {
	  /* strong trigger - imposing XML schema validation */
	  sql = "CREATE TRIGGER sevstl_style_update\n"
	      "BEFORE UPDATE ON 'SE_vector_styled_layers'\nFOR EACH ROW BEGIN\n"
	      "SELECT RAISE(ABORT,'update on SE_vector_styled_layers violates constraint: "
	      "not a valid SLD/SE Vector Style')\n"
	      "WHERE XB_IsSldSeVectorStyle(NEW.style) <> 1;\n"
	      "SELECT RAISE(ABORT,'update on SE_vector_styled_layers violates constraint: "
	      "not an XML Schema Validated SLD/SE Vector Style')\n"
	      "WHERE XB_IsSchemaValidated(NEW.style) <> 1;\nEND";
      }
    else
      {
	  /* relaxed trigger - not imposing XML schema validation */
	  sql = "CREATE TRIGGER sevstl_style_update\n"
	      "BEFORE UPDATE ON 'SE_vector_styled_layers'\nFOR EACH ROW BEGIN\n"
	      "SELECT RAISE(ABORT,'update on SE_vector_styled_layers violates constraint: "
	      "not a valid SLD/SE Vector Style')\n"
	      "WHERE XB_IsSldSeVectorStyle(NEW.style) <> 1;\nEND";
      }
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

static int
create_raster_styled_layers (sqlite3 * sqlite, int relaxed)
{
/* creating the SE_raster_styled_layers table */
    char *sql;
    int ret;
    char *err_msg = NULL;
    sql = "CREATE TABLE SE_raster_styled_layers (\n"
	"coverage_name TEXT NOT NULL,\n"
	"style_id INTEGER NOT NULL,\n"
	"style BLOB NOT NULL,\n"
	"CONSTRAINT pk_serstl PRIMARY KEY " "(coverage_name, style_id))";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE 'SE_raster_styled_layers' error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* creating the SE_vector_styled_layers triggers */
    sql = "CREATE TRIGGER serstl_coverage_name_insert\n"
	"BEFORE INSERT ON 'SE_raster_styled_layers'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on SE_raster_styled_layers violates constraint: "
	"coverage_name value must not contain a single quote')\n"
	"WHERE NEW.coverage_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'insert on SE_raster_styled_layers violates constraint: "
	"coverage_name value must not contain a double quote')\n"
	"WHERE NEW.coverage_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'insert on SE_raster_styled_layers violates constraint: "
	"coverage_name value must be lower case')\n"
	"WHERE NEW.coverage_name <> lower(NEW.coverage_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER serstl_coverage_name_update\n"
	"BEFORE UPDATE OF 'coverage_name' ON 'SE_raster_styled_layers'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on SE_raster_styled_layers violates constraint: "
	"coverage_name value must not contain a single quote')\n"
	"WHERE NEW.coverage_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'update on SE_raster_styled_layers violates constraint: "
	"coverage_name value must not contain a double quote')\n"
	"WHERE NEW.coverage_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'update on SE_raster_styled_layers violates constraint: "
	"coverage_name value must be lower case')\n"
	"WHERE NEW.coverage_name <> lower(NEW.coverage_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    if (relaxed == 0)
      {
	  /* strong trigger - imposing XML schema validation */
	  sql = "CREATE TRIGGER serstl_style_insert\n"
	      "BEFORE INSERT ON 'SE_raster_styled_layers'\nFOR EACH ROW BEGIN\n"
	      "SELECT RAISE(ABORT,'insert on SE_raster_styled_layers violates constraint: "
	      "not a valid SLD/SE Raster Style')\n"
	      "WHERE XB_IsSldSeRasterStyle(NEW.style) <> 1;\n"
	      "SELECT RAISE(ABORT,'insert on SE_raster_styled_layers violates constraint: "
	      "not an XML Schema Validated SLD/SE Raster Style')\n"
	      "WHERE XB_IsSchemaValidated(NEW.style) <> 1;\nEND";
      }
    else
      {
	  /* relaxed trigger - not imposing XML schema validation */
	  sql = "CREATE TRIGGER serstl_style_insert\n"
	      "BEFORE INSERT ON 'SE_raster_styled_layers'\nFOR EACH ROW BEGIN\n"
	      "SELECT RAISE(ABORT,'insert on SE_raster_styled_layers violates constraint: "
	      "not a valid SLD/SE Raster Style')\n"
	      "WHERE XB_IsSldSeRasterStyle(NEW.style) <> 1;\nEND";
      }
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    if (relaxed == 0)
      {
	  /* strong trigger - imposing XML schema validation */
	  sql = "CREATE TRIGGER serstl_style_update\n"
	      "BEFORE UPDATE ON 'SE_raster_styled_layers'\nFOR EACH ROW BEGIN\n"
	      "SELECT RAISE(ABORT,'update on SE_raster_styled_layers violates constraint: "
	      "not a valid SLD/SE Raster Style')\n"
	      "WHERE XB_IsSldSeRasterStyle(NEW.style) <> 1;\n"
	      "SELECT RAISE(ABORT,'update on SE_raster_styled_layers violates constraint: "
	      "not an XML Schema Validated SLD/SE Raster Style')\n"
	      "WHERE XB_IsSchemaValidated(NEW.style) <> 1;\nEND";
      }
    else
      {
	  /* relaxed trigger - not imposing XML schema validation */
	  sql = "CREATE TRIGGER serstl_style_update\n"
	      "BEFORE UPDATE ON 'SE_raster_styled_layers'\nFOR EACH ROW BEGIN\n"
	      "SELECT RAISE(ABORT,'update on SE_raster_styled_layers violates constraint: "
	      "not a valid SLD/SE Raster Style')\n"
	      "WHERE XB_IsSldSeRasterStyle(NEW.style) <> 1;\nEND";
      }
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

static int
create_styled_groups (sqlite3 * sqlite)
{
/* creating the SE_styled_groups table */
    char *sql;
    int ret;
    char *err_msg = NULL;
    sql = "CREATE TABLE SE_styled_groups (\n"
	"group_name TEXT NOT NULL PRIMARY KEY,\n"
	"title TEXT NOT NULL DEFAULT '*** undefined ***',\n"
	"abstract TEXT NOT NULL DEFAULT '*** undefined ***')";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE 'SE_styled_groups' error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* creating the SE_styled_groups triggers */
    sql = "CREATE TRIGGER segrp_group_name_insert\n"
	"BEFORE INSERT ON 'SE_styled_groups'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_groups violates constraint: "
	"group_name value must not contain a single quote')\n"
	"WHERE NEW.group_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_groups violates constraint: "
	"group_name value must not contain a double quote')\n"
	"WHERE NEW.group_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_groups violates constraint: "
	"group_name value must be lower case')\n"
	"WHERE NEW.group_name <> lower(NEW.group_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER segrp_group_name_update\n"
	"BEFORE UPDATE OF 'group_name' ON 'SE_styled_groups'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on SE_raster_styled_layers violates constraint: "
	"group_name value must not contain a single quote')\n"
	"WHERE NEW.group_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'update on SE_styled_groups violates constraint: "
	"group_name value must not contain a double quote')\n"
	"WHERE NEW.group_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'update on SE_styled_groups violates constraint: "
	"group_name value must be lower case')\n"
	"WHERE NEW.group_name <> lower(NEW.group_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

static int
create_styled_group_refs (sqlite3 * sqlite)
{
/* creating the SE_styled_group_refs table */
    char *sql;
    int ret;
    char *err_msg = NULL;
    sql = "CREATE TABLE SE_styled_group_refs (\n"
	"id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
	"group_name TEXT NOT NULL,\n"
	"paint_order INTEGER NOT NULL,\n"
	"f_table_name TEXT,\n"
	"f_geometry_column TEXT,\n"
	"vector_style_id INTEGER,\n"
	"coverage_name TEXT,\n"
	"raster_style_id INTEGER,\n"
	"CONSTRAINT fk_se_refs FOREIGN KEY (group_name) "
	"REFERENCES SE_styled_groups (group_name),\n"
	"CONSTRAINT fk_se_styled_vector FOREIGN KEY "
	"(f_table_name, f_geometry_column, vector_style_id) "
	"REFERENCES SE_vector_styled_layers "
	"(f_table_name, f_geometry_column, style_id),\n"
	"CONSTRAINT fk_se_styled_raster FOREIGN KEY "
	"(coverage_name, raster_style_id) "
	"REFERENCES SE_raster_styled_layers (coverage_name, style_id))";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TABLE 'SE_styled_group_refs' error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* creating the SE_styled_group_refs triggers */
    sql = "CREATE TRIGGER segrrefs_group_name_insert\n"
	"BEFORE INSERT ON 'SE_styled_group_refs'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"group_name value must not contain a single quote')\n"
	"WHERE NEW.group_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"group_name value must not contain a double quote')\n"
	"WHERE NEW.group_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"group_name value must be lower case')\n"
	"WHERE NEW.group_name <> lower(NEW.group_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER segrrefs_group_name_update\n"
	"BEFORE UPDATE OF 'group_name' ON 'SE_styled_group_refs'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"group_name value must not contain a single quote')\n"
	"WHERE NEW.group_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"group_name value must not contain a double quote')\n"
	"WHERE NEW.group_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"group_name value must be lower case')\n"
	"WHERE NEW.group_name <> lower(NEW.group_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER segrrefs_f_table_name_insert\n"
	"BEFORE INSERT ON 'SE_styled_group_refs'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"f_table_name value must not contain a single quote')\n"
	"WHERE NEW.f_table_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"f_table_name value must not contain a double quote')\n"
	"WHERE NEW.f_table_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"f_table_name value must be lower case')\n"
	"WHERE NEW.f_table_name <> lower(NEW.f_table_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER segrrefs_f_table_name_update\n"
	"BEFORE UPDATE OF 'f_table_name' ON 'SE_styled_group_refs'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"f_table_name value must not contain a single quote')\n"
	"WHERE NEW.f_table_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"f_table_name value must not contain a double quote')\n"
	"WHERE NEW.f_table_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"f_table_name value must be lower case')\n"
	"WHERE NEW.f_table_name <> lower(NEW.f_table_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER segrrefs_f_geometry_column_insert\n"
	"BEFORE INSERT ON 'SE_styled_group_refs'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"f_geometry_column value must not contain a single quote')\n"
	"WHERE NEW.f_geometry_column LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"f_geometry_column value must not contain a double quote')\n"
	"WHERE NEW.f_geometry_column LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"f_geometry_column value must be lower case')\n"
	"WHERE NEW.f_geometry_column <> lower(NEW.f_geometry_column);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER segrrefs_f_geometry_column_update\n"
	"BEFORE UPDATE OF 'f_geometry_column' ON 'SE_styled_group_refs'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"f_geometry_column value must not contain a single quote')\n"
	"WHERE NEW.f_geometry_column LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"f_geometry_column value must not contain a double quote')\n"
	"WHERE NEW.f_geometry_column LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"f_geometry_column value must be lower case')\n"
	"WHERE NEW.f_geometry_column <> lower(NEW.f_geometry_column);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER segrrefs_coverage_name_insert\n"
	"BEFORE INSERT ON 'SE_styled_group_refs'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"coverage_name value must not contain a single quote')\n"
	"WHERE NEW.coverage_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"coverage_name value must not contain a double quote')\n"
	"WHERE NEW.coverage_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"coverage_name value must be lower case')\n"
	"WHERE NEW.coverage_name <> lower(NEW.coverage_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER segrrefs_coverage_name_update\n"
	"BEFORE UPDATE OF 'coverage_name' ON 'SE_styled_group_refs'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"coverage_name value must not contain a single quote')\n"
	"WHERE NEW.coverage_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"coverage_name value must not contain a double quote')\n"
	"WHERE NEW.coverage_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"coverage_name value must be lower case')\n"
	"WHERE NEW.coverage_name <> lower(NEW.coverage_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER segrrefs_insert\n"
	"BEFORE INSERT ON 'SE_styled_group_refs'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"cannot reference both Vector and Raster at the same time')\n"
	"WHERE (NEW.f_table_name IS NOT NULL OR NEW.f_geometry_column IS NOT NULL "
	"OR NEW.vector_style_id IS NOT NULL) AND (NEW.coverage_name IS NOT NULL "
	"OR NEW.raster_style_id IS NOT NULL);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER segrrefs_update\n"
	"BEFORE UPDATE ON 'SE_styled_group_refs'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"cannot reference both Vector and Raster at the same time')\n"
	"WHERE (NEW.f_table_name IS NOT NULL OR NEW.f_geometry_column IS NOT NULL "
	"OR NEW.vector_style_id IS NOT NULL) AND (NEW.coverage_name IS NOT NULL "
	"OR NEW.raster_style_id IS NOT NULL);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* creating any Index on SE_styled_group_refs */
    sql = "CREATE INDEX idx_SE_styled_vgroups ON "
	"SE_styled_group_refs "
	"(f_table_name, f_geometry_column, vector_style_id)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("Create Index 'idx_SE_styled_vgroups' error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE INDEX idx_SE_styled_rgroups ON "
	"SE_styled_group_refs " "(coverage_name, raster_style_id)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("Create Index 'idx_SE_styled_rgroups' error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE INDEX idx_SE_styled_groups_paint ON "
	"SE_styled_group_refs " "(group_name, paint_order)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("Create Index 'idx_SE_styled_groups_paint' error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}


static int
create_external_graphics_view (sqlite3 * sqlite)
{
/* creating the SE_external_graphics_view view */
    char *sql_statement;
    int ret;
    char *err_msg = NULL;
    sql_statement =
	sqlite3_mprintf
	("CREATE VIEW SE_external_graphics_view AS\n"
	 "SELECT xlink_href AS xlink_href, title AS title, "
	 "abstract AS abstract, resource AS resource, "
	 "file_name AS file_name, GetMimeType(resource) AS mime_type\n"
	 "FROM SE_external_graphics");
    ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &err_msg);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE VIEW 'SE_external_graphics_view' error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

static int
create_vector_styled_layers_view (sqlite3 * sqlite)
{
/* creating the SE_vector_styled_layers_view view */
    char *sql_statement;
    int ret;
    char *err_msg = NULL;
    sql_statement =
	sqlite3_mprintf ("CREATE VIEW SE_vector_styled_layers_view AS \n"
			 "SELECT f_table_name AS f_table_name, f_geometry_column AS f_geometry_column, "
			 "style_id AS style_id, XB_GetTitle(style) AS title, "
			 "XB_GetAbstract(style) AS abstract, style AS style, "
			 "XB_IsSchemaValidated(style) AS schema_validated, "
			 "XB_GetSchemaURI(style) AS schema_uri\n"
			 "FROM SE_vector_styled_layers");
    ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &err_msg);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE VIEW 'SE_vector_styled_layers_view' error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

static int
create_raster_styled_layers_view (sqlite3 * sqlite)
{
/* creating the SE_raster_styled_layers_view view */
    char *sql_statement;
    int ret;
    char *err_msg = NULL;
    sql_statement =
	sqlite3_mprintf ("CREATE VIEW SE_raster_styled_layers_view AS \n"
			 "SELECT coverage_name AS coverage_name, "
			 "style_id AS style_id, XB_GetTitle(style) AS title, "
			 "XB_GetAbstract(style) AS abstract, style AS style, "
			 "XB_IsSchemaValidated(style) AS schema_validated, "
			 "XB_GetSchemaURI(style) AS schema_uri\n"
			 "FROM SE_raster_styled_layers");
    ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &err_msg);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE VIEW 'SE_raster_styled_layers_view' error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

static int
create_styled_groups_view (sqlite3 * sqlite)
{
/* creating the SE_styled_groups_view view */
    char *sql;
    int ret;
    char *err_msg = NULL;
    sql = "CREATE VIEW SE_styled_groups_view AS\n"
	"SELECT g.group_name AS group_name, g.title AS group_title, "
	"g.abstract AS group_abstract, gr.paint_order AS paint_order, "
	"CASE WHEN (vs.f_table_name IS NOT NULL AND vs.f_geometry_column "
	"IS NOT NULL AND vs.style_id IS NOT NULL) THEN 'vector' "
	"WHEN (vr.coverage_name IS NOT NULL AND vr.style_id IS NOT NULL) THEN 'raster' "
	"ELSE '*** unknown ***' END AS type, "
	"vs.f_table_name AS f_table_name, vs.f_geometry_column AS f_geometry_column, "
	"vs.style_id AS vector_style_id, XB_GetTitle(vs.style) AS vector_style_title, "
	"XB_GetAbstract(vs.style) AS vector_style_abstract, "
	"vs.style AS vector_style, gc.geometry_type AS geometry_type, "
	"gc.coord_dimension AS coord_dimension, gc.srid AS srid, "
	"vr.coverage_name AS coverage_name, vr.style_id AS raster_style_id, "
	"XB_GetTitle(vr.style) AS raster_style_title, "
	"XB_GetAbstract(vr.style) AS raster_style_abstract, "
	"vr.style AS raster_style\n"
	"FROM SE_styled_groups AS g\n"
	"JOIN SE_styled_group_refs AS gr ON (g.group_name = gr.group_name)\n"
	"LEFT JOIN SE_vector_styled_layers AS vs ON (gr.f_table_name = vs.f_table_name "
	"AND gr.f_geometry_column = vs.f_geometry_column AND gr.vector_style_id = vs.style_id)\n"
	"LEFT JOIN geometry_columns AS gc ON (vs.f_table_name = gc.f_table_name "
	"AND vs.f_geometry_column = gc.f_geometry_column)\n"
	"LEFT JOIN SE_raster_styled_layers AS vr ON (gr.coverage_name = vr.coverage_name "
	"AND gr.raster_style_id = vr.style_id)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE VIEW 'SE_styled_groups_view' error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

SPATIALITE_PRIVATE int
createStylingTables (void *p_sqlite, int relaxed)
{
/* Creating the SE Styling tables */
    const char *tables[10];
    int views[9];
    const char **p_tbl;
    int *p_view;
    int ok_table;
    sqlite3 *sqlite = p_sqlite;

/* checking Styling tables */
    tables[0] = "SE_external_graphics";
    tables[1] = "SE_vector_styled_layers";
    tables[2] = "SE_raster_styled_layers";
    tables[3] = "SE_styled_groups";
    tables[4] = "SE_styled_group_refs";
    tables[5] = "SE_external_graphics_view";
    tables[6] = "SE_vector_styled_layers_view";
    tables[7] = "SE_raster_styled_layers_view";
    tables[8] = "SE_styled_groups_view";
    tables[9] = NULL;
    views[0] = 0;
    views[1] = 0;
    views[2] = 0;
    views[3] = 0;
    views[4] = 0;
    views[5] = 1;
    views[6] = 1;
    views[7] = 1;
    views[8] = 1;
    p_tbl = tables;
    p_view = views;
    while (*p_tbl != NULL)
      {
	  ok_table = check_styling_table (sqlite, *p_tbl, *p_view);
	  if (ok_table)
	    {
		spatialite_e
		    ("CreateStylingTables() error: table '%s' already exists\n",
		     *p_tbl);
		goto error;
	    }
	  p_tbl++;
	  p_view++;
      }

/* creating Topology tables */
    if (!create_external_graphics (sqlite))
	goto error;
    if (!create_vector_styled_layers (sqlite, relaxed))
	goto error;
    if (!create_raster_styled_layers (sqlite, relaxed))
	goto error;
    if (!create_styled_groups (sqlite))
	goto error;
    if (!create_styled_group_refs (sqlite))
	goto error;
    if (!create_external_graphics_view (sqlite))
	goto error;
    if (!create_vector_styled_layers_view (sqlite))
	goto error;
    if (!create_raster_styled_layers_view (sqlite))
	goto error;
    if (!create_styled_groups_view (sqlite))
	goto error;
    return 1;

  error:
    return 0;
}

#endif /* end including LIBXML2 */
