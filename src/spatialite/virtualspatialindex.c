/*

 virtualspatialindex.c -- SQLite3 extension [VIRTUAL TABLE RTree metahandler]

 version 3.0, 2011 July 20

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

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifdef SPL_AMALGAMATION		/* spatialite-amalgamation */
#include <spatialite/sqlite3.h>
#else
#include <sqlite3.h>
#endif

#include <spatialite/spatialite.h>
#include <spatialite/gaiaaux.h>
#include <spatialite/gaiageo.h>

#ifdef _WIN32
#define strcasecmp	_stricmp
#endif /* not WIN32 */

static struct sqlite3_module my_spidx_module;


/******************************************************************************
/
/ VirtualTable structs
/
******************************************************************************/

typedef struct VirtualSpatialIndexStruct
{
/* extends the sqlite3_vtab struct */
    const sqlite3_module *pModule;	/* ptr to sqlite module: USED INTERNALLY BY SQLITE */
    int nRef;			/* # references: USED INTERNALLY BY SQLITE */
    char *zErrMsg;		/* error message: USE INTERNALLY BY SQLITE */
    sqlite3 *db;		/* the sqlite db holding the virtual table */
} VirtualSpatialIndex;
typedef VirtualSpatialIndex *VirtualSpatialIndexPtr;

typedef struct VirtualSpatialIndexCursorStruct
{
/* extends the sqlite3_vtab_cursor struct */
    VirtualSpatialIndexPtr pVtab;	/* Virtual table of this cursor */
    int eof;			/* the EOF marker */
    sqlite3_stmt *stmt;
    sqlite3_int64 CurrentRowId;
} VirtualSpatialIndexCursor;
typedef VirtualSpatialIndexCursor *VirtualSpatialIndexCursorPtr;

static void
vspidx_double_quoted_sql (char *buf)
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

static void
vspidx_clean_sql_string (char *buf)
{
/* returns a well formatted TEXT value for SQL */
    char tmp[1024];
    char *in = tmp;
    char *out = buf;
    strcpy (tmp, buf);
    while (*in != '\0')
      {
	  if (*in == '\'')
	      *out++ = '\'';
	  *out++ = *in++;
      }
    *out = '\0';
}

static void
vspidx_dequote (char *buf)
{
/* dequoting an SQL string */
    char tmp[1024];
    char *in = tmp;
    char *out = buf;
    char strip = '\0';
    int first = 0;
    int len = strlen (buf);
    if (buf[0] == '\'' && buf[len - 1] == '\'')
	strip = '\'';
    if (buf[0] == '"' && buf[len - 1] == '"')
	strip = '"';
    if (strip == '\0')
	return;
    strcpy (tmp, buf + 1);
    len = strlen (tmp);
    tmp[len - 1] = '\0';
    while (*in != '\0')
      {
	  if (*in == strip)
	    {
		if (first)
		  {
		      first = 0;
		      in++;
		      continue;
		  }
		else
		  {
		      first = 1;
		      *out++ = *in++;
		      continue;
		  }
	    }
	  first = 0;
	  *out++ = *in++;
      }
    *out = '\0';
}

static int
vspidx_check_rtree (sqlite3 * sqlite, const char *table_name,
		    const char *geom_column)
{
/* checks if the required RTree is actually defined */
    char sql[4096];
    char xtable[1024];
    char xcolumn[1024];
    int ret;
    int i;
    int n_rows;
    int n_columns;
    char **results;
    int count = 0;

    strcpy (xtable, table_name);
    strcpy (xcolumn, geom_column);
    strcpy (sql,
	    "SELECT Count(*) FROM geometry_columns WHERE f_table_name LIKE '");
    vspidx_clean_sql_string (xtable);
    strcat (sql, xtable);
    strcat (sql, "' AND f_geometry_column LIKE '");
    vspidx_clean_sql_string (xcolumn);
    strcat (sql, xcolumn);
    strcat (sql, "' AND spatial_index_enabled = 1");
    ret = sqlite3_get_table (sqlite, sql, &results, &n_rows, &n_columns, NULL);
    if (ret != SQLITE_OK)
	return 0;
    if (n_rows >= 1)
      {
	  for (i = 1; i <= n_rows; i++)
	      count = atoi (results[(i * n_columns) + 0]);
      }
    sqlite3_free_table (results);
    if (count != 1)
	return 0;
    return 1;
}

