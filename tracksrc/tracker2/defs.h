/* defs.h */

/* $Author: espie $
 * $Id: extern.h,v 2.9 1991/12/04 08:28:53 espie Exp espie $
 * $Revision: 2.9 $
 * $Log: extern.h,v $
 * Revision 2.9  1991/12/04  08:28:53  espie
 * Separated mix/stereo stuff.
 */

#include <stdio.h>

/*
 * This is the precise type for storing the samples:
 * should be signed char.
 *
 * Don't change this to 'int' or anything else; I found out the hard way.
 * Apparently, this is used everywhere other than the output drivers,
 * so I don't see any reason to have it be device-dependent.
 * "machine.h" doesn't contain anything else, and things would be simpler
 * for the make if we didn't have it at all.
 * -SH 6/92
 */
typedef signed char SAMPLE;

/* How many audio channels by default (1 = Mono, 2 = Stereo) */
#define DEFAULT_CHANNELS	2

/*
 * What percent of the left/right channel should leak to the other side (-mix)
 */
#define DEFAULT_MIX	30

/* error types. Everything is centralized,
 * and we check in some places (see read, player and str32)
 * that there was no error. Additionnally signal traps work
 * that way too.
 */

#define NONE 0			/* normal state */
#define FILE_TOO_SHORT 1	/* read error */
#define CORRUPT_FILE 2
#define NEXT_SONG 3		/* trap error: goto next song right now */
#define FAULT 4			/* run time problem */
#define ENDED 5			/* the song has ended */
/* unrecoverable problem: typically, trying to jump to nowhere land. */
#define UNRECOVERABLE 6
/* Missing sample. Very common error, not too serious. */
#define SAMPLE_FAULT 7

#define PRINT(x) do { if (!quiet) printf x; } while (0)

#define ACCURACY 16
#define fix_to_int(x) ((x) >> ACCURACY)
#define int_to_fix(x) ((x) << ACCURACY)

#define OLD 0
#define NEW 1 
#define BOTH 2	       /* for when we try to read it as both types.*/
#define NEW_NO_CHECK 3 /* special type: does not check the signature */
#define NUMBER_NOTES 120

/* DO_NOTHING is also used for the automaton */
#define DO_NOTHING 0
#define PLAY 1
#define REPLAY 2

#define MAX_ARP 3

/* 
 * there is no note in each channel initially.  This is defensive
 * programming, because some commands rely on the previous note.
 * Checking that there was no previous note is a way to detect faulty
 * modules.
 */
#define NO_NOTE 255
  
#define SET_SPEED 1
#define SET_SKIP 2
#define SET_FASTSKIP 4

#define NORMAL_SPEED 6
#define NORMAL_FINESPEED 100

#define NUMBER_SAMPLES 32

#define BLOCK_LENGTH 64
#define NUMBER_TRACKS 4
#define NUMBER_PATTERNS 128

#define NUMBER_EFFECTS 16

#define MIN_PITCH 113
#define MAX_PITCH 1023

#define MIN_VOLUME 0
#define MAX_VOLUME 64

#define FUZZ 2			/* the fuzz in note pitch */

/* we refuse to allocate more than 500000 bytes for one sample */
#define MAX_SAMPLE_LENGTH 500000

/* the actual parameters may be split in two halves occasionnally */
#define LOW(para) ((para) & 15)
#define HI(para) ((para) >> 4)

#define MIN(A,B) ((A)<(B) ? (A) : (B))
#define MAX(A,B) ((A)>(B) ? (A) : (B))
#define D fprintf(stderr, "%d\n", __LINE__);

struct channel
{
  struct sample_info *samp;
  int mode;			/* automaton state for the sound generatio */
  unsigned int pointer;		/* current sample position (fixed pos) */
  unsigned int step;		/* sample position increment (gives pitch) */
  int volume;			/* current volume of the sample (0-64) */
  int pitch;			/* current pitch of the sample */
  int note;			/* we have to save the note cause */
				/* we can do an arpeggio without a new note */

  int arp[MAX_ARP];		/* the three pitch values for an arpeggio */
  int arpindex;			/* an index to know which note
				   the arpeggio is doing */

  int viboffset;		/* current offset for vibrato (if any) */
  int vibdepth;			/* depth of vibrato (if any) */

  int slide;			/* step size of pitch slide */

  int pitchgoal;		/* pitch to slide to */
  int pitchrate;		/* step rate for portamento */

  int volumerate;		/* step rate for volume slide */

  int vibrate;			/* step rate for vibrato */
  void (*adjust) ();		/* current command to adjust parameters */
};

