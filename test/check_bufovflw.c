/*

 check_bufovflw.c -- SpatiaLite Test Case - buffer overflows

 Author: Sandro Furieri <a.furieri@lqt.it>

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
    int suffix_len = 12/*8 * 1024*/;	/* 128 KB suffix */
    char *suffix;
    char *table_a;
    char *table_b;
    char *topology;
    char *auth;
    char *kml1;
    char *kml2;
    char *kmlvalue;
    char *pk;
    char *name;
    char *geom;
    char *sql;
    char **results;
    const char *value;
    int rows;
    int columns;

    spatialite_init (0);
    ret = sqlite3_open_v2 (":memory:", &handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "cannot open in-memory db: %s\n", sqlite3_errmsg (handle));
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

/* setting up very long table and column names */
    suffix = malloc(suffix_len);
    memset(suffix, 'z', suffix_len);  
    suffix[suffix_len-1] = '\0';

    table_a = sqlite3_mprintf("table_a_%s", suffix);
    table_b = sqlite3_mprintf("table_b_%s", suffix);
    pk = sqlite3_mprintf("id_%s", suffix);
    name = sqlite3_mprintf("name_%s", suffix);
    geom = sqlite3_mprintf("geom_%s", suffix);
    topology = sqlite3_mprintf("topology_%s_", suffix);

/* creating table "A" */
    sql = sqlite3_mprintf("CREATE TABLE %s (\n"
                          "%s INTEGER NOT NULL PRIMARY KEY,\n"
                          "%s TEXT NOT NULL)", table_a, pk, name);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CREATE TABLE-A error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -3;
    }

/* adding a POINT Geometry to table "A" */
    sql = sqlite3_mprintf("SELECT AddGeometryColumn(%Q, %Q, "
                          "4326, 'POINT', 'XY')", table_a, geom);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "AddGeometryColumn error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -4;
    }

/* creating a Spatial Index on table "A" */
    sql = sqlite3_mprintf("SELECT CreateSpatialIndex(%Q, %Q)", table_a, geom);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CreateSpatialIndex (TABLE-A) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -5;
    }

/* inserting few valid rows on table "A" */
    sql = sqlite3_mprintf("INSERT INTO %s (%s, %s, %s) VALUES "
                          "(1, 'alpha', MakePoint(1, 10, 4326))", table_a, pk, name, geom);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "INSERT INTO error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -6;
    }
    sql = sqlite3_mprintf("INSERT INTO %s (%s, %s, %s) VALUES "
                          "(2, 'beta', MakePoint(2, 20, 4326))", table_a, pk, name, geom);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "INSERT INTO error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -106;
    }
    sql = sqlite3_mprintf("INSERT INTO %s (%s, %s, %s) VALUES "
                          "(3, 'gamma', MakePoint(3, 30, 4326))", table_a, pk, name, geom);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "INSERT INTO error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -206;
    }
    sql = sqlite3_mprintf("INSERT INTO %s (%s, %s, %s) VALUES "
                          "(4, 'delta', MakePoint(4, 40, 4326))", table_a, pk, name, geom);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "INSERT INTO error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -306;
    }
    sql = sqlite3_mprintf("INSERT INTO %s (%s, %s, %s) VALUES "
                          "(5, 'epsilon', MakePoint(5, 50, 4326))", table_a, pk, name, geom);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "INSERT INTO error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -406;
    }

/* inserting few invalid geometries on table "A" */
    sql = sqlite3_mprintf("INSERT INTO %s (%s, %s, %s) VALUES "
                          "(6, 'zeta', MakePoint(10, 10))", table_a, pk, name, geom);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
        sql = sqlite3_mprintf("%s.%s violates Geometry constraint "
                              "[geom-type or SRID not allowed]", table_a, geom);
        ret = strcmp(sql, err_msg);
	sqlite3_free(err_msg);
        sqlite3_free(sql);
        if (ret == 0)
            goto test2;
	sqlite3_close(handle);
	return -7;
    }
test2:
    sql = sqlite3_mprintf("INSERT INTO %s (%s, %s, %s) VALUES "
                          "(7, 'eta', MakePointZ(20, 20, 20, 4326))", table_a, pk, name, geom);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
        sql = sqlite3_mprintf("%s.%s violates Geometry constraint "
                              "[geom-type or SRID not allowed]", table_a, geom);
        ret = strcmp(sql, err_msg);
	sqlite3_free(err_msg);
        sqlite3_free(sql);
        if (ret == 0)
            goto test3;
	sqlite3_close(handle);
	return -8;
    }
