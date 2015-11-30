/* read.c */

/* $Author: espie $
 * $Id: read.c,v 2.7 1991/12/03 23:03:39 espie Exp $
 * $Revision: 2.7 $
 * $Log: read.c,v $
 * Revision 2.7  1991/12/03  23:03:39  espie
 * Added transpose feature.
 *
 * Revision 2.6  1991/12/03  21:24:53  espie
 * Feature fix: length 1 sample should be empty.
 *
 * Revision 2.5  1991/12/03  17:10:11  espie
 * Corrected repeat length problems concerning badly formed files,
 * added signature checking for new tracker files.
 *
 * Revision 2.4  1991/11/19  16:07:19  espie
 * Added comments, moved minor stuff around.
 *
 * Revision 2.3  1991/11/18  14:10:30  espie
 * Corrected small problem with repeat being too short.
 *
 * Revision 2.2  1991/11/18  01:10:45  espie
 * Minor corrections.
 *
 * Revision 2.1  1991/11/17  23:07:58  espie
 * Coded error types. More amiga specific stuff.
 *
 * Revision 2.0  1991/11/17  21:42:08  espie
 * New version.
 *
 * Revision 1.17  1991/11/17  16:30:48  espie
 * Forgot to return a song_info from new_song_info
 *
 * Revision 1.16  1991/11/16  15:50:34  espie
 * Tabs.
 *
 * Revision 1.15  1991/11/16  15:42:43  espie
 * Rationnalized error recovery.
 * There was a bug: you could try to deallocate
 * stuff in no-noland. Also, strings never got
 * to be freed.
 *
 * Revision 1.14  1991/11/15  20:57:34  espie
 * Centralized error control to error_song.
 *
 * Revision 1.13  1991/11/15  18:22:10  espie
 * Added a new test on length, aborts most modules now.
 * Maybe should say it as well.
 *
 * Revision 1.12  1991/11/10  16:26:14  espie
 * Nasty bug regarding evaluation order in getulong.
 * Bitten on the sparc.
 *
 * Revision 1.11  1991/11/09  17:47:33  espie
 * Added checkpoints for early return if file too short.
 *
 * Revision 1.10  1991/11/08  13:35:57  espie
 * Added memory recovery and error control.
 *
 * Revision 1.9  1991/11/08  12:37:37  espie
 * Bug in checkfp: should return an int
 * because characters are signed on some machines,
 * and we want to use it as un unsigned value
 * for computing short values.
 *
 * Revision 1.8  1991/11/07  23:29:09  espie
 * Suppressed ! warning for bad note.
 *
 * Revision 1.7  1991/11/07  21:40:16  espie
 * Added note support.
 *
 * Revision 1.6  1991/11/07  20:12:34  espie
 * Minor problem with version id.
 *
 * Revision 1.5  1991/11/07  20:11:10  espie
 * Added embedded version id.
 *
 * Revision 1.4  1991/11/07  15:27:02  espie
 * Added some error control, essentially checkgetc.
 *
 * Revision 1.3  1991/11/05  22:49:03  espie
 * Modified the format of the dump slightly for
 * a better readability.
 *
 * Revision 1.2  1991/11/04  20:27:05  espie
 * Corrected length and rep_length/rep_offset
 * which are given in words and should be converted to
 * bytes.
 *
 * Revision 1.1  1991/11/04  13:23:59  espie
 * Initial revision
 *
 *
 */

#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "defs.h"

static char *id = "$Id: read.c,v 2.7 1991/12/03 23:03:39 espie Exp $";

static int transpose;

int
find_note (int pitch)
{
  int a, b, i;

  if (pitch == 0)
    return -1;
  a = 0;
  b = NUMBER_NOTES - 1;
  while (b - a > 1)
    {
      i = (a + b) / 2;
      if (pitch_table[i] == pitch)
	return i + transpose;
      if (pitch_table[i] > pitch)
	a = i;
      else
	b = i;
    }
  if (pitch_table[a] - FUZZ <= pitch)
    return a + transpose;
  if (pitch_table[b] + FUZZ >= pitch)
    return b + transpose;
  return NO_NOTE;
}

/* c = checkgetc(f):
 * gets a character from file f.
 * Aborts program if file f is finished
 */

int
checkgetc (FILE *f)
{
  int c;

  if ((c = fgetc (f)) == EOF)
    error = FILE_TOO_SHORT;
  return c;
}

