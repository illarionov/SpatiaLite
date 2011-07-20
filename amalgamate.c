/* 
**
** amalgamate.c
**
** produces the SpatiaLite's AMALGAMATION
**
*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>

#define PREFIX	"./src"

struct masked_keyword
{
    char *keyword;
    struct masked_keyword *next;
};

static void
do_auto_sh (FILE * out)
{
/* producing the auto-sh script [automake chain] */
    fprintf (out, "cd ./amalgamation\n");
    fprintf (out, "echo aclocal\n");
    fprintf (out, "aclocal\n");
    fprintf (out, "echo autoconf\n");
    fprintf (out, "autoconf\n");
    fprintf (out, "echo libtoolize\n");
    fprintf (out, "libtoolize\n");
    fprintf (out, "echo automake --add-missing --foreign\n");
    fprintf (out, "automake --add-missing --foreign\n\n");
}

static void
do_headers (FILE * out, struct masked_keyword *first)
{
/* prepares the headers for SpatiaLite-Amalgamation */
    struct masked_keyword *p;
    char now[64];
    time_t now_time;
    struct tm *tmp;
    now_time = time (NULL);
    tmp = localtime (&now_time);
    strftime (now, 64, "%Y-%m-%d %H:%M:%S %z", tmp);
    fprintf (out,
	     "/******************************************************************************\n");
    fprintf (out,
	     "** This file is an amalgamation of many separate C source files from SpatiaLite\n");
    fprintf (out,
	     "** version 3.0.0.  By combining all the individual C code files into this\n");
    fprintf (out,
	     "** single large file, the entire code can be compiled as a one translation\n");
    fprintf (out,
	     "** unit.  This allows many compilers to do optimizations that would not be\n");
    fprintf (out,
	     "** possible if the files were compiled separately.  Performance improvements\n");
    fprintf (out,
	     "** of 5%% are more are commonly seen when SQLite is compiled as a single\n");
    fprintf (out, "** translation unit.\n");
    fprintf (out, "**\n** This amalgamation was generated on %s.\n\n", now);
    fprintf (out, "Author: Alessandro (Sandro) Furieri <a.furieri@lqt.it>\n\n");
    fprintf (out,
	     "------------------------------------------------------------------------------\n\n");
    fprintf (out, "Version: MPL 1.1/GPL 2.0/LGPL 2.1\n\n");
    fprintf (out,
	     "The contents of this file are subject to the Mozilla Public License Version\n");
    fprintf (out,
	     "1.1 (the \"License\"); you may not use this file except in compliance with\n");
    fprintf (out, "the License. You may obtain a copy of the License at\n");
    fprintf (out, "http://www.mozilla.org/MPL/\n\n");
    fprintf (out,
	     "Software distributed under the License is distributed on an \"AS IS\" basis,\n");
    fprintf (out,
	     "WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License\n");
    fprintf (out,
	     "for the specific language governing rights and limitations under the\n");
    fprintf (out, "License.\n\n");
    fprintf (out, "The Original Code is the SpatiaLite library\n\n");
    fprintf (out,
	     "The Initial Developer of the Original Code is Alessandro Furieri\n\n");
    fprintf (out,
	     "Portions created by the Initial Developer are Copyright (C) 2008\n");
    fprintf (out, "the Initial Developer. All Rights Reserved.\n\n");
    fprintf (out, "Contributor(s):\n");
    fprintf (out, "Klaus Foerster klaus.foerster@svg.cc\n");
    fprintf (out, "Luigi Costalli luigi.costalli@gmail.com\n");
    fprintf (out, "The Vanuatu Team - University of Toronto\n");
    fprintf (out, "\tSupervisor: Greg Wilson gwilson@cs.toronto.ca\n");
    fprintf (out, "\n");
    fprintf (out,
	     "Alternatively, the contents of this file may be used under the terms of\n");
    fprintf (out,
	     "either the GNU General Public License Version 2 or later (the \"GPL\"), or\n");
    fprintf (out,
	     "the GNU Lesser General Public License Version 2.1 or later (the \"LGPL\"),\n");
    fprintf (out,
	     "in which case the provisions of the GPL or the LGPL are applicable instead\n");
    fprintf (out,
	     "of those above. If you wish to allow use of your version of this file only\n");
    fprintf (out,
	     "under the terms of either the GPL or the LGPL, and not to allow others to\n");
    fprintf (out,
	     "use your version of this file under the terms of the MPL, indicate your\n");
    fprintf (out,
	     "decision by deleting the provisions above and replace them with the notice\n");
    fprintf (out,
	     "and other provisions required by the GPL or the LGPL. If you do not delete\n");
    fprintf (out,
	     "the provisions above, a recipient may use your version of this file under\n");
    fprintf (out,
	     "the terms of any one of the MPL, the GPL or the LGPL.\n\n*/\n\n");
    fprintf (out, "#if defined(_WIN32) && !defined(__MINGW32__)\n");
    fprintf (out, "/* MSVC strictly requires this include [off_t] */\n");
    fprintf (out, "#include <sys/types.h>\n");
    fprintf (out, "#endif\n\n");
    fprintf (out, "#include <stdio.h>\n");
    fprintf (out, "#include <stdlib.h>\n");
    fprintf (out, "#include <string.h>\n");
    fprintf (out, "#include <memory.h>\n");
    fprintf (out, "#include <limits.h>\n");
    fprintf (out, "#include <math.h>\n");
    fprintf (out, "#include <float.h>\n");
    fprintf (out, "#include <locale.h>\n");
    fprintf (out, "#include <errno.h>\n\n");
    fprintf (out, "#include <assert.h>\n\n");
    fprintf (out, "#if defined(__MINGW32__) || defined(_WIN32)\n");
    fprintf (out, "#define LIBICONV_STATIC\n");
    fprintf (out, "#include <iconv.h>\n");
    fprintf (out, "#define LIBCHARSET_STATIC\n");
    fprintf (out, "#ifdef _MSC_VER\n");
    fprintf (out, "/* <localcharset.h> isn't supported on OSGeo4W */\n");
    fprintf (out, "/* applying a tricky workaround to fix this issue */\n");
    fprintf (out, "extern const char * locale_charset (void);\n");
    fprintf (out, "#else /* sane Windows - not OSGeo4W */\n");
    fprintf (out, "#include <localcharset.h>\n");
    fprintf (out, "#endif /* end localcharset */\n");
    fprintf (out, "#else /* not WINDOWS */\n");
    fprintf (out, "#ifdef __APPLE__\n");
    fprintf (out, "#include <iconv.h>\n");
    fprintf (out, "#include <localcharset.h>\n");
    fprintf (out, "#else /* not Mac OsX */\n");
    fprintf (out, "#include <iconv.h>\n");
    fprintf (out, "#include <langinfo.h>\n");
    fprintf (out, "#endif\n#endif\n\n");
    fprintf (out, "#if OMIT_GEOS == 0		/* including GEOS */\n");
    fprintf (out, "#include <geos_c.h>\n");
    fprintf (out, "#endif\n\n");
    fprintf (out, "#if OMIT_PROJ == 0		/* including PROJ.4 */\n");
    fprintf (out, "#include <proj_api.h>\n");
    fprintf (out, "#endif\n\n");
    fprintf (out, "#ifdef _WIN32\n");
    fprintf (out, "#define strcasecmp\t_stricmp\n");
    fprintf (out, "#define strncasecmp\t_strnicmp\n");
    fprintf (out, "#define atoll\t_atoi64\n");
    fprintf (out, "#endif /* not WIN32 */\n\n");
    fprintf (out, "/*\n** alias MACROs to avoid any potential collision\n");
    fprintf (out, "** for linker symbols declared into the sqlite3 code\n");
    fprintf (out, "** internally embedded into SpatiaLite\n*/\n");
    p = first;
    while (p)
      {
	  char alias[1024];
	  strcpy (alias, p->keyword);
	  alias[0] = 'S';
	  alias[1] = 'P';
	  alias[2] = 'L';
	  fprintf (out, "#define %s %s\n", p->keyword, alias);
	  p = p->next;
      }
    fprintf (out, "/* end SpatiaLite/sqlite3 alias macros */\n\n");
}

