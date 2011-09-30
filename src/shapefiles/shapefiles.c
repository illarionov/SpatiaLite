/*

 shapefiles.c -- implements shapefile support [import - export]

 version 2.3, 2008 October 13

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

#if defined(_WIN32) && !defined(__MINGW32__)
/* MSVC strictly requires this include [off_t] */
#include <sys/types.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef SPATIALITE_AMALGAMATION
#include <spatialite/sqlite3.h>
#else
#include <sqlite3.h>
#endif

#include <spatialite/gaiaaux.h>
#include <spatialite/gaiageo.h>
#include <spatialite.h>

#include <freexl.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#define strcasecmp	_stricmp
#endif

struct dupl_column
{
/* a column value in a duplicated row */
    int pos;
    char *name;
    int type;
    sqlite3_int64 int_value;
    double dbl_value;
    const char *txt_value;
    const void *blob;
    int size;
    int query_pos;
    struct dupl_column *next;
};

struct dupl_row
{
/* a duplicated row with column values */
    int count;
    struct dupl_column *first;
    struct dupl_column *last;
    const char *table;
};

static void
clean_dupl_row (struct dupl_row *str)
{
/* destroying a duplicated row struct */
    struct dupl_column *p;
    struct dupl_column *pn;
    p = str->first;
    while (p)
      {
	  pn = p->next;
	  free (p->name);
	  free (p);
	  p = pn;
      }
}

static void
add_to_dupl_row (struct dupl_row *str, const char *name)
{
/* adding a column to the duplicated row struct */
    int len;
    struct dupl_column *p = malloc (sizeof (struct dupl_column));
    p->pos = str->count;
    len = strlen (name);
    p->name = malloc (len + 1);
    strcpy (p->name, name);
    str->count++;
    p->type = SQLITE_NULL;
    p->next = NULL;
    if (str->first == NULL)
	str->first = p;
    if (str->last)
	str->last->next = p;
    str->last = p;
}

static void
set_int_value (struct dupl_row *str, int pos, sqlite3_int64 value)
{
/* setting up an integer value */
    struct dupl_column *p = str->first;
    while (p)
      {
	  if (p->pos == pos)
	    {
		p->type = SQLITE_INTEGER;
		p->int_value = value;
		return;
	    }
	  p = p->next;
      }
}

static void
set_double_value (struct dupl_row *str, int pos, double value)
{
/* setting up a double value */
    struct dupl_column *p = str->first;
    while (p)
      {
	  if (p->pos == pos)
	    {
		p->type = SQLITE_FLOAT;
		p->dbl_value = value;
		return;
	    }
	  p = p->next;
      }
}

static void
set_text_value (struct dupl_row *str, int pos, const char *value)
{
/* setting up a text value */
    struct dupl_column *p = str->first;
    while (p)
      {
	  if (p->pos == pos)
	    {
		p->type = SQLITE_TEXT;
		p->txt_value = value;
		return;
	    }
	  p = p->next;
      }
}

static void
set_blob_value (struct dupl_row *str, int pos, const void *blob, int size)
{
/* setting up a blob value */
    struct dupl_column *p = str->first;
    while (p)
      {
	  if (p->pos == pos)
	    {
		p->type = SQLITE_BLOB;
		p->blob = blob;
		p->size = size;
		return;
	    }
	  p = p->next;
      }
}

static void
set_null_value (struct dupl_row *str, int pos)
{
/* setting up a NULL value */
    struct dupl_column *p = str->first;
    while (p)
      {
	  if (p->pos == pos)
	    {
		p->type = SQLITE_NULL;
		return;
	    }
	  p = p->next;
      }
}

static void
reset_query_pos (struct dupl_row *str)
{
/* resetting QueryPos for BLOBs */
    struct dupl_column *p = str->first;
    while (p)
      {
	  p->query_pos = -1;
	  p = p->next;
      }
}

static int
check_dupl_blob2 (struct dupl_column *ptr, const void *blob, int size)
{
/* checking a BLOB value */
    if (ptr->type != SQLITE_BLOB)
	return 0;
    if (ptr->size != size)
	return 0;
    if (memcmp (ptr->blob, blob, size) != 0)
	return 0;
    return 1;
}

static int
check_dupl_blob (struct dupl_row *str, int pos, const void *blob, int size)
{
/* checking a BLOB value */
    struct dupl_column *p = str->first;
    while (p)
      {
	  if (p->query_pos == pos)
	    {
		return check_dupl_blob2 (p, blob, size);
	    }
	  p = p->next;
      }
    return 0;
}

static gaiaDbfFieldPtr
getDbfField (gaiaDbfListPtr list, char *name)
{
/* find a DBF attribute by name */
    gaiaDbfFieldPtr fld = list->First;
    while (fld)
      {
	  if (strcasecmp (fld->Name, name) == 0)
	      return fld;
	  fld = fld->Next;
      }
    return NULL;
}

#ifndef OMIT_ICONV		/* ICONV enabled: supporting SHP */

SPATIALITE_DECLARE int
load_shapefile (sqlite3 * sqlite, char *shp_path, char *table, char *charset,
		int srid, char *column, int coerce2d, int compressed,
		int verbose, int spatial_index, int *rows, char *err_msg)
{
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *errMsg = NULL;
    char sql[65536];
    char dummyName[4096];
    int already_exists = 0;
    int metadata = 0;
    int sqlError = 0;
    gaiaShapefilePtr shp = NULL;
    gaiaDbfFieldPtr dbf_field;
    int cnt;
    int col_cnt;
    int seed;
    int len;
    int dup;
    int idup;
    int current_row;
    char **col_name = NULL;
    unsigned char *blob;
    int blob_size;
    char *geom_type;
    char *txt_dims;
    char *geo_column = column;
    if (!geo_column)
	geo_column = "Geometry";
/* checking if TABLE already exists */
    sprintf (sql,
	     "SELECT name FROM sqlite_master WHERE type = 'table' AND name LIKE '%s'",
	     table);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  if (!err_msg)
	      fprintf (stderr, "load shapefile error: <%s>\n",
		       sqlite3_errmsg (sqlite));
	  else
	      sprintf (err_msg, "load shapefile error: <%s>\n",
		       sqlite3_errmsg (sqlite));
	  return 0;
      }
    while (1)
      {
	  /* scrolling the result set */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	      already_exists = 1;
	  else
	    {
		fprintf (stderr, "load shapefile error: <%s>\n",
			 sqlite3_errmsg (sqlite));
		break;
	    }
      }
    sqlite3_finalize (stmt);
    if (already_exists)
      {
	  if (!err_msg)
	      fprintf (stderr,
		       "load shapefile error: table '%s' already exists\n",
		       table);
	  else
	      sprintf (err_msg,
		       "load shapefile error: table '%s' already exists\n",
		       table);
	  return 0;
      }
/* checking if MetaData GEOMETRY_COLUMNS exists */
    strcpy (sql,
	    "SELECT name FROM sqlite_master WHERE type = 'table' AND name = 'geometry_columns'");
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  if (!err_msg)
	      fprintf (stderr, "load shapefile error: <%s>\n",
		       sqlite3_errmsg (sqlite));
	  else
	      sprintf (err_msg, "load shapefile error: <%s>\n",
		       sqlite3_errmsg (sqlite));
	  return 0;
      }
    while (1)
      {
	  /* scrolling the result set */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	      metadata = 1;
	  else
	    {
		fprintf (stderr, "load shapefile error: <%s>\n",
			 sqlite3_errmsg (sqlite));
		break;
	    }
      }
    sqlite3_finalize (stmt);
    shp = gaiaAllocShapefile ();
    gaiaOpenShpRead (shp, shp_path, charset, "UTF-8");
    if (!(shp->Valid))
      {
	  if (!err_msg)
	    {
		fprintf (stderr,
			 "load shapefile error: cannot open shapefile '%s'\n",
			 shp_path);
		if (shp->LastError)
		    fprintf (stderr, "\tcause: %s\n", shp->LastError);
	    }
	  else
	    {
		char extra[512];
		*extra = '\0';
		if (shp->LastError)
		    sprintf (extra, "\n\tcause: %s\n", shp->LastError);
		sprintf (err_msg,
			 "load shapefile error: cannot open shapefile '%s'%s",
			 shp_path, extra);
	    }
	  gaiaFreeShapefile (shp);
	  return 0;
      }
/* checking for duplicate / illegal column names and antialising them */
    col_cnt = 0;
    dbf_field = shp->Dbf->First;
    while (dbf_field)
      {
	  /* counting DBF fields */
	  col_cnt++;
	  dbf_field = dbf_field->Next;
      }
    col_name = malloc (sizeof (char *) * col_cnt);
    cnt = 0;
    seed = 0;
    dbf_field = shp->Dbf->First;
    while (dbf_field)
      {
	  /* preparing column names */
	  strcpy (dummyName, dbf_field->Name);
	  dup = 0;
	  for (idup = 0; idup < cnt; idup++)
	    {
		if (strcasecmp (dummyName, *(col_name + idup)) == 0)
		    dup = 1;
	    }
	  if (strcasecmp (dummyName, "PK_UID") == 0)
	      dup = 1;
	  if (strcasecmp (dummyName, geo_column) == 0)
	      dup = 1;
	  if (dup)
	      sprintf (dummyName, "COL_%d", seed++);
	  len = strlen (dummyName);
	  *(col_name + cnt) = malloc (len + 1);
	  strcpy (*(col_name + cnt), dummyName);
	  cnt++;
	  dbf_field = dbf_field->Next;
      }
    if (verbose)
	fprintf (stderr,
		 "========\nLoading shapefile at '%s' into SQLite table '%s'\n",
		 shp_path, table);
/* starting a transaction */
    if (verbose)
	fprintf (stderr, "\nBEGIN;\n");
    ret = sqlite3_exec (sqlite, "BEGIN", NULL, 0, &errMsg);
    if (ret != SQLITE_OK)
      {
	  if (!err_msg)
	      fprintf (stderr, "load shapefile error: <%s>\n", errMsg);
	  else
	      sprintf (err_msg, "load shapefile error: <%s>\n", errMsg);
	  sqlite3_free (errMsg);
	  sqlError = 1;
	  goto clean_up;
      }
