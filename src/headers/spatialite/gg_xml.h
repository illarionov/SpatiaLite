/*
 gg_xml.h -- Gaia common support for XML documents
  
 version 4.0, 2012 December 10

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
 
Portions created by the Initial Developer are Copyright (C) 2008-2012
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
 \file gg_xml.h

 Geometry handling functions: XML document
 */

#ifndef _GG_XML_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GG_XML_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/* constant values for XmlBLOB */

/** XmlBLOB internal marker: START */
#define GAIA_XML_START		0x00
/** XmlBLOB internal marker: END */
#define GAIA_XML_END		0xDD
/** XmlBLOB internal marker: HEADER */
#define GAIA_XML_HEADER		0xAB
/** XmlBLOB internal marker: SCHEMA */
#define GAIA_XML_SCHEMA		0xBA
/** XmlBLOB internal marker: CRC32 */
#define GAIA_XML_CRC32		0xBC
/** XmlBLOB internal marker: PAYLOAD */
#define GAIA_XML_PAYLOAD	0xCB

/* bitmasks for XmlBLOB-FLAG */

/** XmlBLOB FLAG - LITTLE_ENDIAN bitmask */
#define GAIA_XML_LITTLE_ENDIAN	0x01
/** XmlBLOB FLAG - COMPRESSED bitmask */
#define GAIA_XML_COMPRESSED	0x02
/** XmlBLOB FLAG - VALIDATED bitmask */
#define GAIA_XML_VALIDATED	0x04


/* function prototypes */

#ifndef DOXYGEN_SHOULD_IGNORE_THIS
#ifdef ENABLE_LIBXML2		/* LIBXML2 enabled: supporting XML documents */
#endif

/**
 return the LIBXML2 version string

 \return a text string identifying the current LIBXML2 version

 \note the version string corresponds to dynamically allocated memory:
 so you are responsible to free() it [unless SQLite will take care
 of memory cleanup via buffer binding].
 */
    GAIAGEO_DECLARE char *gaia_libxml2_version (void);

/**
 Creates an XmlBLOB buffer

 \param xml pointer to the XML document (XmlBLOB payload).
 \param xml_len lenght of the XML document (in bytes).
 \param compressed if TRUE the returned XmlBLOB will be zip-compressed.
 \param schemaURI if not NULL the XML document will be assumed to be valid
  only if it succesfully passes a formal Schema valitadion.
 \param result on completion will containt a pointer to XmlBLOB:
 NULL on failure.
 \param size on completion this variable will contain the XmlBLOB's size (in bytes)
 \param parsing_errors on completion this variable will contain all error/warning
 messages emitted during the XML Parsing step. Can be set to NULL so to ignore any message.
 \param schema_validation_errors on completion this variable will contain all error/warning
 messages emitted during the XML Schema Validation step. Can be set to NULL so to ignore any message.

 \sa gaiaXmlFromBlob, gaiaIsValidXmlBlob, gaiaIsCompressedXmlBlob,
 gaiaIsSchemaValidatedXmlBlob, gaiaXmlBlobHasSchemaURI, gaiaXmlBlobGetDocumentSize,
 gaiaXmlBlobGetSchemaURI, gaiaXmlGetInternalSchemaURI, gaiaXmlBlobGetEncoding,
 gaiaXmlBlobCompression, gaiaXmlBlobGetLastParseError, gaiaXmlBlobGetLastValidateError,
 gaiaXmlBlobGetLastXPathError

 \note the XmlBLOB buffer corresponds to dynamically allocated memory:
 so you are responsible to free() it [unless SQLite will take care
 of memory cleanup via buffer binding].
 */
    GAIAGEO_DECLARE void gaiaXmlToBlob (const char *xml, int xml_len,
					int compressed,
					const char *schamaURI,
					unsigned char **result, int *size,
					char **parsing_errors,
					char **schema_validation_errors);