static void
do_note (FILE * out, const char *file, int mode)
{
/* begin/end file markerts */
    if (mode)
	fprintf (out, "/**************** End file: %s **********/\n\n", file);
    else
	fprintf (out, "\n/**************** Begin file: %s **********/\n", file);
}

static void
do_sqlite3_dll (FILE * out, struct masked_keyword *first)
{
/* inserting #ifdef to build a Windows DLL */
    struct masked_keyword *p;
    fprintf (out, "#ifdef DLL_EXPORT\n");
    fprintf (out, "#define SQLITE_API __declspec(dllexport)\n");
    fprintf (out, "#else\n#define SQLITE_API\n#endif\n\n");
    fprintf (out, "/*\n** the following macros ensure that the sqlite3\n");
    fprintf (out, "** code internally embedded in SpatiaLite never defines\n");
    fprintf (out, "** any linker symbol potentially conflicting with\n");
    fprintf (out, "** an external sqlite3 library\n*/\n");
    p = first;
    while (p)
      {
	  char alias[1024];
	  strcpy (alias, p->keyword);
	  if (strcmp (alias, "sqlite3_column_database_name") == 0 ||
	      strcmp (alias, "sqlite3_column_database_name16") == 0 ||
	      strcmp (alias, "sqlite3_column_table_name") == 0 ||
	      strcmp (alias, "sqlite3_column_table_name16") == 0 ||
	      strcmp (alias, "sqlite3_column_origin_name") == 0 ||
	      strcmp (alias, "sqlite3_column_origin_name16") == 0 ||
	      strcmp (alias, "sqlite3_table_column_metadata") == 0)
	    {
/* avoiding to define METADATA symbols (usually disabled) */
		p = p->next;
		continue;
	    }
	  alias[0] = 'S';
	  alias[1] = 'P';
	  alias[2] = 'L';
	  fprintf (out, "#define %s %s\n", p->keyword, alias);
	  p = p->next;
      }
    fprintf (out, "/* End SpatiaLite alias-MACROs */\n\n");
}

