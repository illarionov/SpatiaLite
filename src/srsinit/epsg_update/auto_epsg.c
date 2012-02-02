/* 
/ auto_epsg
/
/ a tool autogenerating C code for SpatiaLite
/ [spatial_ref_sys self-initialization routines]
/
/ version 1.0, 2012 January 18
/
/ Author: Sandro Furieri a.furieri@lqt.it
/
/ Copyright (C) 2011  Alessandro Furieri
/
/    This program is free software: you can redistribute it and/or modify
/    it under the terms of the GNU General Public License as published by
/    the Free Software Foundation, either version 3 of the License, or
/    (at your option) any later version.
/
/    This program is distributed in the hope that it will be useful,
/    but WITHOUT ANY WARRANTY; without even the implied warranty of
/    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/    GNU General Public License for more details.
/
/    You should have received a copy of the GNU General Public License
/    along with this program.  If not, see <http://www.gnu.org/licenses/>.
/
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct epsg_entry
{
/* a struct wrapping an EPSG entry */
    int srid;
    char *ref_sys_name;
    char *proj4text;
    char *srs_wkt;
    struct epsg_entry *next;
};

struct epsg_dict
{
/* the EPSG dictionary */
    struct epsg_entry *first;
    struct epsg_entry *last;
    struct epsg_entry **sorted;
    int count;
};

static void
free_epsg_entry (struct epsg_entry *p)
{
/* destroying an epsg_entry */
    if (p->ref_sys_name)
	free (p->ref_sys_name);
    if (p->proj4text)
	free (p->proj4text);
    if (p->srs_wkt)
	free (p->srs_wkt);
    free (p);
}

static void
free_epsg (struct epsg_dict *epsg)
{
/* destroying the EPSG dictionary */
    struct epsg_entry *pE;
    struct epsg_entry *pEn;
    pE = epsg->first;
    while (pE)
      {
	  pEn = pE->next;
	  free_epsg_entry (pE);
	  pE = pEn;
      }
    if (epsg->sorted)
	free (epsg->sorted);
}

static void
epsg_insert (struct epsg_dict *epsg, int srid, const char *name,
	     const char *proj4text)
{
/* inserting an entry into the EPSG dictionary */
    int len;
    struct epsg_entry *p = malloc (sizeof (struct epsg_entry));
    p->srid = srid;
    len = strlen (name);
    p->ref_sys_name = malloc (len + 1);
    strcpy (p->ref_sys_name, name);
    len = strlen (proj4text);
    p->proj4text = malloc (len + 1);
    strcpy (p->proj4text, proj4text);
    p->srs_wkt = NULL;
    p->next = NULL;

/* updating the linked list */
    if (epsg->first == NULL)
	epsg->first = p;
    if (epsg->last != NULL)
	epsg->last->next = p;
    epsg->last = p;
}

static void
add_wkt (struct epsg_dict *epsg, int srid, const char *wkt)
{
/* adding the WKT def to some EPSG entry */
    struct epsg_entry *p = epsg->first;
    while (p)
      {
	  if (p->srid == srid)
	    {
		int len = strlen (wkt);
		if (p->srs_wkt)
		    free (p->srs_wkt);
		p->srs_wkt = malloc (len + 1);
		strcpy (p->srs_wkt, wkt);
		return;
	    }
	  p = p->next;
      }
}

