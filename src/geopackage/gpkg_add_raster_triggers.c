/*

    GeoPackage extensions for SpatiaLite / SQLite
 
Version: MPL 1.1/GPL 2.0/LGPL 2.1

The contents of this file are subject to the Mozilla Public License Version
1.1 (the "License"); you may not use this file except in compliance with
the License. You may obtain a copy of the License at
http://www.mozilla.org/MPL/
 
Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
for the specific language governing rights and limitations under the
License.

The Original Code is GeoPackage Extensions

The Initial Developer of the Original Code is Brad Hards (bradh@frogmouth.net)
 
Portions created by the Initial Developer are Copyright (C) 2008
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

#include "spatialite/geopackage.h"
#include "config.h"
#include "geopackage_internal.h"

#ifdef ENABLE_GEOPACKAGE
GEOPACKAGE_DECLARE void
fnct_gpkgAddRasterTriggers (sqlite3_context * context, int argc UNUSED,
			sqlite3_value ** argv)
{
/* SQL function:
/ gpkgAddRasterTriggers(table, column_name)
/
/ Adds Geopackage raster table triggers for the named table and column
/ returns nothing on success, raises exception on error
/
/ There is a 130 character limit on the table name and column name (combined).
/ You may get an error if you use something longer.
*/
    const unsigned char *table;
    const unsigned char *raster_column;
    char *sql_stmt = NULL;
    sqlite3 *sqlite = NULL;
    char *errMsg = NULL;
    int ret = 0;
    
    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
    {
	sqlite3_result_error(context, "gpkgAddRasterTriggers() error: argument 1 [table] is not of the String type", -1);
	return;
    }
    table = sqlite3_value_text (argv[0]);
    
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
    {
	sqlite3_result_error(context, "gpkgAddRasterTriggers() error: argument 2 [raster column] is not of the String type", -1);
	return;
    }
    raster_column = sqlite3_value_text (argv[1]);
    
    /* build the mime_type insert trigger */
    sql_stmt = sqlite3_mprintf(
	"CREATE TRIGGER \"%q_%q_mime_type_insert\"\n"
	"BEFORE INSERT ON \"%q\"\n"
	"FOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'insert on table ''%q'' violates constraint: %q format image/jpeg not specified for table in raster_format_metadata')\n"
	"WHERE (SELECT IsJpegBlob(NEW.%q) = 1)\n"
	"AND NOT('jpeg' IN (SELECT DISTINCT substr(mime_type, 7) FROM raster_format_metadata\n"
	"WHERE r_table_name = '%q' AND r_raster_column= '%q'));\n"
	"SELECT RAISE(ABORT, 'insert on table ''%q'' violates constraint: %q format image/png not specified for table in raster_format_metadata')\n"
	"WHERE (SELECT IsPngBlob(NEW.%q) = 1)\n"
	"AND NOT ('png' IN (SELECT DISTINCT substr(mime_type, 7) FROM raster_format_metadata\n"
	"WHERE r_table_name = '%q' AND r_raster_column= '%q'));\n"
	"SELECT RAISE(ABORT, 'insert on table ''%q'' violates constraint: %q format image/tiff not specified for table in raster_format_metadata')\n"
	"WHERE (SELECT IsTiffBlob(NEW.%q) = 1)\n"
	"AND NOT ('tiff' IN (SELECT DISTINCT substr(mime_type, 7) FROM raster_format_metadata\n"
	"WHERE r_table_name = '%q' AND r_raster_column= '%q'));\n"
	"SELECT RAISE(ABORT, 'insert on table ''%q'' violates constraint: %q format image/x-webp not specified for table in raster_format_metadata')\n"
	"WHERE (SELECT IsWebpBlob(NEW.%q) = 1)\n"
	"AND NOT ('x-webp' IN (SELECT DISTINCT substr(mime_type, 7) FROM raster_format_metadata\n"
	"WHERE r_table_name = '%q' AND r_raster_column = '%q'));\n"
	"SELECT RAISE(ABORT, 'insert on table ''%q'' violates constraint: %q format not specified for table in raster_format_metadata')\n"
	"WHERE /* allowed types from raster_format_metadata records for the %q table and %q column */\n"
	"(SELECT IsJpegBlob(NEW.%q) = 0)\n"
	"AND (SELECT IsPngBlob(NEW.%q) = 0)\n"
	"AND (SELECT IsTiffBlob(NEW.%q) = 0)\n"
	"AND (SELECT IsWebpBlob(NEW.%q) = 0);\n"
	"END",
	table, raster_column,
	table,
	table, raster_column,
	raster_column,
	table, raster_column,
	table, raster_column,
	raster_column,
	table, raster_column,
	table, raster_column,
	raster_column,
	table, raster_column,
	table, raster_column,
	raster_column,
	table, raster_column,
	table, raster_column,
	table, raster_column,
	raster_column,
	raster_column,
	raster_column,
	raster_column);
    
    /* add the insert trigger */
    sqlite = sqlite3_context_db_handle (context);
    ret = sqlite3_exec (sqlite, sql_stmt, NULL, NULL, &errMsg);
    sqlite3_free(sql_stmt);
    if (ret != SQLITE_OK)
    {
	sqlite3_result_error(context, errMsg, -1);
	sqlite3_free(errMsg);
	return;
    }
    
    /* build the mime_type update trigger */
    sql_stmt = sqlite3_mprintf(
	"CREATE TRIGGER \"%q_%q_mime_type_update\"\n"
	"BEFORE UPDATE OF %q ON \"%q\"\n"
	"FOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on table ''%q'' violates constraint: %q format image/jpeg not specified for table in raster_format_metadata')\n"
	"WHERE (SELECT IsJpegBlob(NEW.%q) = 1)\n"
	"AND NOT('jpeg' IN (SELECT DISTINCT substr(mime_type, 7) FROM raster_format_metadata\n"
	"WHERE r_table_name = '%q' AND r_raster_column= '%q'));\n"
	"SELECT RAISE(ABORT, 'update on table ''%q'' violates constraint: %q format image/png not specified for table in raster_format_metadata')\n"
	"WHERE (SELECT IsPngBlob(NEW.%q) = 1)\n"
	"AND NOT ('png' IN (SELECT DISTINCT substr(mime_type, 7) FROM raster_format_metadata\n"
	"WHERE r_table_name = '%q' AND r_raster_column= '%q'));\n"
	"SELECT RAISE(ABORT, 'update on table ''%q'' violates constraint: %q format image/tiff not specified for table in raster_format_metadata')\n"
	"WHERE (SELECT IsTiffBlob(NEW.%q) = 1)\n"
	"AND NOT ('tiff' IN (SELECT DISTINCT substr(mime_type, 7) FROM raster_format_metadata\n"
	"WHERE r_table_name = '%q' AND r_raster_column= '%q'));\n"
	"SELECT RAISE(ABORT, 'update on table ''%q'' violates constraint: %q format image/x-webp not specified for table in raster_format_metadata')\n"
	"WHERE (SELECT IsWebpBlob(NEW.%q) = 1)\n"
	"AND NOT ('x-webp' IN (SELECT DISTINCT substr(mime_type, 7) FROM raster_format_metadata\n"
	"WHERE r_table_name = '%q' AND r_raster_column = '%q'));\n"
	"SELECT RAISE(ABORT, 'update on table ''%q'' violates constraint: %q format not specified for table in raster_format_metadata')\n"
	"WHERE /* allowed types from raster_format_metadata records for the %q table and %q column */\n"
	"(SELECT IsJpegBlob(NEW.%q) = 0)\n"
	"AND (SELECT IsPngBlob(NEW.%q) = 0)\n"
	"AND (SELECT IsTiffBlob(NEW.%q) = 0)\n"
	"AND (SELECT IsWebpBlob(NEW.%q) = 0);\n"
	"END",
	table, raster_column,
	raster_column, table,
	table, raster_column,
	raster_column,
	table, raster_column,
	table, raster_column,
	raster_column,
	table, raster_column,
	table, raster_column,
	raster_column,
	table, raster_column,
	table, raster_column,
	raster_column,
	table, raster_column,
	table, raster_column,
	table, raster_column,
	raster_column,
	raster_column,
	raster_column,
	raster_column);
    
    /* add the update trigger */
    ret = sqlite3_exec (sqlite, sql_stmt, NULL, NULL, &errMsg);
    sqlite3_free(sql_stmt);
    if (ret != SQLITE_OK)
    {
	sqlite3_result_error(context, errMsg, -1);
	return;
    }

}
#endif