test3:
    sql = sqlite3_mprintf("INSERT INTO %s (%s, %s, %s) VALUES "
                          "(8, 'theta', GeomFromText('LINESTRING(0 0, 1 1)', 4326))", table_a, pk, name, geom);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
        sql = sqlite3_mprintf("%s.%s violates Geometry constraint "
                              "[geom-type or SRID not allowed]", table_a, geom);
        ret = strcmp(sql, err_msg);
	sqlite3_free(err_msg);
        sqlite3_free(sql);
        if (ret == 0)
            goto test4;
	sqlite3_close(handle);
	return -9;
    }
test4:

/* checking for validity (table A) */
    sql = sqlite3_mprintf("SELECT Count(*) FROM %s WHERE ROWID IN ("
                          "SELECT ROWID FROM SpatialIndex WHERE "
                          "f_table_name = %Q AND f_geometry_column = %Q "
                          "AND search_frame = BuildCircleMbr(3, 30, 1))", table_a, table_a, geom);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Test TABLE-A error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -10;
    }
    if (rows != 1 || columns != 1) {
        fprintf (stderr, "Unexpected rows/columns (TABLE-A): r=%d c=%d\n", rows, columns);
        return -11;
    }
    value = results[1];
    if (strcmp ("1", value) != 0) {
        fprintf (stderr, "Unexpected result (TABLE-A): %s\n", results[1]);
        return -12;
    }
    sqlite3_free_table (results);

/* creating table "B" */
    sql = sqlite3_mprintf("CREATE TABLE %s AS "
                          "SELECT * FROM %s", table_b, table_a);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CREATE TABLE-B error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -13;
    }

/* recovering Geometry for table "B" */
    sql = sqlite3_mprintf("SELECT RecoverGeometryColumn(%Q, %Q, "
                          "4326, 'POINT', 'XY')", table_b, geom);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "RecoverGeometryColumn error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -14;
    }

/* creating a Spatial Index on table "B" */
    sql = sqlite3_mprintf("SELECT CreateSpatialIndex(%Q, %Q)", table_b, geom);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CreateSpatialIndex (TABLE-B) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -15;
    }

/* checking for validity (table B) */
    sql = sqlite3_mprintf("SELECT Count(*) FROM %s WHERE ROWID IN ("
                          "SELECT ROWID FROM SpatialIndex WHERE "
                          "f_table_name = %Q AND f_geometry_column = %Q "
                          "AND search_frame = BuildCircleMbr(2, 20, 1))", table_b, table_b, geom);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Test TABLE-B error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -16;
    }
    if (rows != 1 || columns != 1) {
        fprintf (stderr, "Unexpected rows/columns (TABLE-B): r=%d c=%d\n", rows, columns);
        return -17;
    }
    value = results[1];
    if (strcmp ("1", value) != 0) {
        fprintf (stderr, "Unexpected result (TABLE-B): %s\n", results[1]);
        return -18;
    }
    sqlite3_free_table (results);

/* deleting one row from Table B */
    sql = sqlite3_mprintf("DELETE FROM %s WHERE %s = 2", table_b, pk);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "DELETE (TABLE-B) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -19;
    }

/* updating one row into Table B */
    sql = sqlite3_mprintf("UPDATE %s SET %s = MakePoint(-3, -30, 4326) "
                          "WHERE %s = 4", table_b, geom, pk);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "UPDATE (TABLE-B) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -20;
    }

/* invalid updates table "B" */
    sql = sqlite3_mprintf("UPDATE %s SET %s = MakePoint(-2, -20) "
                          "WHERE %s = 2", table_b, geom, pk);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
        sql = sqlite3_mprintf("%s.%s violates Geometry constraint "
                              "[geom-type or SRID not allowed]", table_b, geom);
        ret = strcmp(sql, err_msg);
        sqlite3_free(sql);
        if (ret == 0)
            goto test5;
        sqlite3_free(sql);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -21;
    }
test5:
    sql = sqlite3_mprintf("UPDATE %s SET %s = MakePointZ(-2, -20, 20, 4326) "
                          "WHERE %s = 2", table_b, geom, pk);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
        sql = sqlite3_mprintf("%s.%s violates Geometry constraint "
                              "[geom-type or SRID not allowed]", table_b, geom);
        ret = strcmp(sql, err_msg);
        sqlite3_free(sql);
        if (ret == 0)
            goto test6;
        sqlite3_free(sql);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -22;
    }
test6:
    sql = sqlite3_mprintf("UPDATE %s SET %s = GeomFromText('LINESTRING(0 0, 1 1)', 4326) "
                          "WHERE %s = 2", table_b, geom, pk);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
        sql = sqlite3_mprintf("%s.%s violates Geometry constraint "
                              "[geom-type or SRID not allowed]", table_b, geom);
        ret = strcmp(sql, err_msg);
        sqlite3_free(sql);
        if (ret == 0)
            goto test7;
        sqlite3_free(sql);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -23;
    }