static int
is_header (const char *row)
{
/* checks for #include */
    if (strncmp (row, "#include <inttypes.h>", 21) == 0)
      {
	  /* note well: inttypes.h must not be commented */
	  return 0;
      }
    if (strlen (row) >= 8 && strncmp (row, "#include", 8) == 0)
	return 1;
    return 0;
}

static void
do_copy (FILE * out, const char *basedir, const char *file)
{

/* copy a source file suppressimng the boiler-plate and headers */
    char input[1024];
    char row[256];
    char *p = row;
    int c;
    int boiler_plate = 0;
    int boiler_plate_found = 0;
    FILE *in;
    strcpy (input, PREFIX);
    strcat (input, basedir);
    strcat (input, file);
    in = fopen (input, "r");
    if (!in)
      {
	  fprintf (stderr, "Error opening %s\n", input);
	  return;
      }
    do_note (out, file, 0);
    while ((c = getc (in)) != EOF)
      {
	  *p++ = c;
	  if (c == '\n')
	    {
		*p = '\0';
		p = row;
		if (!boiler_plate_found && strlen (row) >= 2
		    && strncmp (row, "/*", 2) == 0)
		  {
		      boiler_plate_found = 1;
		      boiler_plate = 1;
		      continue;
		  }
		if (boiler_plate)
		  {
		      if (strlen (row) >= 2 && strncmp (row, "*/", 2) == 0)
			  boiler_plate = 0;
		      continue;
		  }
		if (is_header (row))
		  {
		      row[strlen (row) - 1] = '\0';
		      fprintf (out, "/* %s */\n", row);
		      continue;
		  }
		fprintf (out, "%s", row);
	    }
      }
    fclose (in);
    do_note (out, file, 1);
}


static void
do_copy_sqlite (FILE * out, const char *basedir, const char *file)
{
/* copy the sqlite3.h header */
    char input[1024];
    char row[256];
    char *p = row;
    int c;
    FILE *in;
    strcpy (input, PREFIX);
    strcat (input, basedir);
    strcat (input, file);
    in = fopen (input, "r");
    if (!in)
      {
	  fprintf (stderr, "Error opening %s\n", input);
	  return;
      }
    do_note (out, file, 0);
    while ((c = getc (in)) != EOF)
      {
	  *p++ = c;
	  if (c == '\n')
	    {
		*p = '\0';
		p = row;
		fprintf (out, "%s", row);
	    }
      }
    fclose (in);
    do_note (out, file, 1);
}

static void
do_copy_plain (FILE * out, const char *file)
{
/* copy a source AS IS */
    char input[1024];
    int c;
    FILE *in;
    strcpy (input, PREFIX);
    strcat (input, file);
    in = fopen (input, "r");
    if (!in)
      {
	  fprintf (stderr, "Error opening %s\n", input);
	  return;
      }
    while ((c = getc (in)) != EOF)
	putc (c, out);
    fclose (in);
}

static void
feed_export_keywords (char *row, struct masked_keyword **first,
		      struct masked_keyword **last)
{
    struct masked_keyword *p;
    char kw[1024];
    int len;
    int i;
    int skip = 0;
    int pos;
    int end = (int) strlen (row);
    for (i = 0; i < end; i++)
      {
	  if (row[i] == ' ' || row[i] == '\t')
	      skip++;
	  else
	      break;
      }
    if (strncmp (row + skip, "SPATIALITE_DECLARE ", 19) == 0)
	skip += 19;
    else if (strncmp (row + skip, "GAIAAUX_DECLARE ", 16) == 0)
	skip += 16;
    else if (strncmp (row + skip, "GAIAEXIF_DECLARE ", 17) == 0)
	skip += 17;
    else if (strncmp (row + skip, "GAIAGEO_DECLARE ", 16) == 0)
	skip += 16;
    else
	return;

