/*
 gg_dxf.h -- Gaia common support for DXF files
  
 version 4.1, 2013 May 14

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
 \file gg_dxf.h

 Geometry handling functions: DXF files
 */

#ifndef _GG_DXF_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GG_DXF_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/* constant values for DXF */

/** import distinct layers */
#define GAIA_DXF_IMPORT_BY_LAYER	1
/** import layers mixed altogether by type */
#define GAIA_DXF_IMPORT_MIXED		2
/** auto-selects 2D or 3D */
#define GAIA_DXF_AUTO_2D_3D		3
/** always force 2D */
#define GAIA_DXF_FORCE_2D		4
/** always force 3D */
#define GAIA_DXF_FORCE_3D		5
/** don't apply any special Ring handling */
#define GAIA_DXF_RING_NONE		6
/** apply special "linked rings" handling */
#define GAIA_DXF_RING_LINKED		7
/** apply special "unlinked rings" handling */
#define GAIA_DXF_RING_UNLINKED		8


/* data structs */


/**
 wrapper for DXF Extra Attribute object
 */
    typedef struct gaia_dxf_extra_attr
    {
/** pointer to Extra Attribute Key value */
	char *key;
/** pointer to Extra Attribute Value string */
	char *value;
/** pointer to next item [linked list] */
	struct gaia_dxf_extra_attr *next;
    } gaiaDxfExtraAttr;
/**
 Typedef for DXF Extra Attribute object

 \sa gaiaDxfExtraAttr
 */
    typedef gaiaDxfExtraAttr *gaiaDxfExtraAttrPtr;

/**
 wrapper for DXF Text object
 */
    typedef struct gaia_dxf_text
    {
/** pointer to Label string */
	char *label;
/** X coordinate */
	double x;
/** Y coordinate */
	double y;
/** Z coordinate */
	double z;
/** label rotation angle */
	double angle;
/** pointer to first Extra Attribute [linked list] */
	gaiaDxfExtraAttrPtr first;
/** pointer to last Extra Attribute [linked list] */
	gaiaDxfExtraAttrPtr last;
/** pointer to next item [linked list] */
	struct gaia_dxf_text *next;
    } gaiaDxfText;
/**
 Typedef for DXF Text object

 \sa gaiaDxfText
 */
    typedef gaiaDxfText *gaiaDxfTextPtr;

/**
 wrapper for DXF Point object
 */
    typedef struct gaia_dxf_point
    {
/** X coordinate */
	double x;
/** Y coordinate */
	double y;
/** Z coordinate */
	double z;
/** pointer to first Extra Attribute [linked list] */
	gaiaDxfExtraAttrPtr first;
/** pointer to last Extra Attribute [linked list] */
	gaiaDxfExtraAttrPtr last;
/** pointer to next item [linked list] */
	struct gaia_dxf_point *next;
    } gaiaDxfPoint;
/**
 Typedef for DXF Point object

 \sa gaiaDxfPoint
 */
    typedef gaiaDxfPoint *gaiaDxfPointPtr;

/**
 wrapper for DXF Polygon interior hole object
 */
    typedef struct gaia_dxf_hole
    {
/** total count of points */
	int points;
/** array of X coordinates */
	double *x;
/** array of Y coordinates */
	double *y;
/** array of Z coordinates */
	double *z;
/** pointer to next item [linked list] */
	struct gaia_dxf_hole *next;
    } gaiaDxfHole;
/**
 Typedef for DXF Point object

 \sa gaiaDxfHole
 */
    typedef gaiaDxfHole *gaiaDxfHolePtr;

/**
 wrapper for DXF Polyline object 
 could be a Linestring or a Polygon depending on the is_closed flag
 */
    typedef struct gaia_dxf_polyline
    {
/** open (Linestring) or closed (Polygon exterior ring) */
	int is_closed;
/** total count of points */
	int points;
/** array of X coordinates */
	double *x;
/** array of Y coordinates */
	double *y;
/** array of Z coordinates */
	double *z;
/** pointer to first Polygon hole [linked list] */
	gaiaDxfHolePtr first_hole;
/** pointer to last Polygon hole [linked list] */
	gaiaDxfHolePtr last_hole;
/** pointer to first Extra Attribute [linked list] */
	gaiaDxfExtraAttrPtr first;
/** pointer to last Extra Attribute [linked list] */
	gaiaDxfExtraAttrPtr last;
/** pointer to next item [linked list] */
	struct gaia_dxf_polyline *next;
    } gaiaDxfPolyline;