static int
parse_epsg (FILE * fl_epsg, struct epsg_dict *epsg)
{
/* parsing the EPSG input file */
    char line[1024];
    char name[512];
    char *out = line;
    int odd_even = 0;
    int c;

    while ((c = getc (fl_epsg)) != EOF)
      {
	  if (c == '\r')
	      continue;
	  if (c == '\n')
	    {
		*out = '\0';
		if (odd_even)
		  {
		      int i;
		      int len = strlen (line);
		      char srid[16];
		      char *p = srid;
		      const char *proj4text = NULL;
		      if (odd_even == 2)
			{
			    if (*line == '#')
			      {
				  /* closing a pending invalid EPSG def */
				  odd_even = 0;
				  *name = '\0';
				  out = line;
				  continue;
			      }
			    return 0;
			}
		      if (*name == '\0')
			  return 0;
		      if (strncmp (line, "# ", 2) == 0)
			{
			    /* probably an invalid EPSG def: skipping */
			    odd_even = 2;
			    *name = '\0';
			    out = line;
			    continue;
			}
		      if (*line != '<')
			  return 0;
		      if (strcmp (line + len - 2, "<>") != 0)
			  return 0;
		      line[len - 2] = '\0';
		      for (i = 1; i < 10; i++)
			{
			    if (line[i] == '>')
			      {
				  *p = '\0';
				  proj4text = line + i + 2;
			      }
			    *p++ = line[i];
			}
		      if (!proj4text)
			  return 0;
		      len = strlen (line);
		      for (i = len - 1; i > 1; i--)
			{
			    if (line[i] == ' ')
				line[i] = '\0';
			    else
				break;
			}
		      epsg_insert (epsg, atoi (srid), name, proj4text);
		      odd_even = 0;
		      *name = '\0';
		  }
		else
		  {
		      if (strncmp (line, "# ", 2) != 0)
			  return 0;
		      strcpy (name, line + 2);
		      odd_even = 1;
		  }
		out = line;
		continue;
	    }
	  *out++ = c;
      }
    return 1;
}

static int
fetch_srid (const char *line)
{
/* attempting to retrieve the WKT own SRID */
    int len = strlen (line);
    int i;
    int cnt = 0;
    for (i = len - 1; i > 1; i--)
      {
	  if (line[i] == ']')
	      cnt++;
	  if (cnt == 3)
	    {
		if (strncmp (line + i, "],AUTHORITY[\"EPSG\",\"", 20) == 0)
		    return atoi (line + i + 20);
		return -1;
	    }
      }
    return -1;
}

static int
parse_wkt (FILE * fl_wkt, struct epsg_dict *epsg)
{
/* parsing the WKT input file */
    char line[8192];
    int srid;
    char *out = line;
    int c;

    while ((c = getc (fl_wkt)) != EOF)
      {
	  if (c == '\r')
	      continue;
	  if (c == '\n')
	    {
		*out = '\0';
		srid = fetch_srid (line);
		if (srid >= 0)
		    add_wkt (epsg, srid, line);
		out = line;
		continue;
	    }
	  *out++ = c;
      }
    return 1;
}

static int
cmp_sort (const void *p1, const void *p2)
{
/* comparison function for QSORT */
    struct epsg_entry *e1 = *(struct epsg_entry **) p1;
    struct epsg_entry *e2 = *(struct epsg_entry **) p2;
    if (e1->srid == e2->srid)
	return 0;
    if (e1->srid < e2->srid)
	return -1;
    return 1;
}

static int
sort_epsg (struct epsg_dict *epsg)
{
/* sorting the EPSG defs by ascending SRID */
    int count = 0;
    int i;
    struct epsg_entry *p = epsg->first;
    while (p)
      {
	  /* counting how many entries are there */
	  count++;
	  p = p->next;
      }
    if (count == 0)
	return 0;
    if (epsg->sorted)
	free (epsg->sorted);
    epsg->sorted = malloc (sizeof (struct epsg_dict *) * count);
    epsg->count = count;
    i = 0;
    p = epsg->first;
    while (p)
      {
	  /* feeding the pointer array */
	  *(epsg->sorted + i) = p;
	  i++;
	  p = p->next;
      }
/* sorting the pointer array by ascending SRID */
    qsort (epsg->sorted, count, sizeof (struct epsg_dict *), cmp_sort);
    return 1;
}

