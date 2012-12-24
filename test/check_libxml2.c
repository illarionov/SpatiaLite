
/*

 check_libxml2.c -- SpatiaLite Test Case

 Author: Sandro Furieri <a.furieri@lqt.it>

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

#include "config.h"

#include "sqlite3.h"
#include "spatialite.h"

#ifdef ENABLE_LIBXML2	/* only if LIBXML2 is supported */

static int
check_bad_xml ()
{
/* parsing a not-well-formed XML Sample */
    FILE *fl;
    int sz = 0;
    int rd;
    unsigned char *xml = NULL;
    unsigned char *p_result = NULL;
    int len;
    char *err1;
    char *err2;
    char *version = gaia_libxml2_version ();
    
    if (version == NULL) {
	fprintf (stderr, "unable to get the library version\n");
	return 0;
    }
    free (version);

/* loading the XMLDocument */
    fl = fopen("books-bad.xml", "rb");
    if (!fl) {
	fprintf (stderr, "cannot open \"books-bad.xml\"\n");
	return 0;
    }
    if (fseek(fl, 0, SEEK_END) == 0)
        sz = ftell(fl);
    xml = (unsigned char *) malloc(sz);
    rewind(fl);
    rd = fread(xml, 1, sz, fl);
    if (rd != sz) {
	fprintf (stderr, "read error \"books-bad.xml\"\n");
	return 0;
    }
    fclose(fl);

/* parsing the XMLDocument */
    gaiaXmlToBlob (xml, rd, 1, NULL, &p_result, &len, &err1, &err2); 
    if (p_result != NULL) {
        fprintf (stderr, "this is not a well-formed XML !!!\n");
        return 0;
    }

    free(xml);
    return 1;
}

static int
check_bad_schema ()
{
/* validating by invalid Schema */
    FILE *fl;
    int sz = 0;
    int rd;
    unsigned char *xml = NULL;
    unsigned char *p_result = NULL;
    int len;
    char *err1;
    char *err2;

/* loading the XMLDocument */
    fl = fopen("books.xml", "rb");
    if (!fl) {
	fprintf (stderr, "cannot open \"books.xml\"\n");
	return 0;
    }
    if (fseek(fl, 0, SEEK_END) == 0)
        sz = ftell(fl);
    xml = (unsigned char *) malloc(sz);
    rewind(fl);
    rd = fread(xml, 1, sz, fl);
    if (rd != sz) {
	fprintf (stderr, "read error \"books.xml\"\n");
	return 0;
    }
    fclose(fl);

/* validating the XMLDocument */
    gaiaXmlToBlob (xml, rd, 1, "books-bad.xsd", &p_result, &len, &err1, &err2); 
    if (p_result != NULL) {
        fprintf (stderr, "this is not a valid XML !!!\n");
        return 0;
    }

    free(xml);
    return 1;
}