/* s = getstring(f, len):
 * gets a soundtracker string from file f.
 * I.e, it is a fixed length string terminated
 * by a 0 if too short
 */

#define MAX_LEN 50

char *
getstring (FILE *f, int len)
{
  static char s[MAX_LEN];
  char *new;
  int i;

  for (i = 0; i < len; i++)
    s[MIN (i, MAX_LEN - 1)] = checkgetc (f);
  s[MIN (len, MAX_LEN - 1)] = '\0';
  new = malloc (strlen (s) + 1);

  return strcpy (new, s);
}

/* byteskip(f, len)
 * same as fseek, xcpt it works on stdin
 */

void
byteskip (FILE *f, int len)
{
  int i;

  for (i = 0; i < len; i++)
    checkgetc (f);
}

/* v = getushort(f)
 * reads an unsigned short from f
 */

int
getushort (FILE *f)
{
  int i;

  i = checkgetc (f) << 8;
  return i | checkgetc (f);
}

void
fill_sample_info (struct sample_info *info, FILE *f)
{
  info->name = getstring (f, 22);
  info->length = getushort (f);
  info->finetune = checkgetc (f);
  info->volume = checkgetc (f);
  info->volume = MIN (info->volume, MAX_VOLUME);
  info->rp_offset = getushort (f);
  info->rp_length = getushort (f);

  /* the next check is for old modules for which
   * the sample data types are a bit confused, so
   * that what we were expecting to be #words is #bytes.
   */
  /* not sure I understand the -1 myself, though it's
   * necessary to play kawai-k1 correctly
   */
  if (info->rp_length + info->rp_offset - 1 > info->length)
    info->rp_offset /= 2;

  if (info->rp_length + info->rp_offset > info->length)
    info->rp_length = info->length - info->rp_offset;

  info->length *= 2;
  info->rp_offset *= 2;
  info->rp_length *= 2;
  /* in all logic, a 2-sized sample could exist,
   * but this is not the case, and even so, some
   * trackers output empty instruments as being 2-sized.
   */
  if (info->length <= 2)
    return;

  info->start = (SAMPLE *) calloc (info->length, 1);

  if (info->rp_length > 2)
    info->rp_start = info->start + info->rp_offset;
  else
    info->rp_start = NULL;

  if (info->length > MAX_SAMPLE_LENGTH)
    error = CORRUPT_FILE;
}

void
fill_song_info (struct song_info *info, FILE *f)
{
  int i;
  int p;

  info->length = checkgetc (f);
  checkgetc (f);
  info->maxpat = -1;
  for (i = 0; i < NUMBER_PATTERNS; i++)
    {
      p = getc (f);
      if (p >= NUMBER_PATTERNS)
	p = 0;
      if (p > info->maxpat)
	info->maxpat = p;
      info->patnumber[i] = p;
    }
  info->maxpat++;
  if (info->maxpat == 0 || info->length == 0)
    error = CORRUPT_FILE;
}

void
fill_event (struct event *e, FILE *f)
{
  int a, b, c, d;

  a = checkgetc (f);
  b = checkgetc (f);
  c = checkgetc (f);
  d = checkgetc (f);
  e->sample_number = a & 0x10 | (c >> 4);
  e->effect = c & 0xf;
  e->parameters = d;
  e->pitch = ((a & 15) << 8) | b;
  e->note = find_note (e->pitch);
}

void
fill_pattern (struct block *pattern, FILE *f)
{
  int i, j;

  for (i = 0; i < BLOCK_LENGTH; i++)
    for (j = 0; j < NUMBER_TRACKS; j++)
      fill_event (&(pattern->e[j][i]), f);
}

void
read_sample (struct sample_info *info, FILE *f)
{
  int i;

  if (info->start)
    {
      fread (info->start, 1, info->length, f);
    }
}

/***
 *
 *  new_XXX: allocates a new structure for a song.
 *  clears each and every field as appropriate.
 *
 ***/

struct song *
new_song (void)
{
  struct song *new;
  int i;

  new = (struct song *) malloc (sizeof (struct song));

  new->title = NULL;
  new->info = NULL;
  for (i = 0; i < NUMBER_SAMPLES; i++)
    new->samples[i] = NULL;
  return new;
}

struct sample_info *
new_sample_info (void)
{
  struct sample_info *new;

  new = (struct sample_info *) malloc (sizeof (struct sample_info));

  new->name = NULL;
  new->length = NULL;
  new->start = NULL;
  new->rp_start = NULL;
  return new;
}

