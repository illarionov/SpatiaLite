/*

 gg_xml.c -- XML Document implementation
    
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#ifdef ENABLE_LIBXML2		/* LIBXML2 enabled: supporting XML documents */

#include <zlib.h>
#include <libxml/parser.h>
#include <libxml/xmlschemas.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <spatialite_private.h>
#include <spatialite/sqlite.h>
#include <spatialite/debug.h>
#include <spatialite/gaiageo.h>
#include <spatialite/gaiaaux.h>

static void
spliteSilentError (void *ctx, const char *msg, ...)
{
/* shutting up XML Errors */
    if (ctx != NULL)
	ctx = NULL;		/* suppressing stupid compiler warnings (unused args) */
    if (msg != NULL)
	ctx = NULL;		/* suppressing stupid compiler warnings (unused args) */
}

static void
spliteParsingError (void *ctx, const char *msg, ...)
{
/* appending to the current Parsing Error buffer */
    struct splite_internal_cache *cache = (struct splite_internal_cache *) ctx;
    gaiaOutBufferPtr buf = (gaiaOutBufferPtr) (cache->xmlParsingErrors);
    char out[65536];
    va_list args;

    if (ctx != NULL)
	ctx = NULL;		/* suppressing stupid compiler warnings (unused args) */

    va_start (args, msg);
    vsnprintf (out, 65536, msg, args);
    gaiaAppendToOutBuffer (buf, out);
    va_end (args);
}

static void
spliteSchemaValidationError (void *ctx, const char *msg, ...)
{
/* appending to the current SchemaValidation Error buffer */
    struct splite_internal_cache *cache = (struct splite_internal_cache *) ctx;
    gaiaOutBufferPtr buf =
	(gaiaOutBufferPtr) (cache->xmlSchemaValidationErrors);
    char out[65536];
    va_list args;

    if (ctx != NULL)
	ctx = NULL;		/* suppressing stupid compiler warnings (unused args) */

    va_start (args, msg);
    vsnprintf (out, 65536, msg, args);
    gaiaAppendToOutBuffer (buf, out);
    va_end (args);
}

static void
spliteResetXmlErrors (struct splite_internal_cache *cache)
{
/* resetting the XML Error buffers */
    gaiaOutBufferPtr buf = (gaiaOutBufferPtr) (cache->xmlParsingErrors);
    gaiaOutBufferReset (buf);
    buf = (gaiaOutBufferPtr) (cache->xmlSchemaValidationErrors);
    gaiaOutBufferReset (buf);
}

GAIAGEO_DECLARE char *
gaiaXmlBlobGetLastParseError (void *ptr)
{
/* get the most recent XML Parse error/warning message */
    struct splite_internal_cache *cache = (struct splite_internal_cache *) ptr;
    gaiaOutBufferPtr buf = (gaiaOutBufferPtr) (cache->xmlParsingErrors);
    return buf->Buffer;
}

GAIAGEO_DECLARE char *
gaiaXmlBlobGetLastValidateError (void *ptr)
{
/* get the most recent XML Validate error/warning message */
    struct splite_internal_cache *cache = (struct splite_internal_cache *) ptr;
    gaiaOutBufferPtr buf =
	(gaiaOutBufferPtr) (cache->xmlSchemaValidationErrors);
    return buf->Buffer;
}

GAIAGEO_DECLARE char *
gaiaXmlBlobGetLastXPathError (void *ptr)
{
/* get the most recent XML Validate error/warning message */
    struct splite_internal_cache *cache = (struct splite_internal_cache *) ptr;
    gaiaOutBufferPtr buf = (gaiaOutBufferPtr) (cache->xmlXPathErrors);
    return buf->Buffer;
}

SPATIALITE_PRIVATE void
splite_free_xml_schema_cache_item (struct splite_xmlSchema_cache_item *p)
{
/* freeing an XmlSchema Cache Item */
    if (p->schemaURI)
	free (p->schemaURI);
    if (p->parserCtxt)
	xmlSchemaFreeParserCtxt (p->parserCtxt);
    if (p->schema)
	xmlSchemaFree (p->schema);
    if (p->schemaDoc)
	xmlFreeDoc (p->schemaDoc);
    p->schemaURI = NULL;
    p->parserCtxt = NULL;
    p->schemaDoc = NULL;
    p->schema = NULL;
}

static int
splite_xmlSchemaCacheFind (struct splite_internal_cache *cache,
			   const char *schemaURI, xmlDocPtr * schema_doc,
			   xmlSchemaParserCtxtPtr * parser_ctxt,
			   xmlSchemaPtr * schema)
{
/* attempting to retrive some XmlSchema from within the Cache */
    int i;
    time_t now;
    struct splite_xmlSchema_cache_item *p;
    for (i = 0; i < MAX_XMLSCHEMA_CACHE; i++)
      {
	  p = &(cache->xmlSchemaCache[i]);
	  if (p->schemaURI)
	    {
		if (strcmp (schemaURI, p->schemaURI) == 0)
		  {
		      /* found a matching cache-item */
		      *schema_doc = p->schemaDoc;
		      *parser_ctxt = p->parserCtxt;
		      *schema = p->schema;
		      /* updating the timestamp */ time (&now);
		      p->timestamp = now;
		      return 1;
		  }
	    }
      }
    return 0;
}