/**
 Extract an XMLDocument from within an XmlBLOB buffer

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).
 \param indent if TRUE the XMLDocument will be properly indented,
  otherwise it will be extracted exactly as it was when loaded.

 \return the pointer to the newly created XMLDocument buffer: NULL on failure

 \sa gaiaXmlToBlob, gaiaXmlFromBlob, gaiaIsValidXmlBlob, gaiaIsCompressedXmlBlob,
 gaiaIsSchemaValidatedXmlBlob, gaiaXmlBlobHasSchemaURI, gaiaXmlBlobGetDocumentSize,
 gaiaXmlBlobGetSchemaURI, gaiaXmlGetInternalSchemaURI, gaiaXmlBlobGetEncoding, 
 gaiaXmlBlobCompression, gaiaXmlBlobGetLastParseError, gaiaXmlBlobGetLastValidateError,
 gaiaXmlBlobGetLastXPathError

 \note the returned XMLDocument will always be encoded as UTF-8 (irrespectively
 from the internal encoding declaration), so to allow any further processing as 
 SQLite TEXT.

 \note the XMLDocument buffer corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE char *gaiaXmlTextFromBlob (const unsigned char *blob,
					       int size, int indent);

/**
 Extract an XMLDocument from within an XmlBLOB buffer

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).
 \param indent if TRUE the XMLDocument will be properly indented,
  otherwise it will be extracted exactly as it was when loaded.

 \sa gaiaXmlToBlob, gaiaXmlTextFromBlob, gaiaIsValidXmlBlob, 
 gaiaIsCompressedXmlBlob, gaiaIsSchemaValidatedXmlBlob, gaiaXmlBlobHasSchemaURI,
 gaiaXmlBlobGetDocumentSize, gaiaXmlBlobGetSchemaURI,
 gaiaXmlGetInternalSchemaURI, gaiaXmlBlobGetEncoding, 
 gaiaXmlBlobCompression, gaiaXmlBlobGetLastParseError, gaiaXmlBlobGetLastValidateError,
 gaiaXmlBlobGetLastXPathError

 \note the returned XMLDocument will always respect the internal encoding declaration,
 and may not support any further processing as SQLite TEXT if it's not UTF-8.

 \note the XMLDocument buffer corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE void gaiaXmlFromBlob (const unsigned char *blob,
					  int size, int indent,
					  unsigned char **result,
					  int *res_size);

/**
 Checks if a BLOB actually is a valid XmlBLOB buffer

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).

 \return TRUE or FALSE

 \sa gaiaXmlToBlob, gaiaXmlFromBlob, gaiaIsCompressedXmlBlob,
 gaiaIsSchemaValidatedXmlBlob, gaiaXmlBlobHasSchemaURI,
 gaiaXmlBlobGetDocumentSize, gaiaXmlBlobGetSchemaURI,
 gaiaXmlGetInternalSchemaURI, gaiaXmlBlobGetEncoding,
 gaiaXmlBlobCompression, gaiaXmlBlobGetLastParseError, gaiaXmlBlobGetLastValidateError,
 gaiaXmlBlobGetLastXPathError
 */
    GAIAGEO_DECLARE int gaiaIsValidXmlBlob (const unsigned char *blob,
					    int size);

/**
 Checks if a valid XmlBLOB buffer is compressed or not

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).

 \return TRUE or FALSE if the BLOB actually is a valid uncompressed XmlBLOB; -1 in any other case.

 \sa gaiaXmlToBlob, gaiaXmlFromBlob, gaiaIsValidXmlBlob, gaiaIsSchemaValidatedXmlBlob,
 gaiaXmlBlobHasSchemaURI, gaiaXmlBlobGetDocumentSize, gaiaXmlBlobGetSchemaURI,
 gaiaXmlGetInternalSchemaURI, gaiaXmlBlobGetEncoding, gaiaXmlBlobCompression, 
 gaiaXmlBlobGetLastParseError, gaiaXmlBlobGetLastValidateError,
 gaiaXmlBlobGetLastXPathError
 */
    GAIAGEO_DECLARE int gaiaIsCompressedXmlBlob (const unsigned char *blob,
						 int size);

/**
 Return another XmlBLOB buffer compressed / uncompressed

 \param blob pointer to the input XmlBLOB buffer.
 \param in_size input XmlBLOB's size (in bytes).
 \param compressed if TRUE the returned XmlBLOB will be zip-compressed.
 \param result on completion will containt a pointer to the output XmlBLOB:
 NULL on failure.
 \param out_size on completion this variable will contain the output XmlBLOB's size (in bytes)

 \sa gaiaXmlToBlob, gaiaXmlFromBlob, gaiaIsValidXmlBlob, gaiaIsSchemaValidatedXmlBlob,
 gaiaXmlBlobHasSchemaURI, gaiaXmlBlobGetDocumentSize, gaiaXmlBlobGetSchemaURI,
 gaiaXmlGetInternalSchemaURI, gaiaXmlBlobGetEncoding, gaiaIsCompressedXmlBlob, 
 gaiaXmlBlobGetLastParseError, gaiaXmlBlobGetLastValidateError,
 gaiaXmlBlobGetLastXPathError

 \note the XmlBLOB buffer corresponds to dynamically allocated memory:
 so you are responsible to free() it [unless SQLite will take care
 of memory cleanup via buffer binding].
 */
    GAIAGEO_DECLARE void gaiaXmlBlobCompression (const unsigned char *blob,
						 int in_size, int compressed,
						 unsigned char **result,
						 int *out_size);