/* creating the Table */
    sprintf (sql, "CREATE TABLE %s", table);
    strcat (sql, " (\nPK_UID INTEGER PRIMARY KEY AUTOINCREMENT");
    cnt = 0;
    dbf_field = shp->Dbf->First;
    while (dbf_field)
      {
	  strcat (sql, ",\n\"");
	  strcat (sql, *(col_name + cnt));
	  cnt++;
	  switch (dbf_field->Type)
	    {
	    case 'C':
		strcat (sql, "\" TEXT");
		break;
	    case 'N':
		fflush (stderr);
		if (dbf_field->Decimals)
		    strcat (sql, "\" DOUBLE");
		else
		  {
		      if (dbf_field->Length <= 18)
			  strcat (sql, "\" INTEGER");
		      else
			  strcat (sql, "\" DOUBLE");
		  }
		break;
	    case 'D':
		strcat (sql, "\" DOUBLE");
		break;
	    case 'F':
		strcat (sql, "\" DOUBLE");
		break;
	    case 'L':
		strcat (sql, "\" INTEGER");
		break;
	    };
	  dbf_field = dbf_field->Next;
      }
    if (metadata)
	strcat (sql, ")");
    else
      {
	  strcat (sql, ",\n");
	  strcat (sql, geo_column);
	  strcat (sql, " BLOB)");
      }
    if (verbose)
	fprintf (stderr, "%s;\n", sql);
    ret = sqlite3_exec (sqlite, sql, NULL, 0, &errMsg);
    if (ret != SQLITE_OK)
      {
	  if (!err_msg)
	      fprintf (stderr, "load shapefile error: <%s>\n", errMsg);
	  else
	      sprintf (err_msg, "load shapefile error: <%s>\n", errMsg);
	  sqlite3_free (errMsg);
	  sqlError = 1;
	  goto clean_up;
      }
    if (metadata)
      {
	  /* creating Geometry column */
	  switch (shp->Shape)
	    {
	    case GAIA_SHP_POINT:
	    case GAIA_SHP_POINTM:
	    case GAIA_SHP_POINTZ:
		geom_type = "POINT";
		break;
	    case GAIA_SHP_MULTIPOINT:
	    case GAIA_SHP_MULTIPOINTM:
	    case GAIA_SHP_MULTIPOINTZ:
		geom_type = "MULTIPOINT";
		break;
	    case GAIA_SHP_POLYLINE:
	    case GAIA_SHP_POLYLINEM:
	    case GAIA_SHP_POLYLINEZ:
		gaiaShpAnalyze (shp);
		if (shp->EffectiveType == GAIA_LINESTRING)
		    geom_type = "LINESTRING";
		else
		    geom_type = "MULTILINESTRING";
		break;
	    case GAIA_SHP_POLYGON:
	    case GAIA_SHP_POLYGONM:
	    case GAIA_SHP_POLYGONZ:
		gaiaShpAnalyze (shp);
		if (shp->EffectiveType == GAIA_POLYGON)
		    geom_type = "POLYGON";
		else
		    geom_type = "MULTIPOLYGON";
		break;
	    };
	  if (coerce2d)
	      shp->EffectiveDims = GAIA_XY;
	  switch (shp->EffectiveDims)
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
	  sprintf (sql, "SELECT AddGeometryColumn('%s', '%s', %d, '%s', '%s')",
		   table, geo_column, srid, geom_type, txt_dims);
	  if (verbose)
	      fprintf (stderr, "%s;\n", sql);
	  ret = sqlite3_exec (sqlite, sql, NULL, 0, &errMsg);
	  if (ret != SQLITE_OK)
	    {
		if (!err_msg)
		    fprintf (stderr, "load shapefile error: <%s>\n", errMsg);
		else
		    sprintf (err_msg, "load shapefile error: <%s>\n", errMsg);
		sqlite3_free (errMsg);
		sqlError = 1;
		goto clean_up;
	    }
	  if (spatial_index)
	    {
		/* creating the Spatial Index */
		sprintf (sql, "SELECT CreateSpatialIndex('%s', '%s')",
			 table, geo_column);
		ret = sqlite3_exec (sqlite, sql, NULL, 0, &errMsg);
		if (ret != SQLITE_OK)
		  {
		      if (!err_msg)
			  fprintf (stderr, "load shapefile error: <%s>\n",
				   errMsg);
		      else
			  sprintf (err_msg, "load shapefile error: <%s>\n",
				   errMsg);
		      sqlite3_free (errMsg);
		      sqlError = 1;
		      goto clean_up;
		  }
	    }
      }
    else
      {
	  /* no Metadata */
	  if (shp->Shape == GAIA_SHP_POLYLINE
	      || shp->Shape == GAIA_SHP_POLYLINEM
	      || shp->Shape == GAIA_SHP_POLYLINEZ
	      || shp->Shape == GAIA_SHP_POLYGON
	      || shp->Shape == GAIA_SHP_POLYGONM
	      || shp->Shape == GAIA_SHP_POLYGONZ)
	    {
		/* 
		/ fixing anyway the Geometry type for 
		/ LINESTRING/MULTILINESTRING or 
		/ POLYGON/MULTIPOLYGON
		*/
		gaiaShpAnalyze (shp);
	    }
      }
    /* preparing the INSERT INTO parametrerized statement */
    sprintf (sql, "INSERT INTO %s (PK_UID,", table);
    cnt = 0;
    dbf_field = shp->Dbf->First;
    while (dbf_field)
      {
	  /* columns corresponding to some DBF attribute */
	  strcat (sql, "\"");
	  strcat (sql, *(col_name + cnt++));
	  strcat (sql, "\" ,");
	  dbf_field = dbf_field->Next;
      }
    strcat (sql, geo_column);	/* the GEOMETRY column */
    strcat (sql, ")\nVALUES (?");
    dbf_field = shp->Dbf->First;
    while (dbf_field)
      {
	  /* column values */
	  strcat (sql, ", ?");
	  dbf_field = dbf_field->Next;
      }
    strcat (sql, ", ?)");	/* the GEOMETRY column */
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  if (!err_msg)
	      fprintf (stderr, "load shapefile error: <%s>\n",
		       sqlite3_errmsg (sqlite));
	  else
	      sprintf (err_msg, "load shapefile error: <%s>\n",
		       sqlite3_errmsg (sqlite));
	  sqlError = 1;
	  goto clean_up;
      }
    current_row = 0;
    while (1)
      {
	  /* inserting rows from shapefile */
	  ret = gaiaReadShpEntity (shp, current_row, srid);
	  if (!ret)
	    {
		if (!(shp->LastError))	/* normal SHP EOF */
		    break;
		if (!err_msg)
		    fprintf (stderr, "%s\n", shp->LastError);
		else
		    sprintf (err_msg, "%s\n", shp->LastError);
		sqlError = 1;
		sqlite3_finalize (stmt);
		goto clean_up;
	    }
	  current_row++;
	  /* binding query params */
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_int (stmt, 1, current_row);
	  cnt = 0;
	  dbf_field = shp->Dbf->First;
	  while (dbf_field)
	    {
		/* column values */
		if (!(dbf_field->Value))
		    sqlite3_bind_null (stmt, cnt + 2);
		else
		  {
		      switch (dbf_field->Value->Type)
			{
			case GAIA_INT_VALUE:
			    sqlite3_bind_int64 (stmt, cnt + 2,
						dbf_field->Value->IntValue);
			    break;
			case GAIA_DOUBLE_VALUE:
			    sqlite3_bind_double (stmt, cnt + 2,
						 dbf_field->Value->DblValue);
			    break;
			case GAIA_TEXT_VALUE:
			    sqlite3_bind_text (stmt, cnt + 2,
					       dbf_field->Value->TxtValue,
					       strlen (dbf_field->Value->
						       TxtValue),
					       SQLITE_STATIC);
			    break;
			default:
			    sqlite3_bind_null (stmt, cnt + 2);
			    break;
			}
		  }
		cnt++;
		dbf_field = dbf_field->Next;
	    }
	  if (shp->Dbf->Geometry)
	    {
		if (compressed)
		    gaiaToCompressedBlobWkb (shp->Dbf->Geometry, &blob,
					     &blob_size);
		else
		    gaiaToSpatiaLiteBlobWkb (shp->Dbf->Geometry, &blob,
					     &blob_size);
		sqlite3_bind_blob (stmt, cnt + 2, blob, blob_size, free);
	    }
	  else
	    {
		/* handling a NULL-Geometry */
		sqlite3_bind_null (stmt, cnt + 2);
	    }
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		if (!err_msg)
		    fprintf (stderr, "load shapefile error: <%s>\n",
			     sqlite3_errmsg (sqlite));
		else
		    sprintf (err_msg, "load shapefile error: <%s>\n",
			     sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		sqlError = 1;
		goto clean_up;
	    }
      }
    sqlite3_finalize (stmt);
  clean_up:
    gaiaFreeShapefile (shp);
    if (col_name)
      {
	  /* releasing memory allocation for column names */
	  for (cnt = 0; cnt < col_cnt; cnt++)
	      free (*(col_name + cnt));
	  free (col_name);
      }
    if (sqlError)
      {
	  /* some error occurred - ROLLBACK */
	  if (verbose)
	      fprintf (stderr, "ROLLBACK;\n");
	  ret = sqlite3_exec (sqlite, "ROLLBACK", NULL, 0, &errMsg);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "load shapefile error: <%s>\n", errMsg);
		sqlite3_free (errMsg);
	    }
	  if (rows)
	      *rows = current_row;
	  return 0;
      }
    else
      {
	  /* ok - confirming pending transaction - COMMIT */
	  if (verbose)
	      fprintf (stderr, "COMMIT;\n");
	  ret = sqlite3_exec (sqlite, "COMMIT", NULL, 0, &errMsg);
	  if (ret != SQLITE_OK)
	    {
		if (!err_msg)
		    fprintf (stderr, "load shapefile error: <%s>\n", errMsg);
		else
		    sprintf (err_msg, "load shapefile error: <%s>\n", errMsg);
		sqlite3_free (errMsg);
		return 0;
	    }
	  if (rows)
	      *rows = current_row;
	  if (verbose)
	      fprintf (stderr,
		       "\nInserted %d rows into '%s' from SHAPEFILE\n========\n",
		       current_row, table);
	  if (err_msg)
	      sprintf (err_msg, "Inserted %d rows into '%s' from SHAPEFILE",
		       current_row, table);
	  return 1;
      }
}

#endif				/* end ICONV (SHP) */

static void
output_prj_file (sqlite3 * sqlite, char *path, char *table, char *column)
{
/* exporting [if possible] a .PRJ file */
    char **results;
    int rows;
    int columns;
    int i;
    char *errMsg = NULL;
    int srid = -1;
    char sql[1024];
    char sql2[1024];
    int ret;
    int rs_srid = 0;
    int rs_srs_wkt = 0;
    const char *name;
    char srsWkt[8192];
    char dummy[8192];
    FILE *out;

/* step I: retrieving the SRID */
    sprintf (sql,
	     "SELECT srid FROM geometry_columns WHERE f_table_name = '%s' AND f_geometry_column = '%s'",
	     table, column);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "dump shapefile MetaData error: <%s>\n", errMsg);
	  sqlite3_free (errMsg);
	  return;
      }
    for (i = 1; i <= rows; i++)
      {
	  srid = atoi (results[(i * columns) + 0]);
      }
    sqlite3_free_table (results);
    if (srid < 0)
      {
	  /* srid still undefined, so we'll read VIEWS_GEOMETRY_COLUMNS */
	  strcpy (sql, "SELECT srid FROM views_geometry_columns ");
	  strcat (sql,
		  "JOIN geometry_columns USING (f_table_name, f_geometry_column) ");
	  sprintf (sql2, "WHERE view_name = '%s' AND view_geometry = '%s'",
		   table, column);
	  strcat (sql, sql2);
	  ret =
	      sqlite3_get_table (sqlite, sql, &results, &rows, &columns,
				 &errMsg);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "dump shapefile MetaData error: <%s>\n",
			 errMsg);
		sqlite3_free (errMsg);
		return;
	    }
	  for (i = 1; i <= rows; i++)
	    {
		srid = atoi (results[(i * columns) + 0]);
	    }
	  sqlite3_free_table (results);
      }
    if (srid < 0)
	return;

/* step II: checking if the SRS_WKT column actually exists */
    ret =
	sqlite3_get_table (sqlite, "PRAGMA table_info(spatial_ref_sys)",
			   &results, &rows, &columns, &errMsg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "dump shapefile MetaData error: <%s>\n", errMsg);
	  sqlite3_free (errMsg);
	  return;
      }
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		name = results[(i * columns) + 1];
		if (strcasecmp (name, "srid") == 0)
		    rs_srid = 1;
		if (strcasecmp (name, "auth_name") == 0)
		    rs_srs_wkt = 1;
	    }
      }
    sqlite3_free_table (results);
    if (rs_srid == 0 || rs_srs_wkt == 0)
	return;

/* step III: fetching WKT SRS */
    *srsWkt = '\0';
    sprintf (sql,
	     "SELECT srs_wkt FROM spatial_ref_sys WHERE srid = %d AND srs_wkt IS NOT NULL",
	     srid);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "dump shapefile MetaData error: <%s>\n", errMsg);
	  sqlite3_free (errMsg);
	  return;
      }
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		strcpy (srsWkt, results[(i * columns) + 0]);
	    }
      }
    sqlite3_free_table (results);
    if (strlen (srsWkt) == 0)
	return;

/* step IV: generating the .PRJ file */
    sprintf (dummy, "%s.prj", path);
    out = fopen (dummy, "wb");
    if (!out)
	goto no_file;
    fprintf (out, "%s\r\n", srsWkt);
    fclose (out);
  no_file:
    return;
}

static void
shp_double_quoted_sql (char *buf)
{
/* well-formatting a string to be used as an SQL name */
    char tmp[1024];
    char *in = tmp;
    char *out = buf;
    strcpy (tmp, buf);
    *out++ = '"';
    while (*in != '\0')
      {
	  if (*in == '"')
	      *out++ = '"';
	  *out++ = *in++;
      }
    *out++ = '"';
    *out = '\0';
}

#ifndef OMIT_ICONV	/* ICONV enabled: supporting SHAPEFILE and DBF */

