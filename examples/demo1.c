/* 

demo1.c

Author: Sandro Furieri a-furieri@lqt.it

This software is provided 'as-is', without any express or implied
warranty.  In no event will the author be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
these headers are required in order to support
SQLite/SpatiaLite
*/
#include <spatialite/sqlite3.h>
#include <spatialite/gaiageo.h>
#include <spatialite.h>

int
main (int argc, char *argv[])
{
    int ret;
    sqlite3 *handle;
    sqlite3_stmt *stmt;
    gaiaGeomCollPtr geom;
    char sql[256];
    int i;
    int ic;
    char **results;
    int n_rows;
    int n_columns;
    char *err_msg = NULL;
    int len;
    char *table_name;
    char **p_geotables = NULL;
    int n_geotables = 0;
    int row_no;
    const void *blob;
    int blob_size;
    int geom_type;
    double measure;


    if (argc != 2)
      {
	  fprintf (stderr, "usage: %s test_db_path\n", argv[0]);
	  return -1;
      }



    /* 
    VERY IMPORTANT: 
    you must initialize the SpatiaLite extension
    BEFORE attempting to perform any other SQLite call 
    */
    spatialite_init (0);


    /* showing the SQLite version */
    printf ("SQLite version: %s\n", sqlite3_libversion ());
    /* showing the SpatiaLite version */
    printf ("SpatiaLite version: %s\n", spatialite_version ());
    printf ("\n\n");



    /* 
    trying to connect the test DB: 
    - this demo is designed to connect the standard test database, but you can
    try this with any SQLite/SpatiaLite DB that you may have

    Please note that this will establish a READ ONLY connection 
    */
    ret = sqlite3_open_v2 (argv[1], &handle, SQLITE_OPEN_READONLY, NULL);
    if (ret != SQLITE_OK)
      {
	  printf ("cannot open '%s': %s\n", argv[1], sqlite3_errmsg (handle));
	  sqlite3_close (handle);
	  return -1;
      }



    /* 
    SQL query #1.
    We'll retrieve GEOMETRY tables from Spatial Metadata. We are assuming this
    query will return only few rows, so this time we'll use the
    sqlite3_get_table() interface.

    This interface is very simple to use, with the result set being returned as
    a rectangular array [n_rows by n_columns] allocated in a temporary memory
    storage. So this interface is well suited for small sized result sets,
    but performs badly when accessing a large sized result set.

    As a side effect of the API, each cell value is returned as text, and it
    is not possible to retrieve the true column types (e.g. INTEGER, FLOAT ...)
    */
    strcpy (sql,
	    "SELECT DISTINCT f_table_name FROM geometry_columns ORDER BY 1");
    ret = sqlite3_get_table (handle, sql, &results, &n_rows, &n_columns,
			     &err_msg);
    if (ret != SQLITE_OK)
      {
	  /* some error occurred */
	  printf ("query#1 SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  goto abort;
      }
    if (n_rows > 1)
      {
	  /* first row always contains column names and is meaningless in this
	     context */
	  n_geotables = n_rows;
	  /* allocating a dynamic pointer array to store geotable names */
	  p_geotables = malloc (sizeof (char *) * n_geotables);
	  for (i = 1; i <= n_rows; i++)
	    {
		/* now we'll fetch one row at each time (we have only one
		column) 

		This is is a simplified demo; but when writing a real
		application you always must check for NULL values.
		*/
		table_name = results[(i * n_columns) + 0];
		/* then store each geotable name into the dynamic pointer array */
		len = strlen (table_name);
		p_geotables[i - 1] = malloc (len + 1);
		strcpy (p_geotables[i - 1], table_name);
	    }
	  /* we can now free the table results */
	  sqlite3_free_table (results);
      }



    for (i = 0; i < n_geotables; i++)
      {
	  /* now we'll scan each geotable we've found in Spatial Metadata */
	  printf ("========= table '%s' ========================\n",
		  p_geotables[i]);



	  /*
	  SQL query #2 
	  we'll retrieve any column from the current geotable we are assuming
	  this query will return lots of rows, so we have to use
	  sqlite3_prepare_v2() interface

	  This interface is a more complex one, but is well suited in order to
	  access huge sized result sets and also provides true value.
	  */
	  sprintf (sql, "SELECT * FROM %s", p_geotables[i]);
	  ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		/* an error occurred */
		printf ("query#2 SQL error: %s\n", sqlite3_errmsg (handle));
		goto abort;
	    }

	  /* 
	  the sqlite3_prepare_v2() call simply parses the SQL statement,
	  checking for syntax validity, allocating internal structures etc
	  but no result set row is really yet available
	  */

	  /* we'll now save the number of columns within the result set */
	  n_columns = sqlite3_column_count (stmt);
	  row_no = 0;


	  while (1)
	    {
		/* this one is an infinite loop, intended to fetch any row */

		/* we are now trying to fetch the next available row */
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE)
		  {
		      /* there are no rows left to fetch, so we exit the loop */
		      break;
		  }
		if (ret == SQLITE_ROW)
		  {
		      /* ok, we've just fetched a valid row to process */
		      row_no++;
		      printf ("row #%d\n", row_no);


		      for (ic = 0; ic < n_columns; ic++)
			{
			    /* 
			    and now we'll fetch column values

			    for each column we'll then get:
			    - the column name
			    - a column value, that can be of type: SQLITE_NULL,
			    SQLITE_INTEGER, SQLITE_FLOAT, SQLITE_TEXT or
			    SQLITE_BLOB, accordingly to the internal DB storage
			    type
			    */
			    printf ("\t%-10s = ",
				    sqlite3_column_name (stmt, ic));
			    switch (sqlite3_column_type (stmt, ic))
			      {
			      case SQLITE_NULL:
				  printf ("NULL");
				  break;
			      case SQLITE_INTEGER:
				  printf ("%d", sqlite3_column_int (stmt, ic));
				  break;
			      case SQLITE_FLOAT:
				  printf ("%1.4lf",
					  sqlite3_column_double (stmt, ic));
				  break;
			      case SQLITE_TEXT:
				  printf ("'%s'",
					  sqlite3_column_text (stmt, ic));
				  break;
			      case SQLITE_BLOB:
				  blob = sqlite3_column_blob (stmt, ic);
				  blob_size = sqlite3_column_bytes (stmt, ic);

				  /* checking if this BLOB is a GEOMETRY */
				  geom =
				      gaiaFromSpatiaLiteBlobWkb (blob,
								 blob_size);
				  if (!geom)
				    {
					/* this is not a GEOMETRY */
					printf ("BLOB [%d bytes]", blob_size);
				    }
				  else
				    {
					geom_type = gaiaGeometryType (geom);
					if (geom_type == GAIA_UNKNOWN)
					    printf ("EMPTY or NULL GEOMETRY");
					else
					  {
					      char *geom_name;
					      if (geom_type == GAIA_POINT)
						  geom_name = "POINT";
					      if (geom_type == GAIA_LINESTRING)
						  geom_name = "LINESTRING";
					      if (geom_type == GAIA_POLYGON)
						  geom_name = "POLYGON";
					      if (geom_type == GAIA_MULTIPOINT)
						  geom_name = "MULTIPOINT";
					      if (geom_type ==
						  GAIA_MULTILINESTRING)
						  geom_name = "MULTILINESTRING";
					      if (geom_type ==
						  GAIA_MULTIPOLYGON)
						  geom_name = "MULTIPOLYGON";
					      if (geom_type ==
						  GAIA_GEOMETRYCOLLECTION)
						  geom_name =
						      "GEOMETRYCOLLECTION";
					      printf ("%s SRID=%d", geom_name,
						      geom->Srid);
					      if (geom_type == GAIA_LINESTRING
						  || geom_type ==
						  GAIA_MULTILINESTRING)
						{
						    gaiaGeomCollLength (geom,
									&measure);
						    printf (" length=%1.2lf",
							    measure);
						}
					      if (geom_type == GAIA_POLYGON ||
						  geom_type ==
						  GAIA_MULTIPOLYGON)
						{
						    gaiaGeomCollArea (geom,
								      &measure);
						    printf (" area=%1.2lf",
							    measure);
						}
					  }
					/* we have now to free the GEOMETRY */
					gaiaFreeGeomColl (geom);
				    }

				  break;
			      };
			    printf ("\n");
			}

		      if (row_no >= 5)
			{
			    /* we'll exit the loop after we've retrieved the
			       first 5 rows - this one is only a demo :-) */
			    break;
			}
		  }
		else
		  {
		      /* some unexpected error occurred */
		      printf ("sqlite3_step() error: %s\n",
			      sqlite3_errmsg (handle));
		      sqlite3_finalize (stmt);
		      goto abort;
		  }
	    }
	  /* we have now to finalize the query, releasing memory */
	  sqlite3_finalize (stmt);
	  printf ("\n\n");
      }



    /* disconnecting the test DB */
    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK)
      {
	  printf ("close() error: %s\n", sqlite3_errmsg (handle));
	  return -1;
      }
    printf ("\n\nsample successfully terminated\n");
    /* we have to free the dynamic pointer array used to store geotable names */
    for (i = 0; i < n_geotables; i++)
      {
	  /* freeing each tablename */
	  free (p_geotables[i]);
      }
    free (p_geotables);
    return 0;

  abort:
    sqlite3_close (handle);
    if (p_geotables)
      {
	  /* we have to free the dynamic pointer array used to store geotable
	     names */
	  for (i = 0; i < n_geotables; i++)
	    {
		/* freeing each tablename */
		free (p_geotables[i]);
	    }
	  free (p_geotables);
      }
    return -1;
}