static void
splite_xmlSchemaCacheInsert (struct splite_internal_cache *cache,
			     const char *schemaURI, xmlDocPtr schema_doc,
			     xmlSchemaParserCtxtPtr parser_ctxt,
			     xmlSchemaPtr schema)
{
/* inserting a new XmlSchema item into the Cache */
    int i;
    int len = strlen (schemaURI);
    time_t now;
    time_t oldest;
    struct splite_xmlSchema_cache_item *pSlot = NULL;
    struct splite_xmlSchema_cache_item *p;
    time (&now);
    oldest = now;
    for (i = 0; i < MAX_XMLSCHEMA_CACHE; i++)
      {
	  p = &(cache->xmlSchemaCache[i]);
	  if (p->schemaURI == NULL)
	    {
		/* found an empty slot */
		pSlot = p;
		break;
	    }
	  if (p->timestamp < oldest)
	    {
		/* saving the oldest slot */
		pSlot = p;
		oldest = p->timestamp;
	    }
      }
/* inserting into the Cache Slot */
    splite_free_xml_schema_cache_item (pSlot);
    pSlot->timestamp = now;
    pSlot->schemaURI = malloc (len + 1);
    strcpy (pSlot->schemaURI, schemaURI);
    pSlot->schemaDoc = schema_doc;
    pSlot->parserCtxt = parser_ctxt;
    pSlot->schema = schema;
}