static int
check_validate (const char *path)
{
/* validating an XML Sample */
    FILE *fl;
    int sz = 0;
    int rd;
    unsigned char *xml = NULL;
    char *schema_uri = NULL;
    char *schema_uri2 = NULL;
    unsigned char *p_result = NULL;
    int len;

/* loading the XMLDocument */
    fl = fopen(path, "rb");
    if (!fl) {
	fprintf (stderr, "cannot open \"%s\"\n", path);
	return 0;
    }
    if (fseek(fl, 0, SEEK_END) == 0)
        sz = ftell(fl);
    xml = (unsigned char *) malloc(sz);
    rewind(fl);
    rd = fread(xml, 1, sz, fl);
    if (rd != sz) {
	fprintf (stderr, "read error \"%s\"\n", path);
	return 0;
    }
    fclose(fl);

/* extracting the Internal SchemaURI */
    schema_uri = gaiaXmlGetInternalSchemaURI (xml, rd);
    if (schema_uri == NULL) {
        fprintf (stderr, "unable to identify the Schema for \"%s\"\n", path);
        return 0;
    }
/* validating the XMLDocument */
    gaiaXmlToBlob (xml, rd, 1, schema_uri, &p_result, &len, NULL, NULL);
    if (p_result == NULL) {
        fprintf (stderr, "unable to validate \"%s\"\n", path);
        return 0;
    }
    if (!gaiaIsSchemaValidatedXmlBlob (p_result, len)) {
        fprintf (stderr, "validation failed: \"%s\"\n", path);
        return 0;
    }
    if (!gaiaXmlBlobHasSchemaURI (p_result, len)) {
        fprintf (stderr, "%s: has no ValidationSchemaURI\n", path);
        return 0;
    }
    schema_uri2 = gaiaXmlBlobGetSchemaURI (p_result, len);
    if (schema_uri2 == NULL) {
        fprintf (stderr, "unable to retrieve the ValidationSchemaURI for \"%s\"\n", path);
        return 0;
    }
    if (strcmp (schema_uri, schema_uri2) != 0) {
        fprintf (stderr, "%s: mismatching SchemaURI \"%s\" (expected \"%s\")\n", schema_uri2, schema_uri);
        return 0;
    }
     
    free (schema_uri);
    free (schema_uri2);
    free(p_result);
    free(xml);

    return 1;
}

static int
check_parse (const char *path)
{
/* parsing an XML Sample */
    FILE *fl;
    int sz = 0;
    int rd;
    unsigned char *xml = NULL;
    int compressed_sz;
    int uncompressed_sz;
    int doc_sz;
    int formatted_sz;
    int formatted_txt_sz;
    unsigned char *p_result = NULL;
    unsigned char *out;
    char *txt;

/* loading the XMLDocument */
    fl = fopen(path, "rb");
    if (!fl) {
	fprintf (stderr, "cannot open \"%s\"\n", path);
	return 0;
    }
    if (fseek(fl, 0, SEEK_END) == 0)
        sz = ftell(fl);
    xml = (unsigned char *) malloc(sz);
    rewind(fl);
    rd = fread(xml, 1, sz, fl);
    if (rd != sz) {
	fprintf (stderr, "read error \"%s\"\n", path);
	return 0;
    }
    fclose(fl);

/* parsing the XMLDocument (no validation / compressed) */
    gaiaXmlToBlob (xml, rd, 1, NULL, &p_result, &compressed_sz, NULL, NULL);
    if (p_result == NULL) {
        fprintf (stderr, "unable to parse(1) \"%s\"\n", path);
        return 0;
    }
    doc_sz = gaiaXmlBlobGetDocumentSize (p_result, compressed_sz);
    gaiaXmlFromBlob (p_result, compressed_sz, 1, &out, &formatted_sz);
    if (out == NULL) {
        fprintf (stderr, "unable to format(1) \"%s\"\n", path);
        return 0;
    }
    free(out);
    txt = gaiaXmlTextFromBlob (p_result, compressed_sz, 1);
    if (txt == NULL) {
        fprintf (stderr, "unable to format-text(1) \"%s\"\n", path);
        return 0;
    }
    formatted_txt_sz = strlen(txt);
    free(txt);
    free(p_result);

/* parsing the XMLDocument (no validation / not compressed) */
    gaiaXmlToBlob (xml, rd, 0, NULL, &p_result, &uncompressed_sz, NULL, NULL);
    if (p_result == NULL) {
        fprintf (stderr, "unable to parse(2) \"%s\"\n", path);
        return 0;
    }
    free(p_result);
    
    if (strcmp(path, "books.xml") == 0) {
        if (compressed_sz != 414) {
            fprintf (stderr, "books.xml: unexpected compressed size %d (expected 414)\n", compressed_sz);
            return 0; 
        }
        if (uncompressed_sz != 762) {
            fprintf (stderr, "books.xml: unexpected compressed size %d (expected 762)\n", uncompressed_sz);
            return 0; 
        }
        if (doc_sz != 741) {
            fprintf (stderr, "books.xml: unexpected document size %d (expected 741)\n", doc_sz);
            return 0; 
        }
        if (formatted_sz != 730) {
            fprintf (stderr, "books.xml: unexpected formatted size %d (expected 730)\n", formatted_sz);
            return 0; 
        }
        if (formatted_txt_sz != 730) {
            fprintf (stderr, "books.xml: unexpected formatted-text size %d (expected 730)\n", formatted_txt_sz);
            return 0; 
        }
    }
    if (strcmp(path, "opera.xml") == 0) {
        if (compressed_sz != 407) {
            fprintf (stderr, "opera.xml: unexpected compressed size %d (expected 407)\n", compressed_sz);
            return 0; 
        }
        if (uncompressed_sz != 933) {
            fprintf (stderr, "opera.xml: unexpected compressed size %d (expected 933)\n", uncompressed_sz);
            return 0; 
        }
        if (doc_sz != 912) {
            fprintf (stderr, "opera.xml: unexpected document size %d (expected 912)\n", doc_sz);
            return 0; 
        }
        if (formatted_sz != 879) {
            fprintf (stderr, "opera.xml: unexpected formatted size %d (expected 879)\n", formatted_sz);
            return 0; 
        }
        if (formatted_txt_sz != 879) {
            fprintf (stderr, "opera.xml: unexpected formatted-text size %d (expected 879)\n", formatted_txt_sz);
            return 0; 
        }
    }
    if (strcmp(path, "movies.xml") == 0) {
        if (compressed_sz != 559) {
            fprintf (stderr, "movies.xml: unexpected compressed size %d (expected 559)\n", compressed_sz);
            return 0; 
        }
        if (uncompressed_sz != 1791) {
            fprintf (stderr, "movies.xml: unexpected compressed size %d (expected 1791)\n", uncompressed_sz);
            return 0; 
        }
        if (doc_sz != 1770) {
            fprintf (stderr, "movies.xml: unexpected document size %d (expected 1770)\n", doc_sz);
            return 0; 
        }
        if (formatted_sz != 1710) {
            fprintf (stderr, "movies.xml: unexpected formatted size %d (expected 1710)\n", formatted_sz);
            return 0; 
        }
        if (formatted_txt_sz != 854) {
            fprintf (stderr, "movies.xml: unexpected formatted-text size %d (expected 854)\n", formatted_txt_sz);
            return 0; 
        }
    }
    free(xml);

    return 1;
}