SPATIALITE_DECLARE int
dump_shapefile (sqlite3 * sqlite, char *table, char *column, char *shp_path,
		char *charset, char *geom_type, int verbose, int *xrows,
		char *err_msg)
{
/* SHAPEFILE dump */
    char sql[1024];
    char dummy[1024];
    int shape = -1;
    int len;
    int ret;
    char *errMsg = NULL;
    sqlite3_stmt *stmt;
    int row1 = 0;
    int n_cols = 0;
    int offset = 0;
    int i;
    int rows = 0;
    int type;
    const void *blob_value;
    gaiaShapefilePtr shp = NULL;
    gaiaDbfListPtr dbf_export_list = NULL;
    gaiaDbfListPtr dbf_list = NULL;
    gaiaDbfListPtr dbf_write;
    gaiaDbfFieldPtr dbf_field;
    int *max_length = NULL;
    int *sql_type = NULL;
    if (geom_type)
      {
	  /* normalizing required geometry type */
	  if (strcasecmp ((char *) geom_type, "POINT") == 0)
	      shape = GAIA_POINT;
	  if (strcasecmp ((char *) geom_type, "LINESTRING") == 0)
	      shape = GAIA_LINESTRING;
	  if (strcasecmp ((char *) geom_type, "POLYGON") == 0)
	      shape = GAIA_POLYGON;
	  if (strcasecmp ((char *) geom_type, "MULTIPOINT") == 0)
	      shape = GAIA_POINT;
      }
    if (shape < 0)
      {
	  /* preparing SQL statement [no type was explicitly required, so we'll read GEOMETRY_COLUMNS */
	  char **results;
	  int rows;
	  int columns;
	  int i;
	  char metatype[256];
	  char metadims[256];
	  sprintf (sql,
		   "SELECT type, coord_dimension FROM geometry_columns WHERE f_table_name = '%s' AND f_geometry_column = '%s'",
		   table, column);
	  ret =
	      sqlite3_get_table (sqlite, sql, &results, &rows, &columns,
				 &errMsg);
	  if (ret != SQLITE_OK)
	    {
		if (!err_msg)
		    fprintf (stderr, "dump shapefile MetaData error: <%s>\n",
			     errMsg);
		else
		    sprintf (err_msg, "dump shapefile MetaData error: <%s>\n",
			     errMsg);
		sqlite3_free (errMsg);
		return 0;
	    }
	  *metatype = '\0';
	  *metadims = '\0';
	  for (i = 1; i <= rows; i++)
	    {
		strcpy (metatype, results[(i * columns) + 0]);
		strcpy (metadims, results[(i * columns) + 1]);
	    }
	  sqlite3_free_table (results);
	  if (strcasecmp (metatype, "POINT") == 0)
	    {
		if (strcasecmp (metadims, "XYZ") == 0
		    || strcmp (metadims, "3") == 0)
		    shape = GAIA_POINTZ;
		else if (strcasecmp (metadims, "XYM") == 0)
		    shape = GAIA_POINTM;
		else if (strcasecmp (metadims, "XYZM") == 0)
		    shape = GAIA_POINTZM;
		else
		    shape = GAIA_POINT;
	    }
	  if (strcasecmp (metatype, "MULTIPOINT") == 0)
	    {
		if (strcasecmp (metadims, "XYZ") == 0
		    || strcmp (metadims, "3") == 0)
		    shape = GAIA_MULTIPOINTZ;
		else if (strcasecmp (metadims, "XYM") == 0)
		    shape = GAIA_MULTIPOINTM;
		else if (strcasecmp (metadims, "XYZM") == 0)
		    shape = GAIA_MULTIPOINTZM;
		else
		    shape = GAIA_MULTIPOINT;
	    }
	  if (strcasecmp (metatype, "LINESTRING") == 0
	      || strcasecmp (metatype, "MULTILINESTRING") == 0)
	    {
		if (strcasecmp (metadims, "XYZ") == 0
		    || strcmp (metadims, "3") == 0)
		    shape = GAIA_LINESTRINGZ;
		else if (strcasecmp (metadims, "XYM") == 0)
		    shape = GAIA_LINESTRINGM;
		else if (strcasecmp (metadims, "XYZM") == 0)
		    shape = GAIA_LINESTRINGZM;
		else
		    shape = GAIA_LINESTRING;
	    }
	  if (strcasecmp (metatype, "POLYGON") == 0
	      || strcasecmp (metatype, "MULTIPOLYGON") == 0)
	    {
		if (strcasecmp (metadims, "XYZ") == 0
		    || strcmp (metadims, "3") == 0)
		    shape = GAIA_POLYGONZ;
		else if (strcasecmp (metadims, "XYM") == 0)
		    shape = GAIA_POLYGONM;
		else if (strcasecmp (metadims, "XYZM") == 0)
		    shape = GAIA_POLYGONZM;
		else
		    shape = GAIA_POLYGON;
	    }
      }
    if (shape < 0)
      {
	  /* preparing SQL statement [type still undefined, so we'll read VIEWS_GEOMETRY_COLUMNS */
	  char **results;
	  int rows;
	  int columns;
	  int i;
	  char metatype[256];
	  char metadims[256];
	  char sql2[1024];
	  strcpy (sql,
		  "SELECT type, coord_dimension FROM views_geometry_columns ");
	  strcat (sql,
		  "JOIN geometry_columns USING (f_table_name, f_geometry_column) ");
	  sprintf (sql2, "WHERE view_name = '%s' AND view_geometry = '%s'",
		   table, column);
	  strcat (sql, sql2);
	  ret =
	      sqlite3_get_table (sqlite, sql, &results, &rows, &columns,
				 &errMsg);
	  if (ret != SQLITE_OK)
	    {
		if (!err_msg)
		    fprintf (stderr, "dump shapefile MetaData error: <%s>\n",
			     errMsg);
		else
		    sprintf (err_msg, "dump shapefile MetaData error: <%s>\n",
			     errMsg);
		sqlite3_free (errMsg);
		return 0;
	    }
	  *metatype = '\0';
	  *metadims = '\0';
	  for (i = 1; i <= rows; i++)
	    {
		strcpy (metatype, results[(i * columns) + 0]);
		strcpy (metadims, results[(i * columns) + 1]);
	    }
	  sqlite3_free_table (results);
	  if (strcasecmp (metatype, "POINT") == 0)
	    {
		if (strcasecmp (metadims, "XYZ") == 0
		    || strcmp (metadims, "3") == 0)
		    shape = GAIA_POINTZ;
		else if (strcasecmp (metadims, "XYM") == 0)
		    shape = GAIA_POINTM;
		else if (strcasecmp (metadims, "XYZM") == 0)
		    shape = GAIA_POINTZM;
		else
		    shape = GAIA_POINT;
	    }
	  if (strcasecmp (metatype, "MULTIPOINT") == 0)
	    {
		if (strcasecmp (metadims, "XYZ") == 0
		    || strcmp (metadims, "3") == 0)
		    shape = GAIA_MULTIPOINTZ;
		else if (strcasecmp (metadims, "XYM") == 0)
		    shape = GAIA_MULTIPOINTM;
		else if (strcasecmp (metadims, "XYZM") == 0)
		    shape = GAIA_MULTIPOINTZM;
		else
		    shape = GAIA_MULTIPOINT;
	    }
	  if (strcasecmp (metatype, "LINESTRING") == 0
	      || strcasecmp (metatype, "MULTILINESTRING") == 0)
	    {
		if (strcasecmp (metadims, "XYZ") == 0
		    || strcmp (metadims, "3") == 0)
		    shape = GAIA_LINESTRINGZ;
		else if (strcasecmp (metadims, "XYM") == 0)
		    shape = GAIA_LINESTRINGM;
		else if (strcasecmp (metadims, "XYZM") == 0)
		    shape = GAIA_LINESTRINGZM;
		else
		    shape = GAIA_LINESTRING;
	    }
	  if (strcasecmp (metatype, "POLYGON") == 0
	      || strcasecmp (metatype, "MULTIPOLYGON") == 0)
	    {
		if (strcasecmp (metadims, "XYZ") == 0
		    || strcmp (metadims, "3") == 0)
		    shape = GAIA_POLYGONZ;
		else if (strcasecmp (metadims, "XYM") == 0)
		    shape = GAIA_POLYGONM;
		else if (strcasecmp (metadims, "XYZM") == 0)
		    shape = GAIA_POLYGONZM;
		else
		    shape = GAIA_POLYGON;
	    }
      }
    if (shape < 0)
      {
	  if (!err_msg)
	      fprintf (stderr,
		       "Unable to detect GeometryType for \"%s\".\"%s\" ... sorry\n",
		       table, column);
	  else
	      sprintf (err_msg,
		       "Unable to detect GeometryType for \"%s\".\"%s\" ... sorry\n",
		       table, column);
	  return 0;
      }
    if (verbose)
	fprintf (stderr,
		 "========\nDumping SQLite table '%s' into shapefile at '%s'\n",
		 table, shp_path);
    /* preparing SQL statement */
    sprintf (sql, "SELECT * FROM \"%s\" WHERE GeometryAliasType(\"%s\") = ",
	     table, column);
    if (shape == GAIA_LINESTRING || shape == GAIA_LINESTRINGZ
	|| shape == GAIA_LINESTRINGM || shape == GAIA_LINESTRINGZM)
      {
	  strcat (sql, "'LINESTRING' OR GeometryAliasType(\"");
	  strcat (sql, (char *) column);
	  strcat (sql, "\") = 'MULTILINESTRING'");
      }
    else if (shape == GAIA_POLYGON || shape == GAIA_POLYGONZ
	     || shape == GAIA_POLYGONM || shape == GAIA_POLYGONZM)
      {
	  strcat (sql, "'POLYGON' OR GeometryAliasType(\"");
	  strcat (sql, (char *) column);
	  strcat (sql, "\") = 'MULTIPOLYGON'");
      }
    else if (shape == GAIA_MULTIPOINT || shape == GAIA_MULTIPOINTZ
	     || shape == GAIA_MULTIPOINTM || shape == GAIA_MULTIPOINTZM)
      {
	  strcat (sql, "'POINT' OR GeometryAliasType(\"");
	  strcat (sql, (char *) column);
	  strcat (sql, "\") = 'MULTIPOINT'");
      }
    else
	strcat (sql, "'POINT'");
/* fetching anyway NULL Geometries */
    strcat (sql, " OR \"");
    strcat (sql, (char *) column);
    strcat (sql, "\" IS NULL");
/* compiling SQL prepared statement */
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto sql_error;
    while (1)
      {
	  /* Pass I - scrolling the result set to compute real DBF attributes' sizes and types */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		/* processing a result set row */
		row1++;
		if (n_cols == 0)
		  {
		      /* this one is the first row, so we are going to prepare the DBF Fields list */
		      n_cols = sqlite3_column_count (stmt);
		      dbf_export_list = gaiaAllocDbfList ();
		      max_length = malloc (sizeof (int) * n_cols);
		      sql_type = malloc (sizeof (int) * n_cols);
		      for (i = 0; i < n_cols; i++)
			{
			    /* initializes the DBF export fields */
			    strcpy (dummy, sqlite3_column_name (stmt, i));
			    gaiaAddDbfField (dbf_export_list, dummy, '\0', 0, 0,
					     0);
			    max_length[i] = 0;
			    sql_type[i] = SQLITE_NULL;
			}
		  }
		for (i = 0; i < n_cols; i++)
		  {
		      /* update the DBF export fields analyzing fetched data */
		      type = sqlite3_column_type (stmt, i);
		      if (type == SQLITE_NULL || type == SQLITE_BLOB)
			  continue;
		      if (type == SQLITE_TEXT)
			{
			    len = sqlite3_column_bytes (stmt, i);
			    sql_type[i] = SQLITE_TEXT;
			    if (len > max_length[i])
				max_length[i] = len;
			}
		      else if (type == SQLITE_FLOAT
			       && sql_type[i] != SQLITE_TEXT)
			  sql_type[i] = SQLITE_FLOAT;	/* promoting a numeric column to be DOUBLE */
		      else if (type == SQLITE_INTEGER &&
			       (sql_type[i] == SQLITE_NULL
				|| sql_type[i] == SQLITE_INTEGER))
			  sql_type[i] = SQLITE_INTEGER;	/* promoting a null column to be INTEGER */
		      if (type == SQLITE_INTEGER && max_length[i] < 18)
			  max_length[i] = 18;
		      if (type == SQLITE_FLOAT && max_length[i] < 24)
			  max_length[i] = 24;
		  }
	    }
	  else
	      goto sql_error;
      }
    if (!row1)
	goto empty_result_set;
    i = 0;
    offset = 0;
    dbf_list = gaiaAllocDbfList ();
    dbf_field = dbf_export_list->First;
    while (dbf_field)
      {
	  /* preparing the final DBF attribute list */
	  if (sql_type[i] == SQLITE_NULL)
	    {
		i++;
		dbf_field = dbf_field->Next;
		continue;
	    }
	  if (sql_type[i] == SQLITE_TEXT)
	    {
		gaiaAddDbfField (dbf_list, dbf_field->Name, 'C', offset,
				 max_length[i], 0);
		offset += max_length[i];
	    }
	  if (sql_type[i] == SQLITE_FLOAT)
	    {
		gaiaAddDbfField (dbf_list, dbf_field->Name, 'N', offset, 19, 6);
		offset += 19;
	    }
	  if (sql_type[i] == SQLITE_INTEGER)
	    {
		gaiaAddDbfField (dbf_list, dbf_field->Name, 'N', offset, 18, 0);
		offset += 18;
	    }
	  i++;
	  dbf_field = dbf_field->Next;
      }
    free (max_length);
    free (sql_type);
    gaiaFreeDbfList (dbf_export_list);
/* resetting SQLite query */
    if (verbose)
	fprintf (stderr, "\n%s;\n", sql);
    ret = sqlite3_reset (stmt);
    if (ret != SQLITE_OK)
	goto sql_error;
/* trying to open shapefile files */
    shp = gaiaAllocShapefile ();
    gaiaOpenShpWrite (shp, shp_path, shape, dbf_list, "UTF-8", charset);
    if (!(shp->Valid))
	goto no_file;
/* trying to export the .PRJ file */
    output_prj_file (sqlite, shp_path, table, column);
    while (1)
      {
	  /* Pass II - scrolling the result set to dump data into shapefile */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		rows++;
		dbf_write = gaiaCloneDbfEntity (dbf_list);
		for (i = 0; i < n_cols; i++)
		  {
		      if (strcasecmp
			  ((char *) column,
			   (char *) sqlite3_column_name (stmt, i)) == 0)
			{
			    /* this one is the internal BLOB encoded GEOMETRY to be exported */
			    if (sqlite3_column_type (stmt, i) != SQLITE_BLOB)
			      {
				  /* this one is a NULL Geometry */
				  dbf_write->Geometry = NULL;
			      }
			    else
			      {
				  blob_value = sqlite3_column_blob (stmt, i);
				  len = sqlite3_column_bytes (stmt, i);
				  dbf_write->Geometry =
				      gaiaFromSpatiaLiteBlobWkb (blob_value,
								 len);
			      }
			}
		      strcpy (dummy, sqlite3_column_name (stmt, i));
		      dbf_field = getDbfField (dbf_write, dummy);
		      if (!dbf_field)
			  continue;
		      if (sqlite3_column_type (stmt, i) == SQLITE_NULL)
			{
			    /* handling NULL values */
			    gaiaSetNullValue (dbf_field);
			}
		      else
			{
			    switch (dbf_field->Type)
			      {
			      case 'N':
				  if (sqlite3_column_type (stmt, i) ==
				      SQLITE_INTEGER)
				      gaiaSetIntValue (dbf_field,
						       sqlite3_column_int64
						       (stmt, i));
				  else if (sqlite3_column_type (stmt, i) ==
					   SQLITE_FLOAT)
				      gaiaSetDoubleValue (dbf_field,
							  sqlite3_column_double
							  (stmt, i));
				  else
				      gaiaSetNullValue (dbf_field);
				  break;
			      case 'C':
				  if (sqlite3_column_type (stmt, i) ==
				      SQLITE_TEXT)
				    {
					strcpy (dummy,
						(char *)
						sqlite3_column_text (stmt, i));
					gaiaSetStrValue (dbf_field, dummy);
				    }
				  else if (sqlite3_column_type (stmt, i) ==
					   SQLITE_INTEGER)
				    {
#if defined(_WIN32) || defined(__MINGW32__)
					/* CAVEAT - M$ runtime doesn't supports %lld for 64 bits */
					sprintf (dummy, "%I64d",
						 sqlite3_column_int64 (stmt,
								       i));
#else
					sprintf (dummy, "%lld",
						 sqlite3_column_int64 (stmt,
								       i));
#endif
					gaiaSetStrValue (dbf_field, dummy);
				    }
				  else if (sqlite3_column_type (stmt, i) ==
					   SQLITE_FLOAT)
				    {
					sprintf (dummy, "%1.6f",
						 sqlite3_column_double (stmt,
									i));
					gaiaSetStrValue (dbf_field, dummy);
				    }
				  else
				      gaiaSetNullValue (dbf_field);
				  break;
			      };
			}
		  }
		if (!gaiaWriteShpEntity (shp, dbf_write))
		    fprintf (stderr, "shapefile write error\n");
		gaiaFreeDbfList (dbf_write);
	    }
	  else
	      goto sql_error;
      }
    sqlite3_finalize (stmt);
    gaiaFlushShpHeaders (shp);
    gaiaFreeShapefile (shp);
    if (verbose)
	fprintf (stderr, "\nExported %d rows into SHAPEFILE\n========\n", rows);
    if (xrows)
	*xrows = rows;
    if (err_msg)
	sprintf (err_msg, "Exported %d rows into SHAPEFILE", rows);
    return 1;
  sql_error:
/* some SQL error occurred */
    sqlite3_finalize (stmt);
    if (dbf_export_list)
	gaiaFreeDbfList (dbf_export_list);
    if (dbf_list)
	gaiaFreeDbfList (dbf_list);
    if (shp)
	gaiaFreeShapefile (shp);
    if (!err_msg)
	fprintf (stderr, "SELECT failed: %s", sqlite3_errmsg (sqlite));
    else
	sprintf (err_msg, "SELECT failed: %s", sqlite3_errmsg (sqlite));
    return 0;
  no_file:
/* shapefile can't be created/opened */
    if (dbf_export_list)
	gaiaFreeDbfList (dbf_export_list);
    if (dbf_list)
	gaiaFreeDbfList (dbf_list);
    if (shp)
	gaiaFreeShapefile (shp);
    if (!err_msg)
	fprintf (stderr, "ERROR: unable to open '%s' for writing", shp_path);
    else
	sprintf (err_msg, "ERROR: unable to open '%s' for writing", shp_path);
    return 0;
  empty_result_set:
