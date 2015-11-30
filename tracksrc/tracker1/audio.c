/* audio.c */

/* $Author: espie $
 * $Id: audio.c,v 2.8 1991/12/03 21:24:53 espie Exp espie $
 * $Revision: 2.8 $
 * $Log: audio.c,v $
 * Revision 2.8  1991/12/03  21:24:53  espie
 * Added comments.
 *
 * Revision 2.7  1991/12/03  20:43:46  espie
 * Added possibility to get back to MONO for the sgi.
 *
 * Revision 2.6  1991/12/03  18:07:38  espie
 * Added stereo capabilities to the indigo version.
 *
 * Revision 2.5  1991/12/03  13:23:10  espie
 * Minor bug: a SAMPLE_FAULT is a minor error,
 * we should first check that there was no other
 * error before setting it.
 *
 * Revision 2.4  1991/11/19  16:07:19  espie
 * Added comments, moved minor stuff around.
 *
 * Revision 2.3  1991/11/18  14:10:30  espie
 * New resample function coming from the player.
 *
 * Revision 2.2  1991/11/18  01:12:31  espie
 * Added more notes.
 *
 * Revision 2.1  1991/11/17  23:07:58  espie
 * Just computes some frequency-related parameters.
 *
 *
 */

#include <math.h>
#include <malloc.h>

#include "defs.h"

static char *id = "$Id: audio.c,v 2.8 1991/12/03 21:24:53 espie Exp espie $";

/* creates a table for converting ``amiga'' pitch
 * to a step rate at a given resampling frequency.
 * For accuracy, we don't use floating point, but
 * instead fixed point ( << ACCURACY).
 */

#define ACCURACY 16
#define AMIGA_CLOCKFREQ 3575872

int step_table[MAX_PITCH];

 /* holds the increment for finding the next sampled
  * byte at a given pitch (see resample() ).
  */

void
create_step_table (int oversample, int output_fr)
                    		/* we sample oversample i for each byte output */
                   		/* output frequency */
{
  double note_fr;		/* note frequency (in Hz) */
  double step;
  int pitch;			/* amiga pitch */

  step_table[0] = 0;
  for (pitch = 1; pitch < MAX_PITCH; pitch++)
    {
      note_fr = AMIGA_CLOCKFREQ / pitch;
      /* int_to_fix(1) is the normalizing factor */
      step = note_fr / output_fr * int_to_fix (1) / oversample;
      step_table[pitch] = (int) step;
    }
}

/* the musical notes correspond to some specific pitch.
 * It's useful to be able to find them back, at least for
 * arpeggii.
 */
int pitch_table[NUMBER_NOTES];

void
create_notes_table (void)
{
  double base, pitch;
  int i;

  base = AMIGA_CLOCKFREQ / 440;
  for (i = 0; i < NUMBER_NOTES; i++)
    {
      pitch = base / pow (2.0, i / 12.0);
      pitch_table[i] = pitch;
    }
}

void
init_tables (int oversample, int frequency)
{
  create_step_table (oversample, frequency);
  create_notes_table ();
}

#define C fix_to_int(ch->pointer)

/* The playing mechanism itself.
 * According to the current channel automata,
 * we resample the instruments in real time to
 * generate output.
 */

void
resample (struct channel *chan, int oversample, int number)
{
  int i;			/* sample counter */
  int channel;			/* channel counter */
  int sampling;			/* oversample counter */
  SAMPLE sample;		/* sample from the channel */
  int byte[NUMBER_TRACKS];

  /* recombinations of the various data */
  struct channel *ch;

  /* check the existence of samples */
  for (channel = 0; channel < NUMBER_TRACKS; channel++)
    if (!chan[channel].samp->start)
      {
	if (!error)
	  error = SAMPLE_FAULT;
	chan[channel].mode = DO_NOTHING;
      }

  /* do the resampling, i.e., actually play sounds */
  for (i = 0; i < number; i++)
    {
      for (channel = 0; channel < NUMBER_TRACKS; channel++)
	{
	  byte[channel] = 0;
	  for (sampling = 0; sampling < oversample; sampling++)
	    {
	      ch = chan + channel;
	      switch (ch->mode)
		{
		case DO_NOTHING:
		  break;
		case PLAY:
		  /* small liability: the sample may have
                   * changed, and we may be out of range.
                   * However, this routine is time-critical,
                   * so we don't check for this very rare case.
                   */
		  sample = ch->samp->start[C];
		  byte[channel] += sample * ch->volume;
		  ch->pointer += ch->step;
		  if (C >= ch->samp->length)
		    {
		      /* is there a replay ? */
		      if (ch->samp->rp_start)
			{
			  ch->mode = REPLAY;
			  ch->pointer -= int_to_fix (ch->samp->length);
			}
		      else
			ch->mode = DO_NOTHING;
		    }
		  break;
		case REPLAY:
		  /* small liability: the sample may have
                   * changed, and we may be out of range.
                   * However, this routine is time-critical,
                   * so we don't check for this very rare case.
                   */
		  sample = ch->samp->rp_start[C];
		  byte[channel] += sample * ch->volume;
		  ch->pointer += ch->step;
		  if (C >= ch->samp->rp_length)
		    ch->pointer -= int_to_fix (ch->samp->rp_length);
		  break;
		}

	    }
	}
      output_samples ((byte[0] + byte[3]) / oversample,
		      (byte[1] + byte[2]) / oversample);
    }


  flush_buffer ();
}

/* setting up a given note */

void
reset_note (struct channel *ch, int note, int pitch)
{
  ch->pointer = 0;
  ch->mode = PLAY;
  ch->pitch = pitch;
  ch->step = step_table[pitch];
  ch->note = note;
  ch->viboffset = 0;
}

/* changing the current pitch (value
 * may be temporary, and not stored
 * in channel pitch, for instance vibratos.
 */
void
set_current_pitch (struct channel *ch, int pitch)
{
  ch->step = step_table[pitch];
}