static void
output_c_code (FILE * out, struct epsg_dict *epsg)
{
/* generating the C code supporting spatial_ref_sys self-initialization */
    struct epsg_entry *p = epsg->first;
    int n;
    int sect = 0;
    int wgs84_sect = 0;
    int def_cnt = 1000;
    int out_cnt;
    const char *in;
    int i;
    int pending_footer = 0;

    fprintf (out,
	     "#ifndef OMIT_EPSG    /* full EPSG initialization enabled */\n\n");

    for (i = 0; i < epsg->count; i++)
      {
	  p = *(epsg->sorted + i);
	  if (p->srid == 4326 || (p->srid >= 32601 && p->srid <= 32766))
	    {
		/* skipping WGS84 defs */
		continue;
	    }

	  if (def_cnt > 100)
	    {
		/* function header */
		fprintf (out, "static void\n");
		fprintf (out,
			 "initialize_epsg_%02d (int filter, struct epsg_defs **first, struct epsg_defs **last)\n",
			 sect++);
		fprintf (out, "{\n/* initializing the EPSG defs list */\n");
		fprintf (out, "    struct epsg_defs *p;\n");
		def_cnt = 0;
	    }
	  pending_footer = 1;

	  /* inserting the main EPSG def */
	  def_cnt++;
	  fprintf (out,
		   "    p = add_epsg_def (filter, first, last, %d, \"epsg\", %d,\n",
		   p->srid, p->srid);
	  fprintf (out, "        \"%s\");\n", p->ref_sys_name);

	  /* inserting the proj4text string */
	  n = 0;
	  in = p->proj4text;
	  while (*in != '\0')
	    {
		fprintf (out, "    add_proj4text (p, %d,\n        \"", n);
		out_cnt = 0;
		while (*in != '\0')
		  {
		      if (*in == '"')
			{
			    fprintf (out, "\\%c", *in++);
			    out_cnt += 2;
			}
		      else
			{
			    fprintf (out, "%c", *in++);
			    out_cnt++;
			}
		      if (out_cnt >= 56)
			  break;
		  }
		fprintf (out, "\");\n");
		n++;
	    }

	  if (p->srs_wkt != NULL)
	    {
		/* inserting the srs_wkt string */ n = 0;
		n = 0;
		in = p->srs_wkt;
		while (*in != '\0')
		  {
		      fprintf (out, "    add_srs_wkt (p, %d,\n        \"", n);
		      out_cnt = 0;
		      while (*in != '\0')
			{
			    if (*in == '"')
			      {
				  fprintf (out, "\\%c", *in++);
				  out_cnt += 2;
			      }
			    else
			      {
				  fprintf (out, "%c", *in++);
				  out_cnt++;
			      }
			    if (out_cnt >= 56)
				break;
			}
		      fprintf (out, "\");\n");
		      n++;
		  }
	    }
	  else
	      fprintf (out, "    add_srs_wkt (p, 0, \"\");\n");

	  if (def_cnt > 100)
	    {
		/* function footer */
		fprintf (out, "}\n\n");
		pending_footer = 0;
	    }
      }
    if (pending_footer)
      {
	  /* function footer */
	  fprintf (out, "}\n\n");
      }

/* function header */
    fprintf (out, "static void\n");
    fprintf (out,
	     "initialize_epsg_extras (int filter, struct epsg_defs **first, struct epsg_defs **last)\n");
    fprintf (out, "{\n/* initializing the EPSG defs list [EXTRA] */\n");
    fprintf (out, "    struct epsg_defs *p;\n");

    fprintf (out,
	     "    p = add_epsg_def (filter, first, last, 40000, \"gfoss.it\", 1,\n");
    fprintf (out, "        \"Italy mainland zone 1 GB Roma40\");\n");
    fprintf (out, "    add_proj4text (p, 0,\n");
    fprintf (out,
	     "        \"+proj=tmerc+lat_0=0 +lon_0=9  +k=0.9996 +x_0=1500000 +y_\");\n");
    fprintf (out, "    add_proj4text (p, 1,\n");
    fprintf (out,
	     "        \"0=0 +ellps=intl +units=m +towgs84=-104.1,-49.1,-9.9,0.97\");\n");
    fprintf (out,
	     "    add_proj4text (p, 2, \"1,-2.917,0.714,-11.68 +no_defs\");\n");
    fprintf (out, "    add_srs_wkt (p, 0, \"\");\n");
    fprintf (out,
	     "    p = add_epsg_def (filter, first, last, 40001, \"gfoss.it\", 2,\n");
    fprintf (out, "        \"Italy mainland zone 2 GB Roma40\");\n");
    fprintf (out, "    add_proj4text (p, 0,\n");
    fprintf (out,
	     "        \"+proj=tmerc +lat_0=0 +lon_0=15 +k=0.9996 +x_0=2520000 +y\");\n");
    fprintf (out, "    add_proj4text (p, 1,\n");
    fprintf (out,
	     "        \"_0=0 +ellps=intl +units=m +towgs84=-104.1,-49.1,-9.9,0.9\");\n");
    fprintf (out,
	     "    add_proj4text (p, 2, \"71,-2.917,0.714,-11.68 +no_defs\");\n");
    fprintf (out, "    add_srs_wkt (p, 0, \"\");\n");
    fprintf (out,
	     "    p = add_epsg_def (filter, first, last, 40002, \"gfoss.it\", 3,\n");
    fprintf (out, "        \"Italy Sardinia GB Roma40\");\n");
    fprintf (out, "    add_proj4text (p, 0,\n");
    fprintf (out,
	     "        \"+proj=tmerc +lat_0=0 +lon_0=9  +k=0.9996 +x_0=1500000 +y\");\n");
    fprintf (out, "    add_proj4text (p, 1,\n");
    fprintf (out,
	     "        \"_0=0 +ellps=intl +units=m +towgs84=-168.6,-34.0,38.6,-0.\");\n");
    fprintf (out,
	     "    add_proj4text (p, 2, \"374,-0.679,-1.379,-9.48 +no_defs\");\n");
    fprintf (out, "    add_srs_wkt (p, 0, \"\");\n");
    fprintf (out,
	     "    p = add_epsg_def (filter, first, last, 40003, \"gfoss.it\", 4,\n");
    fprintf (out, "        \"Italy Sicily GB Roma40\");\n");
    fprintf (out, "    add_proj4text (p, 0,\n");
    fprintf (out,
	     "        \"+proj=tmerc +lat_0=0 +lon_0=9  +k=0.9996 +x_0=1500000 +y\");\n");
    fprintf (out, "    add_proj4text (p, 1,\n");
    fprintf (out,
	     "        \"_0=0 +ellps=intl +units=m +towgs84=-50.2,-50.4,84.8,-0.6\");\n");
    fprintf (out,
	     "    add_proj4text (p, 2, \"90,-2.012,0.459,-28.08  +no_defs\");\n");
    fprintf (out, "    add_srs_wkt (p, 0, \"\");\n");

/* function footer */
    fprintf (out, "}\n\n");

    fprintf (out, "#endif /* full EPSG initialization enabled/disabled */\n\n");

    def_cnt = 1000;
    pending_footer = 0;
    for (i = 0; i < epsg->count; i++)
      {
	  p = *(epsg->sorted + i);
	  if (p->srid == 4326 || (p->srid >= 32601 && p->srid <= 32766))
	      ;
	  else
	    {
		/* skipping not-WGS84 defs */
		continue;
	    }

	  if (def_cnt > 100)
	    {
		/* function header */
		fprintf (out, "static void\n");
		fprintf (out,
			 "initialize_epsg_wgs84_%d (int filter, struct epsg_defs **first, struct epsg_defs **last)\n",
			 wgs84_sect++);
		fprintf (out,
			 "{\n/* initializing the EPSG defs list [WGS84] */\n");
		fprintf (out, "    struct epsg_defs *p;\n");
		def_cnt = 0;
	    }
	  pending_footer = 1;

	  /* inserting the main EPSG def */
	  def_cnt++;
	  fprintf (out,
		   "    p = add_epsg_def (filter, first, last, %d, \"epsg\", %d,\n",
		   p->srid, p->srid);
	  fprintf (out, "        \"%s\");\n", p->ref_sys_name);

	  /* inserting the proj4text string */
	  n = 0;
	  in = p->proj4text;
	  while (*in != '\0')
	    {
		fprintf (out, "    add_proj4text (p, %d,\n        \"", n);
		out_cnt = 0;
		while (*in != '\0')
		  {
		      if (*in == '"')
			{
			    fprintf (out, "\\%c", *in++);
			    out_cnt += 2;
			}
		      else
			{
			    fprintf (out, "%c", *in++);
			    out_cnt++;
			}
		      if (out_cnt >= 56)
			  break;
		  }
		fprintf (out, "\");\n");
		n++;
	    }

	  if (p->srs_wkt != NULL)
	    {
		/* inserting the srs_wkt string */ n = 0;
		n = 0;
		in = p->srs_wkt;
		while (*in != '\0')
		  {
		      fprintf (out, "    add_srs_wkt (p, %d,\n        \"", n);
		      out_cnt = 0;
		      while (*in != '\0')
			{
			    if (*in == '"')
			      {
				  fprintf (out, "\\%c", *in++);
				  out_cnt += 2;
			      }
			    else
			      {
				  fprintf (out, "%c", *in++);
				  out_cnt++;
			      }
			    if (out_cnt >= 56)
				break;
			}
		      fprintf (out, "\");\n");
		      n++;
		  }
	    }
	  else
	      fprintf (out, "    add_srs_wkt (p, 0, \"\");\n");

	  if (def_cnt > 100)
	    {
		/* function footer */
		fprintf (out, "}\n\n");
		pending_footer = 0;
	    }
      }
    if (pending_footer)
      {
	  /* function footer */
	  fprintf (out, "}\n\n");
      }

/* inserting the pilot function */
    fprintf (out, "static void\n");
    fprintf (out,
	     "initialize_epsg (int filter, struct epsg_defs **first, struct epsg_defs **last)\n");
    fprintf (out, "{\n/* initializing the EPSG defs list */\n");
    fprintf (out, "    struct epsg_defs *p;\n ");
    fprintf (out, "/* initializing the EPSG UNKNOWN def [-1] */\n");
    fprintf (out,
	     "    p = add_epsg_def (filter, first, last, -1, \"NONE\", -1, \"UNKNOWN SRS\");\n");
    fprintf (out, "    add_proj4text (p, 0, \"\");\n");
    fprintf (out, "    add_srs_wkt (p, 0, \"\");\n\n");
    fprintf (out, "    if (filter != GAIA_EPSG_WGS84_ONLY)\n    {\n");
    fprintf (out,
	     "#ifndef OMIT_EPSG    /* full EPSG initialization enabled */\n");
    for (i = 0; i < sect; i++)
	fprintf (out,
		 "        initialize_epsg_%02d (filter, first, last);\n", i);
    fprintf (out, "        initialize_epsg_extras (filter, first, last);\n");
    fprintf (out, "#endif /* full EPSG initialization enabled/disabled */\n");

    fprintf (out, "    }\n");
    for (i = 0; i < wgs84_sect; i++)
	fprintf (out,
		 "    initialize_epsg_wgs84_%d (filter, first, last);\n", i);
    fprintf (out, "}\n");
}

