/* commands.c */

/* $Author: espie $
 * $Id: commands.c,v 2.4 1991/12/03 13:23:10 espie Exp espie $
 * $Revision: 2.4 $
 * $Log: commands.c,v $
 * Revision 2.4  1991/12/03  13:23:10  espie
 * Defensive programming: check the range of each note
 * for arpeggio setup.
 *
 * Revision 2.3  1991/11/19  16:07:19  espie
 * Added comments, moved minor stuff around.
 *
 * Revision 2.2  1991/11/18  01:12:31  espie
 * Minor changes.
 *
 * Revision 2.1  1991/11/17  23:07:58  espie
 * Used some constants.
 *
 * Revision 2.0  1991/11/17  21:42:08  espie
 * Structured part of the code, especially replay ``automaton''
 * and setting up of effects.
 *
 * Revision 1.9  1991/11/17  17:09:53  espie
 * Added missing prototypes.
 *
 * Revision 1.8  1991/11/16  15:42:43  espie
 * tabs.
 *
 * Revision 1.7  1991/11/08  14:25:55  espie
 * Dynamic oversample and frequency.
 *
 * Revision 1.6  1991/11/07  21:40:16  espie
 * Added arpeggio.
 *
 * Revision 1.5  1991/11/07  20:12:34  espie
 * Minor problem with version id.
 *
 * Revision 1.4  1991/11/07  20:11:10  espie
 * Added embedded version id.
 *
 * Revision 1.3  1991/11/07  20:05:53  espie
 * Fixed up vibrato depth.
 * Added vibslide and portaslide.
 *
 * Revision 1.2  1991/11/07  15:27:02  espie
 * Added command 9.
 *
 * Revision 1.1  1991/11/06  09:46:06  espie
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include "defs.h"

static char *id = "$Id: commands.c,v 2.4 1991/12/03 13:23:10 espie Exp espie $";

/* sine table for the vibrato effect (could be much more precise) */

int vibrato_table[32] =
{0, 25, 49, 71, 90, 106, 117, 125, 127, 125, 117, 106, 90,
 71, 49, 25, 0, -25, -49, -71, -90, -106, -117, -125, -127, -125,
 -117, -106, -90, -71, -49, -25};

/***
 *
 *
 *  setting up effects/doing effects.
 *  The set_xxx gets called while parsing the effect,
 *  the do_xxx gets called each tick, and update the
 *  sound parameters while playing it.
 *
 *
 ***/


void
do_nothing (struct channel *ch)
{
}

void
set_nothing (struct automaton *a, struct channel *ch)
{
}

/* slide pitch (up or down) */

void
do_slide (struct channel *ch)
{
  ch->pitch += ch->slide;
  ch->pitch = MIN (ch->pitch, MAX_PITCH);
  ch->pitch = MAX (ch->pitch, MIN_PITCH);
  set_current_pitch (ch, ch->pitch);
}

void
set_upslide (struct automaton *a, struct channel *ch)
{
  ch->adjust = do_slide;
  if (a->para)
    ch->slide = a->para;
}

void
set_downslide (struct automaton *a, struct channel *ch)
{
  ch->adjust = do_slide;
  if (a->para)
    ch->slide = -a->para;
}

/* modulating the pitch with vibrato */

void
do_vibrato (struct channel *ch)
{
  int offset;

  /* this is a literal transcription of the protracker
   * code. I should rescale the vibrato table at some point
   */
  ch->viboffset += ch->vibrate;
  ch->viboffset %= 64;
  offset = (vibrato_table[ch->viboffset >> 1] * ch->vibdepth) / 64;
  /* temporary update of only the step value,
   * note that we do not change the saved pitch.
   */
  set_current_pitch (ch, ch->pitch + offset);
}

void
set_vibrato (struct automaton *a, struct channel *ch)
{
  ch->adjust = do_vibrato;
  if (a->para)
    {
      ch->vibrate = HI (a->para);
      ch->vibdepth = LOW (a->para);
    }
}

/* arpeggio looks a bit like chords: we alternate between two
 * or three notes very fast.
 * Issue: we are able to re-generate real chords. Would that be
 * better ? To try.
 */
void
do_arpeggio (struct channel *ch)
{
  if (++ch->arpindex >= MAX_ARP)
    ch->arpindex = 0;
  set_current_pitch (ch, ch->arp[ch->arpindex]);
}

void
set_arpeggio (struct automaton *a, struct channel *ch)
{
  /* normal play is arpeggio with 0/0 */
  if (!a->para)
    return;
  /* arpeggio can be installed relative to the
   * previous note, so we have to check that there
   * actually is a current(previous) note
   */
  if (ch->note == NO_NOTE)
    {
      fprintf (stderr,
	       "No note present for arpeggio");
      error = FAULT;
    }
  else
    {
      int note;

      ch->arp[0] = pitch_table[ch->note];
      note = ch->note + HI (a->para);
      if (note < NUMBER_NOTES)
	ch->arp[1] = pitch_table[note];
      else
	{
	  fprintf (stderr, "Arpeggio note out of range");
	  error = FAULT;
	}
      note = ch->note + LOW (a->para);
      if (note < NUMBER_NOTES)
	ch->arp[2] = pitch_table[note];
      else
	{
	  fprintf (stderr, "Arpeggio note out of range");
	  error = FAULT;
	}
      ch->arpindex = 0;
      ch->adjust = do_arpeggio;
    }
}

