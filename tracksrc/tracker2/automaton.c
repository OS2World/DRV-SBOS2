/* automaton.c */

/* $Author: espie $
 * $Id: automaton.c,v 2.5 1991/11/20 20:46:35 espie Exp espie $
 * $Revision: 2.5 $
 * $Log: automaton.c,v $
 * Revision 2.5  1991/11/20  20:46:35  espie
 * Minor correction.
 *
 * Revision 2.4  1991/11/19  16:07:19  espie
 * Added comments, moved minor stuff around.
 *
 * Revision 2.3  1991/11/18  01:23:30  espie
 * Added two level of fault tolerancy.
 *
 * Revision 2.2  1991/11/18  01:12:31  espie
 * Minor changes.
 *
 * Revision 2.1  1991/11/17  23:07:58  espie
 * Coming from str32.
 *
 *
 */

static char *id = "$Id: automaton.c,v 2.5 1991/11/20 20:46:35 espie Exp espie $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"


/* updates the pattern to play in the automaton.
 * Checks that the pattern actually exists.
 */
void
set_pattern (struct automaton *a)
{
  int p;

  if (a->pattern_num >= a->info->length)
    {
      error = UNRECOVERABLE;
      return;
    }

  /* there is a level of indirection in the format,
   * i.e., patterns can be repeated.
   */
  p = a->info->patnumber[a->pattern_num];
  if (p >= a->info->maxpat)
    {
      error = UNRECOVERABLE;
      return;
    }
  a->pattern = a->info->pblocks + p;
}

/* initialize all the fields of the automaton necessary
 * to play a given song.
 */

void
init_automaton (struct automaton *a, struct song *song)
{
  a->info = song->info;
  a->pattern_num = 0;		/* first pattern */
  a->note_num = 0;		/* first note in pattern */
  a->counter = 0;		/* counter for the effect tempo */
  a->speed = NORMAL_SPEED;	/* this is the default effect tempo */
  a->finespeed = NORMAL_FINESPEED;
  /* this is the fine speed (100%)    */
  a->do_stuff = DO_NOTHING;
  /* some effects affect the automaton,
   * we keep them here.
   */

  error = NONE;			/* Maybe we should not reset errors at
                                 * this point.
                                 */
  set_pattern (a);
}

/* Gets to the next pattern, and displays stuff */

void
advance_pattern (struct automaton *a)
{
  if (pref.verbose)
    PRINT (("\n"));

  PRINT (("%3d", a->pattern_num));
  fflush (stdout);

  if (pref.verbose)
    PRINT (("\n"));

  if (++a->pattern_num >= a->info->length)
    {
      a->pattern_num = 0;
      error = ENDED;
    }
  set_pattern (a);
  a->note_num = 0;
}

/* process all the stuff which we need to advance in the song,
 * including set_speed, set_skip and set_fastskip.
 */
void
next_tick (struct automaton *a)
{
  if (a->do_stuff & SET_SPEED && a->new_speed)
    {
      /* there are three classes of speed changes:
       * 0 does nothing.
       * <32 is the effect speed (resets the fine speed).
       * >=32 changes the finespeed, so this is 32% to 255%
       */
      if (a->new_speed >= 32)
	a->finespeed = a->new_speed;
      else
	{
	  a->speed = a->new_speed;
	  a->finespeed = 100;
	}
    }
  if (++a->counter >= a->speed)
    {
      a->counter = 0;

      if (pref.verbose)
	PRINT (("\n"));

      if (a->do_stuff & SET_FASTSKIP)
	{
	  a->pattern_num = a->new_pattern;
	  set_pattern (a);
	  a->note_num = 0;
	}
      else if (a->do_stuff & SET_SKIP)
	{
	  advance_pattern (a);
	  a->note_num = a->new_note;
	}
      else
	{
	  if (++a->note_num >= BLOCK_LENGTH)
	    advance_pattern (a);
	}
      a->do_stuff = DO_NOTHING;
    }
}