GAIAGEO_DECLARE void
gaiaXmlToBlob (void *p_cache, const char *xml, int xml_len, int compressed,
	       const char *schemaURI, unsigned char **result, int *size,
	       char **parsing_errors, char **schema_validation_errors)
{
/* attempting to build an XmlBLOB buffer */
    xmlDocPtr xml_doc;
    xmlDocPtr schema_doc;
    xmlSchemaPtr schema;
    xmlSchemaParserCtxtPtr parser_ctxt;
    xmlSchemaValidCtxtPtr valid_ctxt;
    int len;
    int zip_len;
    short uri_len = 0;
    uLong crc;
    Bytef *zip_buf;
    unsigned char *buf;
    unsigned char *ptr;
    unsigned char flags = 0x00;
    int endian_arch = gaiaEndianArch ();
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    gaiaOutBufferPtr parsingBuf = (gaiaOutBufferPtr) (cache->xmlParsingErrors);
    gaiaOutBufferPtr schemaValidationBuf =
	(gaiaOutBufferPtr) (cache->xmlSchemaValidationErrors);
    xmlGenericErrorFunc silentError = (xmlGenericErrorFunc) spliteSilentError;
    xmlGenericErrorFunc parsingError = (xmlGenericErrorFunc) spliteParsingError;
    xmlGenericErrorFunc schemaError =
	(xmlGenericErrorFunc) spliteSchemaValidationError;

    spliteResetXmlErrors (cache);

    *result = NULL;
    *size = 0;
    if (parsing_errors)
	*parsing_errors = NULL;
    if (schema_validation_errors)
	*schema_validation_errors = NULL;
    if (xml == NULL)
	return;

    xmlSetGenericErrorFunc (NULL, silentError);

    if (schemaURI != NULL)
      {
	  if (splite_xmlSchemaCacheFind
	      (cache, schemaURI, &schema_doc, &parser_ctxt, &schema))
	      ;
	  else
	    {
		/* preparing the Schema */
		xmlSetGenericErrorFunc (cache, schemaError);
		schema_doc = xmlReadFile ((const char *) schemaURI, NULL, 0);
		if (schema_doc == NULL)
		  {
		      spatialite_e ("unable to load the Schema\n");
		      if (schema_validation_errors)
			  *schema_validation_errors =
			      schemaValidationBuf->Buffer;
		      xmlSetGenericErrorFunc ((void *) stderr, NULL);
		      return;
		  }
		parser_ctxt = xmlSchemaNewDocParserCtxt (schema_doc);
		if (parser_ctxt == NULL)
		  {
		      spatialite_e ("unable to prepare the Schema Context\n");
		      xmlFreeDoc (schema_doc);
		      if (schema_validation_errors)
			  *schema_validation_errors =
			      schemaValidationBuf->Buffer;
		      xmlSetGenericErrorFunc ((void *) stderr, NULL);
		      return;
		  }
		schema = xmlSchemaParse (parser_ctxt);
		if (schema == NULL)
		  {
		      spatialite_e ("invalid Schema\n");
		      xmlFreeDoc (schema_doc);
		      if (schema_validation_errors)
			  *schema_validation_errors =
			      schemaValidationBuf->Buffer;
		      xmlSetGenericErrorFunc ((void *) stderr, NULL);
		      return;
		  }
		splite_xmlSchemaCacheInsert (cache, schemaURI, schema_doc,
					     parser_ctxt, schema);
	    }
      }

/* testing if the XMLDocument is well-formed */
    xmlSetGenericErrorFunc (cache, parsingError);
    xml_doc =
	xmlReadMemory ((const char *) xml, xml_len, "noname.xml", NULL, 0);
    if (xml_doc == NULL)
      {
	  /* parsing error; not a well-formed XML */
	  spatialite_e ("XML parsing error\n");
	  if (parsing_errors)
	      *parsing_errors = parsingBuf->Buffer;
	  xmlSetGenericErrorFunc ((void *) stderr, NULL);
	  return;
      }
    if (parsing_errors)
	*parsing_errors = parsingBuf->Buffer;

    if (schemaURI != NULL)
      {
	  /* Schema validation */
	  xmlSetGenericErrorFunc (cache, schemaError);
	  valid_ctxt = xmlSchemaNewValidCtxt (schema);
	  if (valid_ctxt == NULL)
	    {
		spatialite_e ("unable to prepare a validation context\n");
		xmlFreeDoc (xml_doc);
		if (schema_validation_errors)
		    *schema_validation_errors = schemaValidationBuf->Buffer;
		xmlSetGenericErrorFunc ((void *) stderr, NULL);
		return;
	    }
	  if (xmlSchemaValidateDoc (valid_ctxt, xml_doc) != 0)
	    {
		spatialite_e ("Schema validation failed\n");
		xmlSchemaFreeValidCtxt (valid_ctxt);
		xmlFreeDoc (xml_doc);
		if (schema_validation_errors)
		    *schema_validation_errors = schemaValidationBuf->Buffer;
		xmlSetGenericErrorFunc ((void *) stderr, NULL);
		return;
	    }
	  xmlSchemaFreeValidCtxt (valid_ctxt);
      }
    xmlFreeDoc (xml_doc);

    if (compressed)
      {
	  /* compressing the XML payload */
	  uLong zLen = compressBound (xml_len);
	  zip_buf = malloc (zLen);
	  if (compress (zip_buf, &zLen, (const Bytef *) xml, (uLong) xml_len) !=
	      Z_OK)
	    {
		/* compression error */
		spatialite_e ("XmlBLOB DEFLATE compress error\n");
		free (zip_buf);
		xmlSetGenericErrorFunc ((void *) stderr, NULL);
		return;
	    }
	  zip_len = (int) zLen;
      }
    else
	zip_len = xml_len;

/* reporting errors */
    if (parsing_errors)
	*parsing_errors = parsingBuf->Buffer;
    if (schema_validation_errors)
	*schema_validation_errors = schemaValidationBuf->Buffer;

/* computing the XmlBLOB size */
    len = 21;			/* fixed header-footer size */
    if (schemaURI)
	uri_len = strlen ((const char *) schemaURI);
    len += zip_len;
    len += uri_len;
    buf = malloc (len);
    *buf = GAIA_XML_START;	/* START signature */
    flags |= GAIA_XML_LITTLE_ENDIAN;
    if (compressed)
	flags |= GAIA_XML_COMPRESSED;
    if (schemaURI != NULL)
	flags |= GAIA_XML_VALIDATED;
    *(buf + 1) = flags;		/* XmlBLOB flags */
    *(buf + 2) = GAIA_XML_HEADER;	/* HEADER signature */
    gaiaExport32 (buf + 3, xml_len, 1, endian_arch);	/* the uncompressed XMLDocument size */
    gaiaExport32 (buf + 7, zip_len, 1, endian_arch);	/* the compressed XMLDocument size */
    gaiaExport16 (buf + 11, uri_len, 1, endian_arch);	/* the SchemaURI length in bytes */
    *(buf + 13) = GAIA_XML_SCHEMA;	/* SCHEMA signature */
    ptr = buf + 14;
    if (schemaURI)
      {
	  /* the SchemaURI */
	  memcpy (ptr, schemaURI, uri_len);
	  ptr += uri_len;
      }
    *ptr = GAIA_XML_PAYLOAD;	/* PAYLOAD signature */
    ptr++;
    if (compressed)
      {
	  /* the compressed XML payload */
	  memcpy (ptr, zip_buf, zip_len);
	  free (zip_buf);
	  ptr += zip_len;
      }
    else
      {
	  /* the uncompressed XML payload */
	  memcpy (ptr, xml, xml_len);
	  ptr += xml_len;
      }
    *ptr = GAIA_XML_CRC32;	/* CRC32 signature */
    ptr++;
/* computing the CRC32 */
    crc = crc32 (0L, buf, ptr - buf);
    gaiaExportU32 (ptr, crc, 1, endian_arch);	/* the CRC32 */
    ptr += 4;
    *ptr = GAIA_XML_END;	/* END signature */

    *result = buf;
    *size = len;
    xmlSetGenericErrorFunc ((void *) stderr, NULL);
}

