/* 

demo5.c

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
#include <time.h>

/*
these headers are required in order to support
SQLite/SpatiaLite
*/
#include <sqlite3.h>
#include <spatialite/gaiageo.h>
#include <spatialite.h>

static void
do_print_list (gaiaVectorLayersListPtr list, const char *mode)
{
/* prints the layers list */
    gaiaVectorLayerPtr lyr;
    gaiaLayerAttributeFieldPtr fld;

    printf ("\n****** VectorLayersList (mode=%s) *********\n", mode);
    if (list == NULL)
      {
	  printf ("The VectorLayersList is empty !!!\n\n");
	  return;
      }

    lyr = list->First;
    while (lyr)
      {
	  /* printing the Layer Header */
	  const char *lyr_type = "UnknownType";
	  const char *geom_type = "UnknownType";
	  const char *dims = "UnknownDims";
	  switch (lyr->LayerType)
	    {
	    case GAIA_VECTOR_TABLE:
		lyr_type = "BasedOnSqlTable";
		break;
	    case GAIA_VECTOR_VIEW:
		lyr_type = "BasedOnSqlView";
		break;
	    case GAIA_VECTOR_VIRTUAL:
		lyr_type = "BasedOnVirtualShape";
		break;
	    };
	  switch (lyr->GeometryType)
	    {
	    case GAIA_VECTOR_GEOMETRY:
		geom_type = "GEOMETRY";
		break;
	    case GAIA_VECTOR_POINT:
		geom_type = "POINT";
		break;
	    case GAIA_VECTOR_LINESTRING:
		geom_type = "LINESTRING";
		break;
	    case GAIA_VECTOR_POLYGON:
		geom_type = "POLYGON";
		break;
	    case GAIA_VECTOR_MULTIPOINT:
		geom_type = "MULTIPOINT";
		break;
	    case GAIA_VECTOR_MULTILINESTRING:
		geom_type = "MULTILINESTRING";
		break;
	    case GAIA_VECTOR_MULTIPOLYGON:
		geom_type = "MULTIPOLYGON";
		break;
	    case GAIA_VECTOR_GEOMETRYCOLLECTION:
		geom_type = "GEOMETRYCOLLECTION";
		break;
	    };
	  switch (lyr->Dimensions)
	    {
	    case GAIA_XY:
		dims = "XY";
		break;
	    case GAIA_XY_Z:
		dims = "XYZ";
		break;
	    case GAIA_XY_M:
		dims = "XYM";
		break;
	    case GAIA_XY_Z_M:
		dims = "XYXM";
		break;
	    };
	  printf ("VectorLayer: Type=%s TableName=%s\n", lyr_type,
		  lyr->TableName);
	  printf ("\tGeometryName=%s SRID=%d GeometryType=%s Dims=%s\n",
		  lyr->GeometryName, lyr->Srid, geom_type, dims);
	  if (lyr->ExtentInfos)
	    {
		printf ("\tRowCount=%d\n", lyr->ExtentInfos->Count);
		printf ("\tExtentMin %f / %f\n\tExtentMax %f / %f\n",
			lyr->ExtentInfos->MinX,
			lyr->ExtentInfos->MinY, lyr->ExtentInfos->MaxX,
			lyr->ExtentInfos->MaxY);
	    }
	  if (lyr->AuthInfos)
	      printf ("\tReadOnly=%s Hidden=%s\n",
		      (lyr->AuthInfos->IsReadOnly == 0) ? "FALSE" : "TRUE",
		      (lyr->AuthInfos->IsHidden == 0) ? "FALSE" : "TRUE");
	  fld = lyr->First;
	  while (fld)
	    {
		/* printing AttributeFields infos */
		printf ("\t\tField #%d) FieldName=%s\n", fld->Ordinal,
			fld->AttributeFieldName);
		printf ("\t\t\t");
		if (fld->NullValuesCount)
		    printf ("NullValues=%d ", fld->NullValuesCount);
		if (fld->IntegerValuesCount)
		    printf ("IntegerValues=%d ", fld->IntegerValuesCount);
		if (fld->DoubleValuesCount)
		    printf ("DoubleValues=%d ", fld->DoubleValuesCount);
		if (fld->TextValuesCount)
		    printf ("TextValues=%d ", fld->TextValuesCount);
		if (fld->BlobValuesCount)
		    printf ("BlobValues=%d ", fld->BlobValuesCount);
		printf ("\n");
		if (fld->MaxSize)
		    printf ("\t\t\tMaxSize/Length=%d\n", fld->MaxSize->MaxSize);
		if (fld->IntRange)
#if defined(_WIN32) || defined(__MINGW32__)
/* CAVEAT: M$ runtime doesn't supports %lld for 64 bits */
		    printf ("\t\t\tIntRange %I64d / %I64d\n",
#else
		    printf ("\t\t\tIntRange %lld / %lld\n",
#endif
			    fld->IntRange->MinValue, fld->IntRange->MaxValue);
		if (fld->DoubleRange)
		    printf ("\t\t\tDoubleRange %f / %f\n",
			    fld->DoubleRange->MinValue,
			    fld->DoubleRange->MaxValue);
		fld = fld->Next;
	    }
	  lyr = lyr->Next;
      }
    printf ("\n");
}

int
main (int argc, char *argv[])
{
    int ret;
    sqlite3 *handle;
    const char *table = NULL;
    const char *geometry = NULL;
    gaiaVectorLayersListPtr list;

    if (argc < 2)
      {
	  fprintf (stderr,
		   "usage: %s test_db_path [table_name [geometry_column]]\n",
		   argv[0]);
	  return -1;
      }
    if (argc >= 3)
	table = argv[2];
    if (argc >= 4)
	geometry = argv[3];

/* 
VERY IMPORTANT: 
you must initialize the SpatiaLite extension [and related]
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
- this demo is intended to create an existing, already populated database
*/
    ret = sqlite3_open_v2 (argv[1], &handle,
			   SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  printf ("cannot open '%s': %s\n", argv[1], sqlite3_errmsg (handle));
	  sqlite3_close (handle);
	  return -1;
      }

/* listing layers: FAST mode, not yet updated !!! */
    list =
	gaiaGetVectorLayersList (handle, table, geometry,
				 GAIA_VECTORS_LIST_FAST);
    do_print_list (list, "FAST");
    gaiaFreeVectorLayersList (list);

/* listing layers: PRECISE mode, actually updating before listing !!! */
    list =
	gaiaGetVectorLayersList (handle, table, geometry,
				 GAIA_VECTORS_LIST_PRECISE);
    do_print_list (list, "PRECISE");
    gaiaFreeVectorLayersList (list);

/* disconnecting the test DB */
    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK)
      {
	  printf ("close() error: %s\n", sqlite3_errmsg (handle));
	  return -1;
      }
    spatialite_cleanup ();
    printf ("\n\nsample successfully terminated\n");
    return 0;
}