    if (strncmp (row + skip, "const char *", 12) == 0)
	pos = skip + 12;
    else if (strncmp (row + skip, "unsigned char ", 14) == 0)
	pos = skip + 14;
    else if (strncmp (row + skip, "unsigned short ", 15) == 0)
	pos = skip + 15;
    else if (strncmp (row + skip, "unsigned int ", 13) == 0)
	pos = skip + 13;
    else if (strncmp (row + skip, "char *", 6) == 0)
	pos = skip + 6;
    else if (strncmp (row + skip, "void *", 6) == 0)
	pos = skip + 6;
    else if (strncmp (row + skip, "void ", 5) == 0)
	pos = skip + 5;
    else if (strncmp (row + skip, "int ", 4) == 0)
	pos = skip + 4;
    else if (strncmp (row + skip, "double ", 7) == 0)
	pos = skip + 7;
    else if (strncmp (row + skip, "float ", 6) == 0)
	pos = skip + 6;
    else if (strncmp (row + skip, "short ", 6) == 0)
	pos = skip + 6;
    else if (strncmp (row + skip, "sqlite3_int64 ", 14) == 0)
	pos = skip + 14;
    else if (strncmp (row + skip, "gaiaPointPtr ", 13) == 0)
	pos = skip + 13;
    else if (strncmp (row + skip, "gaiaLinestringPtr ", 18) == 0)
	pos = skip + 18;
    else if (strncmp (row + skip, "gaiaPolygonPtr ", 15) == 0)
	pos = skip + 15;
    else if (strncmp (row + skip, "gaiaRingPtr ", 12) == 0)
	pos = skip + 12;
    else if (strncmp (row + skip, "gaiaGeomCollPtr ", 16) == 0)
	pos = skip + 16;
    else if (strncmp (row + skip, "gaiaDynamicLinePtr ", 19) == 0)
	pos = skip + 19;
    else if (strncmp (row + skip, "gaiaDbfFieldPtr ", 16) == 0)
	pos = skip + 16;
    else if (strncmp (row + skip, "gaiaValuePtr ", 13) == 0)
	pos = skip + 13;
    else if (strncmp (row + skip, "gaiaExifTagListPtr ", 19) == 0)
	pos = skip + 19;
    else if (strncmp (row + skip, "gaiaExifTagPtr ", 15) == 0)
	pos = skip + 15;
    else
	return;

    for (i = pos; i < end; i++)
      {
	  if (row[i] == ' ' || row[i] == '(' || row[i] == '[' || row[i] == ';')
	    {
		end = i;
		break;
	    }
      }
    len = end - pos;
    memcpy (kw, row + pos, len);
    kw[len] = '\0';
    p = *first;
    while (p)
      {
	  if (strcmp (p->keyword, kw) == 0)
	      return;
	  p = p->next;
      }
    p = malloc (sizeof (struct masked_keyword));
    p->keyword = malloc (len + 1);
    strcpy (p->keyword, kw);
    p->next = NULL;
    if (*first == NULL)
	*first = p;
    if (*last != NULL)
	(*last)->next = p;
    *last = p;
}

static void
do_copy_export (FILE * out, const char *file, struct masked_keyword **first,
		struct masked_keyword **last)
{
/* copy a source AS IS */
    char input[1024];
    char row[1024];
    char *p = row;
    int c;
    FILE *in;
    strcpy (input, PREFIX);
    strcat (input, file);
    in = fopen (input, "r");
    if (!in)
      {
	  fprintf (stderr, "Error opening %s\n", input);
	  return;
      }
    while ((c = getc (in)) != EOF)
      {
	  if (c == '\n')
	    {
		*p = '\0';
		feed_export_keywords (row, first, last);
		fprintf (out, "%s\n", row);
		p = row;
		continue;
	    }
	  else
	      *p++ = c;
      }
    fclose (in);
}