GAIAGEO_DECLARE void
gaiaXmlBlobCompression (const unsigned char *blob,
			int in_size, int compressed,
			unsigned char **result, int *out_size)
{
/* Return another XmlBLOB buffer compressed / uncompressed */
    int in_compressed = 0;
    int little_endian = 0;
    unsigned char flag;
    int in_xml_len;
    int in_zip_len;
    short uri_len;
    int out_xml_len;
    int out_zip_len;
    uLong crc;
    Bytef *zip_buf;
    int len;
    char *schemaURI;
    unsigned char *xml;
    unsigned char *buf;
    unsigned char *ptr;
    unsigned char flags;
    int endian_arch = gaiaEndianArch ();

    *result = NULL;
    *out_size = 0;
/* validity check */
    if (!gaiaIsValidXmlBlob (blob, in_size))
	return;			/* cannot be an XmlBLOB */
    flag = *(blob + 1);
    if ((flag & GAIA_XML_LITTLE_ENDIAN) == GAIA_XML_LITTLE_ENDIAN)
	little_endian = 1;
    if ((flag & GAIA_XML_COMPRESSED) == GAIA_XML_COMPRESSED)
	in_compressed = 1;
    in_xml_len = gaiaImport32 (blob + 3, little_endian, endian_arch);
    in_zip_len = gaiaImport32 (blob + 7, little_endian, endian_arch);
    uri_len = gaiaImport16 (blob + 11, little_endian, endian_arch);
    if (uri_len)
	schemaURI = (char *) blob + 14;
    else
	schemaURI = NULL;

    if (in_compressed == compressed)
      {
	  /* unchanged compression */
	  out_xml_len = in_xml_len;
	  out_zip_len = in_zip_len;
	  zip_buf = (unsigned char *) (blob + 15 + uri_len);
      }
    else if (compressed)
      {
	  /* compressing the XML payload */
	  uLong zLen;
	  out_xml_len = in_xml_len;
	  zLen = compressBound (out_xml_len);
	  xml = (unsigned char *) (blob + 15 + uri_len);
	  zip_buf = malloc (zLen);
	  if (compress
	      (zip_buf, &zLen, (const Bytef *) xml,
	       (uLong) out_xml_len) != Z_OK)
	    {
		/* compression error */
		spatialite_e ("XmlBLOB DEFLATE compress error\n");
		free (zip_buf);
		return;
	    }
	  out_zip_len = (int) zLen;
      }
    else
      {
	  /* unzipping the XML payload */
	  uLong refLen = in_xml_len;
	  const Bytef *in = blob + 15 + uri_len;
	  xml = malloc (in_xml_len + 1);
	  if (uncompress (xml, &refLen, in, in_zip_len) != Z_OK)
	    {
		/* uncompress error */
		spatialite_e ("XmlBLOB DEFLATE uncompress error\n");
		free (xml);
		return;
	    }
	  *(xml + in_xml_len) = '\0';
	  out_xml_len = in_xml_len;
	  out_zip_len = out_xml_len;
      }

/* computing the XmlBLOB size */
    len = 21;			/* fixed header-footer size */
    len += out_zip_len;
    len += uri_len;
    buf = malloc (len);
    *buf = GAIA_XML_START;	/* START signature */
    flags = 0x00;
    flags |= GAIA_XML_LITTLE_ENDIAN;
    if (compressed)
	flags |= GAIA_XML_COMPRESSED;
    if (schemaURI != NULL)
	flags |= GAIA_XML_VALIDATED;
    *(buf + 1) = flags;		/* XmlBLOB flags */
    *(buf + 2) = GAIA_XML_HEADER;	/* HEADER signature */
    gaiaExport32 (buf + 3, out_xml_len, 1, endian_arch);	/* the uncompressed XMLDocument size */
    gaiaExport32 (buf + 7, out_zip_len, 1, endian_arch);	/* the compressed XMLDocument size */
    gaiaExport16 (buf + 11, uri_len, 1, endian_arch);	/* the SchemaURI length in bytes */
    *(buf + 13) = GAIA_XML_SCHEMA;	/* SCHEMA signature */
    ptr = buf + 14;
    if (schemaURI)
      {
	  /* the SchemaURI */
	  memcpy (ptr, schemaURI, uri_len);
	  ptr += uri_len;
      }
    *ptr = GAIA_XML_PAYLOAD;	/* PAYLOAD signature */
    ptr++;
    if (in_compressed == compressed)
      {
	  /* the unchanged XML payload */
	  memcpy (ptr, zip_buf, out_zip_len);
	  ptr += out_zip_len;
      }
    else if (compressed)
      {
	  /* the compressed XML payload */
	  memcpy (ptr, zip_buf, out_zip_len);
	  free (zip_buf);
	  ptr += out_zip_len;
      }
    else
      {
	  /* the uncompressed XML payload */
	  memcpy (ptr, xml, out_xml_len);
	  free (xml);
	  ptr += out_xml_len;
      }
    *ptr = GAIA_XML_CRC32;	/* CRC32 signature */
    ptr++;
/* computing the CRC32 */
    crc = crc32 (0L, buf, ptr - buf);
    gaiaExportU32 (ptr, crc, 1, endian_arch);	/* the CRC32 */
    ptr += 4;
    *ptr = GAIA_XML_END;	/* END signature */

    *result = buf;
    *out_size = len;
}

