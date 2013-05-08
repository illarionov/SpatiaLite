/* 
 spatialite.h -- Gaia spatial support for SQLite 
  
 version 4.1, 2013 May 8

 Author: Sandro Furieri a.furieri@lqt.it

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
 
Portions created by the Initial Developer are Copyright (C) 2008-2013
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

/**
 \file spatialite.h

 Main SpatiaLite header file
 */
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifdef _WIN32
#ifdef DLL_EXPORT
#define SPATIALITE_DECLARE __declspec(dllexport)
#else
#define SPATIALITE_DECLARE extern
#endif
#else
#define SPATIALITE_DECLARE __attribute__ ((visibility("default")))
#endif
#endif

#ifndef _SPATIALITE_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _SPATIALITE_H
#endif


#ifdef __cplusplus
extern "C"
{
#endif

#include <spatialite/gaiageo.h>

    typedef struct gaia_wfs_catalog gaiaWFScatalog;
    typedef gaiaWFScatalog *gaiaWFScatalogPtr;

    typedef struct gaia_wfs_item gaiaWFSitem;
    typedef gaiaWFSitem *gaiaWFSitemPtr;

    typedef struct gaia_wfs_schema gaiaWFSschema;
    typedef gaiaWFSschema *gaiaWFSschemaPtr;

    typedef struct gaia_wfs_column gaiaWFScolumn;
    typedef gaiaWFScolumn *gaiaWFScolumnPtr;

/**
 Return the current library version.
 */
    SPATIALITE_DECLARE const char *spatialite_version (void);

/**
 Initializes the internal memory block supporting each connection

 \sa spatialite_init_ex, spatialite_cleanup_ex

 */
    SPATIALITE_DECLARE void *spatialite_alloc_connection (void);

/**
 Initializes the library. 

 This function is now \b DEPRECATED because is not reentrant (not thread safe);
 use spatialite_init_ex() for all new development.

 \param verbose if TRUE a short start-up message is shown on stderr

 \sa spatialite_cleanup, spatialite_init_ex

 \note You absolutely must invoke this function before attempting to perform
 any other SpatiaLite's call.

 */
    SPATIALITE_DECLARE void spatialite_init (int verbose);

/**
 Initializes the library. 

 \param db_handle handle to the current SQLite connection
 \param ptr a memory pointer returned by spatialite_alloc_connection()
 \param verbose if TRUE a short start-up message is shown on stderr

 \sa spatialite_alloc_connection, spatialite_cleanup_ex, spatialite_init

 \note You absolutely must invoke this function before attempting to perform
 any other SpatiaLite's call.

 */
    SPATIALITE_DECLARE void spatialite_init_ex (sqlite3 * db_handle, void *ptr,
						int verbose);

/**
 Initializes the GEOS library. 
 
 \note You are never supposed to invoke this function (internally handled).

 */
    SPATIALITE_DECLARE void spatialite_init_geos (void);

/**
 Cleanup spatialite 

 This function is now \b DEPRECATED; use spatialite_cleanup_ex() for all new development.

 This function performs general cleanup, essentially undoing the effect
 of spatialite_init().

 \sa spatialite_init
*/
    SPATIALITE_DECLARE void spatialite_cleanup (void);

/**
 Cleanup spatialite

 This function performs general cleanup, essentially undoing the effect
 of spatialite_init_ex().

 \param ptr the same memory pointer passed to the corresponding call to
 spatialite_init_ex() and returned by spatialite_alloc_connection()

 \sa spatialite_init_ex, spatialite_alloc_connection
*/
    SPATIALITE_DECLARE void spatialite_cleanup_ex (void *ptr);

/**
 Dumps a full geometry-table into an external Shapefile

 \param sqlite handle to current DB connection
 \param table the name of the table to be exported
 \param column the name of the geometry column
 \param shp_path pathname of the Shapefile to be exported (no suffix) 
 \param charset a valid GNU ICONV charset to be used for DBF text strings
 \param geom_type "POINT", "LINESTRING", "POLYGON", "MULTIPOLYGON" or NULL
 \param verbose if TRUE a short report is shown on stderr
 \param rows on completion will contain the total number of actually exported rows
 \param err_msg on completion will contain an error message (if any)

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int dump_shapefile (sqlite3 * sqlite, char *table,
					   char *column, char *shp_path,
					   char *charset, char *geom_type,
					   int verbose, int *rows,
					   char *err_msg);

/**
 Loads an external Shapefile into a newly created table

 \param sqlite handle to current DB connection
 \param shp_path pathname of the Shapefile to be imported (no suffix) 
 \param table the name of the table to be created
 \param charset a valid GNU ICONV charset to be used for DBF text strings
 \param srid the SRID to be set for Geometries
 \param column the name of the geometry column
 \param coerce2d if TRUE any Geometry will be casted to 2D [XY]
 \param compressed if TRUE compressed Geometries will be created
 \param verbose if TRUE a short report is shown on stderr
 \param spatial_index if TRUE an R*Tree Spatial Index will be created
 \param rows on completion will contain the total number of actually exported rows
 \param err_msg on completion will contain an error message (if any)

 \return 0 on failure, any other value on success

 \sa load_shapefile_ex

 \note this function simply calls load_shapefile_ex by passing 
  implicit gype="AUTO" and pk_column=NULL arguments
 */
    SPATIALITE_DECLARE int load_shapefile (sqlite3 * sqlite, char *shp_path,
					   char *table, char *charset, int srid,
					   char *column, int coerce2d,
					   int compressed, int verbose,
					   int spatial_index, int *rows,
					   char *err_msg);

/**
 Loads an external Shapefile into a newly created table

 \param sqlite handle to current DB connection
 \param shp_path pathname of the Shapefile to be imported (no suffix) 
 \param table the name of the table to be created
 \param charset a valid GNU ICONV charset to be used for DBF text strings
 \param srid the SRID to be set for Geometries
 \param geo_column the name of the geometry column
 \param gtype expected to be one of: "LINESTRING", "LINESTRINGZ", 
  "LINESTRINGM", "LINESTRINGZM", "MULTILINESTRING", "MULTILINESTRINGZ",
  "MULTILINESTRINGM", "MULTILINESTRINGZM", "POLYGON", "POLYGONZ", "POLYGONM", 
  "POLYGONZM", "MULTIPOLYGON", "MULTIPOLYGONZ", "MULTIPOLYGONM", 
  "MULTIPOLYGONZM" or "AUTO".
 \param pk_column name of the Primary Key column; if NULL or mismatching
 then "PK_UID" will be assumed by default.
 \param coerce2d if TRUE any Geometry will be casted to 2D [XY]
 \param compressed if TRUE compressed Geometries will be created
 \param verbose if TRUE a short report is shown on stderr
 \param spatial_index if TRUE an R*Tree Spatial Index will be created
 \param rows on completion will contain the total number of actually exported rows
 \param err_msg on completion will contain an error message (if any)

 \return 0 on failure, any other value on success

 \sa load_shapefile

 \note the Shapefile format doesn't supports any distinction between
  LINESTRINGs and MULTILINESTRINGs, or between POLYGONs and MULTIPOLYGONs;
  as does not allows to clearly distinguish if the M-measure is required.
 \n So a first preliminary scan of the Shapefile is required in order to
  correctly identify the actual payload (gtype = "AUTO", default case).
 \n By explicitly specifying some expected geometry type this first scan
  will be skipped at all thus introducing a noticeable performance gain.
 \n Anyway, declaring a mismatching geometry type will surely cause a failure.
 */
    SPATIALITE_DECLARE int load_shapefile_ex (sqlite3 * sqlite, char *shp_path,
					      char *table, char *charset,
					      int srid, char *geo_column,
					      char *gtype, char *pk_column,
					      int coerce2d, int compressed,
					      int verbose, int spatial_index,
					      int *rows, char *err_msg);

/**
 Loads an external DBF file into a newly created table

 \param sqlite handle to current DB connection
 \param dbf_path pathname of the DBF file to be imported
 \param table the name of the table to be created
 \param charset a valid GNU ICONV charset to be used for DBF text strings
 \param verbose if TRUE a short report is shown on stderr
 \param rows on completion will contain the total number of actually exported rows
 \param err_msg on completion will contain an error message (if any)

 \sa load_dbf_ex

 \note this function simply calls load_dbf_ex by passing an
  implicit pk_column=NULL argument

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int load_dbf (sqlite3 * sqlite, char *dbf_path,
				     char *table, char *charset, int verbose,
				     int *rows, char *err_msg);

/**
 Loads an external DBF file into a newly created table

 \param sqlite handle to current DB connection
 \param dbf_path pathname of the DBF file to be imported
 \param table the name of the table to be created
 \param pk_column name of the Primary Key column; if NULL or mismatching
 then "PK_UID" will be assumed by default.
 \param charset a valid GNU ICONV charset to be used for DBF text strings
 \param verbose if TRUE a short report is shown on stderr
 \param rows on completion will contain the total number of actually exported rows
 \param err_msg on completion will contain an error message (if any)

 \sa load_shapefile

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int load_dbf_ex (sqlite3 * sqlite, char *dbf_path,
					char *table, char *pk_column,
					char *charset, int verbose, int *rows,
					char *err_msg);


/**
 Dumps a full table into an external DBF file

 \param sqlite handle to current DB connection
 \param table the name of the table to be exported
 \param dbf_path pathname of the DBF to be exported 
 \param charset a valid GNU ICONV charset to be used for DBF text strings
 \param err_msg on completion will contain an error message (if any)

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int dump_dbf (sqlite3 * sqlite, char *table,
				     char *dbf_path, char *charset,
				     char *err_msg);

/**
 Loads an external spreadsheet (.xls) file into a newly created table

 \param sqlite handle to current DB connection
 \param path pathname of the spreadsheet file to be imported
 \param table the name of the table to be created
 \param worksheetIndex the index identifying the worksheet to be imported
 \param first_titles if TRUE the first line is assumed to contain column names
 \param rows on completion will contain the total number of actually exported rows
 \param err_msg on completion will contain an error message (if any)

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int load_XL (sqlite3 * sqlite, const char *path,
				    const char *table,
				    unsigned int worksheetIndex,
				    int first_titles, unsigned int *rows,
				    char *err_msg);

/**
 A portable replacement for C99 round()

 \param value a double value

 \return the nearest integeral value
 */
    SPATIALITE_DECLARE double math_round (double value);

/**
 A portable replacement for C99 llabs()

 \param value a 64 bit integer value

 \return the corresponding absolute value
 */
    SPATIALITE_DECLARE sqlite3_int64 math_llabs (sqlite3_int64 value);

/**
 Inserts the inlined EPSG dataset into the "spatial_ref_sys" table

 \param sqlite handle to current DB connection
 \param verbose if TRUE a short report is shown on stderr

 \return 0 on failure, any other value on success

 \sa spatial_ref_sys_init2

 \note this function is internally invoked by the SQL function 
  InitSpatialMetadata(), and is not usually intended for direct use.
  This functions is now deprecated, and will simply call
  spatial_ref_sys_init2(sqlite, GAIA_EPSG_ANY, verbose).
 */
    SPATIALITE_DECLARE int spatial_ref_sys_init (sqlite3 * sqlite, int verbose);

/**
 Inserts the inlined EPSG dataset into the "spatial_ref_sys" table

 \param sqlite handle to current DB connection
 \param mode can be one of GAIA_EPSG_ANY, GAIA_EPSG_NONE or GAIA_EPSG_WGS84_ONLY
 \param verbose if TRUE a short report is shown on stderr

 \return 0 on failure, any other value on success

 \note this function is internally invoked by the SQL function 
  InitSpatialMetadata(), and is not usually intended for direct use.
 */
    SPATIALITE_DECLARE int spatial_ref_sys_init2 (sqlite3 * sqlite, int mode,
						  int verbose);

/**
 Inserts some inlined EPSG definition into the "spatial_ref_sys" table

 \param sqlite handle to current DB connection
 \param srid the SRID value uniquely identifying the required EPSG definition 

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int insert_epsg_srid (sqlite3 * sqlite, int srid);

/**
 Checks if a column is actually defined into the given table

 \param sqlite handle to current DB connection
 \param table the table to be checked
 \param column the column to be checked

 \return 0 on success, any other value on success

 \note internally used to detect if some KML attribute defaults to a constant value
 */
    SPATIALITE_DECLARE int
	is_kml_constant (sqlite3 * sqlite, char *table, char *column);

/**
 Dumps a full geometry-table into an external KML file

 \param sqlite handle to current DB connection
 \param table the name of the table to be exported
 \param geom_col the name of the geometry column
 \param kml_path pathname of the KML file to be exported 
 \param name_col column to be used for KML "name" (may be null)
 \param desc_col column to be used for KML "description" (may be null)
 \param precision number of decimal digits for coordinates

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int dump_kml (sqlite3 * sqlite, char *table,
				     char *geom_col, char *kml_path,
				     char *name_col, char *desc_col,
				     int precision);

/**
 Checks for duplicated rows into the same table

 \param sqlite handle to current DB connection
 \param table name of the table to be checked
 \param dupl_count on completion will contain the number of duplicated rows found

 \sa remove_duplicated_rows
 \note two (or more) rows are assumed to be duplicated if any column

 value (excluding any Primary Key column) is exacly the same
 */
    SPATIALITE_DECLARE void check_duplicated_rows (sqlite3 * sqlite,
						   char *table,
						   int *dupl_count);

/**
 Remove duplicated rows from a table

 \param sqlite handle to current DB connection
 \param table name of the table to be cleaned

 \sa check_duplicated_rows

 \note when two (or more) duplicated rows exist, only the first occurence
 will be preserved, then deleting any further occurrence.
 */
    SPATIALITE_DECLARE void remove_duplicated_rows (sqlite3 * sqlite,
						    char *table);

/**
 Creates a derived table surely containing elementary Geometries

 \param sqlite handle to current DB connection
 \param inTable name of the input table 
 \param geometry name of the Geometry column
 \param outTable name of the output table to be created
 \param pKey name of the Primary Key column in the output table
 \param multiId name of the column identifying origins in the output table

 \note if the input table contains some kind of complex Geometry
 (MULTIPOINT, MULTILINESTRING, MULTIPOLYGON or GEOMETRYCOLLECTION),
 then many rows are inserted into the output table: each single 
 row will contain the same attributes and an elementaty Geometry.
 All the rows created by expanding the same input row will expose
 the same value in the "multiId" column.
 */
    SPATIALITE_DECLARE void elementary_geometries (sqlite3 * sqlite,
						   char *inTable,
						   char *geometry,
						   char *outTable, char *pKey,
						   char *multiId);

/**
 Dumps a full geometry-table into an external GeoJSON file

 \param sqlite handle to current DB connection
 \param table the name of the table to be exported
 \param geom_col the name of the geometry column
 \param outfile_path pathname for the GeoJSON file to be written to
 \param precision number of decimal digits for coordinates
 \param option the format to use for output

 \note valid values for option are:
   - 0 no option
   - 1 GeoJSON MBR
   - 2 GeoJSON Short CRS (e.g EPSG:4326)
   - 3 MBR + Short CRS
   - 4 GeoJSON Long CRS (e.g urn:ogc:def:crs:EPSG::4326)
   - 5 MBR + Long CRS

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int dump_geojson (sqlite3 * sqlite, char *table,
					 char *geom_col, char *outfile_path,
					 int precision, int option);

/**
 Updates the LAYER_STATICS metadata table

 \param sqlite handle to current DB connection
 \param table name of the table to be processed
 \param column name of the geometry to be processed

 \note this function will explore the given table/geometry determining
 the number of rows and the full layer extent; a corresponding table/geometry
 entry is expected to be already declared in the GEOMETRY_COLUMNS table.
 These informations will be permanently stored into the LAYER_STATISTICS
 table; if such table does not yet exists will be implicitly created.
   - if table is NULL, any entry found within GEOMETRY_COLUMNS
     will be processed.
   - if table is not NULL and column is NULL, any geometry
     belonging to the given table will be processed.
   - if both table and column are not NULL, then only the
     given entry will be processed.

 \return 0 on failure, the total count of processed entries on success
 */
    SPATIALITE_DECLARE int update_layer_statistics (sqlite3 * sqlite,
						    const char *table,
						    const char *column);

/**
 Queries the Metadata tables returning the Layer Full Extent

 \param handle SQLite handle to current DB connection.
 \param table VectorLayer Table (or View, or VirtualShape).
 \param geometry Geometry Column name.
 \param mode if TRUE a PESSIMISTIC statistics update will be implied,
  otherwise OPTIMISTIC.
 
 \return the pointer to the newly created Geometry (Envelope): NULL on failure

 \note you are responsible to destroy (before or after) any allocated
 Geometry returned by gaiaGetLayerExtent().
 \n The geometry arg is optional when the table simply has a single Geometry Column,
  and can be NULL in this case.
 \n When the mode arg is set to FALSE (default) then the returned infos
  will be simply retrieved from the staticized statistic tables (faster, but could be inaccurate).
 \n If the mode arg is set to TRUE a preliminary attempt to update the
  statistic tables will be always performed (probably slower, but surely accurate).
 \n If the named Layer doesn't exist, or if it's completely empty (not containing
 any valid Geometry) NULL will be returned.
 */
    SPATIALITE_DECLARE gaiaGeomCollPtr gaiaGetLayerExtent (sqlite3 * handle,
							   const char *table,
							   const char *geometry,
							   int mode);

/**
 Queries the Metadata tables supporting Vector Layers

 \param handle SQLite handle to current DB connection.
 \param table VectorLayer Table (or View, or VirtualShape).
 \param geometry Geometry Column name.
 \param mode one of GAIA_VECTORS_LIST_LOOSE or GAIA_VECTORS_LIST_STRICT.
 
 \return the pointer to the newly created VectorLayersList object: NULL on failure

 \sa gaiaFreeVectorLayersList

 \note you are responsible to destroy (before or after) any allocated
 VectorLayersList returned by gaiaGetVectorLayersList().
 \n If the table arg is NULL all VectorLayers defined within the DB will be reported;
  otherwise only a single Layer will be reported (if existing).
 \n By defining the geometry arg (not NULL) you can further restrict the returned report.
 \n When the mode arg is set to GAIA_VECTORS_LIST_FAST (default) then the returned infos
  will be simply retrieved from the staticized statistic tables (faster, but could be inaccurate).
 \n If the mode arg is set to GAIA_VECTORS_LIST_PRECISE a preliminary attempt to update the
  statistic tables will be always performed (probably slower, but surely accurate).
 */
    SPATIALITE_DECLARE gaiaVectorLayersListPtr gaiaGetVectorLayersList (sqlite3
									*
									handle,
									const
									char
									*table,
									const
									char
									*geometry,
									int
									mode);

/**
 Destroys a VectorLayersList object

 \param ptr pointer to the VectorLayersList object to be destroyed

 \sa gaiaGetVectorLayersList
 */
    SPATIALITE_DECLARE void gaiaFreeVectorLayersList (gaiaVectorLayersListPtr
						      ptr);

/**
 Drops a layer-table, removing any related dependency

 \param sqlite handle to current DB connection
 \param table name of the table to be removed

 \note this function will drop a SpatialTable, SpatialView or VirtualShape being
 properly registered within the Metadata tables.
 \n an eventual Spatial Index will be dropped as well, and any row referring the
 selected table will be removed from the Metadata tables.

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int gaiaDropTable (sqlite3 * sqlite, const char *table);

/**
 Checks a Geometry Column for validity

 \param sqlite handle to current DB connection
 \param table name of the table 
 \param geometry name of the column to be checked
 \param report_path pathname of the report-file
 \param n_rows if this variable is not NULL on successful completion will
 contain the total number of rows found into the checkeck table
 \param n_invalids if this variable is not NULL on successful completion will
 contain the total number of invalid Geometries found into the checkeck table
 \param err_msg if this variable is not NULL and the return status is ZERO
 (failure), an appropriate error message will be returned

 \note this function will check a Geometry Column (layer) for validity;
 a HTML report will be produced.
 \n an eventual error message returned via err_msg requires to be deallocated
 by invoking free()

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int check_geometry_column (sqlite3 * sqlite,
						  const char *table,
						  const char *geom,
						  const char *report_path,
						  int *n_rows, int *n_invalids,
						  char **err_msg);

/**
 Checks all Geometry Columns for validity

 \param sqlite handle to current DB connection
 \param output_dir pathname of the directory to be created for report-files
 \param n_invalids if this variable is not NULL on successful completion will
 contain the total number of invalid Geometries found
 \param err_msg if this variable is not NULL and the return status is ZERO
 (failure), an appropriate error message will be returned

 \note this function will check all Geometry Columns (vector layers) for validity;
 a HTML report will be produced.
 \n an eventual error message returned via err_msg requires to be deallocated
 by invoking free()

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int check_all_geometry_columns (sqlite3 * sqlite,
						       const char *output_dir,
						       int *n_invalids,
						       char **err_msg);

/**
 Sanitizes a Geometry Column making all invalid geometries to be valid

 \param sqlite handle to current DB connection
 \param table name of the table 
 \param geometry name of the column to be checked
 \param tmp_table name of the temporary table
 \param report_path pathname of the report-file
 \param n_invalids if this variable is not NULL on successful completion will
 contain the total number of invalid Geometries found into the sanitize table
 \param n_repaired if this variable is not NULL on successful completion will
 contain the total number of repaired Geometries
 \param n_discarded if this variable is not NULL on successful completion will
 contain the total number of repaired Geometries (by discarding fragments)
 \param n_failures if this variable is not NULL on successful completion will
 contain the total number of repair failures (i.e. Geometries beyond possible repair)
 \param err_msg if this variable is not NULL and the return status is ZERO
 (failure), an appropriate error message will be returned

 \note this function will attempt to make valid all invalid geometries
 found within a Geometry Column (layer); a temporary table is required.
 \n if the process has full success the temprary table will be deleted;
 otherwise it will be preserved for further inspection.
 a HTML report will be produced as well.
 \n an eventual error message returned via err_msg requires to be deallocated
 by invoking free()

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int sanitize_geometry_column (sqlite3 * sqlite,
						     const char *table,
						     const char *geom,
						     const char *tmp_table,
						     const char *report_path,
						     int *n_invalids,
						     int *n_repaired,
						     int *n_discarded,
						     int *n_failures,
						     char **err_msg);

/**
 Sanitizes all Geometry Columns making all invalid geometries to be valid

 \param sqlite handle to current DB connection
 \param tmp_prefix name-prefix for temporary tables
 \param output_dir pathname of the directory to be created for report-files
 \param not_repaired if this variable is not NULL on successful completion will
 contain the total count of repair failures (i.e. Geometries beyond possible repair)
 \param err_msg if this variable is not NULL and the return status is ZERO
 (failure), an appropriate error message will be returned

 \note this function will attempt to make valid all invalid geometries
 found within all Geometry Columns (vector layers); a temporary table is 
 required so to support each input table.
 \n if the process has full success the temprary table will be deleted;
 otherwise it will be preserved for further inspection.
 a HTML report will be produced as well.
 \n an eventual error message returned via err_msg requires to be deallocated
 by invoking free()

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int sanitize_all_geometry_columns (sqlite3 * sqlite,
							  const char
							  *tmp_prefix,
							  const char
							  *output_dir,
							  int *not_repaired,
							  char **err_msg);

/**
 Loads data from some WFS source 

 \param sqlite handle to current DB connection
 \param path_or_url pointer to some WFS-GetFeature XML Document (could be a pathname or an URL).
 \param layer_name the name of the WFS layer. 
 \param swap_axes if TRUE the X and Y axes will be swapped 
 \param table the name of the table to be created
 \param pk_column name of the Primary Key column; if NULL or mismatching
 then "PK_UID" will be assumed by default.
 \param spatial_index if TRUE an R*Tree Spatial Index will be created
 \param rows on completion will contain the total number of actually imported rows
 \param err_msg on completion will contain an error message (if any)
 \param progress_callback pointer to a callback function to be invoked immediately
 after processing each WFS page (could be NULL)
 \param callback_ptr an arbitrary pointer (to be passed as the second argument
 by the callback function).
 
 \sa create_wfs_catalog, load_from_wfs_paged

 \return 0 on failure, any other value on success
 
 \note an eventual error message returned via err_msg requires to be deallocated
 by invoking free()
 \n please note: this one simply is a convenience method, and exactly corresponds
 to load_from_wfs_paged() setting a negative page size. 
 */
    SPATIALITE_DECLARE int load_from_wfs (sqlite3 * sqlite, char *path_or_url,
					  char *layer_name, int swap_axes,
					  char *table, char *pk_column_name,
					  int spatial_index, int *rows,
					  char **err_msg,
					  void (*progress_callback) (int,
								     void *),
					  void *callback_ptr);

/**
 Loads data from some WFS source (using WFS paging)

 \param sqlite handle to current DB connection
 \param path_or_url pointer to some WFS-GetFeature XML Document (could be a pathname or an URL).
 \param layer_name the name of the WFS layer.
 \param swap_axes if TRUE the X and Y axes will be swapped 
 \param table the name of the table to be created
 \param pk_column name of the Primary Key column; if NULL or mismatching
 then "PK_UID" will be assumed by default.
 \param spatial_index if TRUE an R*Tree Spatial Index will be created
 \param page_size max number of features for each single WFS call; if zero or
 negative a single monolithic page is assumed (i.e. paging will not be applied).
 \param rows on completion will contain the total number of actually imported rows
 \param err_msg on completion will contain an error message (if any)
 \param progress_callback pointer to a callback function to be invoked immediately
 after processing each WFS page (could be NULL)
 \param callback_ptr an arbitrary pointer (to be passed as the second argument
 by the callback function).
 
 \sa create_wfs_catalog, load_from_wfs

 \return 0 on failure, any other value on success
 
 \note an eventual error message returned via err_msg requires to be deallocated
 by invoking free()

 \note the progress_callback function must have this signature: 
 \b void \b myfunct(\b int \b count, \b void \b *ptr);
 \n and will cyclically report how many features have been processed since the initial call start.
 */
    SPATIALITE_DECLARE int load_from_wfs_paged (sqlite3 * sqlite,
						char *path_or_url,
						char *layer_name, int swap_axes,
						char *table,
						char *pk_column_name,
						int spatial_index,
						int page_size, int *rows,
						char **err_msg,
						void (*progress_callback) (int,
									   void
									   *),
						void *callback_ptr);

/**
 Creates a Catalog for some WFS service 

 \param path_or_url pointer to some WFS-GetCapabilities XML Document (could be a pathname or an URL). 
 \param err_msg on completion will contain an error message (if any)

 \return the pointer to the corresponding WFS-Catalog object: NULL on failure
 
 \sa destroy_wfs_catalog, get_wfs_catalog_count, get_wfs_catalog_item, load_from_wfs
 
 \note an eventual error message returned via err_msg requires to be deallocated
 by invoking free().\n
 you are responsible to destroy (before or after) any WFS-Catalog returned by create_wfs_catalog().
 */
    SPATIALITE_DECLARE gaiaWFScatalogPtr create_wfs_catalog (char *path_or_url,
							     char **err_msg);

/**
 Destroys a WFS-Catalog object freeing any allocated resource 

 \param handle the pointer to a valid WFS-Catalog returned by a previous call
 to create_wfs_catalog()
 
 \sa create_wfs_catalog
 */
    SPATIALITE_DECLARE void destroy_wfs_catalog (gaiaWFScatalogPtr handle);

/**
 Return the base URL for any WFS-GetFeature call

 \param handle the pointer to a valid WFS-Item returned by a previous call
 to get_wfs_catalog_item().

 \return the base URL for any WFS-GetFeature call: NULL is undefined
 
 \sa create_wfs_catalog, get_wfs_base_describe_url, get_wfs_request_url
 */
    SPATIALITE_DECLARE const char *get_wfs_base_request_url (gaiaWFScatalogPtr
							     handle);

/**
 Return the base URL for any WFS-DescribeFeatureType call

 \param handle the pointer to a valid WFS-Item returned by a previous call
 to get_wfs_catalog_item().

 \return the base URL for any WFS-DescribeFeatureType call: NULL is undefined
 
 \sa create_wfs_catalog, get_wfs_base_request_url, get_wfs_describe_url
 */
    SPATIALITE_DECLARE const char *get_wfs_base_describe_url (gaiaWFScatalogPtr
							      handle);

/**
 Return a GetFeature URL (GET)

 \param handle the pointer to a valid WFS-Item returned by a previous call
 to get_wfs_catalog_item().
 \param name the NAME uniquely identifying the required WFS layer.
 \param version could be "1.0.0" or "1.1.0"; if NULL or invalid "1.1.0"
 will be assumed.
 \param srid the preferred SRS to be used for WFS geometries; if negative
 or mismatching will be simply ignored.
 \param max_features the WFS MAXFEATURES argument; any negative or zero
 value will be ignored.

 \return the GetFeature URL: NULL if any error is found.
 
 \sa get_wfs_base_request_url, get_wfs_describe_url

 \note you are responsible to destroy (before or after) any allocated
 memory returned by get_wfs_request_url().
 */
    SPATIALITE_DECLARE char *get_wfs_request_url (gaiaWFScatalogPtr handle,
						  const char *name,
						  const char *version,
						  int srid, int max_features);

/**
 Return a DescribeFeatureType URL (GET)

 \param handle the pointer to a valid WFS-Item returned by a previous call
 to get_wfs_catalog_item().
 \param name the NAME uniquely identifying the required WFS layer.
 \param version could be "1.0.0" or "1.1.0"; if NULL or invalid "1.1.0"
 will be assumed.

 \return the DescribeFeatureType URL: NULL if any error is found.
 
 \sa get_wfs_base_describe_url, get_wfs_request_url

 \note you are responsible to destroy (before or after) any allocated
 memory returned by get_wfs_describe_url().
 */
    SPATIALITE_DECLARE char *get_wfs_describe_url (gaiaWFScatalogPtr handle,
						   const char *name,
						   const char *version);

/**
 Return the total count of items (aka Layers) defined within a WFS-Catalog object

 \param handle the pointer to a valid WFS-Catalog returned by a previous call
 to create_wfs_catalog()

 \return the total count of items (aka Layers) defined within a WFS-Catalog object: 
 a negative number if the WFS-Catalog isn't valid
 
 \sa create_wfs_catalog, get_wfs_catalog_item
 */
    SPATIALITE_DECLARE int get_wfs_catalog_count (gaiaWFScatalogPtr handle);

/**
 Return the pointer to some specific Layer defined within a WFS-Catalog object

 \param handle the pointer to a valid WFS-Catalog returned by a previous call
 to create_wfs_catalog()
 \param index the relative index identifying the required WFS-Layer (the first 
 Item in the WFS-Catalaog object has index ZERO).

 \return the pointer to the required WFS-Layer object: NULL if the passed index
 isn't valid
 
 \sa create_wfs_catalog, get_wfs_catalog_count, get_wfs_item_name, get_wfs_item_title, 
 get_wfs_item_abstract, get_wfs_layer_srid_count, get_wfs_layer_srid, 
 get_wfs_keyword_count, get_wfs_keyword
 */
    SPATIALITE_DECLARE gaiaWFSitemPtr get_wfs_catalog_item (gaiaWFScatalogPtr
							    handle, int index);

/**
 Return the name corresponding to some WFS-Item (aka Layer) object

 \param handle the pointer to a valid WFS-Item returned by a previous call
 to get_wfs_catalog_item().

 \return the name corresponding to the WFS-Layer object
 
 \sa get_wfs_layer_title, get_wfs_layer_abstract, get_wfs_layer_srid_count, get_wfs_layer_srid, 
 get_wfs_keyword_count, get_wfs_keyword
 */
    SPATIALITE_DECLARE const char *get_wfs_item_name (gaiaWFSitemPtr handle);

/**
 Return the title corresponding to some WFS-Item (aka Layer) object

 \param handle the pointer to a valid WFS-Item returned by a previous call
 to get_wfs_catalog_item().

 \return the title corresponding to the WFS-Layer object
 
 \sa get_wfs_item_name, get_wfs_item_abstract, get_wfs_layer_srid_count, get_wfs_layer_srid, 
 get_wfs_keyword_count, get_wfs_keyword
 */
    SPATIALITE_DECLARE const char *get_wfs_item_title (gaiaWFSitemPtr handle);

/**
 Return the abstract corresponding to some WFS-Item (aka Layer) object

 \param handle the pointer to a valid WFS-Item returned by a previous call
 to get_wfs_catalog_item().

 \return the abstract corresponding to the WFS-Layer object
 
 \sa get_wfs_item_name, get_wfs_item_title, get_wfs_layer_srid_count, get_wfs_layer_srid, 
 get_wfs_keyword_count, get_wfs_keyword
 */
    SPATIALITE_DECLARE const char *get_wfs_item_abstract (gaiaWFSitemPtr
							  handle);

/**
 Return the total count of SRIDs supported by a WFS-Item object

 \param handle the pointer to a valid WFS-Item returned by a previous call
 to get_wfs_catalog_item().

 \return the total count of SRIDs supported by a WFS-Item object: 
 a negative number if the WFS-Item isn't valid
 
 \sa get_wfs_item_name, get_wfs_item_title, get_wfs_item_abstract, 
 get_wfs_layer_srid, get_wfs_keyword_count, get_wfs_keyword
 */
    SPATIALITE_DECLARE int get_wfs_layer_srid_count (gaiaWFSitemPtr handle);

/**
 Return one of the SRIDs supported by a WFS-Item object

 \param handle the pointer to a valid WFS-Item returned by a previous call
 to get_wfs_catalog_item().
 \param index the relative index identifying the required SRID (the first 
 SRID value supported by a WFS-Item object has index ZERO).

 \return the SRID-value: a negative number if the required SRID-value isn't defined.
 
 \sa get_wfs_item_name, get_wfs_item_title, get_wfs_item_abstract, 
 get_wfs_layer_srid_count, get_wfs_keyword_count, get_wfs_keyword
 */
    SPATIALITE_DECLARE int get_wfs_layer_srid (gaiaWFSitemPtr handle,
					       int index);

/**
 Return the total count of Keywords associated to a WFS-Item object

 \param handle the pointer to a valid WFS-Item returned by a previous call
 to get_wfs_catalog_item().

 \return the total count of Keyword associated to a WFS-Item object: 
 a negative number if the WFS-Item isn't valid
 
 \sa get_wfs_item_name, get_wfs_item_title, get_wfs_item_abstract, 
 get_wfs_layer_srid_count, get_wfs_layer_srid, get_wfs_layer_keyword
 */
    SPATIALITE_DECLARE int get_wfs_keyword_count (gaiaWFSitemPtr handle);

/**
 Return one of the Keywords supported by a WFS-Item object

 \param handle the pointer to a valid WFS-Item returned by a previous call
 to get_wfs_catalog_item().
 \param index the relative index identifying the required Keyword (the first 
 Keyword associated to a WFS-Item object has index ZERO).

 \return the Keyword value: NULL if the required Keyword isn't defined.
 
 \sa get_wfs_item_name, get_wfs_item_title, get_wfs_item_abstract, 
 get_wfs_layer_srid_count, get_wfs_layer_srid, get_wfs_layer_keyword
 */
    SPATIALITE_DECLARE const char *get_wfs_keyword (gaiaWFSitemPtr handle,
						    int index);

/**
 Creates a Schema representing some WFS Layer 

 \param path_or_url pointer to some WFS-DescribeFeatureType XML Document (could be a pathname or an URL). 
 \param err_msg on completion will contain an error message (if any)

 \return the pointer to the corresponding WFS-Schema object: NULL on failure
 
 \sa destroy_wfs_schema,get_wfs_schema_column_count, get_wfs_schema_column_info,
 get_wfs_schema_geometry_info
 
 \note an eventual error message returned via err_msg requires to be deallocated
 by invoking free().\n
 you are responsible to destroy (before or after) any WFS-Schema returned by create_wfs_schema().
 */
    SPATIALITE_DECLARE gaiaWFSschemaPtr create_wfs_schema (char *path_or_url,
							   char *layer_name,
							   char **err_msg);

/**
 Destroys a WFS-schema object freeing any allocated resource 

 \param handle the pointer to a valid WFS-Catalog returned by a previous call
 to create_wfs_schema()
 
 \sa create_wfs_schema
 */
    SPATIALITE_DECLARE void destroy_wfs_schema (gaiaWFSschemaPtr handle);

/**
 Return the infos describing some WFS-GeometryColumn object

 \param handle the pointer to a valid WFS-Schema returned by a previous call
 to create_wfs_schema().
 \param name on completion will contain a pointer to the GeometryColumn name
 \param type on completion will contain the GeometryType set for the Column;
 could be one of GAIA_POINT, GAIA_LINESTRING, GAIA_POLYGON, GAIA_MULTIPOINT,
 GAIA_MULTILINESTRING, GAIA_MULTIPOLYGON or GAIA_GEOMETRYCOLLECTION
 \param srid on completion will contain the SRID-value set for the GeometryColumn
 \param dims on completion will contain the dimensions (2 or 3) set
 for the GeometryColumn
 \param nullable on completion will contain a Boolean value; if TRUE
 the Column may contain NULL-values.

 \return TRUE on success, FALSE if any error is encountered or if
 the WFS-Schema hasn't any Geometry-Column defined.
 
 \sa create_wfs_schema, get_wfs_schema_column_count, get_wfs_schema_column,
 get_wfs_schema_column_info
 */
    SPATIALITE_DECLARE int get_wfs_schema_geometry_info (gaiaWFSschemaPtr
							 handle,
							 const char **name,
							 int *type, int *srid,
							 int *dims,
							 int *nullable);

/**
 Return the total count of items (aka Columns) defined within a WFS-Schema object

 \param handle the pointer to a valid WFS-Schema returned by a previous call
 to create_wfs_schema()

 \return the total count of items (aka Columns) defined within a WFS-Schema object: 
 a negative number if the WFS-Schema isn't valid
 
 \sa create_wfs_schema, get_wfs_schema_geometry_info, 
 get_wfs_schema_column, get_wfs_schema_column_info 
 */
    SPATIALITE_DECLARE int get_wfs_schema_column_count (gaiaWFSschemaPtr
							handle);

/**
 Return the pointer to some specific Column defined within a WFS-Schema object

 \param handle the pointer to a valid WFS-Schema returned by a previous call
 to create_wfs_schema()
 \param index the relative index identifying the required WFS-Column (the first 
 Item in the WFS-Schema object has index ZERO).

 \return the pointer to the required WFS-Column object: NULL if the passed index
 isn't valid
 
 \sa create_wfs_schema, get_wfs_schema_geometry_info, 
 get_wfs_schema_column_count, get_wfs_schema_column_info
 */
    SPATIALITE_DECLARE gaiaWFScolumnPtr get_wfs_schema_column (gaiaWFSschemaPtr
							       handle,
							       int index);

/**
 Return the infos describing some WFS-Column object

 \param handle the pointer to a valid WFS-Column returned by a previous call
 to get_wfs_schema_column().
 \param name on completion will contain a pointer to the Column name
 \param type on completion will contain the datatype set for the Column;
 could be one of SQLITE_TEXT, SQLITE_INTEGER or SQLITE_FLOAT
 \param nullable on completion will contain a Boolean value; if TRUE
 the Column may contain NULL-values.

 \return TRUE on success, FALSE if any error is encountered
 
 \sa get_wfs_schema_column, get_wfs_schema_geometry_info
 */
    SPATIALITE_DECLARE int get_wfs_schema_column_info (gaiaWFScolumnPtr handle,
						       const char **name,
						       int *type,
						       int *nullable);

#ifdef __cplusplus
}
#endif

#endif				/* _SPATIALITE_H */
