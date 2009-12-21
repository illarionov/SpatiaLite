/*

 virtualtext.c -- SQLite3 extension [VIRTUAL TABLE accessing CSV/TXT]

 version 2.4, 2009 September 17

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <spatialite/sqlite3.h>
#include <spatialite/spatialite.h>
#include <spatialite/gaiaaux.h>

#define VRTTXT_TEXT		1
#define VRTTXT_INTEGER	2
#define VRTTXT_DOUBLE	3

struct sqlite3_module virtualtext_module;

#define VTXT_ROW_BLOCK	65536

struct row_buffer
{
/* a complete row */
    int n_cells;		/* how many cells are stored into this line */
    char **cells;		/* the cells array */
    char *text;			/* the text-cells array */
    struct row_buffer *next;	/* pointer for linked list */
};

struct row_pointer
{
/* a row pointer */
    long offset;		/* the row start offset */
    int len;			/* the row length */
    char start;			/* the first char of the row - signature */
    char valid;			/* 1=valid - 0=invalid */
    int n_cells;		/* # cells */
};

struct row_block
{
/* a block of row pointers */
    int n_rows;
    struct row_pointer rows[VTXT_ROW_BLOCK];
    struct row_block *next;
};

struct text_buffer
{
    FILE *text_file;		/* the underlaying file */
    void *toUtf8;		/* the UTF-8 ICONV converter */
    char field_separator;
    char text_separator;
    char decimal_separator;
    int max_n_cells;		/* the maximun cell index */
    char **titles;		/* the column titles array */
    char *types;		/* the column types array */
    int n_rows;			/* the number of rows */
    struct row_pointer **rows;	/* the rows array */
    struct row_block *first;	/* pointers to build a linked list of rows */
    struct row_block *last;
    struct row_buffer *current_row_buffer;
};

typedef struct VirtualTextStruct
{
/* extends the sqlite3_vtab struct */
    const sqlite3_module *pModule;	/* ptr to sqlite module: USED INTERNALLY BY SQLITE */
    int nRef;			/* # references: USED INTERNALLY BY SQLITE */
    char *zErrMsg;		/* error message: USED INTERNALLY BY SQLITE */
    sqlite3 *db;		/* the sqlite db holding the virtual table */
    struct text_buffer *buffer;	/* the in-memory buffer storing text */
} VirtualText;
typedef VirtualText *VirtualTextPtr;

typedef struct VirtualTextCursortStruct
{
/* extends the sqlite3_vtab_cursor struct */
    VirtualTextPtr pVtab;	/* Virtual table of this cursor */
    long current_row;		/* the current row ID */
    int eof;			/* the EOF marker */
} VirtualTextCursor;
typedef VirtualTextCursor *VirtualTextCursorPtr;

static int
text_add_block (struct text_buffer *text)
{
/* inserting a block of rows into the text buffer struct */
    struct row_block *block = malloc (sizeof (struct row_block));
    if (block == NULL)
	return 0;
    block->n_rows = 0;
    block->next = NULL;
/* inserting the block of rows into the linked list */
    if (!(text->first))
	text->first = block;
    if (text->last)
	text->last->next = block;
    text->last = block;
    return 1;
}

static struct row_pointer *
get_row_pointer (struct text_buffer *text, long current_row)
{
/* retrieving a ROW POINTER struct */
    if (!text)
	return NULL;
    if (!(text->rows))
	return NULL;
    if (current_row < 0 || current_row >= text->n_rows)
	return NULL;
    return *(text->rows + current_row);
}

static int
text_insert_row (struct text_buffer *text, long offset, int len, char start)
{
/* inserting a row into the text buffer struct */
    struct row_pointer *row;
    if (text->last == NULL)
      {
	  if (!text_add_block (text))
	      return 0;
      }
    if (text->last->n_rows == VTXT_ROW_BLOCK)
      {
	  if (!text_add_block (text))
	      return 0;
      }
    row = text->last->rows + text->last->n_rows;
    row->offset = offset;
    row->len = len;
    row->start = start;
    row->valid = 0;
    row->n_cells = 0;
    (text->last->n_rows)++;
    return 1;
}

static struct text_buffer *
text_buffer_alloc (FILE * fl, void *toUtf8, char field_separator,
		   char text_separator, char decimal_separator)
{
/* allocating and initializing the text buffer struct */
    struct text_buffer *text = malloc (sizeof (struct text_buffer));
    if (text == NULL)
	return NULL;
    text->text_file = fl;
    text->toUtf8 = toUtf8;
    text->field_separator = field_separator;
    text->text_separator = text_separator;
    text->decimal_separator = decimal_separator;
    text->max_n_cells = 0;
    text->titles = NULL;
    text->types = NULL;
    text->n_rows = 0;
    text->rows = NULL;
    text->first = NULL;
    text->last = NULL;
    text->current_row_buffer = NULL;
    return text;
}

