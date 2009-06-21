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

#define PREFIX	"./src"

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
do_headers (FILE * out)
{
/* prepares the headers for SpatiaLite-Amalgamation */
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
	     "** version 2.3.1.  By combining all the individual C code files into this\n");
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
    fprintf (out, "#include <stdio.h>\n");
    fprintf (out, "#include <stdlib.h>\n");
    fprintf (out, "#include <string.h>\n");
    fprintf (out, "#include <memory.h>\n");
    fprintf (out, "#include <limits.h>\n");
    fprintf (out, "#include <math.h>\n");
    fprintf (out, "#include <float.h>\n");
    fprintf (out, "#include <locale.h>\n");
    fprintf (out, "#include <errno.h>\n\n");
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
do_sqlite3_dll (FILE * out)
{
/* inserting #ifdef to build a Windows DLL */
    fprintf (out, "#ifdef DLL_EXPORT\n");
    fprintf (out, "#define SQLITE_API __declspec(dllexport)\n");
    fprintf (out, "#else\n#define SQLITE_API\n#endif\n\n");
}

static int
is_header (const char *row)
{
/* checks for #include */
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
    strcpy (input, PREFIX);
    strcat (input, basedir);
    strcat (input, file);
    FILE *in = fopen (input, "r");
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

/* copy the sqlite3.h untouched*/
    char input[1024];
    char row[256];
    char *p = row;
    int c;
    strcpy (input, PREFIX);
    strcat (input, basedir);
    strcat (input, file);
    FILE *in = fopen (input, "r");
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
    strcpy (input, PREFIX);
    strcat (input, file);
    FILE *in = fopen (input, "r");
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

int
main ()
{

/* produces the AMALGAMATION */
    mkdir ("amalgamation", 0777);
    mkdir ("amalgamation/headers", 0777);
    mkdir ("amalgamation/headers/spatialite", 0777);
/* amalgamating SpatiaLite */
    FILE *out = fopen ("amalgamation/spatialite.c", "wb");
    if (!out)
      {
	  fprintf (stderr, "Error opening amalgamation/amalgamation.c\n");
	  return 1;
      }
    do_headers (out);
    do_copy_sqlite (out, "/headers/spatialite/", "sqlite3.h");
    do_copy (out, "/headers/spatialite/", "sqlite3ext.h");
    do_copy (out, "/headers/", "spatialite.h");
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
    do_copy (out, "/gaiageo/", "gg_shape.c");
    do_copy (out, "/gaiageo/", "gg_transform.c");
    do_copy (out, "/gaiageo/", "gg_wkb.c");
    do_copy (out, "/gaiageo/", "gg_wkt.c");
    do_copy (out, "/spatialite/", "spatialite.c");
    do_copy (out, "/spatialite/", "mbrcache.c");
    do_copy (out, "/spatialite/", "virtualshape.c");
    do_copy (out, "/spatialite/", "virtualnetwork.c");
    do_copy (out, "/spatialite/", "virtualfdo.c");
    do_copy (out, "/virtualtext/", "virtualtext.c");
    do_copy (out, "/versioninfo/", "version.c");
    fclose (out);

/* setting up the HEADERS */
    out = fopen ("amalgamation/headers/spatialite/sqlite3.h", "wb");
    if (!out)
      {
	  fprintf (stderr,
		   "Error opening amalgamation/headers/spatialite/sqlite3.h\n");
	  return 1;
      }
    do_copy_plain (out, "/headers/spatialite/sqlite3.h");
    fclose (out);
    out = fopen ("amalgamation/headers/spatialite/sqlite3ext.h", "wb");
    if (!out)
      {
	  fprintf (stderr,
		   "Error opening amalgamation/headers/spatialite/sqlite3.h\n");
	  return 1;
      }
    do_copy_plain (out, "/headers/spatialite/sqlite3ext.h");
    fclose (out);
    out = fopen ("amalgamation/sqlite3.c", "wb");
    if (!out)
      {
	  fprintf (stderr, "Error opening amalgamation/sqlite3.c\n");
	  return 1;
      }
    do_sqlite3_dll (out);
    do_copy_plain (out, "/sqlite3/sqlite3.c");
    fclose (out);
    out = fopen ("amalgamation/headers/spatialite/gaiaaux.h", "wb");
    if (!out)
      {
	  fprintf (stderr,
		   "Error opening amalgamation/headers/spatialite/gaiaaux.h\n");
	  return 1;
      }
    do_copy_plain (out, "/headers/spatialite/gaiaaux.h");
    fclose (out);
    out = fopen ("amalgamation/headers/spatialite/gaiageo.h", "wb");
    if (!out)
      {
	  fprintf (stderr,
		   "Error opening amalgamation/headers/spatialite/gaiageo.h\n");
	  return 1;
      }
    do_copy_plain (out, "/headers/spatialite/gaiageo.h");
    fclose (out);
    out = fopen ("amalgamation/headers/spatialite/gaiaexif.h", "wb");
    if (!out)
      {
	  fprintf (stderr,
		   "Error opening amalgamation/headers/spatialite/gaiaexif.h\n");
	  return 1;
      }
    do_copy_plain (out, "/headers/spatialite/gaiaexif.h");
    fclose (out);
    out = fopen ("amalgamation/headers/spatialite/spatialite.h", "wb");
    if (!out)
      {
	  fprintf (stderr,
		   "Error opening amalgamation/headers/spatialite/spatialite.h\n");
	  return 1;
      }
    do_copy_plain (out, "/headers/spatialite/spatialite.h");
    fclose (out);
    out = fopen ("amalgamation/headers/spatialite.h", "wb");
    if (!out)
      {
	  fprintf (stderr, "Error opening amalgamation/headers/spatialite.h\n");
	  return 1;
      }
    do_copy_plain (out, "/headers/spatialite.h");
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
    fclose (out);
    out = fopen ("amalgamation/libspatialite.def", "wb");
    if (!out)
      {
	  fprintf (stderr, "Error opening amalgamation/libspatialite.def\n");
	  return 1;
      }
    do_copy_plain (out, "/automake/libspatialite.def");
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
    return 0;
}