static int
vspidx_find_rtree (sqlite3 * sqlite, const char *table_name, char *geom_column)
{
/* attempts to find the corresponding RTree Geometry Column */
    char sql[4096];
    char xtable[1024];
    char xcolumn[1024];
    int ret;
    int i;
    int n_rows;
    int n_columns;
    char **results;
    int count = 0;

    strcpy (xtable, table_name);
    strcpy (sql,
	    "SELECT f_geometry_column FROM geometry_columns WHERE f_table_name LIKE '");
    vspidx_clean_sql_string (xtable);
    strcat (sql, xtable);
    strcat (sql, "' AND spatial_index_enabled = 1");
    ret = sqlite3_get_table (sqlite, sql, &results, &n_rows, &n_columns, NULL);
    if (ret != SQLITE_OK)
	return 0;
    if (n_rows >= 1)
      {
	  for (i = 1; i <= n_rows; i++)
	    {
		count++;
		strcpy (xcolumn, results[(i * n_columns) + 0]);
	    }
      }
    sqlite3_free_table (results);
    if (count != 1)
	return 0;
    strcpy (geom_column, xcolumn);
    return 1;
}

static int
vspidx_create (sqlite3 * db, void *pAux, int argc, const char *const *argv,
	       sqlite3_vtab ** ppVTab, char **pzErr)
{
/* creates the virtual table for R*Tree SpatialIndex metahandling */
    VirtualSpatialIndexPtr p_vt;
    char buf[1024];
    char vtable[1024];
    char xname[1024];
    if (pAux)
	pAux = pAux;		/* unused arg warning suppression */
    if (argc == 3)
      {
	  strcpy (vtable, argv[2]);
	  vspidx_dequote (vtable);
      }
    else
      {
	  *pzErr =
	      sqlite3_mprintf
	      ("[VirtualSpatialIndex module] CREATE VIRTUAL: illegal arg list {void}\n");
	  return SQLITE_ERROR;
      }
    p_vt =
	(VirtualSpatialIndexPtr) sqlite3_malloc (sizeof (VirtualSpatialIndex));
    if (!p_vt)
	return SQLITE_NOMEM;
    p_vt->db = db;
    p_vt->pModule = &my_spidx_module;
    p_vt->nRef = 0;
    p_vt->zErrMsg = NULL;
/* preparing the COLUMNs for this VIRTUAL TABLE */
    strcpy (buf, "CREATE TABLE ");
    strcpy (xname, vtable);
    vspidx_double_quoted_sql (xname);
    strcat (buf, xname);
    strcat (buf,
	    " (f_table_name TEXT, f_geometry_column TEXT, search_frame BLOB)");
    if (sqlite3_declare_vtab (db, buf) != SQLITE_OK)
      {
	  *pzErr =
	      sqlite3_mprintf
	      ("[VirtualSpatialIndex module] CREATE VIRTUAL: invalid SQL statement \"%s\"",
	       buf);
	  return SQLITE_ERROR;
      }
    *ppVTab = (sqlite3_vtab *) p_vt;
    return SQLITE_OK;
}

static int
vspidx_connect (sqlite3 * db, void *pAux, int argc, const char *const *argv,
		sqlite3_vtab ** ppVTab, char **pzErr)
{
/* connects the virtual table - simply aliases vspidx_create() */
    return vspidx_create (db, pAux, argc, argv, ppVTab, pzErr);
}