test7:

/* VACUUM - thus invalidating TABLE-B Spatial Index */
    ret = sqlite3_exec (handle, "VACUUM", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "VACUUM error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -24;
    }

/* checking the Spatial Index on table "B" */
    sql = sqlite3_mprintf("SELECT CheckSpatialIndex(%Q, %Q)", table_b, geom);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CheckSpatialIndex TABLE-B error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -25;
    }
    if (rows != 1 || columns != 1) {
        fprintf (stderr, "Unexpected rows/columns (check RTree TABLE-B): r=%d c=%d\n", rows, columns);
        return -26;
    }
    value = results[1];
    if (strcmp ("0", value) != 0) {
        fprintf (stderr, "Unexpected result (check RTree TABLE-B): %s\n", results[1]);
        return -27;
    }
    sqlite3_free_table (results);

/* checking the Spatial Index all table */
    ret = sqlite3_get_table (handle, "SELECT CheckSpatialIndex()", &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CheckSpatialIndex ALL error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -28;
    }
    if (rows != 1 || columns != 1) {
        fprintf (stderr, "Unexpected rows/columns (check RTree ALL): r=%d c=%d\n", rows, columns);
        return -29;
    }
    value = results[1];
    if (strcmp ("0", value) != 0) {
        fprintf (stderr, "Unexpected result (check RTree ALL): %s\n", results[1]);
        return -30;
    }
    sqlite3_free_table (results);

/* recovering the Spatial Index all table */
    ret = sqlite3_get_table (handle, "SELECT RecoverSpatialIndex()", &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "RecoverSpatialIndex ALL error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -34;
    }
    if (rows != 1 || columns != 1) {
        fprintf (stderr, "Unexpected rows/columns (recover RTree ALL): r=%d c=%d\n", rows, columns);
        return -35;
    }
    value = results[1];
    if (strcmp ("1", value) != 0) {
        fprintf (stderr, "Unexpected result (recover RTree ALL): %s\n", results[1]);
        return -36;
    }
    sqlite3_free_table (results);

/* recovering the Spatial Index on table "B" */
    sql = sqlite3_mprintf("SELECT RecoverSpatialIndex(%Q, %Q)", table_b, geom);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "RecoverSpatialIndex TABLE-B error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -37;
    }
    if (rows != 1 || columns != 1) {
        fprintf (stderr, "Unexpected rows/columns (recover RTree TABLE-B): r=%d c=%d\n", rows, columns);
        return -38;
    }
    value = results[1];
    if (strcmp ("1", value) != 0) {
        fprintf (stderr, "Unexpected result (recover RTree TABLE-B): %s\n", results[1]);
        return -39;
    }
    sqlite3_free_table (results);

/* updating layer statistics for table "B" */
    sql = sqlite3_mprintf("SELECT UpdateLayerStatistics(%Q, %Q)", table_b, geom);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "UpdateLayerStatistics TABLE-B error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -40;
    }
    if (rows != 1 || columns != 1) {
        fprintf (stderr, "Unexpected rows/columns (statistics TABLE-B): r=%d c=%d\n", rows, columns);
        return -41;
    }
    value = results[1];
    if (strcmp ("1", value) != 0) {
        fprintf (stderr, "Unexpected result (statistics TABLE-B): %s\n", results[1]);
        return -42;
    }
    sqlite3_free_table (results);

/* updating layer statistics ALL */
    ret = sqlite3_get_table (handle, "SELECT UpdateLayerStatistics()", &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "UpdateLayerStatistics ALL error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -43;
    }
    if (rows != 1 || columns != 1) {
        fprintf (stderr, "Unexpected rows/columns (statistics ALL): r=%d c=%d\n", rows, columns);
        return -44;
    }
    value = results[1];
    if (strcmp ("1", value) != 0) {
        fprintf (stderr, "Unexpected result (statistics ALL): %s\n", results[1]);
        return -45;
    }
    sqlite3_free_table (results);

/* disabling the Spatial Index on table "B" */
    sql = sqlite3_mprintf("SELECT DisableSpatialIndex(%Q, %Q)", table_b, geom);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "DisableSpatialIndex TABLE-B error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -46;
    }
    if (rows != 1 || columns != 1) {
        fprintf (stderr, "Unexpected rows/columns (disable TABLE-B): r=%d c=%d\n", rows, columns);
        return -47;
    }
    value = results[1];
    if (strcmp ("1", value) != 0) {
        fprintf (stderr, "Unexpected result (disable TABLE-B): %s\n", results[1]);
        return -48;
    }
    sqlite3_free_table (results);