GAIAGEO_DECLARE int
gaiaIsValidXmlBlob (const unsigned char *blob, int blob_size)
{
/* Checks if a BLOB actually is a valid XmlBLOB buffer */
    int little_endian = 0;
    unsigned char flag;
    short uri_len;
    uLong crc;
    uLong refCrc;
    int endian_arch = gaiaEndianArch ();

/* validity check */
    if (blob_size < 21)
	return 0;		/* cannot be an XmlBLOB */
    if (*blob != GAIA_XML_START)
	return 0;		/* failed to recognize START signature */
    if (*(blob + (blob_size - 1)) != GAIA_XML_END)
	return 0;		/* failed to recognize END signature */
    if (*(blob + (blob_size - 6)) != GAIA_XML_CRC32)
	return 0;		/* failed to recognize CRC32 signature */
    if (*(blob + 2) != GAIA_XML_HEADER)
	return 0;		/* failed to recognize HEADER signature */
    if (*(blob + 13) != GAIA_XML_SCHEMA)
	return 0;		/* failed to recognize SCHEMA signature */
    flag = *(blob + 1);
    if ((flag & GAIA_XML_LITTLE_ENDIAN) == GAIA_XML_LITTLE_ENDIAN)
	little_endian = 1;
    uri_len = gaiaImport16 (blob + 11, little_endian, endian_arch);
    if (*(blob + 14 + uri_len) != GAIA_XML_PAYLOAD)
	return 0;

/* verifying the CRC32 */
    crc = crc32 (0L, blob, blob_size - 5);
    refCrc = gaiaImportU32 (blob + blob_size - 5, little_endian, endian_arch);
    if (crc != refCrc)
	return 0;

    return 1;
}

GAIAGEO_DECLARE char *
gaiaXmlTextFromBlob (const unsigned char *blob, int blob_size, int indent)
{
/* attempting to extract an XMLDocument from within an XmlBLOB buffer */
    int compressed = 0;
    int little_endian = 0;
    unsigned char flag;
    int xml_len;
    int zip_len;
    short uri_len;
    unsigned char *xml;
    xmlDocPtr xml_doc;
    xmlChar *out;
    int out_len;
    char *encoding = NULL;
    void *cvt;
    char *utf8;
    int err;
    int endian_arch = gaiaEndianArch ();
    xmlGenericErrorFunc silentError = (xmlGenericErrorFunc) spliteSilentError;

/* validity check */
    if (!gaiaIsValidXmlBlob (blob, blob_size))
	return NULL;		/* cannot be an XmlBLOB */
    flag = *(blob + 1);
    if ((flag & GAIA_XML_LITTLE_ENDIAN) == GAIA_XML_LITTLE_ENDIAN)
	little_endian = 1;
    if ((flag & GAIA_XML_COMPRESSED) == GAIA_XML_COMPRESSED)
	compressed = 1;
    xml_len = gaiaImport32 (blob + 3, little_endian, endian_arch);
    zip_len = gaiaImport32 (blob + 7, little_endian, endian_arch);
    uri_len = gaiaImport16 (blob + 11, little_endian, endian_arch);

    if (compressed)
      {
	  /* unzipping the XML payload */
	  uLong refLen = xml_len;
	  const Bytef *in = blob + 15 + uri_len;
	  xml = malloc (xml_len + 1);
	  if (uncompress (xml, &refLen, in, zip_len) != Z_OK)
	    {
		/* uncompress error */
		spatialite_e ("XmlBLOB DEFLATE uncompress error\n");
		free (xml);
		return NULL;
	    }
	  *(xml + xml_len) = '\0';
      }
    else
      {
	  /* just copying the uncompressed XML payload */
	  xml = malloc (xml_len + 1);
	  memcpy (xml, blob + 15 + uri_len, xml_len);
	  *(xml + xml_len) = '\0';
      }
/* retrieving the XMLDocument encoding */
    xmlSetGenericErrorFunc (NULL, silentError);
    xml_doc =
	xmlReadMemory ((const char *) xml, xml_len, "noname.xml", NULL, 0);
    if (xml_doc == NULL)
      {
	  /* parsing error; not a well-formed XML */
	  xmlSetGenericErrorFunc ((void *) stderr, NULL);
	  return NULL;
      }
    if (xml_doc->encoding)
      {
	  /* using the internal character enconding */
	  int enclen = (int) strlen ((const char *) xml_doc->encoding);
	  encoding = malloc (enclen + 1);
	  strcpy (encoding, (const char *) (xml_doc->encoding));
      }
    else
      {
	  /* no declared encoding: defaulting to UTF-8 */
	  encoding = malloc (6);
	  strcpy (encoding, "UTF-8");
      }

    if (!indent)
      {
	  /* just returning the XMLDocument "as is" */
	  xmlFreeDoc (xml_doc);
	  cvt = gaiaCreateUTF8Converter (encoding);
	  free (encoding);
	  if (cvt == NULL)
	    {
		xmlSetGenericErrorFunc ((void *) stderr, NULL);
		return NULL;
	    }
	  utf8 = gaiaConvertToUTF8 (cvt, (const char *) xml, xml_len, &err);
	  free (xml);
	  gaiaFreeUTF8Converter (cvt);
	  if (utf8 && !err)
	    {
		xmlSetGenericErrorFunc ((void *) stderr, NULL);
		return utf8;
	    }
	  if (utf8)
	      free (utf8);
	  xmlSetGenericErrorFunc ((void *) stderr, NULL);
	  return NULL;
      }

/* properly indenting the XMLDocument */
    xmlDocDumpFormatMemory (xml_doc, &out, &out_len, 1);
    free (xml);
    xmlFreeDoc (xml_doc);
    cvt = gaiaCreateUTF8Converter (encoding);
    free (encoding);
    if (cvt == NULL)
      {
	  xmlSetGenericErrorFunc ((void *) stderr, NULL);
	  return NULL;
      }
    utf8 = gaiaConvertToUTF8 (cvt, (const char *) out, out_len, &err);
    gaiaFreeUTF8Converter (cvt);
    free (out);
    if (utf8 && !err)
      {
	  xmlSetGenericErrorFunc ((void *) stderr, NULL);
	  return utf8;
      }
    if (utf8)
	free (utf8);
    xmlSetGenericErrorFunc ((void *) stderr, NULL);
    return NULL;
}

