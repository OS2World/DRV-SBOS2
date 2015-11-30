/* soundblaster_audio.c */

/* MODIFIED BY Michael Fulbright (MSF) to work with os/2 device driver */

/* $Author: steve $
 * $Id: soundblaster_audio.c,v 1.1 1992/06/24 06:24:17 steve Exp steve $
 * $Revision: 1.1 $
 * $Log: soundblaster_audio.c,v $
 * Revision 1.1  1992/06/24  06:24:17  steve
 * Initial revision
 */

#include <malloc.h>
#include <stdio.h>

/* added by msf */
#include <os2.h>

#include "sblast_user.h"
#include "defs.h"


static char *id = "$Id: soundblaster_audio.c,v 1.1 1992/06/24 06:24:17 steve Exp steve $";

unsigned char *buffer;		/* buffer for ready-to-play samples */
/*int index;  Can't call this index!  Conflicts with index(3) */
static int buf_index;

FILE *audio;			/* /dev/sb_dsp */

/* MSF - we're going to use OS/2 API calls, need a handle number instead*/
HFILE audio_handle;


/* 256th of primary/secondary source for that side. */
static int primary, secondary;

void
set_mix (int percent)
{
  percent *= 256;
  percent /= 100;
  primary = percent;
  secondary = 512 - percent;
}

int
open_audio (int frequency)
{
  USHORT status, freq;
  BYTE   flag;
  ULONG  datlen, parlen, action, temp;
  int    issbpro;
 
  /* MSF - open SBDSP for output */
  status = DosOpen( "SBDSP$", &audio_handle, &action, 0,
		   FILE_NORMAL, FILE_OPEN,
   OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYREADWRITE | OPEN_FLAGS_WRITE_THROUGH
   | OPEN_FLAGS_NOINHERIT | OPEN_FLAGS_NO_CACHE,
			  NULL );

  if (status != 0)
    perror ("Error opening audio device SBDSP$"),
      exit (10);

  /* see if we are on a SBREG or SBPRO */
  status = DosOpen( "SBMIX$", &temp, &action, 0,
		   FILE_NORMAL, FILE_OPEN,
   OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYNONE | OPEN_FLAGS_WRITE_THROUGH
   | OPEN_FLAGS_NOINHERIT | OPEN_FLAGS_NO_CACHE,
			  NULL );

  if (status !=0)
    {
      issbpro=FALSE;
      pref.stereo=FALSE;
    }
  else
    issbpro=TRUE;

  /*  turn stereo on if requested */
  if (issbpro)
    {
      datlen=1;
      parlen=0;
      flag=pref.stereo;
      status=DosDevIOCtl(audio_handle, DSP_CAT, DSP_IOCTL_STEREO,
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

  buffer = malloc (sizeof (SAMPLE) * frequency);	/* Stereo makes x2 */
  buf_index = 0;


  if (pref.stereo)			/* tacky, I know.. */
    return (frequency / 2);
  else
    return (frequency);
}

void
output_samples (int left, int right)
{
  if (pref.stereo)
    {
      buffer[buf_index++] = (((left * primary + right * secondary) / 256)
			 + (1 << 15)) >> 8;
      buffer[buf_index++] = (((right * primary + left * secondary) / 256)
			 + (1 << 15)) >> 8;
    }
  else
    {
      buffer[buf_index++] = (left + right + (1 << 15)) >> 8;
    }


}

void
flush_buffer (void)
{
  ULONG numread;


/*  if (buf_index>15000)
    printf("buf_index=%d\n",buf_index), exit(1);
*/
/*  printf("%d\n", buf_index); */

  DosWrite(audio_handle, buffer, buf_index, &numread);
  if (numread != buf_index)
    fprintf (stderr, "fwrite didn't write all the bytes?\n");
  buf_index = 0;

}



void flush_DMA_buffers()
{
  ULONG status, datlen, parlen;

  printf("Now sending DSP_IOCTL_FLUSH to SBDSP$ to clear DMA buffers.\n");
  /* now tell device driver to flush out internal buffers */
  parlen=0;
  datlen=0;
  status=DosDevIOCtl(audio_handle, DSP_CAT, DSP_IOCTL_FLUSH,
                    NULL, 0, &parlen,
                    NULL, 0, &datlen);
  if (status != 0)
    perror ("Error flushing DMA buffers"),exit (1);

}


/*
 * Closing the BSD SBlast sound device waits for all pending samples to play.
 * I think SysV aborts, so you might have to flush manually with ioctl()
 */
void
close_audio (void)
{
  /*fclose (audio);*/
  DosClose(audio_handle);

}