/* the result set is empty - nothing to do */
    sqlite3_finalize (stmt);
    if (dbf_export_list)
	gaiaFreeDbfList (dbf_export_list);
    if (dbf_list)
	gaiaFreeDbfList (dbf_list);
    if (shp)
	gaiaFreeShapefile (shp);
    if (!err_msg)
	fprintf (stderr,
		 "The SQL SELECT returned an empty result set ... there is nothing to export ...");
    else
	sprintf (err_msg,
		 "The SQL SELECT returned an empty result set ... there is nothing to export ...");
    return 0;
}

SPATIALITE_DECLARE int
load_dbf (sqlite3 * sqlite, char *dbf_path, char *table, char *charset,
	  int verbose, int *rows, char *err_msg)
{
    sqlite3_stmt *stmt;
    int ret;
    char *errMsg = NULL;
    char sql[65536];
    char dummyName[4096];
    int already_exists = 0;
    int sqlError = 0;
    gaiaDbfPtr dbf = NULL;
    gaiaDbfFieldPtr dbf_field;
    int cnt;
    int col_cnt;
    int seed;
    int len;
    int dup;
    int idup;
    int current_row;
    char **col_name = NULL;
    int deleted;
/* checking if TABLE already exists */
    sprintf (sql,
	     "SELECT name FROM sqlite_master WHERE type = 'table' AND name LIKE '%s'",
	     table);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  if (!err_msg)
	      fprintf (stderr, "load DBF error: <%s>\n",
		       sqlite3_errmsg (sqlite));
	  else
	      sprintf (err_msg, "load DBF error: <%s>\n",
		       sqlite3_errmsg (sqlite));
	  return 0;
      }
    while (1)
      {
	  /* scrolling the result set */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	      already_exists = 1;
	  else
	    {
		fprintf (stderr, "load DBF error: <%s>\n",
			 sqlite3_errmsg (sqlite));
		break;
	    }
      }
    sqlite3_finalize (stmt);
    if (already_exists)
      {
	  if (!err_msg)
	      fprintf (stderr, "load DBF error: table '%s' already exists\n",
		       table);
	  else
	      sprintf (err_msg, "load DBF error: table '%s' already exists\n",
		       table);
	  return 0;
      }
    dbf = gaiaAllocDbf ();
    gaiaOpenDbfRead (dbf, dbf_path, charset, "UTF-8");
    if (!(dbf->Valid))
      {
	  if (!err_msg)
	    {
		fprintf (stderr, "load DBF error: cannot open '%s'\n",
			 dbf_path);
		if (dbf->LastError)
		    fprintf (stderr, "\tcause: %s\n", dbf->LastError);
	    }
	  else
	    {
		char extra[512];
		*extra = '\0';
		if (dbf->LastError)
		    sprintf (extra, "\n\tcause: %s", dbf->LastError);
		sprintf (err_msg, "load DBF error: cannot open '%s'%s",
			 dbf_path, extra);
	    }
	  gaiaFreeDbf (dbf);
	  return 0;
      }
/* checking for duplicate / illegal column names and antialising them */
    col_cnt = 0;
    dbf_field = dbf->Dbf->First;
    while (dbf_field)
      {
	  /* counting DBF fields */
	  col_cnt++;
	  dbf_field = dbf_field->Next;
      }
    col_name = malloc (sizeof (char *) * col_cnt);
    cnt = 0;
    seed = 0;
    dbf_field = dbf->Dbf->First;
    while (dbf_field)
      {
	  /* preparing column names */
	  strcpy (dummyName, dbf_field->Name);
	  dup = 0;
	  for (idup = 0; idup < cnt; idup++)
	    {
		if (strcasecmp (dummyName, *(col_name + idup)) == 0)
		    dup = 1;
	    }
	  if (strcasecmp (dummyName, "PK_UID") == 0)
	      dup = 1;
	  if (dup)
	      sprintf (dummyName, "COL_%d", seed++);
	  len = strlen (dummyName);
	  *(col_name + cnt) = malloc (len + 1);
	  strcpy (*(col_name + cnt), dummyName);
	  cnt++;
	  dbf_field = dbf_field->Next;
      }
    if (verbose)
	fprintf (stderr,
		 "========\nLoading DBF at '%s' into SQLite table '%s'\n",
		 dbf_path, table);
/* starting a transaction */
    if (verbose)
	fprintf (stderr, "\nBEGIN;\n");
    ret = sqlite3_exec (sqlite, "BEGIN", NULL, 0, &errMsg);
    if (ret != SQLITE_OK)
      {
	  if (!err_msg)
	      fprintf (stderr, "load DBF error: <%s>\n", errMsg);
	  else
	      sprintf (err_msg, "load DBF error: <%s>\n", errMsg);
	  sqlite3_free (errMsg);
	  sqlError = 1;
	  goto clean_up;
      }
/* creating the Table */
    sprintf (sql, "CREATE TABLE %s", table);
    strcat (sql, " (\nPK_UID INTEGER PRIMARY KEY AUTOINCREMENT");
    cnt = 0;
    dbf_field = dbf->Dbf->First;
    while (dbf_field)
      {
	  strcat (sql, ",\n\"");
	  strcat (sql, *(col_name + cnt));
	  cnt++;
	  switch (dbf_field->Type)
	    {
	    case 'C':
		strcat (sql, "\" TEXT");
		break;
	    case 'N':
		fflush (stderr);
		if (dbf_field->Decimals)
		    strcat (sql, "\" DOUBLE");
		else
		  {
		      if (dbf_field->Length <= 18)
			  strcat (sql, "\" INTEGER");
		      else
			  strcat (sql, "\" DOUBLE");
		  }
		break;
	    case 'D':
		strcat (sql, "\" DOUBLE");
		break;
	    case 'F':
		strcat (sql, "\" DOUBLE");
		break;
	    case 'L':
		strcat (sql, "\" INTEGER");
		break;
	    };
	  dbf_field = dbf_field->Next;
      }
    strcat (sql, ")");
    if (verbose)
	fprintf (stderr, "%s;\n", sql);
    ret = sqlite3_exec (sqlite, sql, NULL, 0, &errMsg);
    if (ret != SQLITE_OK)
      {
	  if (!err_msg)
	      fprintf (stderr, "load DBF error: <%s>\n", errMsg);
	  else
	      sprintf (err_msg, "load DBF error: <%s>\n", errMsg);
	  sqlite3_free (errMsg);
	  sqlError = 1;
	  goto clean_up;
      }
    /* preparing the INSERT INTO parametrerized statement */
    sprintf (sql, "INSERT INTO %s (PK_UID", table);
    cnt = 0;
    dbf_field = dbf->Dbf->First;
    while (dbf_field)
      {
	  /* columns corresponding to some DBF attribute */
	  strcat (sql, ",\"");
	  strcat (sql, *(col_name + cnt++));
	  strcat (sql, "\"");
	  dbf_field = dbf_field->Next;
      }
    strcat (sql, ")\nVALUES (?");
    dbf_field = dbf->Dbf->First;
    while (dbf_field)
      {
	  /* column values */
	  strcat (sql, ", ?");
	  dbf_field = dbf_field->Next;
      }
    strcat (sql, ")");
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  if (!err_msg)
	      fprintf (stderr, "load DBF error: <%s>\n",
		       sqlite3_errmsg (sqlite));
	  else
	      sprintf (err_msg, "load DBF error: <%s>\n",
		       sqlite3_errmsg (sqlite));
	  sqlError = 1;
	  goto clean_up;
      }
    current_row = 0;
    while (1)
      {
	  /* inserting rows from DBF */
	  ret = gaiaReadDbfEntity (dbf, current_row, &deleted);
	  if (!ret)
	    {
		if (!(dbf->LastError))	/* normal DBF EOF */
		    break;
		if (!err_msg)
		    fprintf (stderr, "%s\n", dbf->LastError);
		else
		    sprintf (err_msg, "%s\n", dbf->LastError);
		sqlError = 1;
		goto clean_up;
	    }
	  current_row++;
	  if (deleted)
	    {
		/* skipping DBF deleted row */
		continue;
	    }
	  /* binding query params */
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_int (stmt, 1, current_row);
	  cnt = 0;
	  dbf_field = dbf->Dbf->First;
	  while (dbf_field)
	    {
		/* column values */
		if (!(dbf_field->Value))
		    sqlite3_bind_null (stmt, cnt + 2);
		else
		  {
		      switch (dbf_field->Value->Type)
			{
			case GAIA_INT_VALUE:
			    sqlite3_bind_int64 (stmt, cnt + 2,
						dbf_field->Value->IntValue);
			    break;
			case GAIA_DOUBLE_VALUE:
			    sqlite3_bind_double (stmt, cnt + 2,
						 dbf_field->Value->DblValue);
			    break;
			case GAIA_TEXT_VALUE:
			    sqlite3_bind_text (stmt, cnt + 2,
					       dbf_field->Value->TxtValue,
					       strlen (dbf_field->Value->
						       TxtValue),
					       SQLITE_STATIC);
			    break;
			default:
			    sqlite3_bind_null (stmt, cnt + 2);
			    break;
			}
		  }
		cnt++;
		dbf_field = dbf_field->Next;
	    }
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		if (!err_msg)
		    fprintf (stderr, "load DBF error: <%s>\n",
			     sqlite3_errmsg (sqlite));
		else
		    sprintf (err_msg, "load DBF error: <%s>\n",
			     sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		sqlError = 1;
		goto clean_up;
	    }
      }
    sqlite3_finalize (stmt);
  clean_up:
    gaiaFreeDbf (dbf);
    if (col_name)
      {
	  /* releasing memory allocation for column names */
	  for (cnt = 0; cnt < col_cnt; cnt++)
	      free (*(col_name + cnt));
	  free (col_name);
      }
    if (sqlError)
      {
	  /* some error occurred - ROLLBACK */
	  if (verbose)
	      fprintf (stderr, "ROLLBACK;\n");
	  ret = sqlite3_exec (sqlite, "ROLLBACK", NULL, 0, &errMsg);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "load DBF error: <%s>\n", errMsg);
		sqlite3_free (errMsg);
	    };
	  if (rows)
	      *rows = current_row;
	  return 0;
      }
    else
      {
	  /* ok - confirming pending transaction - COMMIT */
	  if (verbose)
	      fprintf (stderr, "COMMIT;\n");
	  ret = sqlite3_exec (sqlite, "COMMIT", NULL, 0, &errMsg);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "load DBF error: <%s>\n", errMsg);
		sqlite3_free (errMsg);
		return 0;
	    }
	  if (rows)
	      *rows = current_row;
	  if (verbose)
	      fprintf (stderr,
		       "\nInserted %d rows into '%s' from DBF\n========\n",
		       current_row, table);
	  if (err_msg)
	      sprintf (err_msg, "Inserted %d rows into '%s' from DBF",
		       current_row, table);
	  return 1;
      }
}

SPATIALITE_DECLARE int
dump_dbf (sqlite3 * sqlite, char *table, char *dbf_path, char *charset,
	  char *err_msg)
{
/* DBF dump */
    int rows;
    int i;
    char sql[4096];
    char xtable[4096];
    sqlite3_stmt *stmt;
    int row1 = 0;
    int n_cols = 0;
    int offset = 0;
    int type;
    gaiaDbfPtr dbf = NULL;
    gaiaDbfListPtr dbf_export_list = NULL;
    gaiaDbfListPtr dbf_list = NULL;
    gaiaDbfListPtr dbf_write;
    gaiaDbfFieldPtr dbf_field;
    int *max_length = NULL;
    int *sql_type = NULL;
    char dummy[1024];
    int len;
    int ret;
/*
/ preparing SQL statement 
*/
    strcpy (xtable, table);
    shp_double_quoted_sql (xtable);
    sprintf (sql, "SELECT * FROM %s", xtable);
/*
/ compiling SQL prepared statement 
*/
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto sql_error;
    rows = 0;
    while (1)
      {
	  /*
	     / Pass I - scrolling the result set to compute real DBF attributes' sizes and types 
	   */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		/* processing a result set row */
		row1++;
		if (n_cols == 0)
		  {
		      /* this one is the first row, so we are going to prepare the DBF Fields list */
		      n_cols = sqlite3_column_count (stmt);
		      dbf_export_list = gaiaAllocDbfList ();
		      max_length = (int *) malloc (sizeof (int) * n_cols);
		      sql_type = (int *) malloc (sizeof (int) * n_cols);
		      for (i = 0; i < n_cols; i++)
			{
			    /* initializes the DBF export fields */
			    strcpy (dummy, sqlite3_column_name (stmt, i));
			    gaiaAddDbfField (dbf_export_list, dummy, '\0', 0, 0,
					     0);
			    max_length[i] = 0;
			    sql_type[i] = SQLITE_NULL;
			}
		  }
		for (i = 0; i < n_cols; i++)
		  {
		      /* update the DBF export fields analyzing fetched data */
		      type = sqlite3_column_type (stmt, i);
		      if (type == SQLITE_NULL || type == SQLITE_BLOB)
			  continue;
		      if (type == SQLITE_TEXT)
			{
			    len = sqlite3_column_bytes (stmt, i);
			    sql_type[i] = SQLITE_TEXT;
			    if (len > max_length[i])
				max_length[i] = len;
			}
		      else if (type == SQLITE_FLOAT
			       && sql_type[i] != SQLITE_TEXT)
			  sql_type[i] = SQLITE_FLOAT;	/* promoting a numeric column to be DOUBLE */
		      else if (type == SQLITE_INTEGER
			       && (sql_type[i] == SQLITE_NULL
				   || sql_type[i] == SQLITE_INTEGER))
			  sql_type[i] = SQLITE_INTEGER;	/* promoting a null column to be INTEGER */
		      if (type == SQLITE_INTEGER && max_length[i] < 18)
			  max_length[i] = 18;
		      if (type == SQLITE_FLOAT && max_length[i] < 24)
			  max_length[i] = 24;
		  }
	    }
	  else
	      goto sql_error;
      }
    if (!row1)
	goto empty_result_set;
    i = 0;
    offset = 0;
    dbf_list = gaiaAllocDbfList ();
    dbf_field = dbf_export_list->First;
    while (dbf_field)
      {
	  /* preparing the final DBF attribute list */
	  if (sql_type[i] == SQLITE_NULL || sql_type[i] == SQLITE_BLOB)
	    {
		i++;
		dbf_field = dbf_field->Next;
		continue;
	    }
	  if (sql_type[i] == SQLITE_TEXT)
	    {
		gaiaAddDbfField (dbf_list, dbf_field->Name, 'C', offset,
				 max_length[i], 0);
		offset += max_length[i];
	    }
	  if (sql_type[i] == SQLITE_FLOAT)
	    {
		gaiaAddDbfField (dbf_list, dbf_field->Name, 'N', offset, 19, 6);
		offset += 19;
	    }
	  if (sql_type[i] == SQLITE_INTEGER)
	    {
		gaiaAddDbfField (dbf_list, dbf_field->Name, 'N', offset, 18, 0);
		offset += 18;
	    }
	  i++;
	  dbf_field = dbf_field->Next;
      }
    free (max_length);
    free (sql_type);
    gaiaFreeDbfList (dbf_export_list);
    dbf_export_list = NULL;