static int
vspidx_best_index (sqlite3_vtab * pVTab, sqlite3_index_info * pIdxInfo)
{
/* best index selection */
    int i;
    int errors = 0;
    int err = 1;
    int table = 0;
    int geom = 0;
    int mbr = 0;
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    for (i = 0; i < pIdxInfo->nConstraint; i++)
      {
	  /* verifying the constraints */
	  struct sqlite3_index_constraint *p = &(pIdxInfo->aConstraint[i]);
	  if (p->usable)
	    {
		if (p->iColumn == 0 && p->op == SQLITE_INDEX_CONSTRAINT_EQ)
		      table++;
		else if (p->iColumn == 1 && p->op == SQLITE_INDEX_CONSTRAINT_EQ)
		      geom++;
		else if (p->iColumn == 2 && p->op == SQLITE_INDEX_CONSTRAINT_EQ)
		      mbr++;
		else
		    errors++;
	    }
      }
    if (table == 1 && (geom == 0 || geom == 1) && mbr == 1 && errors == 0)
      {
	  /* this one is a valid SpatialIndex query */
	  if (geom == 1)
	      pIdxInfo->idxNum = 1;
	  else
	      pIdxInfo->idxNum = 2;
	  pIdxInfo->estimatedCost = 1.0;
	  for (i = 0; i < pIdxInfo->nConstraint; i++)
	    {
		if (pIdxInfo->aConstraint[i].usable)
		  {
		      pIdxInfo->aConstraintUsage[i].argvIndex = i + 1;
		      pIdxInfo->aConstraintUsage[i].omit = 1;
		  }
	    }
	  err = 0;
      }
    if (err)
      {
	  /* illegal query */
	  pIdxInfo->idxNum = 0;
      }
    return SQLITE_OK;
}

static int
vspidx_disconnect (sqlite3_vtab * pVTab)
{
/* disconnects the virtual table */
    VirtualSpatialIndexPtr p_vt = (VirtualSpatialIndexPtr) pVTab;
    sqlite3_free (p_vt);
    return SQLITE_OK;
}

static int
vspidx_destroy (sqlite3_vtab * pVTab)
{
/* destroys the virtual table - simply aliases vspidx_disconnect() */
    return vspidx_disconnect (pVTab);
}

static int
vspidx_open (sqlite3_vtab * pVTab, sqlite3_vtab_cursor ** ppCursor)
{
/* opening a new cursor */
    VirtualSpatialIndexCursorPtr cursor =
	(VirtualSpatialIndexCursorPtr)
	sqlite3_malloc (sizeof (VirtualSpatialIndexCursor));
    if (cursor == NULL)
	return SQLITE_ERROR;
    cursor->pVtab = (VirtualSpatialIndexPtr) pVTab;
    cursor->stmt = NULL;
    cursor->eof = 1;
    *ppCursor = (sqlite3_vtab_cursor *) cursor;
    return SQLITE_OK;
}

static int
vspidx_close (sqlite3_vtab_cursor * pCursor)
{
/* closing the cursor */
    VirtualSpatialIndexCursorPtr cursor =
	(VirtualSpatialIndexCursorPtr) pCursor;
    if (cursor->stmt)
	sqlite3_finalize (cursor->stmt);
    sqlite3_free (pCursor);
    return SQLITE_OK;
}