static void
do_copy_header (FILE * out, const char *file, struct masked_keyword *first)
{
/* copy a source AS IS */
    struct masked_keyword *p;
    char input[1024];
    int c;
    FILE *in;
    strcpy (input, PREFIX);
    strcat (input, file);
    in = fopen (input, "r");
    if (!in)
      {
	  fprintf (stderr, "Error opening %s\n", input);
	  return;
      }
    fprintf (out, "/*\n** alias MACROs to avoid any potential collision\n");
    fprintf (out, "** for linker symbols declared into the sqlite3 code\n");
    fprintf (out, "** internally embedded into SpatiaLite\n*/\n");
    p = first;
    while (p)
      {
	  char alias[1024];
	  strcpy (alias, p->keyword);
	  alias[0] = 'S';
	  alias[1] = 'P';
	  alias[2] = 'L';
	  fprintf (out, "#define %s %s\n", p->keyword, alias);
	  p = p->next;
      }
    fprintf (out, "/* end SpatiaLite/sqlite3 alias macros */\n\n");
    while ((c = getc (in)) != EOF)
	putc (c, out);
    fclose (in);
}

static void
feed_masked_keywords (char *row, int pos, struct masked_keyword **first,
		      struct masked_keyword **last)
{
    struct masked_keyword *p;
    char kw[1024];
    int len;
    int i;
    int end = (int) strlen (row);
    for (i = pos; i < end; i++)
      {
	  if (row[i] == ' ' || row[i] == '(' || row[i] == '[' || row[i] == ';')
	    {
		end = i;
		break;
	    }
      }
    len = end - pos;
    memcpy (kw, row + pos, len);
    kw[len] = '\0';

/*
** caveat: this symbol is abdolutely required by loadable extension modules 
** and must *never* be masked
*/
    if (strcmp (kw, "sqlite3_extension_init") == 0)
	return;

    p = *first;
    while (p)
      {
	  if (strcmp (p->keyword, kw) == 0)
	      return;
	  p = p->next;
      }
    p = malloc (sizeof (struct masked_keyword));
    p->keyword = malloc (len + 1);
    strcpy (p->keyword, kw);
    p->next = NULL;
    if (*first == NULL)
	*first = p;
    if (*last != NULL)
	(*last)->next = p;
    *last = p;
}

static void
prepare_masked (const char *file, struct masked_keyword **first,
		struct masked_keyword **last)
{
/* feeding the ALIAS-macros */
    char input[1024];
    char row[1024];
    char *p = row;
    int c;
    FILE *in;
    strcpy (input, PREFIX);
    strcat (input, file);
    in = fopen (input, "r");
    if (!in)
      {
	  fprintf (stderr, "Error opening %s\n", input);
	  return;
      }
    while ((c = getc (in)) != EOF)
      {
	  if (c == '\n')
	    {
		*p = '\0';
		if (strncmp (row, "SQLITE_API int ", 15) == 0)
		    feed_masked_keywords (row, 15, first, last);
		else if (strncmp (row, "SQLITE_API double ", 18) == 0)
		    feed_masked_keywords (row, 18, first, last);
		else if (strncmp (row, "SQLITE_API void *", 17) == 0)
		    feed_masked_keywords (row, 17, first, last);
		else if (strncmp (row, "SQLITE_API void ", 16) == 0)
		    feed_masked_keywords (row, 16, first, last);
		else if (strncmp (row, "SQLITE_API char *", 17) == 0)
		    feed_masked_keywords (row, 17, first, last);
		else if (strncmp (row, "SQLITE_API const void *", 23) == 0)
		    feed_masked_keywords (row, 23, first, last);
		else if (strncmp (row, "SQLITE_API const char *", 23) == 0)
		    feed_masked_keywords (row, 23, first, last);
		else if (strncmp (row, "SQLITE_API const char ", 22) == 0)
		    feed_masked_keywords (row, 22, first, last);
		else if (strncmp (row, "SQLITE_API const unsigned char *", 32)
			 == 0)
		    feed_masked_keywords (row, 32, first, last);
		else if (strncmp (row, "SQLITE_API sqlite3_int64 ", 25) == 0)
		    feed_masked_keywords (row, 25, first, last);
		else if (strncmp (row, "SQLITE_API sqlite3_value *", 26) == 0)
		    feed_masked_keywords (row, 26, first, last);
		else if (strncmp (row, "SQLITE_API sqlite3_backup *", 27) == 0)
		    feed_masked_keywords (row, 27, first, last);
		else if (strncmp (row, "SQLITE_API sqlite3_mutex *", 26) == 0)
		    feed_masked_keywords (row, 26, first, last);
		else if (strncmp (row, "SQLITE_API sqlite3_stmt *", 25) == 0)
		    feed_masked_keywords (row, 25, first, last);
		else if (strncmp (row, "SQLITE_API sqlite3_vfs *", 24) == 0)
		    feed_masked_keywords (row, 24, first, last);
		else if (strncmp (row, "SQLITE_API sqlite3 *", 20) == 0)
		    feed_masked_keywords (row, 20, first, last);
		p = row;
		continue;
	    }
	  else
	      *p++ = c;
      }
    fclose (in);
}