static void
text_row_free (struct row_buffer *row)
{
/* memory cleanup - freeing a row buffer */
    int i;
    if (!row)
	return;
    if (row->cells)
      {
	  for (i = 0; i < row->n_cells; i++)
	    {
		if (*(row->cells + i))
		  {
		      free (*(row->cells + i));
		  }
	    }
	  free (row->cells);
      }
    if (row->text)
	free (row->text);
    free (row);
}

static void
text_buffer_free (struct text_buffer *text)
{
/* memory cleanup - freeing the text buffer */
    int i;
    struct row_block *block;
    struct row_block *pn;
    if (!text)
	return;
    block = text->first;
    while (block)
      {
	  pn = block->next;
	  free (block);
	  block = pn;
      }
    if (text->titles)
      {
	  for (i = 0; i < text->max_n_cells; i++)
	    {
		if (*(text->titles + i))
		    free (*(text->titles + i));
	    }
	  free (text->titles);
      }
    if (text->rows)
	free (text->rows);
    if (text->types)
	free (text->types);
    if (text->current_row_buffer)
	text_row_free (text->current_row_buffer);
    gaiaFreeUTF8Converter (text->toUtf8);
    fclose (text->text_file);
    free (text);
}

static int
text_is_integer (char *value)
{
/* checking if this value can be an INTEGER */
    int invalids = 0;
    int digits = 0;
    int signs = 0;
    char last = '\0';
    char *p = value;
    while (*p != '\0')
      {
	  last = *p;
	  if (*p >= '0' && *p <= '9')
	      digits++;
	  else if (*p == '+' || *p == '-')
	      signs++;
	  else
	      signs++;
	  p++;
      }
    if (invalids)
	return 0;
    if (signs > 1)
	return 0;
    if (signs)
      {
	  if (*value == '+' || *value == '-' || last == '+' || last == '-')
	      ;
	  else
	      return 0;
      }
    return 1;
}

static int
text_is_double (char *value, char decimal_separator)
{
/* checking if this value can be a DOUBLE */
    int invalids = 0;
    int digits = 0;
    int signs = 0;
    int points = 0;
    char last = '\0';
    char *p = value;
    while (*p != '\0')
      {
	  last = *p;
	  if (*p >= '0' && *p <= '9')
	      digits++;
	  else if (*p == '+' || *p == '-')
	      points++;
	  else
	    {
		if (decimal_separator == ',')
		  {
		      if (*p == ',')
			  points++;
		      else
			  invalids++;
		  }
		else
		  {
		      if (*p == '.')
			  points++;
		      else
			  invalids++;
		  }
	    }
	  p++;
      }
    if (invalids)
	return 0;
    if (points > 1)
	return 0;
    if (signs > 1)
	return 0;
    if (signs)
      {
	  if (*value == '+' || *value == '-' || last == '+' || last == '-')
	      ;
	  else
	      return 0;
      }
    return 1;
}

static void
text_clean_integer (char *value)
{
/* cleaning an integer value */
    char last;
    char buffer[35536];
    int len = strlen (value);
    last = value[len - 1];
    if (last == '-' || last == '+')
      {
	  /* trailing sign; transforming into a leading sign */
	  *buffer = last;
	  strcpy (buffer + 1, value);
	  buffer[len - 1] = '\0';
	  strcpy (value, buffer);
      }
}

static void
text_clean_double (char *value)
{
/* cleaning an integer value */
    char *p;
    char last;
    char buffer[35536];
    int len = strlen (value);
    last = value[len - 1];
    if (last == '-' || last == '+')
      {
	  /* trailing sign; transforming into a leading sign */
	  *buffer = last;
	  strcpy (buffer + 1, value);
	  buffer[len - 1] = '\0';
	  strcpy (value, buffer);
      }
    p = value;
    while (*p != '\0')
      {
	  /* transforming COMMAs into POINTs */
	  if (*p == ',')
	      *p = '.';
	  p++;
      }
}

static int
text_clean_text (char **value, void *toUtf8)
{
/* cleaning a TEXT value and converting to UTF-8 */
    char *text = *value;
    char *utf8text;
    int err;
    int i;
    int oldlen = strlen (text);
    int newlen;
    for (i = oldlen - 1; i > 0; i++)
      {
	  /* cleaning up trailing spaces */
	  if (text[i] == ' ')
	      text[i] = '\0';
	  else
	      break;
      }
    utf8text = gaiaConvertToUTF8 (toUtf8, text, oldlen, &err);
    if (err)
	return 1;
    newlen = strlen (utf8text);
    if (newlen <= oldlen)
	strcpy (*value, utf8text);
    else
      {
	  free (*value);
	  *value = malloc (newlen + 1);
	  if (*value == NULL)
	    {
		fprintf (stderr, "VirtualText: insufficient memory\n");
		fflush (stderr);
		return 1;
	    }
	  strcpy (*value, utf8text);
      }
    return 0;
}