GAIAGEO_DECLARE void
gaiaXmlFromBlob (const unsigned char *blob, int blob_size, int indent,
		 unsigned char **result, int *res_size)
{
/* attempting to extract an XMLDocument from within an XmlBLOB buffer */
    int compressed = 0;
    int little_endian = 0;
    unsigned char flag;
    int xml_len;
    int zip_len;
    short uri_len;
    unsigned char *xml;
    xmlDocPtr xml_doc;
    xmlChar *out;
    int out_len;
    int endian_arch = gaiaEndianArch ();
    xmlGenericErrorFunc silentError = (xmlGenericErrorFunc) spliteSilentError;
    *result = NULL;
    *res_size = 0;

/* validity check */
    if (!gaiaIsValidXmlBlob (blob, blob_size))
	return;			/* cannot be an XmlBLOB */
    flag = *(blob + 1);
    if ((flag & GAIA_XML_LITTLE_ENDIAN) == GAIA_XML_LITTLE_ENDIAN)
	little_endian = 1;
    if ((flag & GAIA_XML_COMPRESSED) == GAIA_XML_COMPRESSED)
	compressed = 1;
    xml_len = gaiaImport32 (blob + 3, little_endian, endian_arch);
    zip_len = gaiaImport32 (blob + 7, little_endian, endian_arch);
    uri_len = gaiaImport16 (blob + 11, little_endian, endian_arch);

    if (compressed)
      {
	  /* unzipping the XML payload */
	  uLong refLen = xml_len;
	  const Bytef *in = blob + 15 + uri_len;
	  xml = malloc (xml_len + 1);
	  if (uncompress (xml, &refLen, in, zip_len) != Z_OK)
	    {
		/* uncompress error */
		spatialite_e ("XmlBLOB DEFLATE uncompress error\n");
		free (xml);
		return;
	    }
	  *(xml + xml_len) = '\0';
      }
    else
      {
	  /* just copying the uncompressed XML payload */
	  xml = malloc (xml_len + 1);
	  memcpy (xml, blob + 15 + uri_len, xml_len);
	  *(xml + xml_len) = '\0';
      }
    if (!indent)
      {
	  /* just returning the XMLDocument "as is" */
	  *result = xml;
	  *res_size = xml_len;
	  return;
      }

/* properly indenting the XMLDocument */
    xmlSetGenericErrorFunc (NULL, silentError);
    xml_doc =
	xmlReadMemory ((const char *) xml, xml_len, "noname.xml", NULL, 0);
    if (xml_doc == NULL)
      {
	  /* parsing error; not a well-formed XML */
	  *result = xml;
	  *res_size = xml_len;
	  xmlSetGenericErrorFunc ((void *) stderr, NULL);
	  return;
      }
    xmlDocDumpFormatMemory (xml_doc, &out, &out_len, 1);
    free (xml);
    xmlFreeDoc (xml_doc);
    *result = out;
    *res_size = out_len;
    xmlSetGenericErrorFunc ((void *) stderr, NULL);
}

GAIAGEO_DECLARE int
gaiaIsCompressedXmlBlob (const unsigned char *blob, int blob_size)
{
/* Checks if a valid XmlBLOB buffer is compressed or not */
    int compressed = 0;
    unsigned char flag;

/* validity check */
    if (!gaiaIsValidXmlBlob (blob, blob_size))
	return -1;		/* cannot be an XmlBLOB */
    flag = *(blob + 1);
    if ((flag & GAIA_XML_COMPRESSED) == GAIA_XML_COMPRESSED)
	compressed = 1;
    return compressed;
}

