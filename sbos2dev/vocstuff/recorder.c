/* This program is not a good program, but it works for now */


#define INCL_DOSDEVIOCTL

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <os2.h>
#include <getopt.h>
#include "sblast_user.h"

unsigned char string1[]={'C','r','e','a','t','i','v','e',' ','V','o','i','c',
		       'e',' ','F','i','l','e',0x1a,0x1a,0,0x0a,1,0x29,0x11};

unsigned char sndbuf[10000];

#define MIN(x,y)  ((x)>(y)) ? (y) : (x)

#define USAGE \
"-r speed -b output_file"\
"\n"\
"output_file: Sound file name. Reqd\n"\
"speed:     Output rate (samples/sec) (valid range [4000-43478] on SBPro)\n"\
"                                                  [4000-22000] on SBREG)\n"\
"\n"\
"-b:    Raw output (No header). Default is .VOC output\n"


/* global variables used to figure out how user wanted sample to be */
time_t start_time, stop_time;
int readstopped;

static void handler( int sig )
{
  if (sig == SIGINT)
    {
      readstopped=TRUE;
      stop_time=time(NULL);
      printf("Recording stopped - waiting for buffer to empty\n");
      signal(sig, SIG_ACK);
    }

}




int main(int argc, char** argv)
{
   int   tmp;
   USHORT speed, utmp, bufsize;
   USHORT time_constant;
   HFILE  sndhandle;
   ULONG  datlen, parlen, status, action, numread;
   int   voc_file;
   long  length;
   long  desired_length;
   FILE  * outfile;
   char  * outfile_name;
   long  header_offset;
   int   first;


  int           opt, error_flag; /* GNU getopt(3) variables */
  extern int 	optind;
  extern char	*optarg;

  /* Default options */
  speed=8000;
  voc_file=TRUE;

  error_flag = 0;
  while ((opt = getopt (argc, argv,
			"r:R:M:m:bB")) != EOF)
    switch (opt)
      {
      case 'b':                 /* raw output */
      case 'B':
	 voc_file = FALSE;
	 break;
      case 'r':                 /* output speed */
      case 'R':
	utmp = atoi(optarg);
	if ((utmp>0)&&(utmp<43479))
	 {
	  speed=utmp;
	 }
	else
	 {
	   printf("Invalid speed specified, valid limits are 0-43478.\n");
	   exit(1);
	 }
	break;
      default:                  /* unrecognized option */
	error_flag = 1;
	break;
      }

  if (error_flag || !argv[optind])
    {
      fprintf(stderr, "Usage: recorder " USAGE);
      exit(1);
    }

/* get input filename */
   outfile_name = argv[optind];

/* open SB */
   status = DosOpen( "SBDSP$", &sndhandle, &action, 0,
                          FILE_NORMAL, FILE_OPEN,
   OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYNONE | OPEN_FLAGS_WRITE_THROUGH,
			  NULL );

   if (status)
     {
       printf("Error opening sound card.\n");
       exit(1);
     }

   /* set speed */
   datlen=2;
   parlen=0;
   status=DosDevIOCtl(sndhandle, DSP_CAT, DSP_IOCTL_SPEED,
			NULL, 0, &parlen,
			&speed, datlen, &datlen);
   printf("Using speed=%u\n",speed);


   /* set buffer size to be equal to 0.5 sec of input */
   datlen=2;
   parlen=0;
   bufsize = (speed/2);
   status=DosDevIOCtl(sndhandle, DSP_CAT, DSP_IOCTL_BUFSIZE,
		     NULL, 0, &parlen,
		     &bufsize, datlen, &datlen);
   


   /* ok, now lets send something to output */
   outfile = fopen(outfile_name,"wb");
   if (outfile==NULL) {
     printf("Cannot open output file. Aborting\n");
     exit(1);
   }


/* put a voc header on here if requested */
   if (voc_file)
   {
    /* write out header */
    fwrite(string1,1,sizeof(string1),outfile);

    /* now insert speed, length info */
    /* guess length for now */
    length = 0;

    /* mark start of sound data */
    fputc(1, outfile);

    /* length is 3 bytes, LSB first, store length + 2 (why ask why?) */
    /* store pos so we can go back later when we know length */
    header_offset= ftell(outfile);
    length += 2;
    fputc(length&0xff,outfile);
    fputc((length&0xff00)>>8,outfile);
    fputc((length&0xff0000)>>16,outfile);
    length -= 2;

    /* speed byte (SB) is given by
     *      speed < 22222 ->   SB = 256-(1000000/speed)
     *      speed > 22222 ->   SB = 65536-(256000000/speed)     */
     if (speed > 22222)
       time_constant = 65536L - (256000000L / (long)speed);
     else
       time_constant = 256 - (1000000/speed);
    
    fputc(time_constant & 0xff, outfile);
    fputc((time_constant & 0xff00)>>8, outfile);
  }


   /* now record  */
   printf("Hit return to start recording.\n");
   tmp=getchar();
   
   printf("Hit CNTL-C to end recording\n");
   length = 0;
   readstopped=FALSE;
   signal(SIGINT, handler);
   start_time=time(NULL);
   first=TRUE;
   do
     {
       DosRead(sndhandle, &sndbuf, 5000, &numread);
       if (readstopped)
	 {
	   if (first)
	     {
	       /* figure out how many samples we need */
	       desired_length=(long)(difftime(stop_time,start_time)*speed);
	       first=FALSE;
	     }
	   if (desired_length>length)
	     tmp=MIN(desired_length-length,numread);
	   else
	     tmp=0;
	   first=FALSE;
	 }
       else
	 tmp=numread;

       fwrite(sndbuf, 1, tmp, outfile);
       length += tmp;
     }	    
   while((desired_length>length)||(!readstopped));

   /* end file, then stick length in header */
   if (voc_file)
    {
      /* terminate file */
      fputc(0,outfile);

      /* seek back to where length is stored and write it */
      fseek(outfile, header_offset, SEEK_SET);

      /* length is 3 bytes, LSB first, store length + 2 (why ask why?) */
      length += 2;
      fputc(length&0xff,outfile);
      fputc((length&0xff00)>>8,outfile);
      fputc((length&0xff0000)>>16,outfile);
    }

   /* we're outa here */
   fclose(outfile);
   DosClose(sndhandle);
 }