/**
 Checks if a valid XmlBLOB buffer has succesfully passed a formal Schema validation or not

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).

 \return TRUE or FALSE if the BLOB actually is a valid XmlBLOB but not schema-validated; 
  -1 in any other case.

 \sa gaiaXmlToBlob, gaiaXmlFromBlob, gaiaIsValidXmlBlob, gaiaIsCompressedXmlBlob,
 gaiaXmlBlobHasSchemaURI, gaiaXmlBlobGetDocumentSize, gaiaXmlBlobGetSchemaURI,
 gaiaXmlGetInternalSchemaURI, gaiaXmlBlobGetEncoding,
 gaiaXmlBlobCompression, gaiaXmlBlobGetLastParseError,
 gaiaXmlBlobGetLastValidateError, gaiaXmlBlobGetLastXPathError
 */
    GAIAGEO_DECLARE int gaiaIsSchemaValidatedXmlBlob (const unsigned char *blob,
						      int size);

/**
 Checks if a valid XmlBLOB buffer has a SchemaURI or not

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).

 \return TRUE or FALSE if the BLOB actually is a valid XmlBLOB but not having any SchemaURI; 
  -1 in any other case.

 \sa gaiaXmlToBlob, gaiaXmlFromBlob, gaiaIsValidXmlBlob, gaiaIsCompressedXmlBlob,
 gaiaIsSchemaValidatedXmlBlob, gaiaXmlBlobGetDocumentSize, gaiaXmlBlobGetSchemaURI,
 gaiaXmlGetInternalSchemaURI, gaiaXmlBlobGetEncoding, 
 gaiaXmlBlobCompression, gaiaXmlBlobGetLastParseError,
 gaiaXmlBlobGetLastValidateError, gaiaXmlBlobGetLastXPathError
 */
    GAIAGEO_DECLARE int gaiaXmlBlobHasSchemaURI (const unsigned char *blob,
						 int size);

/**
 Return the XMLDocument size (in bytes) from a valid XmlBLOB buffer

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).

 \return the XMLDocument size (in bytes) for any valid XmlBLOB; 
  -1 if the BLOB isn't a valid XmlBLOB.

 \sa gaiaXmlToBlob, gaiaXmlFromBlob, gaiaIsValidXmlBlob,
 gaiaIsCompressedXmlBlob, gaiaIsSchemaValidatedXmlBlob,
 gaiaXmlBlobHasSchemaURI, gaiaXmlBlobGetSchemaURI,
 gaiaXmlGetInternalSchemaURI, gaiaXmlBlobGetEncoding,
 gaiaXmlBlobCompression, gaiaXmlBlobGetLastParseError, 
 gaiaXmlBlobGetLastValidateError, gaiaXmlBlobGetLastXPathError
 */
    GAIAGEO_DECLARE int gaiaXmlBlobGetDocumentSize (const unsigned char *blob,
						    int size);

