/* program to control mixer levels */

/* added by msf */
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <os2.h>
#include "sblast_user.h"

static struct sb_mixer_levels sblevels;
HFILE mixer_handle;

/* vars for getopt */
extern char * optarg;
extern int  optind;


static char USAGE[]={"\nSETMIXER - Set SB PRO mixer levels\n"\
"Options:\n"\
"  -m l,r       - set master volume [0-15]\n"\
"  -v l,r       - set voice volume [0-15]\n"\
"  -f l,r       - set FM volume [0-15]\n"\
"  -c l,r       - set CD IN volume [0-15]\n"\
"  -l l,r       - set LINE IN volume [0-15]\n"\
"  -x l         - set MIC IN volume [0-7]\n"\
"\n  -q           - run quietly (no output)\n"\
"  -h -?        - this help message\n"\
"\nNote: If you specify one number for the options requiring two, then\n"\
"      both left and right channels will be set to that number.\n"};

/* parse argument in form "left,right" */
int getlevels( char * sptr, BYTE * left, BYTE * right )
{

  char * tptr;


  /* see if there is a ',' in the argument */
  if ((tptr=strstr(sptr,",")))
    {
      /* they specified a left,right combination */
      *tptr = '\0';
      *left = (BYTE) atoi(sptr);
      *right = (BYTE) atoi(tptr+1);
    }
  else
    *left = *right = (BYTE) atoi(sptr);

  return TRUE;
}




main(int argc, char **argv )
{
  BYTE   right, left, verbose;
  USHORT status;
  ULONG  datlen, parlen, action;
  int    c;

  /* Strategy - read in current settings, parse commandline for changes
   * to make, then make changes */
  verbose=TRUE;

  /* open mixer */
  status = DosOpen( "SBMIX$", &mixer_handle, &action, 0,
		   FILE_NORMAL, FILE_OPEN,
   OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYNONE | OPEN_FLAGS_WRITE_THROUGH,
			  NULL );

  if (status != 0)
    {
      printf("Error opening mixer device - be sure SBOS2.SYS is loaded.\n");
      printf("This program will not work with a regular SB.\n");
      exit (10);
    }

  /* read current levels */
  datlen=sizeof(struct sb_mixer_levels);
  parlen=0;
  status=DosDevIOCtl(mixer_handle, DSP_CAT, MIXER_IOCTL_READ_LEVELS,
                    NULL, 0, &parlen,
                    &sblevels, datlen, &datlen);
  if (status != 0)
    {
      printf("Error reading mixer levels, exiting.\n");
      exit (1);
    }

  /* now parse commandline */
  while ((c=getopt(argc, argv, "m:v:f:c:l:x:h?q"))!=EOF)
    {
      switch (c)
	{
	case 'm':
	case 'M':
	  /* optarg should point at volume levels */
	  if (getlevels(optarg, &left, &right))
	    {
	      sblevels.master.l = left;
	      sblevels.master.r = right;
	    }
	  break;
	case 'v':
	case 'V':
	  /* optarg should point at volume levels */
	  if (getlevels(optarg, &left, &right))
	    {
	      sblevels.voc.l = left;
	      sblevels.voc.r = right;
	    }
	  break;
	case 'f':
	case 'F':
	  /* optarg should point at volume levels */
	  if (getlevels(optarg, &left, &right))
	    {
	      sblevels.fm.l = left;
	      sblevels.fm.r = right;
	    }
	  break;
	case 'c':
	case 'C':
	  /* optarg should point at volume levels */
	  if (getlevels(optarg, &left, &right))
	    {
	      sblevels.cd.l = left;
	      sblevels.cd.r = right;
	    }
	  break;
	case 'x':
	case 'X':
	  /* optarg should point at volume levels */
	  if (getlevels(optarg, &left, &right))
	    sblevels.mic = left;
	  break;
	case 'q':
	case 'Q':
	  verbose = FALSE;
	  break;

	case 'H':
	case 'h':
	case '?':
	  puts(USAGE);
	  exit(0);	
	  break;
	default:
	  break;
	}
    }


  /* see if there were any options */
  if (optind==1)
    {
      puts(USAGE);
      exit(0);
    }

  /* now set mixer levels */
  status=DosDevIOCtl(mixer_handle, DSP_CAT, MIXER_IOCTL_SET_LEVELS,
                    NULL, 0, &parlen,
                    &sblevels, datlen, &datlen);
  if (status != 0)
    {
      printf("Error setting mixer levels, exiting\n");
      exit (1);
    }


  /* read  in new settings */
  datlen=sizeof(struct sb_mixer_levels);
  parlen=0;
  status=DosDevIOCtl(mixer_handle, DSP_CAT, MIXER_IOCTL_READ_LEVELS,
                    NULL, 0, &parlen,
                    &sblevels, datlen, &datlen);
  if (status != 0)
    {
      printf("Error reading mixer levels, exiting.\n");
      exit (1);
    }

  /* output settings */
  if (verbose)
    {
      printf("New settings:\n");
      printf("MASTER:\t%d\t%d\n",sblevels.master.l,sblevels.master.r);
      printf("VOICE :\t%d\t%d\n",sblevels.voc.l,sblevels.voc.r);
      printf("FM    :\t%d\t%d\n",sblevels.fm.l,sblevels.fm.r);
      printf("LINE  :\t%d\t%d\n",sblevels.line.l,sblevels.line.r);
      printf("CD    :\t%d\t%d\n",sblevels.cd.l, sblevels.cd.r);
      printf("MIC   :\t%d\n",sblevels.mic);
    }

}

