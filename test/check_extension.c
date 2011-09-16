/*

 check_extension.c -- SpatiaLite Test Case

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
    sqlite3 *db_handle = NULL;
    char *sql_statement;
    int ret;
    char *err_msg = NULL;
    int i;
    char **results;
    int rows;
    int columns;

    ret = sqlite3_open_v2 (":memory:", &db_handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "cannot open in-memory db: %s\n", sqlite3_errmsg (db_handle));
      sqlite3_close (db_handle);
      db_handle = NULL;
      return -1;
    }
    
    sqlite3_enable_load_extension (db_handle, 1);
    
    asprintf(&sql_statement, "SELECT load_extension('libspatialite.so')");
    ret = sqlite3_exec (db_handle, sql_statement, NULL, NULL, &err_msg);
    free(sql_statement);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "load_extension() error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -2;
    }

    asprintf(&sql_statement, "SELECT spatialite_version()");
    ret = sqlite3_get_table (db_handle, sql_statement, &results, &rows, &columns, &err_msg);
    free(sql_statement);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "Error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -10;
    }
    if ((rows != 1) || (columns != 1)) {
	fprintf (stderr, "Unexpected error: spatialite_version() bad result: %i/%i.\n", rows, columns);
	return  -11;
    }
    if (strcmp(results[1], VERSION) != 0) {
	fprintf (stderr, "Unexpected error: spatialite_version() bad result: %s.\n", results[1]);
	return  -12;
    }
    sqlite3_free_table (results);

    asprintf(&sql_statement, "SELECT geos_version()");
    ret = sqlite3_get_table (db_handle, sql_statement, &results, &rows, &columns, &err_msg);
    free(sql_statement);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "Error2: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -13;
    }
    if ((rows != 1) || (columns != 1)) {
	fprintf (stderr, "Unexpected error: geos_version() bad result: %i/%i.\n", rows, columns);
	return  -14;
    }
    /* we tolerate any string here, because versions always change */
    if (strlen(results[1]) == 0) {
	fprintf (stderr, "Unexpected error: geos_version() bad result.\n");
	return  -15;
    }
    sqlite3_free_table (results);

    
    asprintf(&sql_statement, "SELECT proj4_version()");
    ret = sqlite3_get_table (db_handle, sql_statement, &results, &rows, &columns, &err_msg);
    free(sql_statement);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "Error3: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -14;
    }
    if ((rows != 1) || (columns != 1)) {
	fprintf (stderr, "Unexpected error: proj4_version() bad result: %i/%i.\n", rows, columns);
	return  -14;
    }
    /* we tolerate any string here, because versions always change */
    if (strlen(results[1]) == 0) {
	fprintf (stderr, "Unexpected error: proj4_version() bad result.\n");
	return  -15;
    }
    sqlite3_free_table (results);
 
    sqlite3_close (db_handle);
    
    sqlite3_reset_auto_extension();
    
    return 0;
}
