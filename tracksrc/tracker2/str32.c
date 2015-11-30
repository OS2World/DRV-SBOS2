/* str32.c */

/* plays sound/noisetracker files on Sparc, silicon graphics, Sound Blaster.
 * Authors  : Liam Corner - zenith@dcs.warwick.ac.uk
 *            Marc Espie - espie@dmi.ens.fr
 *            Steve Haehnichen - shaehnic@ucsd.edu
 *            Michael Fulbright - msf@as.arizona.edu
 *
 * Usage    : tracker [options] <filename>
 *  this version plays compressed files of various types as well.
 */


/* Michael Fulbright - Made this devil work under OS/2
 *                     version 0.91 10/17/92
 *                     Converted to use OS/2 semaphores
 *
 * $Author: steve $
 * $Id: str32.c,v 1.4 1992/06/30 00:51:48 steve Exp steve $
 * $Revision: 1.4 $
 * $Log: str32.c,v $
 * Revision 1.4  1992/06/30  00:51:48  steve
 * Ready to unleash this on the world.
 *
 * Revision 1.3  1992/06/28  10:56:07  steve
 * Added a few cmd line options.
 * Fixed Sound Blaster stereo bugs.
 *
 * Revision 1.2  1992/06/25  17:55:02  steve
 * Reworked command-line parsing to use GNU getopt(3).
 * Generalized auto decompression routines to use a lookup table.
 *
 * Revision 1.2  1992/06/24  22:27:30  steve
 * Redoing command line options etc.
 *
 * Revision 2.9  1991/12/04  08:28:53  espie
 * Separated mix/stereo stuff.
 */

static char *id = "$Id: str32.c,v 1.4 1992/06/30 00:51:48 steve Exp steve $";

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "getopt.h"             /* GNU getopt() package */

#define USAGE \
  "[options] filename [filename [...]]\n"\
  "(Options may be preceeded by - or + and can be uniquely abbreviated.)\n"\
  "-help             Display usage information\n"\
  "-quiet            Print no output other than errors\n"\
  "-picky            Do not tolerate any faults (default is to ignore all)\n"\
  "-mono             Select single audio channel output\n"\
  "-stereo           Select dual audio channel output\n"\
  "-verbose          Show text representation of song\n"\
  "-repeats <count>  Number of repeats (0 is forever) (default 1)\n"\
  "-speed <speed>    Song speed.  Some songs want 60 (default 50)\n"\
  "-mix <percent>    Percent of channel mixing. (0 = spatial, 100 = mono)\n"\
  "-new -old -both   Select default reading type (default is -both)\n"\
  "-frequency <freq>        Set playback frequency in Hz\n"\
  "-oversample <times>      Set oversampling factor\n"\
  "-transpose <half-steps>  Transpose all notes up\n"

/* Command-line options. */
const struct option long_options[] =
{
  {"help",              0, NULL, 'H'},
  {"quiet",             0, NULL, 'Q'},
  {"picky",             0, NULL, 'P'},
  {"new",               0, NULL, 'N'},
  {"old",               0, NULL, 'O'},
  {"both",              0, NULL, 'B'},
  {"mono",              0, NULL, 'M'},
  {"stereo",            0, NULL, 'S'},
  {"verbose",           0, NULL, 'V'},
  {"frequency",         1, NULL, 'f'},
  {"oversample",        1, NULL, 'o'},
  {"transpose",         1, NULL, 't'},
  {"repeats",           1, NULL, 'r'},
  {"speed",             1, NULL, 's'},
  {"mix",               1, NULL, 'm'},
  {NULL, 0, NULL, 0}
};

/*
 * Table for mapping file suffixes to decompression
 * commands.  To add a new decompression method, simply
 * create another entry with the suffix and command to
 * decompress.  Substitude %s in the command for the name of
 * the compressed file.
 * The output from the decompression should be a single MOD file
 * sent to stdout with no other data or text messages.
 */
static struct compression comp_table[] =
{
  /* suffix */  /* Decompression command */
  {".Z",        "zcat %s"},
  {".zoo",      "zoo xpq %s"},
  {".lzh",      "lha pq %s"},
  {".zip",      "unzip -pqq %s"},
  {".arc",      "arc pn %s"},
  {NULL, NULL}
};
    

int quiet = 0;                  /* Global flag for no text output */
struct pref pref;               /* Global user preferences */

/* global variable to catch various types of errors
 * and achieve the desired flow of control
 */
int error;
/* small hack for transposing songs on the fly */
static int transpose;

/* signal handlers */

int
goodbye (int sig)
{
  PRINT (("\nSignal %d\n", sig));
  exit (10);
}

int
nextsong (int sig)
{

  /* emx docs say its dangerous to do i/o now, so commenting these out */
/*  PRINT (("\nSignal %d\n", sig));
  PRINT (("Aborting current song\n"));
*/

  /* clear out buffers */
/*  empty_buffers(); */

  /* re-activate signal */
  signal(sig, SIG_ACK);
  error = NEXT_SONG;
}

/* v = read_env(name, default): reads the value v
 * in the environment, supplies a defaults.
 */
int
read_env (char *name, int def)
{
  char *var;
  int value;

  var = (char *) getenv (name);
  if (!var)
    return def;
  if (sscanf (var, "%d", &value) == 1)
    return value;
  else
    return def;
}