/**
 Return the SchemaURI from a valid XmlBLOB buffer

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).

 \return the SchemaURI for any valid XmlBLOB containing a SchemaURI; 
  NULL in any other case.

 \sa gaiaXmlToBlob, gaiaXmlFromBlob, gaiaIsValidXmlBlob, gaiaIsCompressedXmlBlob,
 gaiaIsSchemaValidatedXmlBlob, gaiaXmlBlobHasSchemaURI, gaiaXmlBlobGetDocumentSize,
 gaiaXmlGetInternalSchemaURI, gaiaXmlBlobGetEncoding, 
 gaiaXmlBlobCompression, gaiaXmlBlobGetLastParseError,
 gaiaXmlBlobGetLastValidateError, gaiaXmlBlobGetLastXPathError

 \note the returned SchemaURI corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE char *gaiaXmlBlobGetSchemaURI (const unsigned char
						   *blob, int size);

/**
 Return the Internal SchemaURI from a valid XmlDocument

 \param xml pointer to the XML document 
 \param xml_len lenght of the XML document (in bytes).

 \return the SchemaURI eventually defined within a valid XMLDocument; 
  NULL if the XMLDocument is invalid, or if it doesn't contain any SchemaURI.

 \sa gaiaXmlToBlob, gaiaXmlFromBlob, gaiaIsValidXmlBlob, gaiaIsCompressedXmlBlob,
 gaiaIsSchemaValidatedXmlBlob, gaiaXmlBlobHasSchemaURI, gaiaXmlBlobGetDocumentSize,
 gaiaXmlBlobGetSchemaURI, gaiaXmlBlobGetEncoding, gaiaXmlBlobGetLastParseError,
 gaiaXmlBlobCompression, gaiaXmlBlobGetLastValidateError, gaiaXmlBlobGetLastXPathError

 \note the returned SchemaURI corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE char *gaiaXmlGetInternalSchemaURI (const char *xml,
							   int xml_len);

/**
 Return the Charset Encoding from a valid XmlBLOB buffer

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).

 \return the Charset Encoding for any valid XmlBLOB explicitly defining an Encoding; 
  NULL in any other case.

 \sa gaiaXmlToBlob, gaiaXmlFromBlob, gaiaIsValidXmlBlob, gaiaIsCompressedXmlBlob,
 gaiaIsSchemaValidatedXmlBlob, gaiaXmlBlobHasSchemaURI, gaiaXmlBlobGetDocumentSize,
 gaiaXmlGetInternalSchemaURI, gaiaXmlBlobSchemaURI, 
 gaiaXmlBlobCompression, gaiaXmlBlobGetLastParseError,
 gaiaXmlBlobGetLastValidateError, gaiaXmlBlobGetLastXPathError

 \note the returned Encoding corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE char *gaiaXmlBlobGetEncoding (const unsigned char
						  *blob, int size);

/**
 Return the most recent XML Parse error/warning (if any)

 \return the most recent XML Parse error/warning message (if any); 
  NULL in any other case.

 \sa gaiaXmlToBlob, gaiaXmlFromBlob, gaiaIsValidXmlBlob, 
 gaiaIsCompressedXmlBlob, gaiaIsSchemaValidatedXmlBlob,
 gaiaXmlBlobHasSchemaURI, gaiaXmlBlobGetDocumentSize, 
 gaiaXmlBlobGetSchemaURI, gaiaXmlGetInternalSchemaURI,
 gaiaXmlBlobCompression, gaiaXmlBlobGetEncoding,
 gaiaXmlBlobGetLastValidateError, gaiaXmlBlobGetLastXPathError

 \note the returned error/warning message corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE char *gaiaXmlBlobGetLastParseError (void);

/**
 Return the most recent XML Validate error/warning (if any)

 \return the most recent XML Validate error/warning message (if any); 
  NULL in any other case.

 \sa gaiaXmlToBlob, gaiaXmlFromBlob, gaiaIsValidXmlBlob,
 gaiaIsCompressedXmlBlob, gaiaIsSchemaValidatedXmlBlob,
 gaiaXmlBlobHasSchemaURI, gaiaXmlBlobGetDocumentSize,
 gaiaXmlBlobGetSchemaURI, gaiaXmlGetInternalSchemaURI,
 gaiaXmlBlobCompression, gaiaXmlBlobGetEncoding,
 gaiaXmlBlobGetLastParseError, gaiaXmlBlobGetLastXPathError

 \note the returned error/warning message corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE char *gaiaXmlBlobGetLastValidateError (void);

/**
 Return the most recent XPath error/warning (if any)

 \return the most recent XPath error/warning message (if any); 
  NULL in any other case.

 \sa gaiaXmlToBlob, gaiaXmlFromBlob, gaiaIsValidXmlBlob,
 gaiaIsCompressedXmlBlob, gaiaIsSchemaValidatedXmlBlob,
 gaiaXmlBlobHasSchemaURI, gaiaXmlBlobGetDocumentSize,
 gaiaXmlBlobGetSchemaURI, gaiaXmlGetInternalSchemaURI,
 gaiaXmlBlobCompression, gaiaXmlBlobGetEncoding,
 gaiaXmlBlobGetLastParseError, gaiaXmlBlobGetLastValidateError

 \note the returned error/warning message corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE char *gaiaXmlBlobGetLastXPathError (void);

#endif				/* end LIBXML2: supporting XML documents */

#ifdef __cplusplus
}
#endif

#endif				/* _GG_XML_H */
