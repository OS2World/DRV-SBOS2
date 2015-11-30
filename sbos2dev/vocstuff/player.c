/* program to play VOC or raw sound files */

#include <stdio.h>
#include <os2.h>
#include <getopt.h>
#include "sblast_user.h"

unsigned char sndbuf[10000];
unsigned char vochdr[]={'C','r','e','a','t','i','v','e',' ','V','o','i','c',
		       'e',' ','F','i','l','e'};


#define USAGE \
"-r speed -s input_file"\
"\n"\
"input_file: Sound file name.\n"\
"speed:     Output rate (samples/sec) (valid range [4000-43478] on SBPro)\n"\
"                                                  [4000-22000] on SBREG)\n"\
"\n" \
"-s: Stereo output (SBPro only)\n"\
"-b: Read in RAW file (no .VOC HDR)\n"


int main(int argc, char** argv)
{
   FILE  * infile;
   char * infile_name;

   int speed_defined;
   int raw_mode;
   FLAG stereo;


   HFILE  sndhandle;
   USHORT speed, utmp, vspeed;
   ULONG ltmp, action, status, datlen, parlen, numwritten;
   int   tmp;

   


  int           opt, error_flag; /* GNU getopt(3) variables */
  extern int 	optind;
  extern char	*optarg;

  /* Default options */
  speed=8000;
  speed_defined=FALSE;
  stereo= FALSE;
  raw_mode = 0;

  error_flag = 0;
  while ((opt = getopt (argc, argv,
			"r:R:sSbB")) != EOF)
    switch (opt)
      {
      case 'r':                 /* output speed */
      case 'R':
	utmp = atoi(optarg);
	if ((utmp>0)&&(utmp<43479))
	 {
	  speed=utmp;
	  speed_defined=TRUE;
	 }
	else
	 {
	   printf("Invalid speed specified, valid limits are 0-43478.\n");
	   exit(1);
	 }
	break;
      case 's':                 /* Stereo output */
      case 'S':
	stereo = TRUE;
	break;
      case 'b':                 /* Raw mode output */
      case 'B':
	raw_mode = TRUE;
	break;
      default:                  /* unrecognized option */
	error_flag = 1;
	break;
      }

  if (error_flag || !argv[optind])
    {
      fprintf(stderr, "Usage: player " USAGE);
      exit(1);
    }

/* get input filename */
  infile_name = argv[optind];

   /* ok, see if sb exists */
   status = DosOpen( "SBDSP$", &sndhandle, &action, 0,
                          FILE_NORMAL, FILE_OPEN,
   OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYNONE | OPEN_FLAGS_WRITE_THROUGH,
			  NULL );

   if (status)
     {
       printf("Error opening sound card.\n");
       exit(1);
     }


   /* set stereo/mono */
   if (stereo) 
     {
       printf("Stereo output requested\n");
       datlen=1;
       parlen=0;
       status=DosDevIOCtl(sndhandle, DSP_CAT, DSP_IOCTL_STEREO,
			  NULL, 0, &parlen,
			  &stereo, datlen, &datlen);
       if (status)
	 {
	   printf("Error setting to stereo mode - is this a SB Pro?\n");
	   exit(1);
	 }
     }

   /* ok, now lets send something to output */
   infile = fopen(infile_name,"rb");
   if (infile == NULL )
     {
       printf("Input file not found, aborting...\n");
       exit(1);
     }

/* Lets see if it is a .VOC file */
  if (!raw_mode)
  {
   fread(sndbuf, 1, sizeof(vochdr), infile);
   if (strncmp(sndbuf, vochdr, sizeof(vochdr))==0)
     {
       printf("Input file identified as a .VOC file.\n");

       /* now read over some more header stuff which just identifies
	* the version number of the file - we'll assume its ok       */
	fseek(infile, 7L, SEEK_CUR);

	/* next byte should be a 1, indicating data follows */
	/* no support for silence or repeating type of blocks yet */
	if (getc(infile)!=1)
	 {
	   printf("This player doesnt understand .VOC file, aborting.\n");
	   exit(1);
	 }

	 /* next three bytes contain the length of the .VOC file */
	 /* we'll ignore this, just play till we reach EOF       */
	 fseek(infile, 3L, SEEK_CUR);

	 /* next two bytes either have the speed and compression type,
	  * if it is a low speed file, else it will contain speed only
	  * for high speed files */
	  utmp=getc(infile)|(getc(infile)<<8);

	 /* compression types only go up to a numeric type of 5 or so,
	  * and the lowest the MSB of the speed for high speed mode
	  * can be is D2, so if high byte of tmp is greater than 5, say,
	  * it should be safe to assume it is a high speed file. */
	  if (utmp > 0x0500)
	   {
	     vspeed=(unsigned int)(256000000L/(65536L-utmp));
	     if (!speed_defined) speed=vspeed;
	     printf("High speed file indentified, speed=%u.",vspeed);
	   }
	  else
	   {
	     vspeed=(unsigned int)(1000000L/(long)(256-(utmp&0xff)));
	     if (!speed_defined) speed=vspeed;
	     printf("Low speed file indentified, speed=%u.",speed);

	     /* see if file is compressed, we dont know how to handle that*/
	     if (utmp > 0xff)
	       {
		printf("Cant handle compressed files, aborting.\n");
		exit(1);
	       }
	   }
     } /* indentified as a .VOC file */
   } /* if (!raw_mode) */

   /* now play to end of file */
   /* set speed */
   datlen=2;
   parlen=0;
   status=DosDevIOCtl(sndhandle, DSP_CAT, DSP_IOCTL_SPEED,
		      NULL, 0, &parlen,
		      &speed, datlen, &datlen);
   printf("Setting speed to %u.\n",speed);

   do
    {
     tmp=fread(sndbuf, 1, 10000, infile);
     DosWrite(sndhandle, &sndbuf, tmp, &numwritten);
    }
   while(!feof(infile));

   fclose(infile);
   DosClose(sndhandle);
  }