/* volume slide. Mostly used to simulate waveform control.
 * (attack/decay/sustain).
 */
void
do_slidevol (struct channel *ch)
{
  ch->volume += ch->volumerate;
  ch->volume = MIN (ch->volume, MAX_VOLUME);
  ch->volume = MAX (ch->volume, MIN_VOLUME);
}

/* note that volumeslide does not have a ``take default''
 * behavior. If para is 0, this is truly a 0 volumeslide.
 * Issue: is the test really necessary ? Can't we do
 * a HI(para) - LOW(para).
 */
void
parse_slidevol (struct channel *ch, int para)
{
  if (LOW (para))
    ch->volumerate = -LOW (para);
  else
    ch->volumerate = HI (para);
}

void
set_slidevol (struct automaton *a, struct channel *ch)
{
  ch->adjust = do_slidevol;
  parse_slidevol (ch, a->para);
}

/* portamento: gets from a given pitch to another.
 * We can simplify the routine by cutting it in
 * a pitch up and pitch down part while setting up
 * the effect.
 */
void
do_portamento (struct channel *ch)
{
  if (ch->pitch < ch->pitchgoal)
    {
      ch->pitch += ch->pitchrate;
      ch->pitch = MIN (ch->pitch, ch->pitchgoal);
    }
  else if (ch->pitch > ch->pitchgoal)
    {
      ch->pitch -= ch->pitchrate;
      ch->pitch = MAX (ch->pitch, ch->pitchgoal);
    }
  set_current_pitch (ch, ch->pitch);
}

/* if para and pitch are 0, this is obviously a continuation
 * of the previous portamento.
 */
void
set_portamento (struct automaton *a, struct channel *ch)
{
  ch->adjust = do_portamento;
  if (a->para)
    ch->pitchrate = a->para;
  if (a->pitch)
    ch->pitchgoal = a->pitch;
}

/*
 * combined commands.
 */

void
do_portaslide (struct channel *ch)
{
  do_portamento (ch);
  do_slidevol (ch);
}

void
set_portaslide (struct automaton *a, struct channel *ch)
{
  ch->adjust = do_portaslide;
  parse_slidevol (ch, a->para);
}

void
do_vibratoslide (struct channel *ch)
{
  do_vibrato (ch);
  do_slidevol (ch);
}

void
set_vibratoslide (struct automaton *a, struct channel *ch)
{
  ch->adjust = do_vibratoslide;
  parse_slidevol (ch, a->para);
}

/***
 *
 *  effects that just need a setup part
 *
 ***/

/* IMPORTANT: because of the special nature of
 * the player, we can't process each effect independently,
 * we have to merge effects from the four channel before
 * doing anything about it. For instance, there can be
 * several speed changein the same note,
 * only the last one takes effect.
 */

void
set_speed (struct automaton *a, struct channel *ch)
{
  a->new_speed = a->para;
  a->do_stuff |= SET_SPEED;
}

void
set_skip (struct automaton *a, struct channel *ch)
{
  /* yep, this is BCD. */
  a->new_note = HI (a->para) * 10 + LOW (a->para);
  a->do_stuff |= SET_SKIP;
}

void
set_fastskip (struct automaton *a, struct channel *ch)
{
  a->new_pattern = a->para;
  a->do_stuff |= SET_FASTSKIP;
}

/* immediate effect: starts the sample somewhere
 * off the start.
 */
void
set_offset (struct automaton *a, struct channel *ch)
{
  ch->pointer = int_to_fix (a->para * 256);
}

/* change the volume of the current channel.
 * Is effective until there is a new set_volume,
 * slide_volume, or an instrument is reloaded
 * explicitly by giving its number. Obviously, if
 * you load an instrument and do a set_volume in the
 * same note, the set_volume will take precedence.
 */
void
set_volume (struct automaton *a, struct channel *ch)
{
  ch->volume = a->para;
}

/* Initialize the whole effect table */

void
init_effects (void (**table) (/* ??? */))
{
  table[0] = set_arpeggio;
  table[15] = set_speed;
  table[13] = set_skip;
  table[11] = set_fastskip;
  table[12] = set_volume;
  table[10] = set_slidevol;
  table[9] = set_offset;
  table[3] = set_portamento;
  table[5] = set_portaslide;
  table[2] = set_upslide;
  table[1] = set_downslide;
  table[4] = set_vibrato;
  table[6] = set_vibratoslide;
  table[14] = set_nothing;
  table[7] = set_nothing;
  table[8] = set_nothing;
}