/* resetting SQLite query */
    ret = sqlite3_reset (stmt);
    if (ret != SQLITE_OK)
	goto sql_error;
/* trying to open the DBF file */
    dbf = gaiaAllocDbf ();
/* xfering export-list ownership */
    dbf->Dbf = dbf_list;
    dbf_list = NULL;
    gaiaOpenDbfWrite (dbf, dbf_path, "UTF-8", charset);
    if (!(dbf->Valid))
	goto no_file;
    while (1)
      {
	  /* Pass II - scrolling the result set to dump data into DBF */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		rows++;
		dbf_write = gaiaCloneDbfEntity (dbf->Dbf);
		for (i = 0; i < n_cols; i++)
		  {
		      strcpy (dummy, sqlite3_column_name (stmt, i));
		      dbf_field = getDbfField (dbf_write, dummy);
		      if (!dbf_field)
			  continue;
		      if (sqlite3_column_type (stmt, i) == SQLITE_NULL
			  || sqlite3_column_type (stmt, i) == SQLITE_BLOB)
			{
			    /* handling NULL values */
			    gaiaSetNullValue (dbf_field);
			}
		      else
			{
			    switch (dbf_field->Type)
			      {
			      case 'N':
				  if (sqlite3_column_type (stmt, i) ==
				      SQLITE_INTEGER)
				      gaiaSetIntValue (dbf_field,
						       sqlite3_column_int64
						       (stmt, i));
				  else if (sqlite3_column_type (stmt, i) ==
					   SQLITE_FLOAT)
				      gaiaSetDoubleValue (dbf_field,
							  sqlite3_column_double
							  (stmt, i));
				  else
				      gaiaSetNullValue (dbf_field);
				  break;
			      case 'C':
				  if (sqlite3_column_type (stmt, i) ==
				      SQLITE_TEXT)
				    {
					strcpy (dummy,
						(char *)
						sqlite3_column_text (stmt, i));
					gaiaSetStrValue (dbf_field, dummy);
				    }
				  else if (sqlite3_column_type (stmt, i) ==
					   SQLITE_INTEGER)
				    {
#if defined(_WIN32) || defined(__MINGW32__)
					/* CAVEAT - M$ runtime doesn't supports %lld for 64 bits */
					sprintf (dummy, "%I64d",
						 sqlite3_column_int64 (stmt,
								       i));
#else
					sprintf (dummy, "%lld",
						 sqlite3_column_int64 (stmt,
								       i));
#endif
					gaiaSetStrValue (dbf_field, dummy);
				    }
				  else if (sqlite3_column_type (stmt, i) ==
					   SQLITE_FLOAT)
				    {
					sprintf (dummy, "%1.6f",
						 sqlite3_column_double (stmt,
									i));
					gaiaSetStrValue (dbf_field, dummy);
				    }
				  else
				      gaiaSetNullValue (dbf_field);
				  break;
			      };
			}
		  }
		if (!gaiaWriteDbfEntity (dbf, dbf_write))
		    fprintf (stderr, "DBF write error\n");
		gaiaFreeDbfList (dbf_write);
	    }
	  else
	      goto sql_error;
      }
    sqlite3_finalize (stmt);
    gaiaFlushDbfHeader (dbf);
    gaiaFreeDbf (dbf);
    if (!err_msg)
	fprintf (stderr, "Exported %d rows into the DBF file\n", rows);
    else
	sprintf (err_msg, "Exported %d rows into the DBF file\n", rows);
    return 1;
  sql_error:
/* some SQL error occurred */
    sqlite3_finalize (stmt);
    if (dbf_export_list)
	gaiaFreeDbfList (dbf_export_list);
    if (dbf_list)
	gaiaFreeDbfList (dbf_list);
    if (dbf)
	gaiaFreeDbf (dbf);
    if (!err_msg)
	fprintf (stderr, "dump DBF file error: %s\n", sqlite3_errmsg (sqlite));
    else
	sprintf (err_msg, "dump DBF file error: %s\n", sqlite3_errmsg (sqlite));
    return 0;
  no_file:
/* DBF can't be created/opened */
    if (dbf_export_list)
	gaiaFreeDbfList (dbf_export_list);
    if (dbf_list)
	gaiaFreeDbfList (dbf_list);
    if (dbf)
	gaiaFreeDbf (dbf);
    if (!err_msg)
	fprintf (stderr, "ERROR: unable to open '%s' for writing\n", dbf_path);
    else
	sprintf (err_msg, "ERROR: unable to open '%s' for writing\n", dbf_path);
    return 0;
  empty_result_set:
/* the result set is empty - nothing to do */
    sqlite3_finalize (stmt);
    if (dbf_export_list)
	gaiaFreeDbfList (dbf_export_list);
    if (dbf_list)
	gaiaFreeDbfList (dbf_list);
    if (dbf)
	gaiaFreeDbf (dbf);
    if (!err_msg)
	fprintf (stderr,
		 "The SQL SELECT returned an empty result set ... there is nothing to export ...\n");
    else
	sprintf (err_msg,
		 "The SQL SELECT returned an empty result set ... there is nothing to export ...\n");
    return 0;
}

#endif				/* end ICONV (SHP and DBF) */

SPATIALITE_DECLARE int
is_kml_constant (sqlite3 * sqlite, char *table, char *column)
{
/* checking a possible column name for KML dump */
    char sql[1024];
    int ret;
    int k = 1;
    const char *name;
    char **results;
    int rows;
    int columns;
    int i;
    char *errMsg = NULL;

    sprintf (sql, "PRAGMA table_info(%s)", table);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    if (ret != SQLITE_OK)
	return 1;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		name = results[(i * columns) + 1];
		if (strcasecmp (name, column) == 0)
		    k = 0;
	    }
      }
    sqlite3_free_table (results);
    return k;
}

SPATIALITE_DECLARE int
dump_kml (sqlite3 * sqlite, char *table, char *geom_col, char *kml_path,
	  char *name_col, char *desc_col, int precision)
{
/* dumping a  geometry table as KML */
    char sql[4096];
    char xname[1024];
    char xdesc[1024];
    sqlite3_stmt *stmt = NULL;
    FILE *out = NULL;
    int ret;
    int rows = 0;
    int is_const = 1;

/* opening/creating the KML file */
    out = fopen (kml_path, "wb");
    if (!out)
	goto no_file;

/* preparing SQL statement */
    if (name_col == NULL)
	strcpy (xname, "'name'");
    else
      {
	  is_const = is_kml_constant (sqlite, table, name_col);
	  if (is_const)
	      sprintf (xname, "'%s'", name_col);
	  else
	      strcpy (xname, name_col);
      }
    if (desc_col == NULL)
	strcpy (xdesc, "'description'");
    else
      {
	  is_const = is_kml_constant (sqlite, table, desc_col);
	  if (is_const)
	      sprintf (xdesc, "'%s'", desc_col);
	  else
	      strcpy (xdesc, desc_col);
      }
    sprintf (sql, "SELECT AsKML(%s, %s, %s, %d) FROM %s ", xname, xdesc,
	     geom_col, precision, table);
/* excluding NULL Geometries */
    strcat (sql, "WHERE ");
    strcat (sql, geom_col);
    strcat (sql, " IS NOT NULL");
/* compiling SQL prepared statement */
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto sql_error;

    while (1)
      {
	  /* scrolling the result set */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		/* processing a result set row */
		if (rows == 0)
		  {
		      fprintf (out,
			       "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n");
		      fprintf (out,
			       "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\r\n");
		      fprintf (out, "<Document>\r\n");
		  }
		rows++;
		fprintf (out, "\t%s\r\n", sqlite3_column_text (stmt, 0));
	    }
	  else
	      goto sql_error;
      }
    if (!rows)
	goto empty_result_set;


    fprintf (out, "</Document>\r\n");
    fprintf (out, "</kml>\r\n");
    sqlite3_finalize (stmt);
    fclose (out);
    return 1;

  sql_error:
/* some SQL error occurred */
    if (stmt)
	sqlite3_finalize (stmt);
    if (out)
	fclose (out);
    sqlite3_finalize (stmt);
    fprintf (stderr, "Dump KML error: %s\n", sqlite3_errmsg (sqlite));
    return 0;
  no_file:
/* KML file can't be created/opened */
    if (stmt)
	sqlite3_finalize (stmt);
    if (out)
	fclose (out);
    fprintf (stderr, "ERROR: unable to open '%s' for writing\n", kml_path);
    return 0;
  empty_result_set:
/* the result set is empty - nothing to do */
    if (stmt)
	sqlite3_finalize (stmt);
    if (out)
	fclose (out);
    fprintf (stderr,
	     "The SQL SELECT returned an empty result set\n... there is nothing to export ...\n");
    return 0;
}

static int
is_table (sqlite3 * sqlite, const char *table)
{
/* check if this one really is a TABLE */
    char sql[8192];
    int ret;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    int ok = 0;

    strcpy (sql, "SELECT tbl_name FROM sqlite_master ");
    strcat (sql, "WHERE type = 'table' AND tbl_name LIKE '");
    strcat (sql, table);
    strcat (sql, "'");
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQLite SQL error: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  return ok;
      }
    if (rows < 1)
	;
    else
	ok = 1;
    sqlite3_free_table (results);
    return ok;
}

SPATIALITE_DECLARE void
check_duplicated_rows (sqlite3 * sqlite, char *table, int *dupl_count)
{
/* Checking a Table for Duplicate rows */
    char sql[8192];
    char col_list[4196];
    int first = 1;
    char xname[1024];
    int pk;
    int ret;
    char **results;
    int rows;
    int columns;
    int i;
    char *errMsg = NULL;
    sqlite3_stmt *stmt = NULL;

    *dupl_count = 0;

    if (is_table (sqlite, table) == 0)
      {
	  fprintf (stderr, ".chkdupl %s: no such table\n", table);
	  return;
      }
/* extracting the column names (excluding any Primary Key) */
    sprintf (sql, "PRAGMA table_info(%s)", table);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQLite SQL error: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  return;
      }
    if (rows < 1)
	;
    else
      {
	  *col_list = '\0';
	  for (i = 1; i <= rows; i++)
	    {
		strcpy (xname, results[(i * columns) + 1]);
		pk = atoi (results[(i * columns) + 5]);
		if (!pk)
		  {
		      if (first)
			  first = 0;
		      else
			  strcat (col_list, ", ");
		      shp_double_quoted_sql (xname);
		      strcat (col_list, xname);
		  }
	    }
      }
    sqlite3_free_table (results);
    /* preparing the SQL statement */
    strcpy (sql, "SELECT Count(*) AS \"[dupl-count]\", ");
    strcat (sql, col_list);
    strcat (sql, "\nFROM ");
    strcat (sql, table);
    strcat (sql, "\nGROUP BY ");
    strcat (sql, col_list);
    strcat (sql, "\nHAVING \"[dupl-count]\" > 1");
    strcat (sql, "\nORDER BY \"[dupl-count]\" DESC");
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n", sqlite3_errmsg (sqlite));
	  return;
      }
    while (1)
      {
	  /* fetching the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		/* fetching a row */
		*dupl_count += sqlite3_column_int (stmt, 0) - 1;
	    }
	  else
	    {
		fprintf (stderr, "SQL error: %s", sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return;
	    }
      }
    sqlite3_finalize (stmt);
    if (*dupl_count)
	fprintf (stderr, "%d duplicated rows found !!!\n", *dupl_count);
    else
	fprintf (stderr, "No duplicated rows have been identified\n");
}

static int
do_delete_duplicates2 (sqlite3 * sqlite, sqlite3_stmt * stmt1,
		       struct dupl_row *value_list, int *count)
{
/* deleting duplicate rows [actual delete] */
    int cnt = 0;
    int row_no = 0;
    char sql[8192];
    char where[4196];
    char condition[1024];
    int ret;
    sqlite3_stmt *stmt2 = NULL;
    struct dupl_column *col;
    int first = 1;
    int qcnt = 0;
    int param = 1;
    int match;
    int n_cols;
    int col_no;