struct automaton
{
  int pattern_num;		/* the pattern in the song */
  int note_num;			/* the note in the pattern */
  struct block *pattern;	/* the physical pattern */
  struct song_info *info;	/* we need the song_info */

  int counter;			/* the fine position inside the effect */
  int speed;			/* the `speed', number of effect repeats */
  int finespeed;		/* the finespeed, base is 100 */

  int do_stuff;			/* keeping some stuff to do */
  /* ... and parameters for it: */
  int new_speed, new_note, new_pattern, new_fastspeed;

  int pitch, note, para;	/* some extra parameters effects need */
};

struct pref			/* User preference settings */
{
  int type, speed, tolerate, repeats, stereo, verbose;
};

struct sample_info
{
  char *name;
  int length, rp_offset, rp_length;
  int volume;
  int finetune;
  SAMPLE *start, *rp_start;
};

struct event
{
  unsigned char sample_number;
  unsigned char effect;
  unsigned char parameters;
  unsigned char note;
  int pitch;
};

struct block
{
  struct event e[NUMBER_TRACKS][BLOCK_LENGTH];
};


struct song_info
{
  int length;
  int maxpat;
  char patnumber[NUMBER_PATTERNS];
  struct block *pblocks;
};

struct song
{
  char *title;
  /* sample 0 is always a dummy sample */
  struct sample_info *samples[NUMBER_SAMPLES];
  struct song_info *info;
};

struct compression
{
  char *extension;		/* The last part of the filename */
  char *decomp_cmd;		/* The command to decompress to stdout */
};

extern int error;
extern int quiet;
extern int pitch_table[];	/* pitch of each and every note */
extern struct pref pref;	/* user preferences */

/* xxx_audio.c */

/* real_freq = open_audio(ask_freq):
 * try to open audio with a sampling rate of ask_freq.
 * We get the real frequency back. If we ask for 0, we
 * get the ``preferred'' frequency.
 */
int open_audio (int frequency);

/* close_audio():
 * returns the audio to the system control, doing necessary
 * cleanup
 */
void close_audio (void);

/* set_mix(percent): set mix channels level.
 * 0: spatial stereo. 100: mono.
 */
void set_mix (int percent);

/* output_samples(l, r): outputs a pair of stereo samples.
 * Samples are 15 bits signed.
 */
void output_samples (int left, int right);

/* flush_buffer(): call from time to time, because buffering
 * is done by the program to get better (?) performance.
 */
/* void flush_buffer (unsigned long param); */

/* automaton.c */

/* init_automaton(a, song):
 * put the automaton a in the right state to play song.
 */
void init_automaton (struct automaton *a, struct song *song);

/* next_tick(a):
 * set up everything for the next tick.
 */
void next_tick (struct automaton *a);

/* read.c */

/* s = read_song(f, type):
 * tries to read f as a song of type NEW/OLD.
 * returns NULL (and an error) if it doesn't work.
 * Returns a dynamic song structure if successful.
 */
struct song *read_song (FILE *f, int type, int tr);

/* dump_song(s):
 * displays some information pertinent to the given
 * song s.
 */
void dump_song (struct song *song);

/* release_song(s):
 * release all the memory song occupies.
 */
void release_song (struct song *song);

/* commands.c */

/* init_effects(): sets up all data for the effects */
void init_effects (void (**table) (/* ??? */));

/* do_nothing: this is the default behavior for an effect.
 */
void do_nothing (struct channel *ch);

/* audio.c */

/* init_tables(oversample, frequency):
 * precomputes the step_table and the pitch_table
 * according to the desired oversample and frequency.
 * This is static, you can call it again whenever you want.
 */
void init_tables (int oversample, int frequency);

/* resample(chan, oversample, number):
 * send number samples out computed according
 * to the current state of chan[0:NUMBER_CHANNELS],
 * and oversample.
 */
void resample (struct channel *chan, int oversample, int number);

/* reset_note(ch, note, pitch):
 * set channel ch to play note at pitch pitch
 */
void reset_note (struct channel *ch, int note, int pitch);

/* set_current_pitch(ch, pitch):
 * set channel ch to play at pitch pitch
 */
void set_current_pitch (struct channel *ch, int pitch);

/* player.c */

/* init_player(oversample, frequency):
 * sets up the player for a given oversample and
 * output frequency.
 */
void init_player (int o, int f);

/* play_song(song, pref):
 * plays the song according to the current pref.
 */
void play_song (struct song *song, struct pref *pref);

