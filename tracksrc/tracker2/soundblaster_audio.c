/* soundblaster_audio.c */

/* MODIFIED BY Michael Fulbright (MSF) to work with os/2 device driver */
/* MSF (10-21-92) Now detects card type, no need for two versions of program */
/*                                                                    */
/* Now uses two threads - one to generate output and one to ferry it
 * over to the device driver as fast as it can handle it (which is
 * always slower than we can generate it).
*/

/* $Author: steve $
 * $Id: soundblaster_audio.c,v 1.1 1992/06/24 06:24:17 steve Exp steve $
 * $Revision: 1.1 $
 * $Log: soundblaster_audio.c,v $
 * Revision 1.1  1992/06/24  06:24:17  steve
 * Initial revision
 */

#include <stdio.h>

/* added by msf */
#define INCL_DOSMEMMGR
#include <os2.h>


/* this is so we allocation memory ok with DosAllocMem */
#define fALLOC      PAG_COMMIT | PAG_READ | PAG_WRITE

#include "sblast_user.h"
#include "defs.h"


static char *id = "$Id: soundblaster_audio.c,v 1.1 1992/06/24 06:24:17 steve Exp steve $";

/*unsigned char *buffer;*/		/* buffer for ready-to-play samples */
#define BUFFER_SIZE 200000
unsigned char buffer[BUFFER_SIZE];


/* MSF - going to implement a circular buffer */
static int headptr, tailptr, bufsize;


/* MSF - we're going to use OS/2 API calls, need a handle number instead*/
HFILE audio_handle;
HFILE mixer_handle;



/* ID number of output thread */
int threadid;

/* 256th of primary/secondary source for that side. */
static int primary, secondary;


/* indicate that task ending */
static int terminate=0;

/* indicates to flush_buffers() if SBDSP$ is open or not */
static int handle_valid=0;

/* indicates that flush_buffer is currently engaged in a write that */
/* it shouldnt be interrupted during */
static int doing_write=0;

/* indicates to flush_buffers() that it should clear out buffer */
static int empty_buf=0;

/* forward declarations */
void flush_buffer(ULONG param);


void
set_mix (int percent)
{
  percent *= 256;
  percent /= 100;
  primary = percent;
  secondary = 512 - percent;
}



void open_buffer(void)
{
  ULONG status;
  APIRET rc;

  /* allocate buffer - change to your liking. */
  bufsize = BUFFER_SIZE;
  rc = DosAllocMem((PVOID *) &buffer, 200000, fALLOC); 
  if (rc)
    {
      printf("Error allocating memory for buffer, exiting...\n");
      exit(10);
    }
  headptr=tailptr=0;
  
  if (buffer == NULL)
    {
      printf("Couldnt allocatate buffer, exiting\n");
      exit(1);
    }
#ifdef DEBUG
  else
    printf("Allocated buffer ok\n");
#endif

  /* start output thread */
  terminate = 0; /* make sure it doesnt quit */
/*  threadid = _beginthread(flush_buffer, NULL, 0x8000); */

  /* start thread with DosCreateThread call */
  DosCreateThread(&threadid, flush_buffer, 0, 0, 8192 );

  if (threadid==-1)
    {
      printf("error starting thread flush_buffer, exiting\n");
      exit(1);
    }

}

void close_buffer(void)
{
  terminate=1;
/*  free(buffer); */
}
  
int
open_audio (int frequency)
{
  USHORT status, freq;
  BYTE   flag;
  ULONG  datlen, parlen, action;
  int    issbpro;

  /* MSF - open SBDSP for output */
  status = DosOpen( "SBDSP$", &audio_handle, &action, 0,
		   FILE_NORMAL, FILE_OPEN,
   OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYNONE | OPEN_FLAGS_WRITE_THROUGH
   | OPEN_FLAGS_NOINHERIT | OPEN_FLAGS_NO_CACHE,
			  NULL );

  if (status != 0)
    perror ("Error opening audio device"),
      exit (10);


  /* now open mixer */
  status = DosOpen( "SBMIX$", &mixer_handle, &action, 0,
		   FILE_NORMAL, FILE_OPEN,
   OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYNONE | OPEN_FLAGS_WRITE_THROUGH
   | OPEN_FLAGS_NOINHERIT | OPEN_FLAGS_NO_CACHE,
			  NULL );

  /* if we cant open mixer, but we could open sbdsp$, means we are probably
   * running on an SB Pro */
  if (status != 0)
    {
      issbpro=FALSE;
      mixer_handle=0;
      pref.stereo=FALSE;
    }
  else
    issbpro=TRUE;

  /* initialize buffer */
  headptr=tailptr=0;
  
  /* let flush_buffer know writes to SBDSP now valid */
  handle_valid = 1;

  /*  turn stereo on if requested */
  if (issbpro)
    {
      datlen=1;
      parlen=0;
      flag=pref.stereo;
      status=DosDevIOCtl(mixer_handle, DSP_CAT, DSP_IOCTL_STEREO,
			 NULL, 0, &parlen,
			 &flag, 1, &datlen);
      if (status != 0)
	perror ("Error setting stereo/mono"),exit (1);
    }

  if (pref.stereo)
    frequency *= 2;		/* XXX Stereo takes twice the speed */

  if (frequency == 0)
    frequency = -1;		/* read current frequency from driver */

  /* set speed */
  datlen=2;
  parlen=0;
  freq = (USHORT) frequency;
  status=DosDevIOCtl(audio_handle, DSP_CAT, DSP_IOCTL_SPEED,
		     NULL, 0, &parlen,
		     &freq, 2, &datlen);
  frequency=freq;
  if (status!=0)
    perror ("Error setting frequency"), exit (1);

  if (pref.stereo)			/* tacky, I know.. */
    return (frequency / 2);
  else
    return (frequency);
}