GAIAGEO_DECLARE int
gaiaIsSchemaValidatedXmlBlob (const unsigned char *blob, int blob_size)
{
/* Checks if a valid XmlBLOB buffer has succesfully passed a formal Schema validation or not */
    int validated = 0;
    unsigned char flag;

/* validity check */
    if (!gaiaIsValidXmlBlob (blob, blob_size))
	return -1;		/* cannot be an XmlBLOB */
    flag = *(blob + 1);
    if ((flag & GAIA_XML_VALIDATED) == GAIA_XML_VALIDATED)
	validated = 1;
    return validated;
}

GAIAGEO_DECLARE int
gaiaXmlBlobGetDocumentSize (const unsigned char *blob, int blob_size)
{
/* Return the XMLDocument size (in bytes) from a valid XmlBLOB buffer */
    int little_endian = 0;
    unsigned char flag;
    int xml_len;
    int endian_arch = gaiaEndianArch ();

/* validity check */
    if (!gaiaIsValidXmlBlob (blob, blob_size))
	return -1;		/* cannot be an XmlBLOB */
    flag = *(blob + 1);
    if ((flag & GAIA_XML_LITTLE_ENDIAN) == GAIA_XML_LITTLE_ENDIAN)
	little_endian = 1;
    xml_len = gaiaImport32 (blob + 3, little_endian, endian_arch);
    return xml_len;
}

GAIAGEO_DECLARE int
gaiaXmlBlobHasSchemaURI (const unsigned char *blob, int blob_size)
{
/* Checks if a valid XmlBLOB buffer has a SchemaURI or not */
    int little_endian = 0;
    unsigned char flag;
    short uri_len;
    int endian_arch = gaiaEndianArch ();

/* validity check */
    if (!gaiaIsValidXmlBlob (blob, blob_size))
	return -1;		/* cannot be an XmlBLOB */
    flag = *(blob + 1);
    if ((flag & GAIA_XML_LITTLE_ENDIAN) == GAIA_XML_LITTLE_ENDIAN)
	little_endian = 1;
    uri_len = gaiaImport16 (blob + 11, little_endian, endian_arch);
    if (!uri_len)
	return 0;
    else
	return 1;
}

GAIAGEO_DECLARE char *
gaiaXmlBlobGetSchemaURI (const unsigned char *blob, int blob_size)
{
/* Return the SchemaURI from a valid XmlBLOB buffer */
    int little_endian = 0;
    unsigned char flag;
    short uri_len;
    char *uri;
    int endian_arch = gaiaEndianArch ();

/* validity check */
    if (!gaiaIsValidXmlBlob (blob, blob_size))
	return NULL;		/* cannot be an XmlBLOB */
    flag = *(blob + 1);
    if ((flag & GAIA_XML_LITTLE_ENDIAN) == GAIA_XML_LITTLE_ENDIAN)
	little_endian = 1;
    uri_len = gaiaImport16 (blob + 11, little_endian, endian_arch);
    if (!uri_len)
	return NULL;

    uri = malloc (uri_len + 1);
    memcpy (uri, blob + 14, uri_len);
    *(uri + uri_len) = '\0';
    return uri;
}

GAIAGEO_DECLARE char *
gaiaXmlGetInternalSchemaURI (void *p_cache, const char *xml, int xml_len)
{
/* Return the internally defined SchemaURI from a valid XmlDocument */
    xmlDocPtr xml_doc;
    char *uri = NULL;
    xmlXPathContextPtr xpathCtx;
    xmlXPathObjectPtr xpathObj;
    xmlGenericErrorFunc silentError = (xmlGenericErrorFunc) spliteSilentError;

/* retrieving the XMLDocument internal SchemaURI (if any) */
    xmlSetGenericErrorFunc (NULL, silentError);
    xml_doc =
	xmlReadMemory ((const char *) xml, xml_len, "noname.xml", NULL, 0);
    if (xml_doc == NULL)
      {
	  /* parsing error; not a well-formed XML */
	  xmlSetGenericErrorFunc ((void *) stderr, NULL);
	  return NULL;
      }

    if (vxpath_eval_expr
	(p_cache, xml_doc, "/*/@xsi:schemaLocation", &xpathCtx, &xpathObj))
      {
	  /* attempting first to extract xsi:schemaLocation */
	  xmlNodeSetPtr nodeset = xpathObj->nodesetval;
	  xmlNodePtr node;
	  int num_nodes = (nodeset) ? nodeset->nodeNr : 0;
	  if (num_nodes == 1)
	    {
		node = nodeset->nodeTab[0];
		if (node->type == XML_ATTRIBUTE_NODE)
		  {
		      if (node->children != NULL)
			{
			    if (node->children->content != NULL)
			      {
				  const char *str =
				      (const char *) (node->children->content);
				  const char *ptr = str;
				  int i;
				  int len = strlen (str);
				  for (i = len - 1; i >= 0; i--)
				    {
					if (*(str + i) == ' ')
					  {
					      /* last occurrence of SPACE [namespace/schema separator] */
					      ptr = str + i + 1;
					      break;
					  }
				    }
				  len = strlen (ptr);
				  uri = malloc (len + 1);
				  strcpy (uri, ptr);
			      }
			}
		  }
	    }
	  if (uri != NULL)
	      xmlXPathFreeContext (xpathCtx);
	  xmlXPathFreeObject (xpathObj);
      }
    if (uri == NULL)
      {
	  /* checking for xsi:noNamespaceSchemaLocation */
	  if (vxpath_eval_expr
	      (p_cache, xml_doc, "/*/@xsi:noNamespaceSchemaLocation", &xpathCtx,
	       &xpathObj))
	    {
		xmlNodeSetPtr nodeset = xpathObj->nodesetval;
		xmlNodePtr node;
		int num_nodes = (nodeset) ? nodeset->nodeNr : 0;
		if (num_nodes == 1)
		  {
		      node = nodeset->nodeTab[0];
		      if (node->type == XML_ATTRIBUTE_NODE)
			{
			    if (node->children != NULL)
			      {
				  if (node->children->content != NULL)
				    {
					int len =
					    strlen ((const char *)
						    node->children->content);
					uri = malloc (len + 1);
					strcpy (uri,
						(const char *) node->
						children->content);
				    }
			      }
			}
		  }
		xmlXPathFreeContext (xpathCtx);
		xmlXPathFreeObject (xpathObj);
	    }
      }

    xmlFreeDoc (xml_doc);
    xmlSetGenericErrorFunc ((void *) stderr, NULL);
    return uri;
}