#endif 

int main (int argc, char *argv[])
{
    int ret;
    sqlite3 *handle;

    spatialite_init (0);
    ret = sqlite3_open_v2 (":memory:", &handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "cannot open in-memory db: %s\n", sqlite3_errmsg (handle));
	sqlite3_close(handle);
	return -1;
    }

#ifdef ENABLE_LIBXML2	/* only if LIBXML2 is supported */

    if (!check_parse("books.xml")) {
        fprintf (stderr, "unable to parse \"books.xml\"\n");
        return -2;
    }
    if (!check_parse("opera.xml")) {
        fprintf (stderr, "unable to parse \"opera.xml\"\n");
        return -3;
    }
    if (!check_parse("movies.xml")) {
        fprintf (stderr, "unable to parse \"movies.xml\"\n");
        return -4;
    }

    if (!check_validate("books.xml")) {
        fprintf (stderr, "unable to validate \"books.xml\"\n");
        return -5;
    }
    if (!check_validate("opera.xml")) {
        fprintf (stderr, "unable to validate \"opera.xml\"\n");
        return -6;
    }
    if (!check_validate("movies.xml")) {
        fprintf (stderr, "unable to validate \"movies.xml\"\n");
        return -7;
    }

    if (!check_bad_xml()) {
        fprintf (stderr, "unable to test not well-formed XML\n");
        return -8;
    }
    if (!check_bad_schema()) {
        fprintf (stderr, "unable to test invalid Schema\n");
        return -9;
    }

#endif
    
    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (handle));
	return -10;
    }
        
    spatialite_cleanup();
    
    return 0;
}