static struct row_buffer *
text_create_row (char **fields, char *text_mark, int max_cell)
{
/* creating a row struct */
    int i;
    struct row_buffer *row = malloc (sizeof (struct row_buffer));
    row->n_cells = max_cell;
    if (max_cell < 0)
	row->cells = NULL;
    else
      {
	  row->cells = malloc (sizeof (char *) * (max_cell));
	  if (row->cells == NULL)
	    {
		fprintf (stderr, "VirtualText: insufficient memory\n");
		fflush (stderr);
		return NULL;
	    }
	  row->text = malloc (sizeof (char) * (max_cell));
	  if (row->text == NULL)
	    {
		fprintf (stderr, "VirtualText: insufficient memory\n");
		fflush (stderr);
		return NULL;
	    }
	  for (i = 0; i < row->n_cells; i++)
	    {
		/* setting cell values */
		*(row->cells + i) = *(fields + i);
		*(row->text + i) = *(text_mark + i);
	    }
      }
    return row;
}

static struct row_buffer *
text_read_row (struct text_buffer *text, struct row_pointer *ptr,
	       int preliminary)
{
/* parsing a single row from the underlaying text file */
    int fld;
    int len;
    int max_cell;
    int is_text = 0;
    int is_string = 0;
    char last = '\0';
    char *fields[4096];
    char text_mark[4096];
    char buffer[35536];
    char *p = buffer;
    char buf_in[65536];
    char *in = buf_in;
    struct row_buffer *row;
    if (fseek (text->text_file, ptr->offset, SEEK_SET) < 0)
      {
	  fprintf (stderr, "VirtualText: corrupted text file\n");
	  fflush (stderr);
	  return NULL;
      }
    if ((int)fread (buf_in, 1, ptr->len, text->text_file) != ptr->len)
      {
	  fprintf (stderr, "VirtualText: corrupted text file\n");
	  fflush (stderr);
	  return NULL;
      }
    if (*buf_in != ptr->start)
      {
	  fprintf (stderr, "VirtualText: corrupted text file\n");
	  fflush (stderr);
	  return NULL;
      }
    if (*(buf_in + ptr->len - 1) != '\n')
      {
	  fprintf (stderr, "VirtualText: corrupted text file\n");
	  fflush (stderr);
	  return NULL;
      }
    buf_in[ptr->len] = '\0';
    for (fld = 0; fld < 4096; fld++)
      {
	  /* preparing an empty row */
	  fields[fld] = NULL;
	  text_mark[fld] = 0;
      }
    fld = 0;
    while (*in != '\0')
      {
	  /* parsing the row, one char at each time */
	  if (*in == '\r' && !is_string)
	    {
		last = *in++;
		continue;
	    }
	  if (*in == text->field_separator && !is_string)
	    {
		/* inserting a field into the fields tmp array */
		last = *in;
		*p = '\0';
		len = strlen (buffer);
		if (len)
		  {
		      fields[fld] = malloc (len + 1);
		      strcpy (fields[fld], buffer);
		      text_mark[fld] = is_text;
		  }
		fld++;
		p = buffer;
		*p = '\0';
		is_text = 0;
		in++;
		continue;
	    }
	  if (*in == text->text_separator)
	    {
		/* found a text separator */
		if (is_string)
		  {
		      is_string = 0;
		      last = *in;
		      is_text = 1;
		  }
		else
		  {
		      if (last == text->text_separator)
			  *p++ = text->text_separator;
		      is_string = 1;
		  }
		in++;
		continue;
	    }
	  last = *in;
	  if (*in == '\n' && !is_string)
	    {
		/* inserting the row into the text buffer */
		*p = '\0';
		len = strlen (buffer);
		if (len)
		  {
		      fields[fld] = malloc (len + 1);
		      strcpy (fields[fld], buffer);
		  }
		fld++;
		p = buffer;
		*p = '\0';
		max_cell = -1;
		for (fld = 0; fld < 4096; fld++)
		  {
		      if (fields[fld])
			{
			    max_cell = fld;
			}
		  }
		max_cell++;
		if (!preliminary)
		  {
		      if (max_cell != ptr->n_cells)
			{
			    for (fld = 0; fld < 4096; fld++)
			      {
				  if (fields[fld])
				      free (fields[fld]);
			      }
			    return NULL;
			}
		  }
		row = text_create_row (fields, text_mark, max_cell);
		return row;
	    }
	  *p++ = *in++;
      }
    return NULL;
}