/*
 * Check if the filename ends in an extension (suffix).
 */
int
check_ext (char *ext, char *s)
{
  char *p;
  int i;
  int ext_len = strlen (ext);
  int s_len = strlen (s);

  if (s_len < ext_len)
    return 0;
  
  for (i = 0; i < ext_len; i++)
    if (s[s_len - ext_len + i] != ext[i])
      return 0;

  return 1;
}

struct song *
do_read_song (char *s, int type)
{
  struct song *song;
  struct compression *comp;
  FILE *fp;
  char buf[200];

  PRINT (("Loading %s... ", s));
  fflush (stdout);

  /* Check if the filename matches any compression extentions */
  for (comp = comp_table; comp->extension != NULL; comp++)
    {
      if (check_ext (comp->extension, s))
        {
          sprintf (buf, comp->decomp_cmd, s);

          fp = popen (buf, "rb");
          if (fp == NULL)
            {
              fprintf (stderr, "Unable to open compressed file %s\n", s);
              return NULL;
            }
          song = read_song (fp, type, transpose);

	  /* now flush pipe, needed for compressed files that fail */
	  while (fgetc(fp) != EOF);
	  
          pclose (fp);
          break;
        }
    }

  /* No compression extentions matched, so just load it straight */
  if (!comp->extension)
    {
      fp = fopen (s, "rb");
      if (fp == NULL)
        {
          fprintf (stderr, "Unable to open tune file %s\n", s);
          return NULL;
        }
      song = read_song (fp, type, transpose);
      fclose (fp);
    }
  
  if (song)
    PRINT (("Ok!\n"));
  return song;
}


int
main (int argc, char **argv)
{

  int frequency, oversample, stereo;
  struct song *song;
  int index;
  int c;
  int opt, error_flag;


  /* supposed to make wildcard expansion */
  _wildcard(&argc,&argv);

  signal (2, nextsong); 
  signal (SIGBREAK, goodbye);

  /* identify ourselves */
  printf("Tracker2 V0.91 for the SBOS2 package\n");


  /* Read environment variables */
  frequency = read_env ("FREQUENCY", 0);
  oversample = read_env ("OVERSAMPLE", 1);
  transpose = read_env ("TRANSPOSE", 0);

  if (getenv ("MONO"))
    pref.stereo = 0;
  else if (getenv ("STEREO"))
    pref.stereo = 1;
  else
    pref.stereo = DEFAULT_CHANNELS - 1;
  pref.type = BOTH;
  pref.repeats = 1;
  pref.speed = 50;
  pref.tolerate = 2;
  pref.verbose = 0;
  set_mix (DEFAULT_MIX);        /* 0 = full stereo, 100 = mono */

  error_flag = 0;
  while ((opt = getopt_long_only (argc, argv, "", long_options, NULL)) != EOF)
    {
      switch (opt)
        {
        case 'H':               /* help */
          error_flag++;
          break;
        case 'Q':               /* quiet */
          quiet++;
          break;
        case 'P':               /* abort on faults (be picky) */
          pref.tolerate = 0;
          break;
        case 'N':               /* new tracker type */
          pref.type = NEW;
          break;
        case 'O':               /* old tracker type */
          pref.type = OLD;
          break;
        case 'B':               /* both tracker types */
          pref.type = BOTH;
          break;
        case 'M':               /* mono */
          pref.stereo = 0;
          break;
        case 'S':               /* stereo */
          pref.stereo = 1;
          break;
        case 'V':
          pref.verbose = 1;
          break;
        case 'f':               /* frequency */
          frequency = atoi (optarg);
          break;
        case 'o':               /* oversampling */
          oversample = atoi (optarg);
          break;
        case 't':               /* transpose half-steps*/
          transpose = atoi (optarg);
          break;
        case 'r':               /* number of repeats */
          pref.repeats = atoi (optarg);
          break;
        case 's':               /* speed */
          pref.speed = atoi (optarg);
          break;
        case 'm':               /* % of channel mix.  100=mono */
          set_mix (atoi (optarg));
          break;
        default:                /* ??? */
          error_flag++;
          break;
        }
    }
  if (error_flag || !argv[optind])
    {
      fprintf (stderr, "Usage: %s " USAGE, argv[0]);
      exit(1);
    }



  frequency = open_audio (frequency);
  init_player (oversample, frequency);

  while (argv[optind])
    {
      switch (pref.type)
        {
        case BOTH:
          song = do_read_song (argv[optind], NEW);
          if (!song)
            song = do_read_song (argv[optind], OLD);
          break;
        case OLD:
          song = do_read_song (argv[optind], pref.type);
          break;
        case NEW:
          /* this is explicitly flagged as a new module,
           * so we don't need to look for a signature.
           */
          song = do_read_song (argv[optind], NEW_NO_CHECK);
          break;
        }

      printf("Back from read_song\n");
      optind++;
      if (song == NULL)
        continue;

      dump_song (song);


      /* set up output buffer and thread */
      open_buffer();

      play_song (song, &pref);
      release_song (song);

      /* finish playing song */
      flush_out_buffer();

      close_buffer(); /* release output buffer and thread */

    }

  close_audio ();
  return 0;
}