void
output_samples (int left, int right)
{
  ULONG status, numtoread;
  static ULONG element_cnt=0;


  /* ok, loop till its ok to write (use 2 instead of 1 to be sure we
   * can fit a stereo sample. 
  */
  numtoread=0;
  while (((headptr+2)%bufsize)==tailptr)
    {
#ifdef DEBUG
      numtoread++;
      if (!(numtoread%10000))
	printf("In outputsamples giving up rest of time slice\n");
#endif
      DosSleep(0);
    }

  

  if (pref.stereo)
    {
      buffer[headptr] = (((left * primary + right * secondary) / 256)
			 + (1 << 15)) >> 8;
      headptr = (headptr++)%bufsize;
      buffer[headptr] = (((right * primary + left * secondary) / 256)
			 + (1 << 15)) >> 8;
      headptr = (headptr++)%bufsize;
    }
  else
    {
      buffer[headptr] = (left + right + (1 << 15)) >> 8;
      headptr = (headptr++)%bufsize;
#ifdef DEBUG
      if (!(headptr%25000)||(headptr==tailptr))
      printf("out -> headptr,tailptr=%d %d\n",headptr,tailptr);
#endif
    }


}


/* this runs as a separate thread, outputing stuff to sbos2 */
void
flush_buffer (ULONG param)
{
  ULONG numread, status, numtowrite;
  unsigned char * startptr;

  /* wait for things to get started */
  while(headptr==tailptr)
    {
      if (terminate) 
	break;
      else
	DosSleep(0); 
    }


  for(;terminate==0;)
    {
      if (empty_buf)
	{
	  headptr=tailptr;
	  empty_buf=FALSE;
	}

      /* see if there is anything to read  */
      while((headptr==tailptr) || (!handle_valid))
	{
	  if (terminate) 
	    break;
	  else
	    DosSleep(0); 
	}


      if (headptr>tailptr)
	numtowrite=headptr-tailptr;
      else if (tailptr>headptr)
	numtowrite=bufsize-tailptr;
      else
	numtowrite=0;

      /* dont write too much */
      numtowrite = MIN(64000,numtowrite);
      startptr = buffer+tailptr;
      if (numtowrite && handle_valid)
	{
	  doing_write=1;
	  status=DosWrite(audio_handle, startptr, numtowrite, &numread);
	  doing_write=0;
	  if (numread != numtowrite)
	    {
	      DosWrite(1,
	       "DosWrite didnt write all bytes in flush_buffer\n\r",
		       48, &status );
	      tailptr = (tailptr+numread)%bufsize;
	    }
	  else
	    {
	      tailptr = (tailptr+numread)%bufsize;
	    }
	}
    }

}



/* this routine is called to empty buffer at the end of a song */
void flush_out_buffer(void)
{
  ULONG status, datlen, parlen;

  /* this just waits till flush_buffer() finishes sending the last song */
#ifdef DEBUG
  printf("In flush_out_buffer waiting on internal buffer to empty\n");
  printf("Tailptr, headptr = %d %d \n",tailptr,headptr);
#endif
  while(headptr!=tailptr)
    DosSleep(0); 

  /* now tell device driver to flush out internal buffers */
  printf("Now sending DSP_IOCTL_FLUSH to SBDSP$ to clear DMA buffers.\n");
  parlen=0;
  datlen=0;
  status=DosDevIOCtl(audio_handle, DSP_CAT, DSP_IOCTL_FLUSH,
                    NULL, 0, &parlen,
                    NULL, 0, &datlen);
  if (status != 0)
    perror ("Error flushing DMA buffers"),exit (1);

}  


/* this function is called to immediately abort a playback */
void empty_buffers(void)
{
  ULONG status, datlen, parlen;

  /* this will cause flush_buffer() to stop sending data to SBDSP$ */
  printf("Notifying flush_buffers() to clear buffer.\n");
  empty_buf=TRUE;

  /* wait till flush_buffer() empties buffers */
  printf("Waiting on flush_buffers() to empty buffer.\n");
  while (headptr!=tailptr)
    DosSleep(0);

#ifdef THISISDONEINFLUSHOUTBUFFERS
  printf("Now sending DSP_IOCTL_FLUSH to SBDSP$ to clear DMA buffers.\n");
  /* now tell device driver to flush out internal buffers */
  parlen=0;
  datlen=0;
  status=DosDevIOCtl(audio_handle, DSP_CAT, DSP_IOCTL_FLUSH,
                    NULL, 0, &parlen,
                    NULL, 0, &datlen);
  if (status != 0)
    perror ("Error flushing DMA buffers"),exit (1);
#endif
}

/*
 * Closing the BSD SBlast sound device waits for all pending samples to play.
 * I think SysV aborts, so you might have to flush manually with ioctl()
 */
void
close_audio (void)
{

  /* let flush_buffers know we're exiting */
  handle_valid = 0;

  /* make make sure flush_buffers isnt doing anything important */
  while(doing_write);

  headptr=tailptr=0;

  /* now close driver, since flush_buffers is finished */
  if (mixer_handle)
    DosClose(mixer_handle);


#ifdef DEBUG
  printf("In close_audio, closing sbdsp\n");
#endif
  DosClose(audio_handle); 

#ifdef DEBUG
  printf("Back From DosClose\n");
  printf("Leaving close_audio, headptr,tailptr=%d %d\n",headptr,tailptr);
#endif
}
