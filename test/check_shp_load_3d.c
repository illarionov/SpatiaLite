/*

 check_shp_load_3d.c -- SpatiaLite Test Case

 Author: Brad Hards <bradh@frogmouth.net>

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

Contributor(s):
Brad Hards <bradh@frogmouth.net>

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

#include "sqlite3.h"
#include "spatialite.h"

int main (int argc, char *argv[])
{
    int ret;
    sqlite3 *handle;
    char *err_msg = NULL;
    int row_count;
    const char *sql;
    char **results;
    int rows;
    int columns;

    spatialite_init (0);
    ret = sqlite3_open_v2 (":memory:", &handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "cannot open in-memory database: %s\n", sqlite3_errmsg (handle));
	sqlite3_close(handle);
	return -1;
    }
    
    ret = sqlite3_exec (handle, "SELECT InitSpatialMetadata()", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -2;
    }

    ret = load_shapefile (handle, "shp/merano-3d/polygons", "polygons", "CP1252", 25832, 
			  "geom", 0, 1, 1, 0, &row_count, err_msg);
    if (!ret) {
        fprintf (stderr, "load_shapefile() error: %s\n", err_msg);
	sqlite3_close(handle);
	return -3;
    }

    ret = update_layer_statistics (handle, "polygons", "geom");
    if (!ret) {
        fprintf (stderr, "update_layer_statistics() error\n", err_msg);
	sqlite3_close(handle);
	return -8;
    }

    ret = load_shapefile (handle, "shp/merano-3d/roads", "roads", "CP1252", 25832, 
			  "geom", 0, 0, 1, 0, &row_count, err_msg);
    if (!ret) {
        fprintf (stderr, "load_shapefile() error: %s\n", err_msg);
	sqlite3_close(handle);
	return -4;
    }

    ret = load_shapefile (handle, "shp/merano-3d/points", "points", "CP1252", 25832, 
			  "geom", 0, 0, 1, 0, &row_count, err_msg);
    if (!ret) {
        fprintf (stderr, "load_shapefile() error: %s\n", err_msg);
	sqlite3_close(handle);
	return -5;
    }
    
    elementary_geometries (handle, "points", "geom", "elem_point", "pk_elem", "mul_id");
    elementary_geometries (handle, "roads", "geom", "elem_linestring", "pk_elem", "mul_id");
    elementary_geometries (handle, "polygons", "geom", "elem_poly", "pk_elem", "mul_id");

    ret = update_layer_statistics (handle, NULL, NULL);
    if (!ret) {
        fprintf (stderr, "update_layer_statistics() error\n", err_msg);
	sqlite3_close(handle);
	return -8;
    }

    sql = "SELECT row_count, extent_min_x, extent_max_y "
          "FROM layer_statistics "
          "WHERE raster_layer = 0 AND table_name LIKE 'roads' "
          "AND geometry_column LIKE 'geom'";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "Error: %s\n", err_msg);
        sqlite3_free (err_msg);
        return -9;
    }
    if (rows != 1) {
        fprintf (stderr, "Unexpected num of rows for LAYER_STATISTICS.\n");
        return  -10;
    }
    if (atoi(results[columns+0]) != 18) {
        fprintf (stderr, "Unexpected error: LAYER_STATISTICS bad result row_count: %s\n", results[columns+0]);
        return -11;
    }
    if (atof(results[columns+1]) != 666057.648017325) {
        fprintf (stderr, "Unexpected error: LAYER_STATISTICS bad result extent_min_x: %s\n", results[columns+1]);
        return -12;
    }
    if (atof(results[columns+2]) != 5170671.31627796) {
        fprintf (stderr, "Unexpected error: LAYER_STATISTICS bad result extent_max_y: %s\n", results[columns+2]);
        return -13;
    }
    sqlite3_free_table (results);

    ret = sqlite3_exec (handle, "DROP TABLE polygons", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "DROP polygons error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -6;
    }
    
    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (handle));
	return -7;
    }

    spatialite_cleanup();
    sqlite3_reset_auto_extension();
    return 0;
}