    *count = 0;
    reset_query_pos (value_list);

/* preparing the query statement */
    strcpy (sql, "SELECT ROWID");
    strcpy (where, "\nWHERE ");
    col = value_list->first;
    while (col)
      {
	  if (col->type == SQLITE_BLOB)
	    {
		strcat (sql, ", ");
		strcat (sql, col->name);
		col->query_pos = qcnt++;
	    }
	  else if (col->type == SQLITE_NULL)
	    {
		if (first)
		  {
		      first = 0;
		      strcpy (condition, col->name);
		  }
		else
		  {
		      strcpy (condition, " AND ");
		      strcat (condition, col->name);
		  }
		strcat (condition, " IS NULL");
		strcat (where, condition);
	    }
	  else
	    {
		if (first)
		  {
		      first = 0;
		      strcpy (condition, col->name);
		  }
		else
		  {
		      strcpy (condition, " AND ");
		      strcat (condition, col->name);
		  }
		strcat (condition, " = ?");
		strcat (where, condition);
		col->query_pos = param++;
	    }
	  col = col->next;
      }
    strcat (sql, "\nFROM ");
    strcat (sql, value_list->table);
    strcat (sql, where);

    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt2, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n", sqlite3_errmsg (sqlite));
	  goto error;
      }

    sqlite3_reset (stmt2);
    sqlite3_clear_bindings (stmt2);
    col = value_list->first;
    while (col)
      {
	  /* binding query params */
	  if (col->type == SQLITE_INTEGER)
	      sqlite3_bind_int64 (stmt2, col->query_pos, col->int_value);
	  if (col->type == SQLITE_FLOAT)
	      sqlite3_bind_double (stmt2, col->query_pos, col->dbl_value);
	  if (col->type == SQLITE_TEXT)
	      sqlite3_bind_text (stmt2, col->query_pos, col->txt_value,
				 strlen (col->txt_value), SQLITE_STATIC);
	  col = col->next;
      }

    while (1)
      {
	  /* fetching the result set rows */
	  ret = sqlite3_step (stmt2);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		/* fetching a row */
		match = 1;
		n_cols = sqlite3_column_count (stmt2);
		for (col_no = 1; col_no < n_cols; col_no++)
		  {
		      /* checking blob columns */
		      if (sqlite3_column_type (stmt2, col_no) == SQLITE_BLOB)
			{
			    const void *blob =
				sqlite3_column_blob (stmt2, col_no);
			    int blob_size =
				sqlite3_column_bytes (stmt2, col_no);
			    if (check_dupl_blob
				(value_list, col_no - 1, blob, blob_size) == 0)
				match = 0;
			}
		      else
			  match = 0;
		      if (match == 0)
			  break;
		  }
		if (match == 0)
		    continue;
		row_no++;
		if (row_no > 1)
		  {
		      /* deleting any duplicated row except the first one */
		      sqlite3_reset (stmt1);
		      sqlite3_clear_bindings (stmt1);
		      sqlite3_bind_int64 (stmt1, 1,
					  sqlite3_column_int64 (stmt2, 0));
		      ret = sqlite3_step (stmt1);
		      if (ret == SQLITE_DONE || ret == SQLITE_ROW)
			  cnt++;
		      else
			{
			    fprintf (stderr, "SQL error: %s\n",
				     sqlite3_errmsg (sqlite));
			    goto error;
			}
		  }
	    }
	  else
	    {
		fprintf (stderr, "SQL error: %s\n", sqlite3_errmsg (sqlite));
		goto error;
	    }
      }
    if (stmt2)
	sqlite3_finalize (stmt2);
    *count = cnt;
    return 1;

  error:
    if (stmt2)
	sqlite3_finalize (stmt2);
    *count = 0;

    return 0;
}

static int
do_delete_duplicates (sqlite3 * sqlite, const char *sql1, const char *sql2,
		      struct dupl_row *value_list, int *count)
{
/* deleting duplicate rows */
    sqlite3_stmt *stmt1 = NULL;
    sqlite3_stmt *stmt2 = NULL;
    int ret;
    int xcnt;
    int cnt = 0;
    int n_cols;
    int col_no;
    char *sql_err = NULL;

    *count = 0;

/* the complete operation is handled as an unique SQL Transaction */
    ret = sqlite3_exec (sqlite, "BEGIN", NULL, NULL, &sql_err);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "BEGIN TRANSACTION error: %s\n", sql_err);
	  sqlite3_free (sql_err);
	  return 0;
      }
/* preparing the main SELECT statement */
    ret = sqlite3_prepare_v2 (sqlite, sql1, strlen (sql1), &stmt1, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n", sqlite3_errmsg (sqlite));
	  return 0;
      }
/* preparing the DELETE statement */
    ret = sqlite3_prepare_v2 (sqlite, sql2, strlen (sql2), &stmt2, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n", sqlite3_errmsg (sqlite));
	  goto error;
      }

    while (1)
      {
	  /* fetching the result set rows */
	  ret = sqlite3_step (stmt1);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		/* fetching a row */
		sqlite3_reset (stmt2);
		sqlite3_clear_bindings (stmt2);
		n_cols = sqlite3_column_count (stmt1);
		for (col_no = 1; col_no < n_cols; col_no++)
		  {
		      /* saving column values */
		      if (sqlite3_column_type (stmt1, col_no) == SQLITE_INTEGER)
			  set_int_value (value_list, col_no - 1,
					 sqlite3_column_int64 (stmt1, col_no));
		      if (sqlite3_column_type (stmt1, col_no) == SQLITE_FLOAT)
			  set_double_value (value_list, col_no - 1,
					    sqlite3_column_double (stmt1,
								   col_no));
		      if (sqlite3_column_type (stmt1, col_no) == SQLITE_TEXT)
			{
			    const char *xtext =
				(const char *) sqlite3_column_text (stmt1,
								    col_no);
			    set_text_value (value_list, col_no - 1, xtext);
			}
		      if (sqlite3_column_type (stmt1, col_no) == SQLITE_BLOB)
			{
			    const void *blob =
				sqlite3_column_blob (stmt1, col_no);
			    int blob_size =
				sqlite3_column_bytes (stmt1, col_no);
			    set_blob_value (value_list, col_no - 1, blob,
					    blob_size);
			}
		      if (sqlite3_column_type (stmt1, col_no) == SQLITE_NULL)
			  set_null_value (value_list, col_no - 1);
		  }
		if (do_delete_duplicates2 (sqlite, stmt2, value_list, &xcnt))
		    cnt += xcnt;
		else
		    goto error;
	    }
	  else
	    {
		fprintf (stderr, "SQL error: %s\n", sqlite3_errmsg (sqlite));
		goto error;
	    }
      }

    sqlite3_finalize (stmt1);
    sqlite3_finalize (stmt2);

/* confirm the still pending Transaction */
    ret = sqlite3_exec (sqlite, "COMMIT", NULL, NULL, &sql_err);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "COMMIT TRANSACTION error: %s\n", sql_err);
	  sqlite3_free (sql_err);
	  return 0;
      }

    *count = cnt;
    return 1;

  error:
    *count = 0;
    if (stmt1)
	sqlite3_finalize (stmt1);
    if (stmt2)
	sqlite3_finalize (stmt2);

/* performing a ROLLBACK anyway */
    ret = sqlite3_exec (sqlite, "ROLLBACK", NULL, NULL, &sql_err);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ROLLBACK TRANSACTION error: %s\n", sql_err);
	  sqlite3_free (sql_err);
	  return 0;
      }

    return 0;
}

SPATIALITE_DECLARE void
remove_duplicated_rows (sqlite3 * sqlite, char *table)
{
/* attempting to delete Duplicate rows from a table */
    struct dupl_row value_list;
    char sql[8192];
    char sql2[1024];
    char col_list[4196];
    int first = 1;
    char xname[1024];
    int pk;
    int ret;
    char **results;
    int rows;
    int columns;
    int i;
    char *errMsg = NULL;
    int count;

    value_list.count = 0;
    value_list.first = NULL;
    value_list.last = NULL;
    value_list.table = table;

    if (is_table (sqlite, table) == 0)
      {
	  fprintf (stderr, ".remdupl %s: no such table\n", table);
	  return;
      }
/* extracting the column names (excluding any Primary Key) */
    sprintf (sql, "PRAGMA table_info(%s)", table);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQLite SQL error: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  return;
      }
    if (rows < 1)
	;
    else
      {
	  *col_list = '\0';
	  for (i = 1; i <= rows; i++)
	    {
		strcpy (xname, results[(i * columns) + 1]);
		pk = atoi (results[(i * columns) + 5]);
		if (!pk)
		  {
		      if (first)
			  first = 0;
		      else
			  strcat (col_list, ", ");
		      shp_double_quoted_sql (xname);
		      strcat (col_list, xname);
		      add_to_dupl_row (&value_list, xname);
		  }
	    }
      }
    sqlite3_free_table (results);
/* preparing the SQL statement (identifying duplicated rows) */
    strcpy (sql, "SELECT Count(*) AS \"[dupl-count]\", ");
    strcat (sql, col_list);
    strcat (sql, "\nFROM ");
    strcat (sql, table);
    strcat (sql, "\nGROUP BY ");
    strcat (sql, col_list);
    strcat (sql, "\nHAVING \"[dupl-count]\" > 1");
/* preparing the SQL statement [delete] */
    strcpy (sql2, "DELETE FROM ");
    strcat (sql2, table);
    strcat (sql2, " WHERE ROWID = ?");

    if (do_delete_duplicates (sqlite, sql, sql2, &value_list, &count))
      {
	  if (!count)
	      fprintf (stderr, "No duplicated rows have been identified\n");
	  else
	      fprintf (stderr, "%d duplicated rows deleted from: %s\n", count,
		       table);
      }
    clean_dupl_row (&value_list);
}

static int
check_elementary (sqlite3 * sqlite, const char *inTable, const char *geom,
		  const char *outTable, const char *pKey, const char *multiID,
		  char *type, int *srid, char *coordDims)
{
/* preliminary check for ELEMENTARY GEOMETRIES */
    char sql[8192];
    int ret;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    int ok = 0;
    int i;
    char *gtp;
    char *dims;
    char *quoted;

/* fetching metadata */
    strcpy (sql, "SELECT type, coord_dimension, srid ");
    strcat (sql, "FROM geometry_columns WHERE f_table_name LIKE '");
    quoted = gaiaSingleQuotedSql (inTable);
    if (quoted)
      {
	  strcat (sql, quoted);
	  free (quoted);
      }
    strcat (sql, "' AND f_geometry_column LIKE '");
    quoted = gaiaSingleQuotedSql (geom);
    if (quoted)
      {
	  strcat (sql, quoted);
	  free (quoted);
      }
    strcat (sql, "'");
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		gtp = results[(i * columns) + 0];
		dims = results[(i * columns) + 1];
		*srid = atoi (results[(i * columns) + 2]);
		if (strcasecmp (gtp, "POINT") == 0
		    || strcasecmp (gtp, "MULTIPOINT") == 0)
		    strcpy (type, "POINT");
		else if (strcasecmp (gtp, "LINESTRING") == 0
			 || strcasecmp (gtp, "MULTILINESTRING") == 0)
		    strcpy (type, "LINESTRING");
		else if (strcasecmp (gtp, "POLYGON") == 0
			 || strcasecmp (gtp, "MULTIPOLYGON") == 0)
		    strcpy (type, "POLYGON");
		else
		    strcpy (type, "GEOMETRY");
		strcpy (coordDims, dims);
		ok = 1;
	    }
      }
    sqlite3_free_table (results);
    if (!ok)
	return 0;

/* checking if PrimaryKey already exists */
    strcpy (sql, "PRAGMA table_info(\"");
    quoted = gaiaDoubleQuotedSql (inTable);
    if (quoted)
      {
	  strcat (sql, quoted);
	  free (quoted);
      }
    strcat (sql, "\")");
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		if (strcasecmp (pKey, results[(i * columns) + 1]) == 0)
		    ok = 0;
	    }
      }
    sqlite3_free_table (results);
    if (!ok)
	return 0;

/* checking if MultiID already exists */
    strcpy (sql, "PRAGMA table_info(\"");
    quoted = gaiaDoubleQuotedSql (inTable);
    if (quoted)
      {
	  strcat (sql, quoted);
	  free (quoted);
      }
    strcat (sql, "\")");
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		if (strcasecmp (multiID, results[(i * columns) + 1]) == 0)
		    ok = 0;
	    }
      }
    sqlite3_free_table (results);
    if (!ok)
	return 0;

/* cheching if Output Table already exists */
    strcpy (sql, "SELECT Count(*) FROM sqlite_master WHERE type ");
    strcat (sql, "LIKE 'table' AND tbl_name LIKE '");
    quoted = gaiaSingleQuotedSql (outTable);
    if (quoted)
      {
	  strcat (sql, quoted);
	  free (quoted);
      }
    strcat (sql, "'");
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		if (atoi (results[(i * columns) + 0]) != 0)
		    ok = 0;
	    }
      }
    sqlite3_free_table (results);

    return ok;
}

static gaiaGeomCollPtr
elemGeomFromPoint (gaiaPointPtr pt, int srid)
{
/* creating a Geometry containing a single Point */
    gaiaGeomCollPtr g = NULL;
    switch (pt->DimensionModel)
      {
      case GAIA_XY_Z_M:
	  g = gaiaAllocGeomCollXYZM ();
	  break;
      case GAIA_XY_Z:
	  g = gaiaAllocGeomCollXYZ ();
	  break;
      case GAIA_XY_M:
	  g = gaiaAllocGeomCollXYM ();
	  break;
      default:
	  g = gaiaAllocGeomColl ();
	  break;
      };
    if (!g)
	return NULL;
    g->Srid = srid;
    g->DeclaredType = GAIA_POINT;
    switch (pt->DimensionModel)
      {
      case GAIA_XY_Z_M:
	  gaiaAddPointToGeomCollXYZM (g, pt->X, pt->Y, pt->Z, pt->M);
	  break;
      case GAIA_XY_Z:
	  gaiaAddPointToGeomCollXYZ (g, pt->X, pt->Y, pt->Z);
	  break;
      case GAIA_XY_M:
	  gaiaAddPointToGeomCollXYM (g, pt->X, pt->Y, pt->M);
	  break;
      default:
	  gaiaAddPointToGeomColl (g, pt->X, pt->Y);
	  break;
      };
    return g;
}

static gaiaGeomCollPtr
elemGeomFromLinestring (gaiaLinestringPtr ln, int srid)
{
/* creating a Geometry containing a single Linestring */
    gaiaGeomCollPtr g = NULL;
    gaiaLinestringPtr ln2;
    int iv;
    double x;
    double y;
    double z;
    double m;
    switch (ln->DimensionModel)
      {
      case GAIA_XY_Z_M:
	  g = gaiaAllocGeomCollXYZM ();
	  break;
      case GAIA_XY_Z:
	  g = gaiaAllocGeomCollXYZ ();
	  break;
      case GAIA_XY_M:
	  g = gaiaAllocGeomCollXYM ();
	  break;
      default:
	  g = gaiaAllocGeomColl ();
	  break;
      };
    if (!g)
	return NULL;
    g->Srid = srid;
    g->DeclaredType = GAIA_LINESTRING;
    ln2 = gaiaAddLinestringToGeomColl (g, ln->Points);
    switch (ln->DimensionModel)
      {
      case GAIA_XY_Z_M:
	  for (iv = 0; iv < ln->Points; iv++)
	    {
		gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		gaiaSetPointXYZM (ln2->Coords, iv, x, y, z, m);
	    }
	  break;
      case GAIA_XY_Z:
	  for (iv = 0; iv < ln->Points; iv++)
	    {
		gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		gaiaSetPointXYZ (ln2->Coords, iv, x, y, z);
	    }
	  break;
      case GAIA_XY_M:
	  for (iv = 0; iv < ln->Points; iv++)
	    {
		gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		gaiaSetPointXYM (ln2->Coords, iv, x, y, m);
	    }
	  break;
      default:
	  for (iv = 0; iv < ln->Points; iv++)
	    {
		gaiaGetPoint (ln->Coords, iv, &x, &y);
		gaiaSetPoint (ln2->Coords, iv, x, y);
	    }
	  break;
      };
    return g;
}