static void
do_copy_ext (FILE * out, const char *basedir, const char *file)
{
/* copy the sqlite3ext.h header */
    char input[1024];
    char row[1024];
    char *p = row;
    int c;
    FILE *in;
    strcpy (input, PREFIX);
    strcat (input, basedir);
    strcat (input, file);
    in = fopen (input, "r");
    if (!in)
      {
	  fprintf (stderr, "Error opening %s\n", input);
	  return;
      }
    do_note (out, file, 0);
    while ((c = getc (in)) != EOF)
      {
	  if (c == '\n')
	    {
		*p = '\0';
		if (strlen (row) > 16)
		  {
		      if (strncmp (row, "#define sqlite3_", 16) == 0)
			{
			    row[8] = 'S';
			    row[9] = 'P';
			    row[10] = 'L';
			}
		  }
		if (strcmp (row, "#include \"sqlite3.h\"") == 0)
		    fprintf (out, "/* %s */\n", row);
		else
		    fprintf (out, "%s\n", row);
		p = row;
		continue;
	    }
	  else
	      *p++ = c;
      }
    fclose (in);
    do_note (out, file, 1);
}

static void
do_makefile (FILE * out)
{
/* generating the Makefile.am for headers */
    fprintf (out, "\nnobase_include_HEADERS = \\\n");
    fprintf (out, "\tspatialite.h \\\n");
    fprintf (out, "\tspatialite/gaiaexif.h \\\n");
    fprintf (out, "\tspatialite/gaiaaux.h \\\n");
    fprintf (out, "\tspatialite/gaiageo.h \\\n");
    fprintf (out, "\tspatialite/sqlite3.h \\\n");
    fprintf (out, "\tspatialite/sqlite3ext.h \\\n");
    fprintf (out, "\tspatialite/spatialite.h\n");
}

static void
do_output_dll_defs (FILE * out, struct masked_keyword *first,
		    struct masked_keyword *first_defn)
{
    struct masked_keyword *p;
    fprintf (out, "LIBRARY spatialite.dll\r\n");
    fprintf (out, "EXPORTS\r\n");
/* exporting symbols not found by automatic search */
    fprintf (out, "gaiaAddLinestringToGeomColl\r\n");
    fprintf (out, "gaiaAppendPointToDynamicLine\r\n");
    fprintf (out, "gaiaPrependPointToDynamicLine\r\n");
    fprintf (out, "gaiaReverseDynamicLine\r\n");
    fprintf (out, "gaiaDynamicLineSplitBefore\r\n");
    fprintf (out, "gaiaDynamicLineSplitAfter\r\n");
    fprintf (out, "gaiaDynamicLineJoinAfter\r\n");
    fprintf (out, "gaiaDynamicLineJoinBefore\r\n");
    fprintf (out, "gaiaGeomCollSimplifyPreserveTopology");
    p = first_defn;
    while (p)
      {
/* SpatiaLite Symbols */
	  fprintf (out, "%s\r\n", p->keyword);
	  p = p->next;
      }
    p = first;
    while (p)
      {
/* SQLite Symbols */
	  char alias[1024];
	  strcpy (alias, p->keyword);
	  alias[0] = 'S';
	  alias[1] = 'P';
	  alias[2] = 'L';
	  fprintf (out, "%s\r\n", alias);
	  p = p->next;
      }
}

static void
free_masked_keywords (struct masked_keyword *first,
		      struct masked_keyword *first_defn)
{
    struct masked_keyword *p = first;
    struct masked_keyword *pn;
    while (p)
      {
/* freeing masked keywords */
	  pn = p->next;
	  free (p->keyword);
	  free (p);
	  p = pn;
      }
    p = first_defn;
    while (p)
      {
/* freeing export keyworks */
	  pn = p->next;
	  free (p->keyword);
	  free (p);
	  p = pn;
      }
}

