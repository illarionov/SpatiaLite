/*

 check_spatialindex.c -- SpatiaLite Test Case

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

    ret = load_shapefile (handle, "shp/foggia/local_councils", "Councils",
			  "CP1252", 23032, "geom", 1, 0, 1, 0, &row_count,
			  err_msg);
    if (!ret) {
        fprintf (stderr, "load_shapefile() error: %s\n", err_msg);
	sqlite3_close(handle);
	return -3;
    }
    if (row_count != 61) {
	fprintf (stderr, "unexpected number of rows loaded: %i\n", row_count);
	sqlite3_close(handle);
	return -4;
    }
    
    ret = sqlite3_get_table (handle, "SELECT lc_name FROM Councils WHERE MbrWithin(geom, BuildMbr(1040523, 4010000, 1140523, 4850000)) ORDER BY lc_name;",
			     &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -5;
    }
    if ((rows != 22) || (columns != 1)) {
	fprintf (stderr, "Unexpected error: select lc_name bad result: %i/%i.\n", rows, columns);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return  -6;
    }
    if (strcmp(results[1], "Ascoli Satriano") != 0) {
	fprintf (stderr, "unexpected result at 1: %s\n", results[1]);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -7;
    }
    if (strcmp(results[22], "Zapponeta") != 0) {
	fprintf (stderr, "unexpected result at 22: %s\n", results[22]);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -8;
    }
    sqlite3_free_table (results);

    ret = sqlite3_exec (handle, "SELECT CreateSpatialIndex('Councils', 'geom');",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CreateMbrCache error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -9;
    }

    rows = 0;
    columns = 0;
    ret = sqlite3_get_table (handle, "SELECT pkid FROM idx_Councils_geom;",
			     &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Error in idx SELECT: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -10;
    }
    if ((rows != 61) || (columns != 1)) {
	fprintf (stderr, "Unexpected error: select pkid bad idx result: %i/%i.\n", rows, columns);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return  -11;
    }
    sqlite3_free_table (results);

    rows = 0;
    columns = 0;
    ret = sqlite3_get_table (handle, "SELECT lc_name FROM Councils WHERE ROWID IN (SELECT pkid FROM idx_Councils_geom WHERE xmin > 1040523 AND ymin > 4010000 AND xmax < 1140523 AND ymax < 4850000) ORDER BY lc_name;",
			     &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Error in Mbr SELECT Contains: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -12;
    }
    if ((rows != 22) || (columns != 1)) {
	fprintf (stderr, "Unexpected error: select lc_name bad idx result: %i/%i.\n", rows, columns);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return  -13;
    }
    if (strcmp(results[1], "Ascoli Satriano") != 0) {
	fprintf (stderr, "unexpected mbr result at 1: %s\n", results[1]);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -14;
    }
    if (strcmp(results[22], "Zapponeta") != 0) {
	fprintf (stderr, "unexpected mbr result at 22: %s\n", results[22]);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -15;
    }
    sqlite3_free_table (results);

    ret = sqlite3_exec (handle, "INSERT INTO Councils (lc_name, geom) VALUES ('Quairading', GeomFromText('MULTIPOLYGON(((997226.750031 4627372.000018, 997301.750031 4627332.000018, 997402.500031 4627344.000018, 997541.500031 4627326.500018,997226.750031 4627372.000018)))', 23032));",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "INSERT error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -16;
    }

    rows = 0;
    columns = 0;
    ret = sqlite3_get_table (handle, "SELECT lc_name FROM Councils WHERE ROWID IN (SELECT pkid FROM idx_Councils_geom WHERE xmin > 1040523 AND ymin > 4010000 AND xmax < 1140523 AND ymax < 4850000) ORDER BY lc_name;",
			     &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Error in Mbr SELECT2: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -17;
    }
    if ((rows != 22) || (columns != 1)) {
	fprintf (stderr, "Unexpected error: select lc_name bad idx result: %i/%i.\n", rows, columns);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -18;
    }
    if (strcmp(results[1], "Ascoli Satriano") != 0) {
	fprintf (stderr, "unexpected mbr result at 1: %s\n", results[1]);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -19;
    }
    if (strcmp(results[22], "Zapponeta") != 0) {
	fprintf (stderr, "unexpected mbr result at 22: %s\n", results[22]);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -20;
    }
    sqlite3_free_table (results);
  
    ret = sqlite3_exec (handle, "UPDATE Councils SET geom = GeomFromText('MULTIPOLYGON(((987226.750031 4627372.000018, 997301.750031 4627331.000018, 997402.500032 4627344.000018, 997541.500031 4627326.500018, 987226.750031 4627372.000018)))', 23032) WHERE lc_name = \"Quairading\";",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "UPDATE error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -34;
    }

    rows = 0;
    columns = 0;
    ret = sqlite3_get_table (handle, "SELECT lc_name FROM Councils WHERE ROWID IN (SELECT pkid FROM idx_Councils_geom WHERE xmin > 1040523 AND ymin > 4010000 AND xmax < 1140523 AND ymax < 4850000) ORDER BY lc_name;",
			     &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Error in Mbr SELECT3: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -35;
    }
    if ((rows != 22) || (columns != 1)) {
	fprintf (stderr, "Unexpected error: select lc_name bad idx result: %i/%i.\n", rows, columns);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -36;
    }
    if (strcmp(results[1], "Ascoli Satriano") != 0) {
	fprintf (stderr, "unexpected mbr result at 1: %s\n", results[1]);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -37;
    }
    if (strcmp(results[22], "Zapponeta") != 0) {
	fprintf (stderr, "unexpected mbr result at 22: %s\n", results[22]);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -38;
    }
    sqlite3_free_table (results);

    ret = sqlite3_exec (handle, "DELETE FROM Councils WHERE lc_name = \"Ascoli Satriano\";",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "UPDATE error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -39;
    }

    rows = 0;
    columns = 0;
    ret = sqlite3_get_table (handle, "SELECT lc_name FROM Councils WHERE ROWID IN (SELECT pkid FROM idx_Councils_geom WHERE xmin > 1040523 AND ymin > 4010000 AND xmax < 1140523 AND ymax < 4850000) ORDER BY lc_name;",
			     &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Error in Mbr SELECT3: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -40;
    }
    if ((rows != 21) || (columns != 1)) {
	fprintf (stderr, "Unexpected error: select lc_name bad idx result: %i/%i.\n", rows, columns);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -41;
    }
    if (strcmp(results[1], "Cagnano Varano") != 0) {
	fprintf (stderr, "unexpected mbr result at 1: %s\n", results[1]);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -42;
    }
    if (strcmp(results[21], "Zapponeta") != 0) {
	fprintf (stderr, "unexpected mbr result at 21: %s\n", results[21]);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -43;
    }
    sqlite3_free_table (results);

    rows = 0;
    columns = 0;
    ret = sqlite3_get_table (handle, "SELECT rowid, nodeno FROM idx_Councils_geom_rowid ORDER BY rowid;",
			     &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Error in Mbr SELECT: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -44;
    }
    if ((rows != 61) || (columns != 2)) {
	fprintf (stderr, "Unexpected error: select rowid bad idx result: %i/%i.\n", rows, columns);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return  -45;
    }
    if (strcmp(results[2], "1") != 0) {
	fprintf (stderr, "unexpected mbr result at 1: %s\n", results[2]);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -46;
    }
    if (strcmp(results[12], "6") != 0) {
	fprintf (stderr, "unexpected mbr result at 6: %s\n", results[12]);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -47;
    }
    sqlite3_free_table (results);

    ret = sqlite3_exec (handle, "SELECT CheckSpatialIndex('Councils', 'geom');", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CheckSpatialIndex error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -48;
    }

    ret = sqlite3_exec (handle, "SELECT RebuildGeometryTriggers('Councils', 'geom');", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Rebuild triggers error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -49;
    }

    ret = sqlite3_exec (handle, "SELECT DisableSpatialIndex('Councils', 'geom');", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Disable index error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -50;
    }

    ret = sqlite3_exec (handle, "SELECT RebuildGeometryTriggers('Councils', 'geom');", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Disable index error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -51;
    }

    ret = sqlite3_exec (handle, "DROP TABLE Councils;", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "DROP TABLE Councils error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -52;
    }

    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (handle));
	return -53;
    }
    
    spatialite_cleanup();
    sqlite3_reset_auto_extension();
    return 0;
}