static struct text_buffer *
text_parse (char *path, char *encoding, char first_line_titles,
	    char field_separator, char text_separator, char decimal_separator)
{
/* trying to open and parse the text file */
    int c;
    int len;
    int fld;
    int is_string = 0;
    char col_types[4096];
    char buffer[35536];
    char *p = buffer;
    struct text_buffer *text;
    int nrows;
    int max_cols;
    int errs;
    struct row_block *block;
    struct row_buffer *row;
    void *toUtf8;
    int ir;
    char title[64];
    char *first_valid_row;
    int i;
    char *name;
    FILE *in;
    long cur_pos = 0;
    long begin_offset = 0;
    char start;
    int new_row = 1;
/* trying to open the text file */
    in = fopen (path, "rb");
    if (!in)
      {
	  fprintf (stderr, "VirtualText: open error '%s'\n", path);
	  fflush (stderr);
	  return NULL;
      }
    toUtf8 = gaiaCreateUTF8Converter (encoding);
    if (!toUtf8)
      {
	  fprintf (stderr, "VirtualText: illegal charset\n");
	  fflush (stderr);
	  return NULL;
      }
    text =
	text_buffer_alloc (in, toUtf8, field_separator, text_separator,
			   decimal_separator);
    if (text == NULL)
      {
	  fprintf (stderr, "VirtualText: insufficient memory\n");
	  fflush (stderr);
	  return NULL;
      }
    while ((c = getc (in)) != EOF)
      {
	  /* parsing the file, one char at each time */
	  cur_pos++;
	  if (c == '\r' && !is_string)
	      continue;
	  if (new_row)
	    {
		/* a new row begins here */
		new_row = 0;
		start = (char) c;
		begin_offset = cur_pos - 1;
		p = buffer;
	    }
	  if (c == text_separator)
	    {
		/* found a text separator */
		if (is_string)
		    is_string = 0;
		else
		    is_string = 1;
		continue;
	    }
	  if (c == '\n' && !is_string)
	    {
		/* inserting the row into the text buffer */
		len = cur_pos - begin_offset;
		if (!text_insert_row (text, begin_offset, len, start))
		  {
		      text_buffer_free (text);
		      fprintf (stderr, "VirtualText: insufficient memory\n");
		      fflush (stderr);
		      return NULL;
		  }
		new_row = 1;
		continue;
	    }
	  *p++ = c;
      }
/* checking if the text file really seems to contain a table */
    nrows = 0;
    errs = 0;
    block = text->first;
    while (block)
      {
	  for (i = 0; i < block->n_rows; i++)
	    {
		if (first_line_titles && block == text->first && i == 0)
		  {
		      /* skipping first line */
		      continue;
		  }
		nrows++;
	    }
	  block = block->next;
      }
    if (nrows == 0)
      {
	  text_buffer_free (text);
	  return NULL;
      }
/* going to check the column types */
    first_valid_row = malloc (sizeof (char) * 4096);
    if (first_valid_row == NULL)
      {
	  text_buffer_free (text);
	  fprintf (stderr, "VirtualText: insufficient memory\n");
	  fflush (stderr);
	  return NULL;
      }
    for (fld = 0; fld < 4096; fld++)
      {
	  /* initally assuming any cell contains TEXT */
	  *(col_types + fld) = VRTTXT_TEXT;
	  *(first_valid_row + fld) = 1;
      }
    nrows = 0;
    max_cols = 0;
    block = text->first;
    while (block)
      {
	  for (i = 0; i < block->n_rows; i++)
	    {
		row = text_read_row (text, block->rows + i, 1);
		if (row == NULL)
		  {
		      text_buffer_free (text);
		      return NULL;
		  }
		if (first_line_titles && block == text->first && i == 0)
		  {
		      /* skipping first line */
		      for (fld = 0; fld < row->n_cells; fld++)
			{
			    if (*(row->cells + fld))
			      {
				  if (fld > max_cols)
				      max_cols = fld;
			      }
			}
		      (block->rows + i)->valid = 1;
		      (block->rows + i)->n_cells = row->n_cells;
		      text_row_free (row);
		      continue;
		  }
		for (fld = 0; fld < row->n_cells; fld++)
		  {
		      if (*(row->cells + fld))
			{
			    if ((fld + 1) > max_cols)
				max_cols = fld + 1;
			    if (text_is_integer (*(row->cells + fld))
				&& !(*(row->text + fld)))
			      {
				  if (*(first_valid_row + fld))
				    {
					*(col_types + fld) = VRTTXT_INTEGER;
					*(first_valid_row + fld) = 0;
				    }
			      }
			    else if (text_is_double
				     (*(row->cells + fld), decimal_separator)
				     && !(*(row->text + fld)))
			      {
				  if (*(first_valid_row + fld))
				    {
					*(col_types + fld) = VRTTXT_DOUBLE;
					*(first_valid_row + fld) = 0;
				    }
				  else
				    {
					/* promoting an INTEGER column to be of the DOUBLE type */
					if (*(col_types + fld) ==
					    VRTTXT_INTEGER)
					    *(col_types + fld) = VRTTXT_DOUBLE;
				    }
			      }
			    else
			      {
				  /* this column is anyway of the TEXT type */
				  *(col_types + fld) = VRTTXT_TEXT;
				  if (*(first_valid_row + fld))
				      *(first_valid_row + fld) = 0;
			      }
			}
		  }
		nrows++;
		(block->rows + i)->valid = 1;
		(block->rows + i)->n_cells = row->n_cells;
		text_row_free (row);
	    }
	  block = block->next;
      }
    free (first_valid_row);
    if (nrows == 0 && max_cols == 0)
      {
	  text_buffer_free (text);
	  return NULL;
      }
    text->n_rows = nrows;
    text->max_n_cells = max_cols;
    text->types = malloc (sizeof (char) * text->max_n_cells);
    if (text->types == NULL)
      {
	  text_buffer_free (text);
	  fprintf (stderr, "VirtualText: insufficient memory\n");
	  fflush (stderr);
	  return NULL;
      }
    memcpy (text->types, col_types, text->max_n_cells);
/* preparing the column names */
    text->titles = malloc (sizeof (char *) * text->max_n_cells);
    if (text->titles == NULL)
      {
	  text_buffer_free (text);
	  fprintf (stderr, "VirtualText: insufficient memory\n");
	  fflush (stderr);
	  return NULL;
      }
    block = text->first;
    if (first_line_titles && block)
      {
	  row = text_read_row (text, block->rows + 0, 0);
	  if (row == NULL)
	    {
		text_buffer_free (text);
		return NULL;
	    }
	  for (fld = 0; fld < text->max_n_cells; fld++)
	    {
		if (fld >= row->n_cells)
		  {
		      /* this column name is NULL; setting a default name */
		      sprintf (title, "COL%03d", fld + 1);
		      len = strlen (title);
		      *(text->titles + fld) = malloc (len + 1);
		      if (*(text->titles + fld) == NULL)
			{
			    text_buffer_free (text);
			    fprintf (stderr,
				     "VirtualText: insufficient memory\n");
			    fflush (stderr);
			    return NULL;
			}
		      strcpy (*(text->titles + fld), title);
		  }
		else
		  {
		      if (*(row->cells + fld))
			{
			    len = strlen (*(row->cells + fld));
			    *(text->titles + fld) = malloc (len + 1);
			    if (*(text->titles + fld) == NULL)
			      {
				  text_buffer_free (text);
				  fprintf (stderr,
					   "VirtualText: insufficient memory\n");
				  fflush (stderr);
				  return NULL;
			      }
			    strcpy (*(text->titles + fld), *(row->cells + fld));
			    name = *(text->titles + fld);
			    for (i = 0; i < len; i++)
			      {
				  /* masking any space in the column name */
				  if (*(name + i) == ' ')
				      *(name + i) = '_';
			      }
			}
		      else
			{
			    /* this column name is NULL; setting a default name */
			    sprintf (title, "COL%03d", fld + 1);
			    len = strlen (title);
			    *(text->titles + fld) = malloc (len + 1);
			    strcpy (*(text->titles + fld), title);
			}
		  }
	    }
	  text_row_free (row);
      }
    else
      {
	  for (fld = 0; fld < text->max_n_cells; fld++)
	    {
		sprintf (title, "COL%03d", fld + 1);
		len = strlen (title);
		*(text->titles + fld) = malloc (len + 1);
		if (*(text->titles + fld) == NULL)
		  {
		      text_buffer_free (text);
		      fprintf (stderr, "VirtualText: insufficient memory\n");
		      fflush (stderr);
		      return NULL;
		  }
		strcpy (*(text->titles + fld), title);
	    }
      }
/* ok, we can now go to prepare the rows array */
    text->rows = malloc (sizeof (struct row_pointer *) * text->n_rows);
    if (text->rows == NULL)
      {
	  text_buffer_free (text);
	  fprintf (stderr, "VirtualText: insufficient memory\n");
	  fflush (stderr);
	  return NULL;
      }
    ir = 0;
    block = text->first;
    while (block)
      {
	  for (i = 0; i < block->n_rows; i++)
	    {
		if (first_line_titles && block == text->first && i == 0)
		  {
		      /* skipping first line */
		      continue;
		  }
		if ((block->rows + i)->valid)
		    *(text->rows + ir++) = block->rows + i;
	    }
	  block = block->next;
      }
    return text;
}