int
main (int argc, char *argv[])
{
/*
/
/ Please note: no args are supported !!!
/
/ we'll expect to find two input files respectively named:
/ - epsg
/ - wkt
/
/ the C code will be generated into: epsg_inlined.c
/
*/
    FILE *fl_epsg = NULL;
    FILE *fl_wkt = NULL;
    FILE *fl_out = NULL;
    struct epsg_dict epsg;
/* initializing the EPSG dictionary */
    epsg.first = NULL;
    epsg.last = NULL;
    epsg.sorted = NULL;
    epsg.count = 0;
/* opening the EPSG input file */
    fl_epsg = fopen ("epsg", "rb");
    if (fl_epsg == NULL)
      {
	  fprintf (stderr, "ERROR: unable to open the \"epsg\" input file\n");
	  goto stop;
      }

/* opening the WKT input file */
    fl_wkt = fopen ("wkt", "rb");
    if (fl_wkt == NULL)
      {
	  fprintf (stderr, "ERROR: unable to open the \"wkt\" input file\n");
	  goto stop;
      }

    if (!parse_epsg (fl_epsg, &epsg))
      {
	  fprintf (stderr, "ERROR: malformed EPSG input file\n");
	  goto stop;
      }

    if (!parse_wkt (fl_wkt, &epsg))
      {
	  fprintf (stderr, "ERROR: malformed WKT input file\n");
	  goto stop;
      }

    if (!sort_epsg (&epsg))
      {
	  fprintf (stderr, "ERROR: unable to sort EPSG entries\n");
	  goto stop;
      }

/* opening the output file */
    fl_out = fopen ("epsg_inlined.c", "wb");
    if (fl_out == NULL)
      {
	  fprintf (stderr,
		   "ERROR: unable to open the \"epsg_inlined.c\" output file\n");
	  goto stop;
      }

    output_c_code (fl_out, &epsg);
  stop:
    free_epsg (&epsg);
    if (fl_epsg)
	fclose (fl_epsg);
    if (fl_wkt)
	fclose (fl_wkt);
    if (fl_out)
	fclose (fl_out);
    return 0;
}