int
main ()
{
    struct masked_keyword *first = NULL;
    struct masked_keyword *last = NULL;
    struct masked_keyword *first_def = NULL;
    struct masked_keyword *last_def = NULL;
    FILE *out;

/* produces the AMALGAMATION */
    mkdir ("amalgamation", 0777);
    mkdir ("amalgamation/headers", 0777);
    mkdir ("amalgamation/headers/spatialite", 0777);
/* amalgamating SpatiaLite */
    prepare_masked ("/sqlite3/sqlite3.c", &first, &last);
    out = fopen ("amalgamation/spatialite.c", "wb");
    if (!out)
      {
	  fprintf (stderr, "Error opening amalgamation/amalgamation.c\n");
	  return 1;
      }
    do_headers (out, first);
    do_copy_sqlite (out, "/headers/spatialite/", "sqlite3.h");
    do_copy_ext (out, "/headers/spatialite/", "sqlite3ext.h");
    do_copy (out, "/headers/", "spatialite.h");
    do_copy (out, "/headers/spatialite/", "gaiaaux.h");
    do_copy (out, "/headers/spatialite/", "gaiaexif.h");
    do_copy (out, "/headers/spatialite/", "gaiageo.h");
    do_copy (out, "/headers/spatialite/", "spatialite.h");
    do_copy (out, "/gaiaaux/", "gg_sqlaux.c");
    do_copy (out, "/gaiaaux/", "gg_utf8.c");
    do_copy (out, "/gaiaexif/", "gaia_exif.c");
    do_copy (out, "/gaiageo/", "gg_advanced.c");
    do_copy (out, "/gaiageo/", "gg_endian.c");
    do_copy (out, "/gaiageo/", "gg_geometries.c");
    do_copy (out, "/gaiageo/", "gg_relations.c");
    do_copy (out, "/gaiageo/", "gg_geoscvt.c");
    do_copy (out, "/gaiageo/", "gg_shape.c");
    do_copy (out, "/gaiageo/", "gg_transform.c");
    do_copy (out, "/gaiageo/", "gg_wkb.c");
    do_copy (out, "/gaiageo/", "gg_geodesic.c");
    do_copy (out, "/spatialite/", "spatialite.c");
    do_copy (out, "/spatialite/", "mbrcache.c");
    do_copy (out, "/spatialite/", "virtualshape.c");
    do_copy (out, "/spatialite/", "virtualdbf.c");
    do_copy (out, "/spatialite/", "virtualnetwork.c");
    do_copy (out, "/spatialite/", "virtualspatialindex.c");
    do_copy (out, "/spatialite/", "virtualfdo.c");
    do_copy (out, "/virtualtext/", "virtualtext.c");
    do_copy (out, "/versioninfo/", "version.c");
    do_copy (out, "/gaiageo/", "gg_wkt.c");
    do_copy (out, "/srsinit/", "srs_init.c");
    do_copy (out, "/gaiageo/", "gg_vanuatu.c");
    do_copy (out, "/gaiageo/", "gg_ewkt.c");
    do_copy (out, "/gaiageo/", "gg_geoJSON.c");
    do_copy (out, "/gaiageo/", "gg_kml.c");
    do_copy (out, "/gaiageo/", "gg_gml.c");
    fclose (out);

/* setting up the HEADERS */
    out = fopen ("amalgamation/headers/spatialite/sqlite3.h", "wb");
    if (!out)
      {
	  fprintf (stderr,
		   "Error opening amalgamation/headers/spatialite/sqlite3.h\n");
	  return 1;
      }
    do_copy_header (out, "/headers/spatialite/sqlite3.h", first);
    fclose (out);
    out = fopen ("amalgamation/headers/spatialite/sqlite3ext.h", "wb");
    if (!out)
      {
	  fprintf (stderr,
		   "Error opening amalgamation/headers/spatialite/sqlite3.h\n");
	  return 1;
      }
    do_copy_header (out, "/headers/spatialite/sqlite3ext.h", first);
    fclose (out);
    out = fopen ("amalgamation/sqlite3.c", "wb");
    if (!out)
      {
	  fprintf (stderr, "Error opening amalgamation/sqlite3.c\n");
	  return 1;
      }
    do_sqlite3_dll (out, first);
    prepare_masked ("/sqlite3/sqlite3.c", &first, &last);
    do_copy_plain (out, "/sqlite3/sqlite3.c");
    fclose (out);
    out = fopen ("amalgamation/headers/spatialite/gaiaaux.h", "wb");
    if (!out)
      {
	  fprintf (stderr,
		   "Error opening amalgamation/headers/spatialite/gaiaaux.h\n");
	  return 1;
      }
    do_copy_export (out, "/headers/spatialite/gaiaaux.h", &first_def,
		    &last_def);
    fclose (out);
    out = fopen ("amalgamation/headers/spatialite/gaiageo.h", "wb");
    if (!out)
      {
	  fprintf (stderr,
		   "Error opening amalgamation/headers/spatialite/gaiageo.h\n");
	  return 1;
      }
    do_copy_export (out, "/headers/spatialite/gaiageo.h", &first_def,
		    &last_def);
    fclose (out);
    out = fopen ("amalgamation/headers/spatialite/gaiaexif.h", "wb");
    if (!out)
      {
	  fprintf (stderr,
		   "Error opening amalgamation/headers/spatialite/gaiaexif.h\n");
	  return 1;
      }
    do_copy_export (out, "/headers/spatialite/gaiaexif.h", &first_def,
		    &last_def);
    fclose (out);
    out = fopen ("amalgamation/headers/spatialite/spatialite.h", "wb");
    if (!out)
      {
	  fprintf (stderr,
		   "Error opening amalgamation/headers/spatialite/spatialite.h\n");
	  return 1;
      }
    do_copy_export (out, "/headers/spatialite/spatialite.h", &first_def,
		    &last_def);
    fclose (out);
    out = fopen ("amalgamation/headers/spatialite.h", "wb");
    if (!out)
      {
	  fprintf (stderr, "Error opening amalgamation/headers/spatialite.h\n");
	  return 1;
      }
    do_copy_export (out, "/headers/spatialite.h", &first_def, &last_def);
    fclose (out);
    out = fopen ("amalgamation/headers/Makefile.am", "wb");
    if (!out)
      {
	  fprintf (stderr, "Error opening amalgamation/headers/Makefile.am\n");
	  return 1;
      }
    do_makefile (out);
    fclose (out);

/* setting up the AUTOMAKE stuff */
    out = fopen ("amalgamation/AUTHORS", "wb");
    if (!out)
      {
	  fprintf (stderr, "Error opening amalgamation/AUTHORS\n");
	  return 1;
      }
    do_copy_plain (out, "/automake/AUTHORS");
    fclose (out);
    out = fopen ("amalgamation/COPYING", "wb");
    if (!out)
      {
	  fprintf (stderr, "Error opening amalgamation/COPYING\n");
	  return 1;
      }
    do_copy_plain (out, "/automake/COPYING");
    fclose (out);
    out = fopen ("amalgamation/INSTALL", "wb");
    if (!out)
      {
	  fprintf (stderr, "Error opening amalgamation/INSTALL\n");
	  return 1;
      }
    do_copy_plain (out, "/automake/INSTALL");
    fclose (out);
    out = fopen ("amalgamation/README", "wb");
    if (!out)
      {
	  fprintf (stderr, "Error opening amalgamation/README\n");
	  return 1;
      }
    do_copy_plain (out, "/automake/README");
    fclose (out);
    out = fopen ("amalgamation/configure.ac", "wb");
    if (!out)
      {
	  fprintf (stderr, "Error opening amalgamation/configure.ac\n");
	  return 1;
      }
    do_copy_plain (out, "/automake/configure.ac");
    fclose (out);
    out = fopen ("amalgamation/Makefile.am", "wb");
    if (!out)
      {
	  fprintf (stderr, "Error opening amalgamation/Makefile.am\n");
	  return 1;
      }
    do_copy_plain (out, "/automake/Makefile.am");
    fclose (out);
    out = fopen ("amalgamation/makefile.vc", "wb");
    if (!out)
      {
	  fprintf (stderr, "Error opening amalgamation/makefile.vc\n");
	  return 1;
      }
    do_copy_plain (out, "/automake/makefile.vc");
    fclose (out);
    out = fopen ("amalgamation/nmake.opt", "wb");
    if (!out)
      {
	  fprintf (stderr, "Error opening amalgamation/nmake.opt\n");
	  return 1;
      }
    do_copy_plain (out, "/automake/nmake.opt");
    out = fopen ("amalgamation/libspatialite.def", "wb");
    if (!out)
      {
	  fprintf (stderr, "Error opening amalgamation/libspatialite.def\n");
	  return 1;
      }
    do_copy_plain (out, "/automake/libspatialite.def");
    fclose (out);
    out = fopen ("amalgamation/libspatialite.def-update", "wb");
    if (!out)
      {
	  fprintf (stderr, "Error opening amalgamation/libspatialite.def\n");
	  return 1;
      }
    do_output_dll_defs (out, first, first_def);
    fclose (out);
    out = fopen ("amalgamation/spatialite.pc.in", "wb");
    if (!out)
      {
	  fprintf (stderr, "Error opening amalgamation/spatialite.pc.in\n");
	  return 1;
      }
    do_copy_plain (out, "/automake/spatialite.pc.in");
    fclose (out);
    out = fopen ("amalgamation/auto-sh", "wb");
    if (!out)
      {
	  fprintf (stderr, "Error opening amalgamation/auto-sh\n");
	  return 1;
      }
    do_auto_sh (out);
    fclose (out);
    free_masked_keywords (first, first_def);
    return 0;
}