static gaiaGeomCollPtr
elemGeomFromPolygon (gaiaPolygonPtr pg, int srid)
{
/* creating a Geometry containing a single Polygon */
    gaiaGeomCollPtr g = NULL;
    gaiaPolygonPtr pg2;
    gaiaRingPtr rng;
    gaiaRingPtr rng2;
    int ib;
    int iv;
    double x;
    double y;
    double z;
    double m;
    switch (pg->DimensionModel)
      {
      case GAIA_XY_Z_M:
	  g = gaiaAllocGeomCollXYZM ();
	  break;
      case GAIA_XY_Z:
	  g = gaiaAllocGeomCollXYZ ();
	  break;
      case GAIA_XY_M:
	  g = gaiaAllocGeomCollXYM ();
	  break;
      default:
	  g = gaiaAllocGeomColl ();
	  break;
      };
    if (!g)
	return NULL;
    g->Srid = srid;
    g->DeclaredType = GAIA_POLYGON;
    rng = pg->Exterior;
    pg2 = gaiaAddPolygonToGeomColl (g, rng->Points, pg->NumInteriors);
    rng2 = pg2->Exterior;
    switch (pg->DimensionModel)
      {
      case GAIA_XY_Z_M:
	  for (iv = 0; iv < rng->Points; iv++)
	    {
		gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
		gaiaSetPointXYZM (rng2->Coords, iv, x, y, z, m);
	    }
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		rng = pg->Interiors + ib;
		rng2 = gaiaAddInteriorRing (pg2, ib, rng->Points);
		for (iv = 0; iv < rng->Points; iv++)
		  {
		      gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
		      gaiaSetPointXYZM (rng2->Coords, iv, x, y, z, m);
		  }
	    }
	  break;
      case GAIA_XY_Z:
	  for (iv = 0; iv < rng->Points; iv++)
	    {
		gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
		gaiaSetPointXYZ (rng2->Coords, iv, x, y, z);
	    }
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		rng = pg->Interiors + ib;
		rng2 = gaiaAddInteriorRing (pg2, ib, rng->Points);
		for (iv = 0; iv < rng->Points; iv++)
		  {
		      gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
		      gaiaSetPointXYZ (rng2->Coords, iv, x, y, z);
		  }
	    }
	  break;
      case GAIA_XY_M:
	  for (iv = 0; iv < rng->Points; iv++)
	    {
		gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
		gaiaSetPointXYM (rng2->Coords, iv, x, y, m);
	    }
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		rng = pg->Interiors + ib;
		rng2 = gaiaAddInteriorRing (pg2, ib, rng->Points);
		for (iv = 0; iv < rng->Points; iv++)
		  {
		      gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
		      gaiaSetPointXYM (rng2->Coords, iv, x, y, m);
		  }
	    }
	  break;
      default:
	  for (iv = 0; iv < rng->Points; iv++)
	    {
		gaiaGetPoint (rng->Coords, iv, &x, &y);
		gaiaSetPoint (rng2->Coords, iv, x, y);
	    }
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		rng = pg->Interiors + ib;
		rng2 = gaiaAddInteriorRing (pg2, ib, rng->Points);
		for (iv = 0; iv < rng->Points; iv++)
		  {
		      gaiaGetPoint (rng->Coords, iv, &x, &y);
		      gaiaSetPoint (rng2->Coords, iv, x, y);
		  }
	    }
	  break;
      };
    return g;
}

SPATIALITE_DECLARE void
elementary_geometries (sqlite3 * sqlite,
		       char *inTable, char *geometry, char *outTable,
		       char *pKey, char *multiId)
{
/* attempting to create a derived table surely containing elemetary Geoms */
    char type[128];
    int srid;
    char dims[64];
    char sql[8192];
    char sql2[8192];
    char sql3[8192];
    char sql4[8192];
    char sqlx[1024];
    char sql_geom[1024];
    char dummy[1024];
    char *quoted;
    int ret;
    int comma = 0;
    char *errMsg = NULL;
    int i;
    char **results;
    int rows;
    int columns;
    int geom_idx = -1;
    sqlite3_stmt *stmt_in = NULL;
    sqlite3_stmt *stmt_out = NULL;
    int n_columns;
    sqlite3_int64 id = 0;

    if (check_elementary
	(sqlite, inTable, geometry, outTable, pKey, multiId, type, &srid,
	 dims) == 0)
      {
	  fprintf (stderr, ".elemgeo: invalid args\n");
	  return;
      }

/* starts a transaction */
    ret = sqlite3_exec (sqlite, "BEGIN", NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  goto abort;
      }

    strcpy (sql, "SELECT ");
    strcpy (sql2, "INSERT INTO \"");
    strcpy (sql3, ") VALUES (NULL, ?");
    strcpy (sql4, "CREATE TABLE \"");
    quoted = gaiaDoubleQuotedSql (outTable);
    if (quoted)
      {
	  strcat (sql2, quoted);
	  strcat (sql4, quoted);
	  free (quoted);
      }
    strcat (sql2, "\" (\"");
    quoted = gaiaDoubleQuotedSql (pKey);
    if (quoted)
      {
	  strcat (sql2, quoted);
	  free (quoted);
      }
    strcat (sql2, "\", \"");
    quoted = gaiaDoubleQuotedSql (multiId);
    if (quoted)
      {
	  strcat (sql2, quoted);
	  free (quoted);
      }
    strcat (sql2, "\"");
    strcat (sql4, "\" (\n\t\"");
    quoted = gaiaDoubleQuotedSql (pKey);
    if (quoted)
      {
	  strcat (sql4, quoted);
	  free (quoted);
      }
    strcat (sql4, "\" INTEGER PRIMARY KEY AUTOINCREMENT");
    strcat (sql4, ",\n\t\"");
    quoted = gaiaDoubleQuotedSql (multiId);
    if (quoted)
      {
	  strcat (sql4, quoted);
	  free (quoted);
      }
    strcat (sql4, "\" INTEGER NOT NULL");

    strcpy (sqlx, "PRAGMA table_info(\"");
    quoted = gaiaDoubleQuotedSql (inTable);
    if (quoted)
      {
	  strcat (sqlx, quoted);
	  free (quoted);
      }
    strcat (sqlx, "\")");
    ret = sqlite3_get_table (sqlite, sqlx, &results, &rows, &columns, &errMsg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  goto abort;
      }
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		if (comma)
		    strcat (sql, ", \"");
		else
		  {
		      comma = 1;
		      strcat (sql, "\"");
		  }
		strcat (sql2, ", \"");
		quoted = gaiaDoubleQuotedSql (results[(i * columns) + 1]);
		if (quoted)
		  {
		      strcat (sql, quoted);
		      strcat (sql2, quoted);
		      free (quoted);
		  }
		strcat (sql2, "\"");
		strcat (sql3, ", ?");

		if (strcasecmp (geometry, results[(i * columns) + 1]) == 0)
		    geom_idx = i - 1;
		else
		  {
		      strcat (sql4, ",\n\t\"");
		      quoted = gaiaDoubleQuotedSql (results[(i * columns) + 1]);
		      if (quoted)
			{
			    strcat (sql4, quoted);
			    free (quoted);
			}
		      strcat (sql4, "\" ");
		      strcat (sql4, results[(i * columns) + 2]);
		      if (atoi (results[(i * columns) + 3]) != 0)
			  strcat (sql4, " NOT NULL");
		  }
	    }
      }
    sqlite3_free_table (results);
    if (geom_idx < 0)
	goto abort;

    strcat (sql, " FROM \"");
    quoted = gaiaDoubleQuotedSql (inTable);
    if (quoted)
      {
	  strcat (sql, quoted);
	  free (quoted);
      }
    strcat (sql, "\"");
    strcat (sql2, sql3);
    strcat (sql2, ")");
    strcat (sql4, ")");

    strcpy (sql_geom, "SELECT AddGeometryColumn('");
    quoted = gaiaSingleQuotedSql (outTable);
    if (quoted)
      {
	  strcat (sql_geom, quoted);
	  free (quoted);
      }
    strcat (sql_geom, "', '");
    quoted = gaiaSingleQuotedSql (geometry);
    if (quoted)
      {
	  strcat (sql_geom, quoted);
	  free (quoted);
      }
    strcat (sql_geom, "', ");
    sprintf (dummy, "%d, '", srid);
    strcat (sql_geom, dummy);
    quoted = gaiaSingleQuotedSql (type);
    if (quoted)
      {
	  strcat (sql_geom, quoted);
	  free (quoted);
      }
    strcat (sql_geom, "', '");
    quoted = gaiaSingleQuotedSql (dims);
    if (quoted)
      {
	  strcat (sql_geom, quoted);
	  free (quoted);
      }
    strcat (sql_geom, "')");

/* creating the output table */
    ret = sqlite3_exec (sqlite, sql4, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  goto abort;
      }
/* creating the output Geometry */
    ret = sqlite3_exec (sqlite, sql_geom, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  goto abort;
      }

/* preparing the INPUT statement */
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_in, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n", sqlite3_errmsg (sqlite));
	  goto abort;
      }

/* preparing the OUTPUT statement */
    ret = sqlite3_prepare_v2 (sqlite, sql2, strlen (sql2), &stmt_out, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n", sqlite3_errmsg (sqlite));
	  goto abort;
      }

/* data transfer */
    n_columns = sqlite3_column_count (stmt_in);
    while (1)
      {
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		gaiaGeomCollPtr g =
		    gaiaFromSpatiaLiteBlobWkb ((const unsigned char *)
					       sqlite3_column_blob (stmt_in,
								    geom_idx),
					       sqlite3_column_bytes (stmt_in,
								     geom_idx));
		if (!g)
		  {
		      /* NULL input geometry */
		      sqlite3_reset (stmt_out);
		      sqlite3_clear_bindings (stmt_out);
		      sqlite3_bind_int64 (stmt_out, 1, id);
		      sqlite3_bind_null (stmt_out, geom_idx + 2);

		      for (i = 0; i < n_columns; i++)
			{
			    int type = sqlite3_column_type (stmt_in, i);
			    if (i == geom_idx)
				continue;
			    switch (type)
			      {
			      case SQLITE_INTEGER:
				  sqlite3_bind_int64 (stmt_out, i + 2,
						      sqlite3_column_int
						      (stmt_in, i));
				  break;
			      case SQLITE_FLOAT:
				  sqlite3_bind_double (stmt_out, i + 2,
						       sqlite3_column_double
						       (stmt_in, i));
				  break;
			      case SQLITE_TEXT:
				  sqlite3_bind_text (stmt_out, i + 2,
						     (const char *)
						     sqlite3_column_text
						     (stmt_in, i),
						     sqlite3_column_bytes
						     (stmt_in, i),
						     SQLITE_STATIC);
				  break;
			      case SQLITE_BLOB:
				  sqlite3_bind_blob (stmt_out, i + 2,
						     sqlite3_column_blob
						     (stmt_in, i),
						     sqlite3_column_bytes
						     (stmt_in, i),
						     SQLITE_STATIC);
				  break;
			      case SQLITE_NULL:
			      default:
				  sqlite3_bind_null (stmt_out, i + 2);
				  break;
			      };
			}

		      ret = sqlite3_step (stmt_out);
		      if (ret == SQLITE_DONE || ret == SQLITE_ROW)
			  ;
		      else
			{
			    fprintf (stderr, "[OUT]step error: %s\n",
				     sqlite3_errmsg (sqlite));
			    goto abort;
			}
		  }
		else
		  {
		      /* separating Elementary Geoms */
		      gaiaPointPtr pt;
		      gaiaLinestringPtr ln;
		      gaiaPolygonPtr pg;
		      gaiaGeomCollPtr outGeom;
		      pt = g->FirstPoint;
		      while (pt)
			{
			    /* separating Points */
			    outGeom = elemGeomFromPoint (pt, g->Srid);
			    sqlite3_reset (stmt_out);
			    sqlite3_clear_bindings (stmt_out);
			    sqlite3_bind_int64 (stmt_out, 1, id);
			    if (!outGeom)
				sqlite3_bind_null (stmt_out, geom_idx + 2);
			    else
			      {
				  unsigned char *blob;
				  int size;
				  gaiaToSpatiaLiteBlobWkb (outGeom, &blob,
							   &size);
				  sqlite3_bind_blob (stmt_out, geom_idx + 2,
						     blob, size, free);
				  gaiaFreeGeomColl (outGeom);
			      }

			    for (i = 0; i < n_columns; i++)
			      {
				  int type = sqlite3_column_type (stmt_in, i);
				  if (i == geom_idx)
				      continue;
				  switch (type)
				    {
				    case SQLITE_INTEGER:
					sqlite3_bind_int64 (stmt_out, i + 2,
							    sqlite3_column_int
							    (stmt_in, i));
					break;
				    case SQLITE_FLOAT:
					sqlite3_bind_double (stmt_out, i + 2,
							     sqlite3_column_double
							     (stmt_in, i));
					break;
				    case SQLITE_TEXT:
					sqlite3_bind_text (stmt_out, i + 2,
							   (const char *)
							   sqlite3_column_text
							   (stmt_in, i),
							   sqlite3_column_bytes
							   (stmt_in, i),
							   SQLITE_STATIC);
					break;
				    case SQLITE_BLOB:
					sqlite3_bind_blob (stmt_out, i + 2,
							   sqlite3_column_blob
							   (stmt_in, i),
							   sqlite3_column_bytes
							   (stmt_in, i),
							   SQLITE_STATIC);
					break;
				    case SQLITE_NULL:
				    default:
					sqlite3_bind_null (stmt_out, i + 2);
					break;
				    };
			      }

			    ret = sqlite3_step (stmt_out);
			    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
				;
			    else
			      {
				  fprintf (stderr, "[OUT]step error: %s\n",
					   sqlite3_errmsg (sqlite));
				  goto abort;
			      }
			    pt = pt->Next;
			}
		      ln = g->FirstLinestring;
		      while (ln)
			{
			    /* separating Linestrings */
			    outGeom = elemGeomFromLinestring (ln, g->Srid);
			    sqlite3_reset (stmt_out);
			    sqlite3_clear_bindings (stmt_out);
			    sqlite3_bind_int64 (stmt_out, 1, id);
			    if (!outGeom)
				sqlite3_bind_null (stmt_out, geom_idx + 2);
			    else
			      {
				  unsigned char *blob;
				  int size;
				  gaiaToSpatiaLiteBlobWkb (outGeom, &blob,
							   &size);
				  sqlite3_bind_blob (stmt_out, geom_idx + 2,
						     blob, size, free);
				  gaiaFreeGeomColl (outGeom);
			      }

			    for (i = 0; i < n_columns; i++)
			      {
				  int type = sqlite3_column_type (stmt_in, i);
				  if (i == geom_idx)
				      continue;
				  switch (type)
				    {
				    case SQLITE_INTEGER:
					sqlite3_bind_int64 (stmt_out, i + 2,
							    sqlite3_column_int
							    (stmt_in, i));
					break;
				    case SQLITE_FLOAT:
					sqlite3_bind_double (stmt_out, i + 2,
							     sqlite3_column_double
							     (stmt_in, i));
					break;
				    case SQLITE_TEXT:
					sqlite3_bind_text (stmt_out, i + 2,
							   (const char *)
							   sqlite3_column_text
							   (stmt_in, i),
							   sqlite3_column_bytes
							   (stmt_in, i),
							   SQLITE_STATIC);
					break;
				    case SQLITE_BLOB:
					sqlite3_bind_blob (stmt_out, i + 2,
							   sqlite3_column_blob
							   (stmt_in, i),
							   sqlite3_column_bytes
							   (stmt_in, i),
							   SQLITE_STATIC);
					break;
				    case SQLITE_NULL:
				    default:
					sqlite3_bind_null (stmt_out, i + 2);
					break;
				    };
			      }

			    ret = sqlite3_step (stmt_out);
			    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
				;
			    else
			      {
				  fprintf (stderr, "[OUT]step error: %s\n",
					   sqlite3_errmsg (sqlite));
				  goto abort;
			      }
			    ln = ln->Next;
			}
		      pg = g->FirstPolygon;
		      while (pg)
			{
			    /* separating Polygons */
			    outGeom = elemGeomFromPolygon (pg, g->Srid);
			    sqlite3_reset (stmt_out);
			    sqlite3_clear_bindings (stmt_out);
			    sqlite3_bind_int64 (stmt_out, 1, id);
			    if (!outGeom)
				sqlite3_bind_null (stmt_out, geom_idx + 2);
			    else
			      {
				  unsigned char *blob;
				  int size;
				  gaiaToSpatiaLiteBlobWkb (outGeom, &blob,
							   &size);
				  sqlite3_bind_blob (stmt_out, geom_idx + 2,
						     blob, size, free);
				  gaiaFreeGeomColl (outGeom);
			      }

			    for (i = 0; i < n_columns; i++)
			      {
				  int type = sqlite3_column_type (stmt_in, i);
				  if (i == geom_idx)
				      continue;
				  switch (type)
				    {
				    case SQLITE_INTEGER:
					sqlite3_bind_int64 (stmt_out, i + 2,
							    sqlite3_column_int
							    (stmt_in, i));
					break;
				    case SQLITE_FLOAT:
					sqlite3_bind_double (stmt_out, i + 2,
							     sqlite3_column_double
							     (stmt_in, i));
					break;
				    case SQLITE_TEXT:
					sqlite3_bind_text (stmt_out, i + 2,
							   (const char *)
							   sqlite3_column_text
							   (stmt_in, i),
							   sqlite3_column_bytes
							   (stmt_in, i),
							   SQLITE_STATIC);
					break;
				    case SQLITE_BLOB:
					sqlite3_bind_blob (stmt_out, i + 2,
							   sqlite3_column_blob
							   (stmt_in, i),
							   sqlite3_column_bytes
							   (stmt_in, i),
							   SQLITE_STATIC);
					break;
				    case SQLITE_NULL:
				    default:
					sqlite3_bind_null (stmt_out, i + 2);
					break;
				    };
			      }

			    ret = sqlite3_step (stmt_out);
			    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
				;
			    else
			      {
				  fprintf (stderr, "[OUT]step error: %s\n",
					   sqlite3_errmsg (sqlite));
				  goto abort;
			      }
			    pg = pg->Next;
			}
		      gaiaFreeGeomColl (g);
		  }
		id++;
	    }
	  else
	    {
		fprintf (stderr, "[IN]step error: %s\n",
			 sqlite3_errmsg (sqlite));
		goto abort;
	    }
      }
    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_out);