static int
vtxt_create (sqlite3 * db, void *pAux, int argc, const char *const *argv,
	     sqlite3_vtab ** ppVTab, char **pzErr)
{
/* creates the virtual table connected to some TEXT file */
    char path[2048];
    char encoding[128];
    const char *vtable;
    const char *pEncoding = NULL;
    int len;
    struct text_buffer *text = NULL;
    const char *pPath = NULL;
    char field_separator = '\t';
    char text_separator = '"';
    char decimal_separator = '.';
    char first_line_titles = 1;
    int i;
    char sql[4096];
    int seed;
    int dup;
    int idup;
    char dummyName[4096];
    char **col_name = NULL;
    VirtualTextPtr p_vt;
    if (pAux)
	pAux = pAux;		/* unused arg warning suppression */
/* checking for TEXTfile PATH */
    if (argc >= 5 && argc <= 9)
      {
	  vtable = argv[1];
	  pPath = argv[3];
	  len = strlen (pPath);
	  if ((*(pPath + 0) == '\'' || *(pPath + 0) == '"')
	      && (*(pPath + len - 1) == '\'' || *(pPath + len - 1) == '"'))
	    {
		/* the path is enclosed between quotes - we need to dequote it */
		strcpy (path, pPath + 1);
		len = strlen (path);
		*(path + len - 1) = '\0';
	    }
	  else
	      strcpy (path, pPath);
	  pEncoding = argv[4];
	  len = strlen (pEncoding);
	  if ((*(pEncoding + 0) == '\'' || *(pEncoding + 0) == '"')
	      && (*(pEncoding + len - 1) == '\''
		  || *(pEncoding + len - 1) == '"'))
	    {
		/* the charset-name is enclosed between quotes - we need to dequote it */
		strcpy (encoding, pEncoding + 1);
		len = strlen (encoding);
		*(encoding + len - 1) = '\0';
	    }
	  else
	      strcpy (encoding, pEncoding);
	  if (argc >= 6)
	    {
		if (*(argv[5]) == '0' || *(argv[5]) == 'n' || *(argv[5]) == 'N')
		    first_line_titles = 0;
	    }
	  if (argc >= 7)
	    {
		if (strcasecmp (argv[6], "COMMA") == 0)
		    decimal_separator = ',';
	    }
	  if (argc >= 8)
	    {
		if (strcasecmp (argv[7], "SINGLEQUOTE") == 0)
		    text_separator = '\'';
	    }
	  if (argc == 9)
	    {
		if (strlen (argv[8]) == 3)
		  {
		      if (strcasecmp (argv[8], "TAB") == 0)
			  field_separator = '\t';
		      if (*(argv[8] + 0) == '\'' && *(argv[8] + 2) == '\'')
			  field_separator = *(argv[8] + 1);
		  }
	    }
      }
    else
      {
	  *pzErr =
	      sqlite3_mprintf
	      ("[VirtualText module] CREATE VIRTUAL: illegal arg list\n"
	       "\t\t{ text_path, encoding [, first_row_as_titles [, [decimal_separator [, text_separator, [field_separator] ] ] ] }\n");
	  return SQLITE_ERROR;
      }
    p_vt = (VirtualTextPtr) sqlite3_malloc (sizeof (VirtualText));
    if (!p_vt)
	return SQLITE_NOMEM;
    p_vt->pModule = &virtualtext_module;
    p_vt->nRef = 0;
    p_vt->zErrMsg = NULL;
    p_vt->db = db;
    text =
	text_parse (path, encoding, first_line_titles, field_separator,
		    text_separator, decimal_separator);
    if (!text)
      {
	  /* something is going the wrong way; creating a stupid default table */
	  fprintf (stderr, "VirtualText: invalid data source\n");
	  fflush (stderr);
	  sprintf (sql, "CREATE TABLE %s (ROWNO INTEGER)", vtable);
	  if (sqlite3_declare_vtab (db, sql) != SQLITE_OK)
	    {
		*pzErr =
		    sqlite3_mprintf
		    ("[VirtualText module] cannot build a table from TEXT file\n");
		return SQLITE_ERROR;
	    }
	  p_vt->buffer = NULL;
	  *ppVTab = (sqlite3_vtab *) p_vt;
	  return SQLITE_OK;
      }
    p_vt->buffer = text;
/* preparing the COLUMNs for this VIRTUAL TABLE */
    sprintf (sql, "CREATE TABLE %s (ROWNO INTEGER", vtable);
    col_name = malloc (sizeof (char *) * text->max_n_cells);
    seed = 0;
    for (i = 0; i < text->max_n_cells; i++)
      {
	  strcat (sql, ", ");
	  sprintf (dummyName, "\"%s\"", *(text->titles + i));
	  dup = 0;
	  for (idup = 0; idup < i; idup++)
	    {
		if (strcasecmp (dummyName, *(col_name + idup)) == 0)
		    dup = 1;
	    }
	  if (strcasecmp (dummyName, "PKUID") == 0)
	      dup = 1;
	  if (strcasecmp (dummyName, "Geometry") == 0)
	      dup = 1;
	  if (dup)
	      sprintf (dummyName, "COL_%d", seed++);
	  len = strlen (dummyName);
	  *(col_name + i) = malloc (len + 1);
	  strcpy (*(col_name + i), dummyName);
	  strcat (sql, dummyName);
	  if (*(text->types + i) == VRTTXT_INTEGER)
	      strcat (sql, " INTEGER");
	  else if (*(text->types + i) == VRTTXT_DOUBLE)
	      strcat (sql, " DOUBLE");
	  else
	      strcat (sql, " TEXT");
      }
    strcat (sql, ")");
    if (col_name)
      {
	  /* releasing memory allocation for column names */
	  for (i = 0; i < text->max_n_cells; i++)
	      free (*(col_name + i));
	  free (col_name);
      }
    if (sqlite3_declare_vtab (db, sql) != SQLITE_OK)
      {
	  *pzErr =
	      sqlite3_mprintf
	      ("[VirtualText module] CREATE VIRTUAL: invalid SQL statement \"%s\"",
	       sql);
	  return SQLITE_ERROR;
      }
    *ppVTab = (sqlite3_vtab *) p_vt;
    return SQLITE_OK;
}