/**
 Typedef for DXF Polyline object

 \sa gaiaDxfPolyline
 */
    typedef gaiaDxfPolyline *gaiaDxfPolylinePtr;

/**
 wrapper for DXF Block object
 */
    typedef struct gaia_dxf_block
    {
/** pointer to Layer Name string */
	char *layer_name;
/** pointer to Block ID string */
	char *block_id;
/** pointer to DXF Text object */
	gaiaDxfTextPtr text;
/** pointer to DXF Point object */
	gaiaDxfPointPtr point;
/** pointer to DXF Polyline object */
	gaiaDxfPolylinePtr line;
/** pointer to next item [linked list] */
	struct gaia_dxf_block *next;
    } gaiaDxfBlock;
/**
 Typedef for DXF Block object

 \sa gaiaDxfBlock
 */
    typedef gaiaDxfBlock *gaiaDxfBlockPtr;

/**
 wrapper for DXF Layer object
 */
    typedef struct gaia_dxf_layer
    {
/** pointer to Layer Name string */
	char *layer_name;
/** pointer to first DXF Text object [linked list] */
	gaiaDxfTextPtr first_text;
/** pointer to last DXF Text object [linked list] */
	gaiaDxfTextPtr last_text;
/** pointer to first DXF Point object [linked list] */
	gaiaDxfPointPtr first_point;
/** pointer to lasst DXF Point object [linked list] */
	gaiaDxfPointPtr last_point;
/** pointer to first DXF Polyline (Linestring) object [linked list] */
	gaiaDxfPolylinePtr first_line;
/** pointer to last DXF Polyline (Linestring) object [linked list] */
	gaiaDxfPolylinePtr last_line;
/** pointer to first DXF Polyline (Polygon) object [linked list] */
	gaiaDxfPolylinePtr first_polyg;
/** pointer to last DXF Polyline (Polygon) object [linked list] */
	gaiaDxfPolylinePtr last_polyg;
/** boolean flag: contains 3d Text objects */
	int is3Dtext;
/** boolean flag: contains 3d Point objects */
	int is3Dpoint;
/** boolean flag: contains 3d Polyline (Linestring) objects */
	int is3Dline;
/** boolean flag: contains 3d Polyline (Polygon) objects */
	int is3Dpolyg;
/** boolean flag: contains Text Extra Attributes */
	int hasExtraText;
/** boolean flag: contains Point Extra Attributes */
	int hasExtraPoint;
/** boolean flag: contains Polyline (Linestring) Extra Attributes */
	int hasExtraLine;
/** boolean flag: contains Polyline (Polygon) Extra Attributes */
	int hasExtraPolyg;
/** pointer to next item [linked list] */
	struct gaia_dxf_layer *next;
    } gaiaDxfLayer;
/**
 Typedef for DXF Layer object

 \sa gaiaDxfLayer
 */
    typedef gaiaDxfLayer *gaiaDxfLayerPtr;

/**
 wrapper for DXF Parser object
 */
    typedef struct gaia_dxf_parser
    {
/** OUT: pointer to first DXF Layer object [linked list] */
	gaiaDxfLayerPtr first;
/** OUT: pointer to last DXF Layer object [linked list] */
	gaiaDxfLayerPtr last;
/** IN: parser option - dimension handlig */
	int force_dims;
/** IN: parser option - the SRID */
	int srid;
/** IN: parser option - pointer the single Layer Name string */
	const char *selected_layer;
/** IN: parser option - pointer to prefix string for DB tables */
	const char *prefix;
/** IN: parser option - linked rings special handling */
	int linked_rings;
/** IN: parser option - unlinked rings special handling */
	int unlinked_rings;
/** internal parser variable */
	int line_no;
/** internal parser variable */
	int op_code_line;
/** internal parser variable */
	int op_code;
/** internal parser variable */
	int section;
/** internal parser variable */
	int tables;
/** internal parser variable */
	int blocks;
/** internal parser variable */
	int entities;
/** internal parser variable */
	int is_layer;
/** internal parser variable */
	int is_block;
/** internal parser variable */
	int is_text;
/** internal parser variable */
	int is_point;
/** internal parser variable */
	int is_polyline;
/** internal parser variable */
	int is_lwpolyline;
/** internal parser variable */
	int is_vertex;
/** internal parser variable */
	int is_insert;
/** internal parser variable */
	int eof;
/** internal parser variable */
	int error;
/** internal parser variable */
	char *curr_block_id;
/** internal parser variable */
	char *curr_layer_name;
/** internal parser variable */
	gaiaDxfText curr_text;
/** internal parser variable */
	gaiaDxfPoint curr_point;
/** internal parser variable */
	int is_closed_polyline;
/** internal parser variable */
	gaiaDxfPointPtr first_pt;
/** internal parser variable */
	gaiaDxfPointPtr last_pt;
/** internal parser variable */
	char *extra_key;
/** internal parser variable */
	char *extra_value;
/** internal parser variable */
	gaiaDxfExtraAttrPtr first_ext;
/** internal parser variable */
	gaiaDxfExtraAttrPtr last_ext;
/** internal parser variable */
	gaiaDxfBlockPtr first_blk;
/** internal parser variable */
	gaiaDxfBlockPtr last_blk;
/** internal parser variable */
	int undeclared_layers;
    } gaiaDxfParser;