struct song_info *
new_song_info (void)
{
  struct song_info *new;

  new = (struct song_info *) malloc (sizeof (struct song_info));

  new->length = 0;
  new->maxpat = -1;
  new->pblocks = NULL;
  return new;
}

/* release_song(song): gives back all memory
 * occupied by song. Assume that each structure
 * has been correctly allocated by a call to the
 * corresponding new_XXX function.
 */
void
release_song (struct song *song)
{
  int i;

  for (i = 0; i < 31; i++)
    {
      if (song->samples[i])
	{
	  if (song->samples[i]->start)
	    free (song->samples[i]->start);
	  if (song->samples[i]->name)
	    free (song->samples[i]->name);
	  free (song->samples[i]);
	}
    }
  if (song->info)
    {
      if (song->info->pblocks)
	free (song->info->pblocks);
      free (song->info);
    }
  if (song->title)
    free (song->title);
  free (song);
}

/* error_song(song): what we should return
 * if there was an error. Actually, is mostly
 * useful for its side effects.
 */
struct song *
error_song (struct song *song)
{
  release_song (song);
  fprintf (stderr, "Error while reading file\n");
  return NULL;
}

/* bad_sig(f): read the signature on file f
 * and returns !0 if it is not a known sig.
 */
int
bad_sig (FILE *f)
{
  char a, b, c, d;

  a = fgetc (f);
  b = fgetc (f);
  c = fgetc (f);
  d = fgetc (f);
  if (a == 'M' && b == '.' && c == 'K' && d == '.')
    return 0;
  if (a == 'M' && b == '&' && c == 'K' && d == '!')
    return 0;
  if (a == 'F' && b == 'L' && c == 'T' && d == '4')
    return 0;
  return 1;
}

/* s = read_song(f, type): tries to read a song s
 * of type NEW/OLD in file f. Might fail, i.e.,
 * returns NULL if file is not a mod file of the
 * correct type.
 */
struct song *
read_song (FILE *f, int type, int tr)
{
  struct song *song;
  int i;
  int ninstr;

  error = NONE;
  transpose = tr;
  if (type == NEW || type == NEW_NO_CHECK)
    ninstr = 31;
  else
    ninstr = 15;

  song = new_song ();
  song->title = getstring (f, 20);
  if (error != NONE)
    return error_song (song);

  for (i = 0; i <= 31; i++)
    song->samples[i] = new_sample_info ();

  for (i = 1; i <= ninstr; i++)
    {
      fill_sample_info (song->samples[i], f);
      if (error != NONE)
	return error_song (song);
    }
  song->info = new_song_info ();

  fill_song_info (song->info, f);

  if (error != NONE)
    return error_song (song);

  if (type == NEW && bad_sig (f))
    return error_song (song);

  if (type == NEW_NO_CHECK)
    byteskip (f, 4);


  song->info->pblocks = (struct block *)
    malloc (sizeof (struct block) * song->info->maxpat);

  for (i = 0; i < song->info->maxpat; i++)
    {
      fill_pattern (song->info->pblocks + i, f);
      if (error != NONE)
	return error_song (song);
    }

  for (i = 1; i <= ninstr; i++)
    read_sample (song->samples[i], f);

  if (error != NONE)
    return error_song (song);
  return song;
}

/***
 *
 *  dump_block/dump_song:
 *  shows most of the readable info
 *  concerning a module on the screen.
 *
 ***/

void
dump_block (struct block *b)
{
  int i, j;

  for (i = 0; i < BLOCK_LENGTH; i++)
    {
      for (j = 0; j < NUMBER_TRACKS; j++)
	{
	  PRINT (("%8d%5d%2d%4d", b->e[j][i].sample_number,
		  b->e[j][i].pitch, b->e[j][i].effect,
		  b->e[j][i].parameters));
	}
      PRINT (("\n"));
    }
}

void
dump_song (struct song *song)
{
  int i;

  PRINT (("%s\n", song->title));

  for (i = 1; i <= 31; i++)
    {
      if (song->samples[i]->start)
	{
	  PRINT (("\t%-30s", song->samples[i]->name));
	  PRINT (("%5d", song->samples[i]->length));
	  if (song->samples[i]->rp_length > 2)
	    PRINT (("(%5d %5d)\n", song->samples[i]->rp_offset,
		    song->samples[i]->rp_length));
	  else
	    PRINT (("\n"));
	}
    }
}