GAIAGEO_DECLARE char *
gaiaXmlBlobGetEncoding (const unsigned char *blob, int blob_size)
{
/* Return the Charset Encoding from a valid XmlBLOB buffer */
    int compressed = 0;
    int little_endian = 0;
    unsigned char flag;
    int xml_len;
    int zip_len;
    short uri_len;
    unsigned char *xml;
    xmlDocPtr xml_doc;
    char *encoding = NULL;
    int endian_arch = gaiaEndianArch ();
    xmlGenericErrorFunc silentError = (xmlGenericErrorFunc) spliteSilentError;

/* validity check */
    if (!gaiaIsValidXmlBlob (blob, blob_size))
	return NULL;		/* cannot be an XmlBLOB */
    flag = *(blob + 1);
    if ((flag & GAIA_XML_LITTLE_ENDIAN) == GAIA_XML_LITTLE_ENDIAN)
	little_endian = 1;
    if ((flag & GAIA_XML_COMPRESSED) == GAIA_XML_COMPRESSED)
	compressed = 1;
    xml_len = gaiaImport32 (blob + 3, little_endian, endian_arch);
    zip_len = gaiaImport32 (blob + 7, little_endian, endian_arch);
    uri_len = gaiaImport16 (blob + 11, little_endian, endian_arch);

    if (compressed)
      {
	  /* unzipping the XML payload */
	  uLong refLen = xml_len;
	  const Bytef *in = blob + 15 + uri_len;
	  xml = malloc (xml_len + 1);
	  if (uncompress (xml, &refLen, in, zip_len) != Z_OK)
	    {
		/* uncompress error */
		spatialite_e ("XmlBLOB DEFLATE uncompress error\n");
		free (xml);
		return NULL;
	    }
	  *(xml + xml_len) = '\0';
      }
    else
      {
	  /* just copying the uncompressed XML payload */
	  xml = malloc (xml_len + 1);
	  memcpy (xml, blob + 15 + uri_len, xml_len);
	  *(xml + xml_len) = '\0';
      }
/* retrieving the XMLDocument encoding */
    xmlSetGenericErrorFunc (NULL, silentError);
    xml_doc =
	xmlReadMemory ((const char *) xml, xml_len, "noname.xml", NULL, 0);
    if (xml_doc == NULL)
      {
	  /* parsing error; not a well-formed XML */
	  xmlSetGenericErrorFunc ((void *) stderr, NULL);
	  return NULL;
      }
    free (xml);
    if (xml_doc->encoding)
      {
	  /* using the internal character enconding */
	  int enclen = strlen ((const char *) xml_doc->encoding);
	  encoding = malloc (enclen + 1);
	  strcpy (encoding, (const char *) xml_doc->encoding);
	  xmlFreeDoc (xml_doc);
	  xmlSetGenericErrorFunc ((void *) stderr, NULL);
	  return encoding;
      }
    xmlFreeDoc (xml_doc);
    xmlSetGenericErrorFunc ((void *) stderr, NULL);
    return NULL;
}

GAIAGEO_DECLARE char *
gaia_libxml2_version (void)
{
/* return the current LIBXML2 version */
    int len;
    char *version;
    const char *ver = LIBXML_DOTTED_VERSION;
    len = strlen (ver);
    version = malloc (len + 1);
    strcpy (version, ver);
    return version;
}

#endif /* end LIBXML2: supporting XML documents */