/* creating an MbrCache on table "B" */
    sql = sqlite3_mprintf("SELECT CreateMbrCache(%Q, %Q)", table_b, geom);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CreateMbrCache TABLE-B error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -49;
    }
    if (rows != 1 || columns != 1) {
        fprintf (stderr, "Unexpected rows/columns (disable TABLE-B): r=%d c=%d\n", rows, columns);
        return -50;
    }
    value = results[1];
    if (strcmp ("1", value) != 0) {
        fprintf (stderr, "Unexpected result (disable TABLE-B): %s\n", results[1]);
        return -51;
    }
    sqlite3_free_table (results);

/* checking the MbrCache on table "B" */
    sql = sqlite3_mprintf("SELECT Count(*) FROM %s WHERE ROWID IN ("
                          "SELECT ROWID FROM cache_%s_%s WHERE "
                          "mbr = FilterMbrIntersects(0, 0, 11, 11))", table_b, table_b, geom);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Test MbrCache error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -52;
    }
    if (rows != 1 || columns != 1) {
        fprintf (stderr, "Unexpected rows/columns (MbrCache): r=%d c=%d\n", rows, columns);
        return -53;
    }
    value = results[1];
    if (strcmp ("1", value) != 0) {
        fprintf (stderr, "Unexpected result (MbrCache): %s\n", results[1]);
        return -54;
    }
    sqlite3_free_table (results);

/* disabling the Spatial Index [MbrCache] on table "B" */
    sql = sqlite3_mprintf("SELECT DisableSpatialIndex(%Q, %Q)", table_b, geom);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "DisableSpatialIndex TABLE-B error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -55;
    }
    if (rows != 1 || columns != 1) {
        fprintf (stderr, "Unexpected rows/columns (disable TABLE-B): r=%d c=%d\n", rows, columns);
        return -56;
    }
    value = results[1];
    if (strcmp ("1", value) != 0) {
        fprintf (stderr, "Unexpected result (disable TABLE-B): %s\n", results[1]);
        return -57;
    }
    sqlite3_free_table (results);

/* discarding geometry from table "B" */
    sql = sqlite3_mprintf("SELECT DiscardGeometryColumn(%Q, %Q)", table_b, geom);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "DiscardGeometryColumn TABLE-B error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -58;
    }
    if (rows != 1 || columns != 1) {
        fprintf (stderr, "Unexpected rows/columns (discard TABLE-B): r=%d c=%d\n", rows, columns);
        return -59;
    }
    value = results[1];
    if (strcmp ("1", value) != 0) {
        fprintf (stderr, "Unexpected result (discard TABLE-B): %s\n", results[1]);
        return -60;
    }
    sqlite3_free_table (results);

#ifdef GEOS_ADVANCED		/* GEOS advanced features */

/* creating a topology */
    sql = sqlite3_mprintf("SELECT CreateTopologyTables(%Q, 4326, 'XY')", topology);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CreateTopologyTables error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -61;
    }
    if (rows != 1 || columns != 1) {
        fprintf (stderr, "Unexpected rows/columns (CreateTopologyTables): r=%d c=%d\n", rows, columns);
        return -62;
    }
    value = results[1];
    if (strcmp ("1", value) != 0) {
        fprintf (stderr, "Unexpected result (CreateTopologyTables): %s\n", results[1]);
        return -63;
    }
    sqlite3_free_table (results);

    sqlite3_free(table_a);
    sqlite3_free(table_b);
    sqlite3_free(pk);
    sqlite3_free(name);
    sqlite3_free(geom);
    sqlite3_free(topology);

#endif /* end GEOS advanced features */

/* inserting a CRS (very long auth) */
    auth = sqlite3_mprintf("authority_%s", suffix);
    sql = sqlite3_mprintf("INSERT INTO spatial_ref_sys (srid, auth_name, auth_srid, "
                          "ref_sys_name, proj4text, srtext) VALUES (NULL, %Q, 1122, "
                          "'silly CRS', 'silly proj def', 'silly wkt def')", auth);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "INSERT CRS error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -64;
    }

/* checking for validity (SRID from Auth) */
    sql = sqlite3_mprintf("SELECT SridFromAuthCrs(%Q, %d)", auth, 1122);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Test TABLE-A error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -65;
    }
    if (rows != 1 || columns != 1) {
        fprintf (stderr, "Unexpected rows/columns (SRID from Auth): r=%d c=%d\n", rows, columns);
        return -66;
    }
    value = results[1];
    if (strcmp ("325834", value) != 0) {
        fprintf (stderr, "Unexpected result (SRID from Auth): %s\n", results[1]);
        return -67;
    }
    sqlite3_free_table (results);

