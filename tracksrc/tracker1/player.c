/* player.c */

/* $Author: espie $
 * $Id: player.c,v 2.10 1991/12/03 23:03:39 espie Exp espie $
 * $Revision: 2.10 $
 * $Log: player.c,v $
 * Revision 2.10  1991/12/03  23:03:39  espie
 * Added transpose feature.
 *
 * Revision 2.9  1991/12/03  21:24:53  espie
 * Reverted to previous behaviour because of intromusic6.b.
 *
 * Revision 2.8  1991/12/03  20:43:46  espie
 * Added possibility to get back to MONO for the sgi.
 *
 * Revision 2.7  1991/12/03  18:07:38  espie
 * Added stereo capabilities to the indigo version.
 *
 * Revision 2.6  1991/12/03  17:12:33  espie
 * Minor changes ??
 *
 * Revision 2.5  1991/11/19  16:07:19  espie
 * Added comments, moved minor stuff around.
 *
 * Revision 2.4  1991/11/18  14:10:30  espie
 * Moved resample part and empty sample test to audio.
 *
 * Revision 2.3  1991/11/18  01:23:30  espie
 * Added two level of fault tolerancy.
 *
 * Revision 2.2  1991/11/18  01:12:31  espie
 * Added some control on the number of replays,
 * and better error recovery.
 *
 * Revision 2.1  1991/11/17  23:07:58  espie
 * Coming from str32.
 *
 */

static char *id = "$Id: player.c,v 2.10 1991/12/03 23:03:39 espie Exp espie $";

#include <stdio.h>

#include "defs.h"

/* init_channel(ch, dummy):
 * setup channel, with initially
 * a dummy sample ready to play,
 * and no note.
 */
void
init_channel (struct channel *ch, struct sample_info *dummy)
{
  ch->samp = dummy;
  ch->mode = DO_NOTHING;
  ch->pointer = 0;
  ch->step = 0;
  ch->volume = 0;
  ch->pitch = 0;
  ch->note = NO_NOTE;

  /* we don't setup arpeggio values. */
  ch->viboffset = 0;
  ch->vibdepth = 0;

  ch->slide = 0;

  ch->pitchgoal = 0;
  ch->pitchrate = 0;

  ch->volumerate = 0;

  ch->vibrate = 0;
  ch->adjust = do_nothing;
}

int VSYNC;			/* base number of sample to output */
void (*eval[NUMBER_EFFECTS]) ();

 /* the effect table */
int oversample;			/* oversample value */
int frequency;			/* output frequency */
int channel;			/* channel loop counter */

struct channel chan[NUMBER_TRACKS];

 /* every channel */
int countdown;			/* keep playing the tune or not */

struct song_info *info;
struct sample_info **voices;

struct automaton a;


void
init_player (int o, int f)
{
  oversample = o;
  frequency = f;
  init_tables (oversample, frequency);
  init_effects (eval);
}

void
setup_effect (struct channel *ch, struct automaton *a, struct event *e)
{
  int samp, cmd;

  /* retrieves all the parameters */
  samp = e[a->note_num].sample_number;
  a->note = e[a->note_num].note;
  if (a->note != NO_NOTE)
    a->pitch = pitch_table[a->note];
  else
    a->pitch = e[a->note_num].pitch;
  cmd = e[a->note_num].effect;
  a->para = e[a->note_num].parameters;

  if (a->pitch >= MAX_PITCH)
    {
      fprintf (stderr, "Pitch out of bounds %d\n", a->pitch);
      a->pitch = 0;
      error = FAULT;
    }

  if (pref.verbose)
    {
      if (samp == 0 && a->pitch == 0 && cmd == 0)
	PRINT (("-----------  "));
      else
	{
	  PRINT (("%2d %s ", samp, a->pitch ? "N" : "-"));
	  if (cmd == 0 && a->para == 0)
	    PRINT (("------  "));
	  else
	    PRINT (("%2d %3d  ", cmd, a->para));
	}
      fflush (stdout);
    }

  /* load new instrument */
  if (samp)
    {
      /* note that we can change sample in the middle
       * of a note. This is a *feature*, not a bug (see
       * intromusic6.b)
       */
      ch->samp = voices[samp];
      ch->volume = voices[samp]->volume;
    }
  /* check for a new note: cmd 3 (portamento)
   * is the special case where we do not restart
   * the note.
   */
  if (a->pitch && cmd != 3)
    reset_note (ch, a->note, a->pitch);
  ch->adjust = do_nothing;
  /* do effects */
  (eval[cmd]) (a, ch);
}

void
play_song (struct song *song, struct pref *pref)
{
  init_automaton (&a, song);
  VSYNC = frequency * 100 / pref->speed;
  /* a repeats of 0 is infinite replays */
  if (pref->repeats)
    countdown = pref->repeats;
  else
    countdown = 1;

  info = song->info;
  voices = song->samples;

  for (channel = 0; channel < NUMBER_TRACKS; channel++)
    init_channel (chan + channel, voices[0]);

  PRINT (("(%d):\n", info->length));

  while (countdown)
    {
      for (channel = 0; channel < NUMBER_TRACKS; channel++)
	if (a.counter == 0)
	  /* setup effects */
	  setup_effect (chan + channel, &a, a.pattern->e[channel]);
	else
	  /* do the effects */
	  (chan[channel].adjust) (chan + channel);

      /* advance player for the next tick */
      next_tick (&a);
      /* actually output samples */
      resample (chan, oversample, VSYNC / a.finespeed);

      switch (error)
	{
	case NONE:
	  break;
	case ENDED:
	  if (pref->repeats)
	    countdown--;
	  break;
	case SAMPLE_FAULT:
	  if (!pref->tolerate)
	    countdown = 0;
	  break;
	case FAULT:
	  if (pref->tolerate < 2)
	    countdown = 0;
	  break;
	case NEXT_SONG:
	case UNRECOVERABLE:
	  countdown = 0;
	  break;
	default:
	  break;
	}
      error = NONE;
    }

  PRINT (("\n"));
}