/**
 Typedef for DXF Layer object

 \sa gaiaDxfParser
 */
    typedef gaiaDxfParser *gaiaDxfParserPtr;


/* function prototypes */


/**
 Creates a DXF Parser object

 \param srid the SRID value to be used for all Geometries
 \param force_dims should be one of GAIA_DXF_AUTO_2D_3D, GAIA_DXF_FORCE_2D 
 or GAIA_DXF_FORCE_3D
 \param prefix an optional prefix to be used for DB target tables 
 (could be NULL)
 \param selected_layers if set, only the DXF Layer of corresponding name will 
 be imported (could be NULL)
 \param special_rings rings handling: should be one of GAIA_DXF_RING_NONE, 
 GAIA_DXF_RING_LINKED of GAIA_DXF_RING_UNLINKED

 \return the pointer to a DXF Parser object

 \sa gaiaDestroyDxfParser, gaiaParseDxfFile, gaiaLoadFromDxfParser

 \note the DXF Parser object corresponds to dynamically allocated memory:
 so you are responsible to destroy this object before or later by invoking
 gaiaDestroyDxfParser().
 */
    GAIAGEO_DECLARE gaiaDxfParserPtr gaiaCreateDxfParser (int srid,
							  int force_dims,
							  const char *prefix,
							  const char
							  *selected_layer,
							  int special_rings);

/**
 Destroying a DXF Parser object

 \param parser pointer to DXF Parser object

 \sa gaiaCreateDxfParser

 \note the pointer to the DXF Parser object to be finalized is expected
 to be the one returned by a previous call to gaiaCreateDxfParser.
 */
    GAIAGEO_DECLARE void gaiaDestroyDxfParser (gaiaDxfParserPtr parser);

/**
 Parsing a DXF file

 \param parser pointer to DXF Parser object
 \param dxf_path pathname of the DXF external file to be parsed

 \return 0 on failure, any other value on success

 \sa gaiaCreateDxfParser, gaiaDestroyDxfParser, gaiaLoadFromDxfParser

 \note the pointer to the DXF Parser object is expected to be the one 
 returned by a previous call to gaiaCreateDxfParser.
 A DXF Parser object can be used only a single time to parse a DXF file
 */
    GAIAGEO_DECLARE int gaiaParseDxfFile (gaiaDxfParserPtr parser,
					  const char *dxf_path);

/**
 Populating a DB so to permanently store all Geometries from a DXF Parser

 \param db_handle handle to a valid DB connection
 \param parser pointer to DXF Parser object
 \param mode should be one of GAIA_DXF_IMPORT_BY_LAYER or GAIA_DXF_IMPORT_MIXED
 \param append boolean flag: if set and some required DB table already exists 
  will attempt to append further rows into the existing table.
  otherwise an error will be returned.

 \return 0 on failure, any other value on success

 \sa gaiaCreateDxfParser, gaiaDestroyDxfParser, gaiaParseDxfFile

 \note the pointer to the DXF Parser object is expected to be the one 
 returned by a previous call to gaiaCreateDxfParser and previously used
 for a succesfull call to gaiaParseDxfFile
 */
    GAIAGEO_DECLARE int gaiaLoadFromDxfParser (sqlite3 * db_handle,
					       gaiaDxfParserPtr parser,
					       int mode, int append);

#ifdef __cplusplus
}
#endif

#endif				/* _GG_DXF_H */