/* deleting the odd CRS */
    sql = "DELETE FROM spatial_ref_sys WHERE srid = 325834";
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "DELETE (CRS) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -68;
    }

    sqlite3_free(auth);

/* checking AsKML (1) */
    kml1 = sqlite3_mprintf("kml_name_%s", suffix);
    kml2 = sqlite3_mprintf("kml_description_%s", suffix);
    kmlvalue = sqlite3_mprintf("<Placemark><name>%s</name><description>%s</description>"
                               "<Point><coordinates>1,2</coordinates></Point></Placemark>",
                               kml1, kml2);
    sql = sqlite3_mprintf("SELECT AsKml(%Q, %Q, MakePoint(1, 2, 4326), 300)", kml1, kml2);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Test AsKML#1 error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -69;
    }
    if (rows != 1 || columns != 1) {
        fprintf (stderr, "Unexpected rows/columns (AsKML#1): r=%d c=%d\n", rows, columns);
        return -70;
    }
    value = results[1];
    if (strcmp (kmlvalue, value) != 0) {
        fprintf (stderr, "Unexpected result (AsKML#1): %s\n", results[1]);
        return -71;
    }
    sqlite3_free_table (results);
    sqlite3_free(kml1);
    sqlite3_free(kml2);
    sqlite3_free(kmlvalue);

/* checking AsKML (2) */
    kml1 = sqlite3_mprintf("%1.0f", 1E250);
    kml2 = sqlite3_mprintf("%2.0f", 2E250);
    kmlvalue = sqlite3_mprintf("<Placemark><name>%s.000000</name><description>%s.000000</description>"
                               "<Point><coordinates>1.11111111111111116,2.222222222222222321"
                               "</coordinates></Point></Placemark>",
                               kml1, kml2);
    sql = sqlite3_mprintf("SELECT AsKml(%s, %s, MakePoint("
                          "1.1111111111111111111111111111111111111111111111111111111111111111111111111, "
                          "2.2222222222222222222222222222222222222222222222222222222222222222222222222, "
                          "4326), 300)", kml1, kml2);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Test AsKML#2 error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -72;
    }
    if (rows != 1 || columns != 1) {
        fprintf (stderr, "Unexpected rows/columns (AsKML#2): r=%d c=%d\n", rows, columns);
        return -73;
    }
    value = results[1];
    if (strcmp (kmlvalue, value) != 0) {
        fprintf (stderr, "Unexpected result (AsKML#2): %s\n", results[1]);
        return -74;
    }
    sqlite3_free_table (results);
    sqlite3_free(kml1);
    sqlite3_free(kml2);
    sqlite3_free(kmlvalue);

/* checking AsGML (1) */
    kmlvalue = "<gml:Point srsName=\"EPSG:4326\"><gml:coordinates>"
               "1.11111111111111116,2.222222222222222321</gml:coordinates></gml:Point>";
    sql = "SELECT AsGml(MakePoint("
          "1.1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111, "
          "2.2222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222, "
          "4326), 300)";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Test AsGML#1 error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -75;
    }
    if (rows != 1 || columns != 1) {
        fprintf (stderr, "Unexpected rows/columns (AsGML#1): r=%d c=%d\n", rows, columns);
        return -76;
    }
    value = results[1];
    if (strcmp (kmlvalue, value) != 0) {
        fprintf (stderr, "Unexpected result (AsGML#1): %s\n", results[1]);
        return -77;
    }
    sqlite3_free_table (results);

/* checking AsGML (2) */
    kmlvalue = "<gml:Point srsName=\"EPSG:4326\"><gml:pos srsDimension=\"2\">"
               "1.11111111111111116 2.222222222222222321</gml:pos></gml:Point>";
    sql = "SELECT AsGml(3, MakePoint("
          "1.1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111, "
          "2.2222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222, "
          "4326), 300)";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Test AsGML#2 error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -78;
    }
    if (rows != 1 || columns != 1) {
        fprintf (stderr, "Unexpected rows/columns (AsGML#2): r=%d c=%d\n", rows, columns);
        return -79;
    }
    value = results[1];
    if (strcmp (kmlvalue, value) != 0) {
        fprintf (stderr, "Unexpected result (AsGML#2): %s\n", results[1]);
        return -80;
    }
    sqlite3_free_table (results);

    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (handle));
	return -81;
    }
    
    spatialite_cleanup();
    free(suffix);
    
    return 0;
}