/* commits the transaction */
    ret = sqlite3_exec (sqlite, "COMMIT", NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  goto abort;
      }
    return;

  abort:
    if (stmt_in)
	sqlite3_finalize (stmt_in);
    if (stmt_out)
	sqlite3_finalize (stmt_out);
}

#ifndef OMIT_FREEXL	/* including FreeXL */

SPATIALITE_DECLARE int
load_XL (sqlite3 * sqlite, const char *path, const char *table,
	 unsigned int worksheetIndex, int first_titles, unsigned int *rows,
	 char *err_msg)
{
/* loading an XL spreadsheet as a new DB table */
    sqlite3_stmt *stmt;
    unsigned int current_row;
    int ret;
    char *errMsg = NULL;
    char xname[1024];
    char dummyName[4096];
    char sql[65536];
    int sqlError = 0;
    const void *xl_handle;
    unsigned int info;
    unsigned short columns;
    unsigned short col;
    FreeXL_CellValue cell;
    int already_exists = 0;
/* checking if TABLE already exists */
    sprintf (sql,
	     "SELECT name FROM sqlite_master WHERE type = 'table' AND name LIKE '%s'",
	     table);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  if (!err_msg)
	      fprintf (stderr, "load XL error: <%s>\n",
		       sqlite3_errmsg (sqlite));
	  else
	      sprintf (err_msg, "load XL error: <%s>\n",
		       sqlite3_errmsg (sqlite));
	  return 0;
      }
    while (1)
      {
	  /* scrolling the result set */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	      already_exists = 1;
	  else
	    {
		fprintf (stderr, "load XL error: <%s>\n",
			 sqlite3_errmsg (sqlite));
		break;
	    }
      }
    sqlite3_finalize (stmt);
    if (already_exists)
      {
	  if (!err_msg)
	      fprintf (stderr, "load XL error: table '%s' already exists\n",
		       table);
	  else
	      sprintf (err_msg, "load XL error: table '%s' already exists\n",
		       table);
	  return 0;
      }
/* opening the .XLS file [Workbook] */
    ret = freexl_open (path, &xl_handle);
    if (ret != FREEXL_OK)
	goto error;
/* checking if Password protected */
    ret = freexl_get_info (xl_handle, FREEXL_BIFF_PASSWORD, &info);
    if (ret != FREEXL_OK)
	goto error;
    if (info != FREEXL_BIFF_PLAIN)
	goto error;
/* Worksheet entries */
    ret = freexl_get_info (xl_handle, FREEXL_BIFF_SHEET_COUNT, &info);
    if (ret != FREEXL_OK)
	goto error;
    if (info == 0)
	goto error;
    if (worksheetIndex < info)
	;
    else
	goto error;
    ret = freexl_select_active_worksheet (xl_handle, worksheetIndex);
    if (ret != FREEXL_OK)
	goto error;
    ret = freexl_worksheet_dimensions (xl_handle, rows, &columns);
    if (ret != FREEXL_OK)
	goto error;
/* starting a transaction */
    ret = sqlite3_exec (sqlite, "BEGIN", NULL, 0, &errMsg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "load XL error: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  sqlError = 1;
	  goto clean_up;
      }
/* creating the Table */
    strcpy (xname, table);
    shp_double_quoted_sql (xname);
    sprintf (sql, "CREATE TABLE %s", xname);
    strcat (sql, " (\nPK_UID INTEGER PRIMARY KEY AUTOINCREMENT");
    for (col = 0; col < columns; col++)
      {
	  if (first_titles)
	    {
		/* fetching column names */
		for (col = 0; col < columns; col++)
		  {
		      ret = freexl_get_cell_value (xl_handle, 0, col, &cell);
		      if (ret != FREEXL_OK)
			  sprintf (dummyName, "col_%d", col);
		      else
			{
			    if (cell.type == FREEXL_CELL_INT)
				sprintf (dummyName, "%d", cell.value.int_value);
			    else if (cell.type == FREEXL_CELL_DOUBLE)
				sprintf (dummyName, "%1.2f",
					 cell.value.double_value);
			    else if (cell.type == FREEXL_CELL_TEXT
				     || cell.type == FREEXL_CELL_SST_TEXT
				     || cell.type == FREEXL_CELL_DATE
				     || cell.type == FREEXL_CELL_DATETIME
				     || cell.type == FREEXL_CELL_TIME)
			      {
				  int len = strlen (cell.value.text_value);
				  if (len < 256)
				      strcpy (dummyName, cell.value.text_value);
				  else
				      sprintf (dummyName, "col_%d", col);
			      }
			    else
				sprintf (dummyName, "col_%d", col);
			}
		      shp_double_quoted_sql (dummyName);
		      strcat (sql, ", ");
		      strcat (sql, dummyName);
		  }
	    }
	  else
	    {
		/* setting default column names */
		for (col = 0; col < columns; col++)
		  {
		      sprintf (dummyName, "col_%d", col);
		      shp_double_quoted_sql (dummyName);
		      strcat (sql, ", ");
		      strcat (sql, dummyName);
		  }
	    }
      }
    strcat (sql, ")");
    ret = sqlite3_exec (sqlite, sql, NULL, 0, &errMsg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "load XL error: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  sqlError = 1;
	  goto clean_up;
      }
/* preparing the INSERT INTO parameterized statement */
    strcpy (xname, table);
    shp_double_quoted_sql (xname);
    sprintf (sql, "INSERT INTO %s (PK_UID", xname);
    for (col = 0; col < columns; col++)
      {
	  if (first_titles)
	    {
		ret = freexl_get_cell_value (xl_handle, 0, col, &cell);
		if (ret != FREEXL_OK)
		    sprintf (dummyName, "col_%d", col);
		else
		  {
		      if (cell.type == FREEXL_CELL_INT)
			  sprintf (dummyName, "%d", cell.value.int_value);
		      else if (cell.type == FREEXL_CELL_DOUBLE)
			  sprintf (dummyName, "%1.2f", cell.value.double_value);
		      else if (cell.type == FREEXL_CELL_TEXT
			       || cell.type == FREEXL_CELL_SST_TEXT
			       || cell.type == FREEXL_CELL_DATE
			       || cell.type == FREEXL_CELL_DATETIME
			       || cell.type == FREEXL_CELL_TIME)
			{
			    int len = strlen (cell.value.text_value);
			    if (len < 256)
				strcpy (dummyName, cell.value.text_value);
			    else
				sprintf (dummyName, "col_%d", col);
			}
		      else
			  sprintf (dummyName, "col_%d", col);
		  }
		shp_double_quoted_sql (dummyName);
		strcat (sql, ", ");
		strcat (sql, dummyName);
	    }
	  else
	    {
		/* setting default column names  */
		sprintf (dummyName, "col_%d", col);
		shp_double_quoted_sql (dummyName);
		strcat (sql, ", ");
		strcat (sql, dummyName);
	    }
      }
    strcat (sql, ")\nVALUES (NULL");
    for (col = 0; col < columns; col++)
      {
	  /* column values */
	  strcat (sql, ", ?");
      }
    strcat (sql, ")");
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "load XL error: %s\n", sqlite3_errmsg (sqlite));
	  sqlError = 1;
	  goto clean_up;
      }
    if (first_titles)
	current_row = 1;
    else
	current_row = 0;
    while (current_row < *rows)
      {
	  /* binding query params */
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  for (col = 0; col < columns; col++)
	    {
		/* column values */
		ret =
		    freexl_get_cell_value (xl_handle, current_row, col, &cell);
		if (ret != FREEXL_OK)
		    sqlite3_bind_null (stmt, col + 1);
		else
		  {
		      switch (cell.type)
			{
			case FREEXL_CELL_INT:
			    sqlite3_bind_int (stmt, col + 1,
					      cell.value.int_value);
			    break;
			case FREEXL_CELL_DOUBLE:
			    sqlite3_bind_double (stmt, col + 1,
						 cell.value.double_value);
			    break;
			case FREEXL_CELL_TEXT:
			case FREEXL_CELL_SST_TEXT:
			case FREEXL_CELL_DATE:
			case FREEXL_CELL_DATETIME:
			case FREEXL_CELL_TIME:
			    sqlite3_bind_text (stmt, col + 1,
					       cell.value.text_value,
					       strlen (cell.value.text_value),
					       SQLITE_STATIC);
			    break;
			default:
			    sqlite3_bind_null (stmt, col + 1);
			    break;
			};
		  }
	    }
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		fprintf (stderr, "load XL error: %s\n",
			 sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		sqlError = 1;
		goto clean_up;
	    }
	  current_row++;
      }
    sqlite3_finalize (stmt);
  clean_up:
    if (sqlError)
      {
	  /* some error occurred - ROLLBACK */
	  ret = sqlite3_exec (sqlite, "ROLLBACK", NULL, 0, &errMsg);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "load XL error: %s\n", errMsg);
		sqlite3_free (errMsg);
	    }
	  fprintf (stderr,
		   "XL not loaded\n\n\na ROLLBACK was automatically performed\n");
      }
    else
      {
	  /* ok - confirming pending transaction - COMMIT */
	  ret = sqlite3_exec (sqlite, "COMMIT", NULL, 0, &errMsg);
	  if (ret != SQLITE_OK)
	    {
		if (!err_msg)
		    fprintf (stderr, "load XL error: %s\n", errMsg);
		else
		    sprintf (err_msg, "load XL error: %s\n", errMsg);
		sqlite3_free (errMsg);
		return 0;
	    }
	  if (first_titles)
	      *rows = *rows - 1;	/* allow for header row */
	  fprintf (stderr, "XL loaded\n\n%d inserted rows\n", *rows);
      }
    freexl_close (xl_handle);
    return 1;

  error:
    freexl_close (xl_handle);
    if (!err_msg)
	fprintf (stderr, "XL datasource '%s' is not valid\n", path);
    else
	sprintf (err_msg, "XL datasource '%s' is not valid\n", path);
    return 0;
}

#endif /* FreeXL enabled/disabled */
