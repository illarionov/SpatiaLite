/*

 check_sql_stmt.c -- SpatiaLite Test Case

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
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <fnmatch.h>

#include "sqlite3.h"
#include "spatialite.h"

struct test_data
{
    char * test_case_name;
    char * database_name;
    char * sql_statement;
    int expected_rows;
    int expected_columns;
    char ** expected_results;
    int *expected_precision;
};

int do_one_case (const struct test_data *data)
{
    sqlite3 *db_handle = NULL;
    int ret;
    char *err_msg = NULL;
    int i;
    char **results;
    int rows;
    int columns;

    fprintf(stderr, "Test case: %s\n", data->test_case_name);
    // This hack checks if the name ends with _RO
    if (strncmp("_RO", data->database_name + strlen(data->database_name) - 3, strlen("_RO")) == 0) {
	fprintf(stderr, "opening read_only\n");
	ret = sqlite3_open_v2 (data->database_name, &db_handle, SQLITE_OPEN_READONLY, NULL);
    } else {
	ret = sqlite3_open_v2 (data->database_name, &db_handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    }
    if (ret != SQLITE_OK) {
      fprintf (stderr, "cannot open %s db: %s\n", data->database_name, sqlite3_errmsg (db_handle));
      sqlite3_close (db_handle);
      db_handle = NULL;
      return -1;
    }
    
    ret = sqlite3_exec (db_handle, "SELECT InitSpatialMetadata()", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(db_handle);
	return -2;
    }

    ret = sqlite3_get_table (db_handle, data->sql_statement, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "Error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -10;
    }
    if ((rows != data->expected_rows) || (columns != data->expected_columns)) {
	fprintf (stderr, "Unexpected error: bad result: %i/%i.\n", rows, columns);
	return  -11;
    }
    for (i = 0; i < (data->expected_rows + 1) * data->expected_columns; ++i) {
	if (results[i] != NULL && data->expected_precision[i] == 0) {
	    data->expected_precision[i] = strlen(results[i]);
	}
	if (results[i] == NULL) {
	    if (strcmp("(NULL)", data->expected_results[i]) == 0) {
		/* we expected this */
		continue;
	    } else {
		fprintf (stderr, "Null value at %i.\n", i);
		fprintf (stderr, "Expected value was: %s\n", data->expected_results[i]);
		return  -12;
	    }
	} else if (strlen(results[i]) == 0) {
	    fprintf (stderr, "zero length result at %i\n", i);
	    fprintf (stderr, "Expected value was    : %s|\n", data->expected_results[i]);
	    return -13;
	} else if (strncmp(results[i], data->expected_results[i], data->expected_precision[i]) != 0) {
	    fprintf (stderr, "Unexpected value at %i: %s|\n", i, results[i]);
	    fprintf (stderr, "Expected value was    : %s|\n", data->expected_results[i]);
	    return  -14;
	}
    }
    sqlite3_free_table (results);

    sqlite3_close (db_handle);
    
    return 0;
}

int get_clean_line(FILE *f, char ** line)
{
    size_t len = 0;
    ssize_t num_read = 0;
    size_t end = 0;
    char *tmp_line = NULL;
    num_read = getline(&(tmp_line), &len, f);
    if (num_read < 1) {
	fprintf(stderr, "failed to read at %li: %zi\n", ftell(f), num_read);
	return -1;
    }
    /* trim the trailing new line and any comments */
    for (end = 0; end < num_read; ++end) {
	if (*(tmp_line + end) == '\n')
	    break;
	if (*(tmp_line + end) == '#')
	    break;
    }
    /* trim any trailing spaces */
    while (end > 0) {
	if (*(tmp_line + end -1) != ' ') {
	    break;
	}
	*(tmp_line + end -1) = '\0';
	end--;
    }
    *line = malloc(end + 1);
    memcpy(*line, tmp_line, end);
    (*line)[end] = '\0';
    free(tmp_line);
    return 0;
}

void handle_precision(char *expected_result, int *precision)
{
    int i;
    int result_len = strlen(expected_result);
    *precision = 0;
    for (i = result_len - 1; i >= 0; --i) {
	if (expected_result[i] == ':') {
	    expected_result[i] = '\0';
	    *precision = atoi(&(expected_result[i + 1]));
	    break;
	}
    }
}

struct test_data *read_one_case(const char *filepath)
{
    int num_results;
    int i;
    size_t len;
    ssize_t num_read;
    char *tmp;
    FILE *f;

    f = fopen(filepath, "r");
    
    struct test_data* data;
    data = malloc(sizeof(struct test_data));
    get_clean_line(f, &(data->test_case_name));
    get_clean_line(f, &(data->database_name));
    get_clean_line(f, &(data->sql_statement));
    get_clean_line(f, &(tmp));
    data->expected_rows = atoi(tmp);
    free(tmp);
    get_clean_line(f, &(tmp));
    data->expected_columns = atoi(tmp);
    free(tmp);
    num_results = (data->expected_rows + 1) * data->expected_columns;
    data->expected_results = malloc(num_results * sizeof(char*));
    data->expected_precision = malloc(num_results * sizeof(int));
    for (i = 0; i < num_results; ++i) {
	get_clean_line(f, &(data->expected_results[i]));
	handle_precision(data->expected_results[i], &(data->expected_precision[i]));
    }
    fclose(f);
    return data;
}

void cleanup_test_data(struct test_data *data)
{
    int i;
    int num_results = (data->expected_rows + 1) * (data->expected_columns);

    for (i = 0; i < num_results; ++i) {
	free(data->expected_results[i]);
    }
    free(data->expected_results);
    free(data->expected_precision);
    free(data->test_case_name);
    free(data->database_name);
    free(data->sql_statement);
    free(data);
}

int test_case_filter(const struct dirent *entry)
{
    int ret = 0;
    return (fnmatch("*.testcase", entry->d_name, FNM_PERIOD) == 0);
}

int main (int argc, char *argv[])
{
    struct dirent **namelist;
    int n;
    int i;
    int result = 0;

    spatialite_init (0);

    n = scandir("sql_stmt_tests", &namelist, test_case_filter, alphasort);
    if (n < 0) {
	perror("scandir");
	return -1;
    } else {
	for (i = 0; i < n; ++i) {
	    struct test_data *data;
	    char *path;
	    if (asprintf(&path, "sql_stmt_tests/%s", namelist[i]->d_name) < 0) {
		result = -1;
		break;
	    }
	    data = read_one_case(path);
	    free(path);
	    
	    result = do_one_case(data);
	    
	    cleanup_test_data(data);
	    if (result != 0) {
		break;
	    }
	    free(namelist[i]);
	}
	free(namelist);
    }

    spatialite_cleanup();
    sqlite3_reset_auto_extension();

    return result;
}