static int
vtxt_connect (sqlite3 * db, void *pAux, int argc, const char *const *argv,
	      sqlite3_vtab ** ppVTab, char **pzErr)
{
/* connects the virtual table to some shapefile - simply aliases vshp_create() */
    return vtxt_create (db, pAux, argc, argv, ppVTab, pzErr);
}

static int
vtxt_best_index (sqlite3_vtab * pVTab, sqlite3_index_info * pIndex)
{
/* best index selection */
    if (pVTab || pIndex)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

static int
vtxt_disconnect (sqlite3_vtab * pVTab)
{
/* disconnects the virtual table */
    VirtualTextPtr p_vt = (VirtualTextPtr) pVTab;
    if (p_vt->buffer)
	text_buffer_free (p_vt->buffer);
    sqlite3_free (p_vt);
    return SQLITE_OK;
}

static int
vtxt_destroy (sqlite3_vtab * pVTab)
{
/* destroys the virtual table - simply aliases vtxt_disconnect() */
    return vtxt_disconnect (pVTab);
}

static int
vtxt_open (sqlite3_vtab * pVTab, sqlite3_vtab_cursor ** ppCursor)
{
/* opening a new cursor */
    struct text_buffer *text;
    struct row_pointer *pRow;
    VirtualTextCursorPtr cursor =
	(VirtualTextCursorPtr) sqlite3_malloc (sizeof (VirtualTextCursor));
    if (cursor == NULL)
	return SQLITE_NOMEM;
    cursor->pVtab = (VirtualTextPtr) pVTab;
    cursor->current_row = 0;
    cursor->eof = 0;
    *ppCursor = (sqlite3_vtab_cursor *) cursor;
    text = cursor->pVtab->buffer;
    if (!text)
	cursor->eof = 1;
    else
      {
	  if (text->current_row_buffer)
	    {
		text_row_free (text->current_row_buffer);
		text->current_row_buffer = NULL;
	    }
	  pRow = get_row_pointer (text, cursor->current_row);
	  if (pRow)
	    {
		text->current_row_buffer = text_read_row (text, pRow, 0);
		if (text->current_row_buffer == NULL)
		    cursor->eof = 1;
	    }
	  else
	      cursor->eof = 1;
      }
    return SQLITE_OK;
}

static int
vtxt_close (sqlite3_vtab_cursor * pCursor)
{
/* closing the cursor */
    VirtualTextCursorPtr cursor = (VirtualTextCursorPtr) pCursor;
    sqlite3_free (cursor);
    return SQLITE_OK;
}

static int
vtxt_filter (sqlite3_vtab_cursor * pCursor, int idxNum, const char *idxStr,
	     int argc, sqlite3_value ** argv)
{
/* setting up a cursor filter */
    if (pCursor || idxNum || idxStr || argc || argv)
	pCursor = pCursor;	/* unused arg warning suppression */
    return SQLITE_OK;
}

static int
vtxt_next (sqlite3_vtab_cursor * pCursor)
{
/* fetching next row from cursor */
    struct text_buffer *text;
    struct row_pointer *pRow;
    VirtualTextCursorPtr cursor = (VirtualTextCursorPtr) pCursor;
    text = cursor->pVtab->buffer;
    if (!text)
	cursor->eof = 1;
    else
      {
	  if (text->current_row_buffer)
	    {
		text_row_free (text->current_row_buffer);
		text->current_row_buffer = NULL;
	    }
	  cursor->current_row++;
	  pRow = get_row_pointer (text, cursor->current_row);
	  if (pRow)
	    {
		text->current_row_buffer = text_read_row (text, pRow, 0);
		if (text->current_row_buffer == NULL)
		    cursor->eof = 1;
	    }
	  else
	      cursor->eof = 1;
      }
    return SQLITE_OK;
}

static int
vtxt_eof (sqlite3_vtab_cursor * pCursor)
{
/* cursor EOF */
    VirtualTextCursorPtr cursor = (VirtualTextCursorPtr) pCursor;
    return cursor->eof;
}

static int
vtxt_column (sqlite3_vtab_cursor * pCursor, sqlite3_context * pContext,
	     int column)
{
/* fetching value for the Nth column */
    struct row_buffer *row;
    int nCol = 1;
    int i;
    char buf[4096];
    VirtualTextCursorPtr cursor = (VirtualTextCursorPtr) pCursor;
    struct text_buffer *text = cursor->pVtab->buffer;
    if (column == 0)
      {
	  /* the ROWNO column */
	  sqlite3_result_int (pContext, cursor->current_row);
	  return SQLITE_OK;
      }
    row = text->current_row_buffer;
    if (row == NULL)
	return SQLITE_ERROR;
    for (i = 0; i < text->max_n_cells; i++)
      {
	  if (nCol == column)
	    {
		if (i >= row->n_cells)
		    sqlite3_result_null (pContext);
		else
		  {
		      if (*(row->cells + i))
			{
			    if (*(text->types + i) == VRTTXT_INTEGER)
			      {
				  strcpy (buf, *(row->cells + i));
				  text_clean_integer (buf);
				  sqlite3_result_int64 (pContext, atol (buf));
			      }
			    else if (*(text->types + i) == VRTTXT_DOUBLE)
			      {
				  strcpy (buf, *(row->cells + i));
				  text_clean_double (buf);
				  sqlite3_result_double (pContext, atof (buf));
			      }
			    else
			      {
				  if (text_clean_text
				      (row->cells + i, text->toUtf8))
				    {
					fprintf (stderr,
						 "VirtualText: encoding error\n");
					fflush (stderr);
				    }
				  sqlite3_result_text (pContext,
						       *(row->cells + i),
						       strlen (*
							       (row->cells +
								i)),
						       SQLITE_STATIC);
			      }
			}
		      else
			  sqlite3_result_null (pContext);
		  }
	    }
	  nCol++;
      }
    return SQLITE_OK;
}

static int
vtxt_rowid (sqlite3_vtab_cursor * pCursor, sqlite_int64 * pRowid)
{
/* fetching the ROWID */
    VirtualTextCursorPtr cursor = (VirtualTextCursorPtr) pCursor;
    *pRowid = cursor->current_row;
    return SQLITE_OK;
}

static int
vtxt_update (sqlite3_vtab * pVTab, int argc, sqlite3_value ** argv,
	     sqlite_int64 * pRowid)
{
/* generic update [INSERT / UPDATE / DELETE */
    if (pVTab || argc || argv || pRowid)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_READONLY;
}

static int
vtxt_begin (sqlite3_vtab * pVTab)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

static int
vtxt_sync (sqlite3_vtab * pVTab)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

static int
vtxt_commit (sqlite3_vtab * pVTab)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

static int
vtxt_rollback (sqlite3_vtab * pVTab)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

int
sqlite3VirtualTextInit (sqlite3 * db)
{
    int rc = SQLITE_OK;
    virtualtext_module.iVersion = 1;
    virtualtext_module.xCreate = &vtxt_create;
    virtualtext_module.xConnect = &vtxt_connect;
    virtualtext_module.xBestIndex = &vtxt_best_index;
    virtualtext_module.xDisconnect = &vtxt_disconnect;
    virtualtext_module.xDestroy = &vtxt_destroy;
    virtualtext_module.xOpen = &vtxt_open;
    virtualtext_module.xClose = &vtxt_close;
    virtualtext_module.xFilter = &vtxt_filter;
    virtualtext_module.xNext = &vtxt_next;
    virtualtext_module.xEof = &vtxt_eof;
    virtualtext_module.xColumn = &vtxt_column;
    virtualtext_module.xRowid = &vtxt_rowid;
    virtualtext_module.xUpdate = &vtxt_update;
    virtualtext_module.xBegin = &vtxt_begin;
    virtualtext_module.xSync = &vtxt_sync;
    virtualtext_module.xCommit = &vtxt_commit;
    virtualtext_module.xRollback = &vtxt_rollback;
    virtualtext_module.xFindFunction = NULL;
    sqlite3_create_module_v2 (db, "VirtualText", &virtualtext_module, NULL, 0);
    return rc;
}

int
virtualtext_extension_init (sqlite3 * db)
{
    return sqlite3VirtualTextInit (db);
}