static int
vspidx_filter (sqlite3_vtab_cursor * pCursor, int idxNum, const char *idxStr,
	       int argc, sqlite3_value ** argv)
{
/* setting up a cursor filter */
    char table_name[1024];
    char geom_column[1024];
    char idx_name[1024];
    char sql[4096];
    gaiaGeomCollPtr geom = NULL;
    int ok_table = 0;
    int ok_geom = 0;
    const unsigned char *blob;
    int size;
    int exists;
    int ret;
    sqlite3_stmt *stmt;
    float minx;
    float miny;
    float maxx;
    float maxy;
    double tic;
    double tic2;
    VirtualSpatialIndexCursorPtr cursor =
	(VirtualSpatialIndexCursorPtr) pCursor;
    VirtualSpatialIndexPtr spidx = (VirtualSpatialIndexPtr) cursor->pVtab;
    if (idxStr)
	idxStr = idxStr;	/* unused arg warning suppression */
    cursor->eof = 1;
    if (idxNum == 1 && argc == 3)
      {
	  /* retrieving the Table/Column/MBR params */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	    {
		strcpy (table_name,
			(const char *) sqlite3_value_text (argv[0]));
		ok_table = 1;
	    }
	  if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	    {
		strcpy (geom_column,
			(const char *) sqlite3_value_text (argv[1]));
		ok_geom = 1;
	    }
	  if (sqlite3_value_type (argv[2]) == SQLITE_BLOB)
	    {
		blob = sqlite3_value_blob (argv[2]);
		size = sqlite3_value_bytes (argv[2]);
		geom = gaiaFromSpatiaLiteBlobWkb (blob, size);
	    }
	  if (ok_table && ok_geom && geom)
	      ;
	  else
	    {
		/* invalid args */
		goto stop;
	    }
      }
    if (idxNum == 2 && argc == 2)
      {
	  /* retrieving the Table/MBR params */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	    {
		strcpy (table_name,
			(const char *) sqlite3_value_text (argv[0]));
		ok_table = 1;
	    }
	  if (sqlite3_value_type (argv[1]) == SQLITE_BLOB)
	    {
		blob = sqlite3_value_blob (argv[1]);
		size = sqlite3_value_bytes (argv[1]);
		geom = gaiaFromSpatiaLiteBlobWkb (blob, size);
	    }
	  if (ok_table && geom)
	      ;
	  else
	    {
		/* invalid args */
		goto stop;
	    }
      }

/* checking if the corresponding R*Tree exists */
    if (ok_geom)
	exists = vspidx_check_rtree (spidx->db, table_name, geom_column);
    else
	exists = vspidx_find_rtree (spidx->db, table_name, geom_column);
    if (!exists)
	goto stop;

/* building the RTree query */
    sprintf (idx_name, "idx_%s_%s", table_name, geom_column);
    vspidx_double_quoted_sql (idx_name);
    sprintf (sql, "SELECT pkid FROM %s WHERE ", idx_name);
    strcat (sql, "xmin <= ? AND xmax >= ? AND ymin <= ? AND ymax >= ?");
    ret = sqlite3_prepare_v2 (spidx->db, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto stop;
/* binding stmt params [MBR] */
    gaiaMbrGeometry (geom);

/* adjusting the MBR so to compensate for DOUBLE/FLOAT truncations */
    minx = (float)(geom->MinX);
    miny = (float)(geom->MinY);
    maxx = (float)(geom->MaxX);
    maxy = (float)(geom->MaxY);
    tic = fabs (geom->MinX - minx);
    tic2 = fabs (geom->MinY - miny);
    if (tic2 > tic)
	tic = tic2;
    tic2 = fabs (geom->MaxX - maxx);
    if (tic2 > tic)
	tic = tic2;
    tic2 = fabs (geom->MaxY - maxy);
    if (tic2 > tic)
	tic = tic2;
    tic *= 2.0;
    sqlite3_bind_double (stmt, 1, geom->MaxX + tic);
    sqlite3_bind_double (stmt, 2, geom->MinX - tic);
    sqlite3_bind_double (stmt, 3, geom->MaxY + tic);
    sqlite3_bind_double (stmt, 4, geom->MinY - tic);
    cursor->stmt = stmt;
    cursor->eof = 0;
/* fetching the first ResultSet's row */
    ret = sqlite3_step (cursor->stmt);
    if (ret == SQLITE_ROW)
	cursor->CurrentRowId = sqlite3_column_int64 (cursor->stmt, 0);
    else
	cursor->eof = 1;
  stop:
    return SQLITE_OK;
}

static int
vspidx_next (sqlite3_vtab_cursor * pCursor)
{
/* fetching a next row from cursor */
    int ret;
    VirtualSpatialIndexCursorPtr cursor =
	(VirtualSpatialIndexCursorPtr) pCursor;
    ret = sqlite3_step (cursor->stmt);
    if (ret == SQLITE_ROW)
	cursor->CurrentRowId = sqlite3_column_int64 (cursor->stmt, 0);
    else
	cursor->eof = 1;
    return SQLITE_OK;
}

static int
vspidx_eof (sqlite3_vtab_cursor * pCursor)
{
/* cursor EOF */
    VirtualSpatialIndexCursorPtr cursor =
	(VirtualSpatialIndexCursorPtr) pCursor;
    return cursor->eof;
}

static int
vspidx_column (sqlite3_vtab_cursor * pCursor, sqlite3_context * pContext,
	       int column)
{
/* fetching value for the Nth column */
    VirtualSpatialIndexCursorPtr cursor =
	(VirtualSpatialIndexCursorPtr) pCursor;
    if (cursor || column)
	cursor = cursor;	/* unused arg warning suppression */
    if (column)
	column = column;	/* unused arg warning suppression */
    sqlite3_result_null (pContext);
    return SQLITE_OK;
}

static int
vspidx_rowid (sqlite3_vtab_cursor * pCursor, sqlite_int64 * pRowid)
{
/* fetching the ROWID */
    VirtualSpatialIndexCursorPtr cursor =
	(VirtualSpatialIndexCursorPtr) pCursor;
    *pRowid = cursor->CurrentRowId;
    return SQLITE_OK;
}

static int
vspidx_update (sqlite3_vtab * pVTab, int argc, sqlite3_value ** argv,
	       sqlite_int64 * pRowid)
{
/* generic update [INSERT / UPDATE / DELETE */
    if (pRowid || argc || argv || pVTab)
	pRowid = pRowid;	/* unused arg warning suppression */
/* read only datasource */
    return SQLITE_READONLY;
}

static int
vspidx_begin (sqlite3_vtab * pVTab)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

static int
vspidx_sync (sqlite3_vtab * pVTab)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

static int
vspidx_commit (sqlite3_vtab * pVTab)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

static int
vspidx_rollback (sqlite3_vtab * pVTab)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

int
sqlite3VirtualSpatialIndexInit (sqlite3 * db)
{
    int rc = SQLITE_OK;
    my_spidx_module.iVersion = 1;
    my_spidx_module.xCreate = &vspidx_create;
    my_spidx_module.xConnect = &vspidx_connect;
    my_spidx_module.xBestIndex = &vspidx_best_index;
    my_spidx_module.xDisconnect = &vspidx_disconnect;
    my_spidx_module.xDestroy = &vspidx_destroy;
    my_spidx_module.xOpen = &vspidx_open;
    my_spidx_module.xClose = &vspidx_close;
    my_spidx_module.xFilter = &vspidx_filter;
    my_spidx_module.xNext = &vspidx_next;
    my_spidx_module.xEof = &vspidx_eof;
    my_spidx_module.xColumn = &vspidx_column;
    my_spidx_module.xRowid = &vspidx_rowid;
    my_spidx_module.xUpdate = &vspidx_update;
    my_spidx_module.xBegin = &vspidx_begin;
    my_spidx_module.xSync = &vspidx_sync;
    my_spidx_module.xCommit = &vspidx_commit;
    my_spidx_module.xRollback = &vspidx_rollback;
    my_spidx_module.xFindFunction = NULL;
    sqlite3_create_module_v2 (db, "VirtualSpatialIndex", &my_spidx_module, NULL,
			      0);
    return rc;
}

int
virtual_spatialindex_extension_init (sqlite3 * db)
{
    return sqlite3VirtualSpatialIndexInit (db);
}
