/*======================================================================

MSF - This file is from the SBlast-BSD-1.4 release, the original file
      header is given below. Most of the comment are from the
      original file. I've added OS/2 specific comments.

MSF - I've added some features from SBlast-BSD-1.5, though not all.


   Device driver for the Creative Labs Sound Blaster card.
   [ This file is a part of SBlast-BSD-1.4 ]

   Steve Haehnichen <shaehnic@ucsd.edu>
 
   $Id: sb_driver.c,v 1.29 1992/06/13 01:46:43 steve Exp steve $

   Copyright (C) 1992 Steve Haehnichen.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 * $Log: sb_driver.c,v $
 * Revision 1.29  1992/06/13  01:46:43  steve
 * Released in SBlast-BSD-1.4

======================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "demo.h"
#include "sblast.h"
#include "sb_regs.h"		/* Register and command values */


/*
 * If this is defined, DSP ioctls will be allowed on any device.
 * This is a debugging thing, and should be disabled before final
 * installation.  (It allows me to send a DSP_RESET to the SB
 * while the DSP device is "busy" making horrible noises.)
 */

/* MSF - not used by code - currently it always allows ioctls */
/* #define 	ALWAYS_ALLOW_DSP_IOCTLS */

/*
 * The Mixer is an odd deal.  I think Mixer ioctls should be
 * allowed on any device.  It's very safe.
 * If all your users complain that other users are messing up the
 * mixer while they are playing, well, undef it then. :-)
 */

/* MSF - currently code always allows MIXER ioctls indep of this define */
#define 	ALWAYS_ALLOW_MIXER_IOCTLS

/*
 * This is the highest DSP speed that will use "Low-Speed" mode.
 * Anything greater than this will use the "High-Speed" mode instead.
 */
/* MSF - On a regular sound blaster (not PRO) this is the max speed, period. */
#define		MAX_LOW_SPEED	22222

/*
 * This is the speed the DSP will start at when you reboot the system.
 * If I can read the speed registers, then maybe I should just use
 * the current card setting.  Does it matter?
 */
#define		INITIAL_DSP_SPEED	11000

/*
 * SB Device minor numbers
 * I have copied the numbers used in Brian Smith's ISC Unix driver,
 * with the faint hope of future compatibility.
 * The listed device names are simply the ones that I am using.
 * Note that there is absolutely no CMS support in this driver.
 */

/* MSF - I only used the DSP and MIXER values, as I havent implemented the
 *       FM or MIDI driver. Volunteers?
 */
#define SB_CMS_NUM  	0	/* NOT USED */
#define SB_FM_NUM    	1	/* NOT USED */
#define SB_DSP_NUM    	2	/* sbdsp$   DSP DAC/ADC */
#define SB_MIDI_NUM   	3	/* NOT USED */
#define SB_MIXER_NUM  	4	/* sbmix$   Non-exclusive-use Mixer */
#define SB_BIGGEST_MINOR 4	/* Highest minor number used. */

/*
 * This is the "channel" and IRQ used for the FM timer functions.
 * Since I can't seem to get one to trigger yet,
 * I don't know what they should be.  (see fm_delay() comments)
 */

/* MSF - Ignored in current code, since I dont care about the FM stuff */
#define TIMER_CHANNEL		BOTH
#define FM_TIMER_IRQ		8


#define GOOD 1
#define FAIL 0
#define ON   1
#define OFF  0

/* These are used to indicate what the DSP last did,
   and can safely do again without preparation */
#define NONE_READY	0
#define READ_READY	1
#define WRITE_READY	2

/* Number of "cycles" in one second.  Shouldn't this be somewhere else? */
#define HZ 		1000

#define TIMEOUT		(10 * HZ) /* FM interrupt patience in clock ticks */

/* Semaphore return codes.  This tells the reason for the wakeup(). */
#define WOKEN_BY_INTERRUPT	1
#define WOKEN_BY_TIMEOUT	2

/* How many times should it cycle to wait for a DSP acknowledge? */
#define DSP_LOOP_MAX		1000

/* Interrupt-generating device units.  Just arbitrary numbers. */
#define DSP_UNIT		0
#define FM_TIMER_UNIT		1
#define MIXER_UNIT              2
#define NUM_UNITS		3 /* How many units above */

enum { PLAY, RECORD };		/* DSP sampling directions */

/* Handy macro from Brian Smith's/Lance Norskog's driver */
#define crosses_64k_boundary(a,b) ((((int)(a) & 0xffff) + (b)) > 0x10000)

/*
 * This is the DSP memory buffer size.  Choose wisely. :-)
 * I started with 21K buffers, and then later changed to 64K buffers.
 * This hogs more kernel memory, but gives fewer Pops and
 * needs servicing less often.  Adjust to taste.
 * Note that it must be an EVEN number if you want stereo to work.
 * (See dsp_find_buffers() for a better explanation
 * Note that the same amount of memory (192K) is hogged, regardless of
 * the buffer size you choose.  This makes it easier to allocate page-safe
 * buffers, but should probably be changed to attempt to pack more into
 * one 64K page.  Be really sure you understand what you are doing if
 * you change the size of memory_chunk[].
 * 64K-2 is the biggest buffer you can have.
 * Smaller buffer sizes make aborts and small reads more responsive.
 * For real-time stuff, perhaps ~10K would be better.
 */

/* MSF - I use the same strategy for finding DMA buffers as described in
 *       in the comment above. However - I allocate the buffers when the
 *       DSP driver is opened and release the memory when it is closed,
 *       whereas the original code allocated the buffer at initialization.
 */

/* Pointer to a 32 bit physical memory location of our DMA buffer(s) */
static _32bits near memory_chunk; /*  returned from AllocPhys */

/* GDT selectors used for 2 DMA bufs */
static unsigned near dma_selector1, dma_selector2;

/* This structure will be instantiated as a single global holder
   for all DSP-related variables and flags. */
struct sb_dsp_type
{
  unsigned int	speed;		/* DSP sampling rate */
  unsigned long	timeout;	/* Timeout for one DSP buffer */
  BYTE	compression;		/* Current DAC decompression mode */
  FLAG	hispeed;		/* 1 = High Speed DSP mode, 0 = low speed */
  FLAG  in_stereo;		/* 1 = currently in stereo, 0 = mono */
  BYTE	start_command;		/* current DSP start command */
  int	error;			/* Current error status on read/write */
  int	semaphore;		/* Silly place-holder int for dsp_dma_start */
  void  (*cont_xfer)(void);	/* Function to call to continue a DMA xfer */
  char	*buf[2];		/* Two pointers to mono-page buffers */
  long	phys_buf[2];		/* Physical addresses for dsp buffers */
  FLAG	full[2];		/* True when that buffer is full */
  unsigned int	used[2];	/* Buffer bytes used by read/write */
  BYTE  active;			/* The buffer currently engaged in DMA */
  BYTE  hi;			/* The buffer being filled/emptied by user */
  FLAG  speaker_on;             /* MSF - added to keep up with speaker stat */
  unsigned int last_xfer_size;  /* how many bytes we last xfer'd */
  unsigned int bufsize;         /* Size of each DMA buffer */
  int   overrun_errno;          /* error on sampling overflow (0=no error) */
  BYTE  state;                  /* Either READ_READY, WRITE_READY, or 0 */
};
struct sb_dsp_type near dsp;	/* 'dsp' structure used everywhere */

/* This will be used as a global holder for the current Sound Blaster
   general status values. */
struct sb_status_type
{
  FLAG	dsp_in_use;		/* DSP or MIDI open for reading or writing */
  FLAG  fm_in_use;		/* FM open for ioctls */
  FLAG  cms_in_use;		/* CMS open for ioctls */
  
  FLAG  alive;                  /* Card present? */
  unsigned int addr;		/* Sound Blaster card address */
  unsigned int irq;             /* Sound Blaster irq # */
  int   sbpro;                  /* TRUE if card is a SB Pro */
  int  *wake[NUM_UNITS];	/* What to wakeup on interrupt */
  long block_id[NUM_UNITS];     /* MSF - OS/2 block id */
};
struct sb_status_type near status; /* Global current status */

/*
 * Here is the screwey register ordering for the first operator
 * of each FM cell.  The second operators are all +3 from the first
 * operator of the same cell.
 */
const  char near fm_op_order[] =
{ 0x00, 0x01, 0x02, 0x08, 0x09, 0x0a, 0x10, 0x11, 0x12 };

/*
 * Forward declarations galore!
 */
int sb_probe (int address, int doprobe);
int sb_attach (int address, int irq, int issbpro);
void far sb_intr (void);
int sb_open (int);
int sb_close (int,void far *);
int sb_ioctl (int,ioctl_hdr *);
int sb_read ( rw_hdr *);
int sb_write ( rw_hdr *);
int sb_flush ( void far * );
static void sb_sendb (unsigned select_addr, BYTE reg,
		      unsigned data_addr, BYTE value);
static int dsp_reset (void);
static unsigned int dsp_open (void);
static int dsp_close (void far *);
static int dsp_set_speed (unsigned int *speed);
static int dsp_command (BYTE val);
static int dsp_ioctl (ioctl_hdr *);
static int dsp_set_voice (int on);
static int dsp_flush_dac (void far *);
static int dsp_set_compression (int mode);
static int dsp_set_stereo (FLAG on);
static int dsp_set_bufsize( unsigned int  *);
static int dsp_write (rw_hdr *);
static int dsp_read (rw_hdr *);
static int GrabGDTSel(void);
static int dsp_find_buffers (void);
static void dsp_dma_start (int dir);
static void far dsp_next_write (void);
static void far dsp_next_read (void);


/* MSF - I have ifdef'd out FM and MIDI support for now, but the code is
 *       still here so you can fiddle around. You probably want to get the
 *       original code for Sblast from alt.sources too.
 */
#ifdef FM_SUPPORT
static int fm_open (int flags);
static int fm_close (void);
static int fm_reset (void);
static int fm_send (int channel, unsigned char reg, unsigned char val);
static int fm_ioctl (int cmd, caddr_t arg);
static int fm_delay (int pause);
static int fm_play_note (struct sb_fm_note *n);
static int fm_set_params (struct sb_fm_params *p);
static int fm_write (struct uio *uio);
#endif


static void mixer_send (BYTE reg, BYTE val);
static BYTE mixer_read_reg (BYTE reg);
static int mixer_ioctl (ioctl_hdr *);
static int mixer_reset (void);
static int mixer_set_levels (struct sb_mixer_levels *l);
static int mixer_set_params (struct sb_mixer_params *p);
static int mixer_get_levels (struct sb_mixer_levels *l);
static int mixer_get_params (struct sb_mixer_params *params);

#ifdef MIDI_SUPPORT
static int midi_open (int flags);
static int midi_close ();
static int midi_read (struct uio *uio);
static int midi_write (struct uio *uio);
static int midi_ioctl (int cmd, caddr_t arg);
#endif

/* OS/2 driver related stuffs */
#define outb(x,y) out_port((x),(y))
#define inb(x) in_port((x))
#define MIN(x,y)  ((x)>(y)) ? (y) : (x)


#define ESUCCESS 0
#define EIO      1
#define ERANGE   2
#define EINVAL   3

void tenmicrosec(void);
void tenmicrosec( void )
{
  int i;
  unsigned int count, newcount, temp, elapsed;

/* New function will monitor timer 0 until 12 ticks (0.84 microsec per
tick) have passed, or we have looped 100 times (arbitrary, used to
prevent a lockup) */

/* MSF - Note this function will always do 100 loops IF you have reprogrammed
 *       timer 0 to count to a value less than 255 
 */

#define Timer0ReadCmd 0x0
#define Timer0Cntrl   0x43
#define Timer0Count   0x40

  /* read current timer 0 value */
  outb(Timer0Cntrl, Timer0ReadCmd);
  count=(unsigned int)inb(Timer0Count); /* keep LSB */
  temp =(unsigned int)inb(Timer0Count); /* trash MSB */

  
  for (elapsed=0,i=0; (i<100) && (elapsed<12);i++)
    {
      outb(Timer0Cntrl, Timer0ReadCmd);
      newcount=(unsigned int)inb(Timer0Count); /* keep LSB */
      temp =(unsigned int)inb(Timer0Count); /* trash MSB */
      
      if (newcount>count)
	elapsed += (newcount-count);
      else if (newcount<count)
	elapsed += 255-count+newcount;

      newcount=count;
    }
}


/* 
 * Probing returns 1 if the
 * card is detected.  Note that we are not using the "official" Adlib
 * of checking for the presence of the card because it's tacky and
 * takes too much mucking about.  We just attempt to reset the DSP and
 * assume that a DSP-READY signal means the card is there.  If you
 * have another card that fools this probe, I'd really like to hear
 * about it. :-) Future versions may query the DSP version number
 * instead.
 */

/* MODIFIED BY MSF - Pass it address of SB */
/* Returns 1 if SB found, 0 otherwise */
/* do_probe=FALSE indicates not to test for card (if they have a PAS16)*/
int
sb_probe (int address, int doprobe)
{
  status.addr = (unsigned int) address;

  if (doprobe)
    {
      if (dsp_reset () == GOOD)
	status.alive = 1;
      else
	status.alive = 0;
    }
  else
    status.alive = 1;

  return (status.alive);
}

/* This is called by the kernel if probe() is successful. */

/* MODIFIED BY MSF - accepts address, irq of SB */
char near attach_message[]={"Sound card detected.\r\n"};
char near infomsg[]={"Driver initialized with following configuration:\n\r"};
char near irq_message[]={"IRQ = "};
char near addr_message[]={"Port address = 0x"};
char near sbpromsg[]={"Card is a SB PRO.\n\r"};
char near sbregmsg[]={"Card is a SB REG.\n\r"};
char near mem_error[]={"Error allocating memory for DMA buffers, aborting.\r\n"};

char near gdt_error[]={"Error allocating 2 GDT selecttors.\n\r"};
char near tempstr[10];
int
sb_attach (int address, int irq, int issbpro, int quiet)
{
  int i, len, irqtmp;

  /* output a message */
  if (!quiet)
    {
/*      DosWrite(1, (void far *) attach_message, strlen(attach_message), &i); */
      DosWrite(1, (void far *) infomsg, strlen(infomsg), &i);
    }
  if (irq==9) 
    irqtmp=2;
  else
    irqtmp=irq;
  len = ItoA(irqtmp,tempstr);
  tempstr[len]='\r';
  tempstr[len+1]='\n';
  len += 2;
  if (!quiet)
    {
      DosWrite(1, (void far *) irq_message, 6, &i);
      DosWrite(1, (void far *) tempstr, len, &i);
    }
  if (address==0x220) i=220;
  else if (address==0x240) i=240;
  len = ItoA(i,tempstr);
  tempstr[0]='0';
  tempstr[len]='\r';
  tempstr[len+1]='\n';
  len += 2;

  if (!quiet)
    {
      DosWrite(1, (void far *) addr_message, 17, &i);
      DosWrite(1, (void far *) tempstr, len, &i);

      if (issbpro)
	DosWrite(1, (void far *) sbpromsg, strlen(sbpromsg), &i);
      else
	DosWrite(1, (void far *) sbregmsg, strlen(sbregmsg), &i);
    }

  /* initial settings */
  status.fm_in_use = 0;
  status.dsp_in_use = FALSE;
  status.addr = address;
  status.irq  = irq;
  status.sbpro = issbpro;

  /* allocate 2 GDT selectors for later use */
  if (GrabGDTSel())
    {
      if (!quiet)
	DosWrite(1, (void far *) gdt_error, strlen(gdt_error), &i);
      return(1);
    }

  /* MSF - probably not necessary */
  for (i = 0; i < NUM_UNITS; i++)
    status.wake[i] = NULL;

  /*
   * These are default startup settings.
   * I'm wondering if I should just leave the card in the same
   * speed/stereo state that it is in.  I decided to leave the mixer
   * alone, and like that better.  Ideas?
   */
  dsp.compression = PCM_8;
  dsp.speed = INITIAL_DSP_SPEED;

  /* following NOT to be done for regular SB */
  if (issbpro)
    dsp_set_stereo (FALSE);


  dsp.bufsize = DSP_MAX_BUFSIZE;

  return (0);
}

/*
 * This gets called anytime there is an interrupt on the SB IRQ.
 * Great effort is made to service DSP interrupts immediately
 * to keep the buffers flowing and pops small.  I wish the card
 * had a bigger internal buffer so we didn't have to worry.
 */ 
void far 
sb_intr (void)
{
  int unit=DSP_UNIT;/* MSF-interupt assumed to always be due to DSP transfer*/


  /*
   * If the DSP is doing a transfer, and the interrupt was on the
   * DSP unit (as opposed to, say, the FM unit), then hurry up and
   * call the function to continue the DSP transfer.
   */
   if (dsp.cont_xfer)
   (*dsp.cont_xfer)(); 

  /*
   * If this Unit is expecting an interrupt, then set the semaphore
   * and wake up the sleeper.  Otherwise, we probably weren't expecting
   * this interrupt, so just ignore it.  This also happens when two
   * interrupts are acked in a row, without sleeping in between.
   * Not really a problem, but there should be a clean way to fix it.
   */
  if (status.wake[unit] != NULL)
    {
      *status.wake[unit] = WOKEN_BY_INTERRUPT;

      /* unblock wait_for_intr */
      unblock(status.block_id[unit]);
      status.wake[unit]=NULL;
    }
  else
    {
     /* DPRINTF (("sb_intr(): An ignored interrupt!\n")); */
    }

  /* let kernel know we're done */
  EOI(status.irq);

  return;

}

/* currently only opens DSP, MIXER parts of chip */
int
sb_open (int dev)
{


  if (!status.alive)
    return (ERROR|DONE|GEN_FAIL);

  switch (dev)
    {
    case SB_DSP_NUM:
      return (dsp_open ());

#ifdef FM_SUPPORT
    case SB_FM_NUM:
      return (fm_open ());
#endif
#ifdef MIDI_SUPPORT
    case SB_MIDI_NUM:
      return (midi_open ());
#endif
    case SB_MIXER_NUM:
      return (DONE);
      
    case SB_CMS_NUM:		/* No CMS support (yet) */
    default:
      return (DONE|ERROR|GEN_FAIL);
    }
}

int
sb_close (dev,req_hdr)
int dev;
void far *req_hdr;
{
  switch (dev)
    {
    case SB_DSP_NUM:
      return (dsp_close (req_hdr));
#ifdef FM_SUPPORT
    case SB_FM_NUM:
      return (fm_close ());
#endif
#ifdef MIDI_SUPPORT
    case SB_MIDI_NUM:
      return (midi_close ());
#endif
    case SB_MIXER_NUM:
      /* always allow someone to open the mixer */
      return (DONE);

    case SB_CMS_NUM:
    default:
      return (ERROR|DONE|GEN_FAIL);
    }
}

/*
 * This routes all the ioctl calls to the right place.
 * In general, Mixer ioctls should be allowed on any of the devices,
 * because it makes things easier and is guaranteed hardware-safe.
 */

int
sb_ioctl (dev, req_hdr)
int dev;
ioctl_hdr *req_hdr;
{


  /* if this is a request for the DSP check to see if its been opened */
  if ((dev==SB_DSP_NUM)&&(!status.alive))
    return (DONE|ERROR|GEN_FAIL);

  /* see if category code is correct for SB driver */
  if (req_hdr->funct_cat != DSP_CAT)
    return (DONE|ERROR|GEN_FAIL);

  /* ok, lets see what they want us to do */
  switch (req_hdr->funct_cod)
    {
    case MIXER_IOCTL_RESET:
    case MIXER_IOCTL_SET_LEVELS:
    case MIXER_IOCTL_SET_PARAMS:
    case MIXER_IOCTL_READ_LEVELS:
    case MIXER_IOCTL_READ_PARAMS:
      if (status.sbpro)
	{
	  if(mixer_ioctl (req_hdr)!=ESUCCESS)
	    return(DONE|ERROR|GEN_FAIL);
	  else
	    return(DONE);
	}

    case DSP_IOCTL_FLUSH:
    case DSP_IOCTL_RESET:
    case DSP_IOCTL_SPEED:
    case DSP_IOCTL_STEREO:
    case DSP_IOCTL_BUFSIZE:
    case DSP_IOCTL_OVERRUN_ERRNO:
      if (dsp_ioctl (req_hdr)!=ESUCCESS)
	return(DONE|ERROR|GEN_FAIL);
      else
	return(DONE);

    default: 
      /* we got a bad code, so exit */
      return(DONE|ERROR|GEN_FAIL);
    }
}


/* Multiplexes a read to the different devices */
int
sb_read (req_hdr)
rw_hdr *req_hdr;
{
  int dev=SB_DSP_NUM;

  if (!status.alive)
    return (ERROR|DONE|GEN_FAIL);

  switch (dev)
    {
    case SB_DSP_NUM:
      return (dsp_read (req_hdr));

#ifdef MIDI_SUPPORT
    case SB_MIDI_NUM:
      return (midi_read (uio));
#endif


#ifdef FM_SUPPORT
    case SB_FM_NUM:		/* No reading from FM, right? */
      return (ENXIO);
#endif
    case SB_CMS_NUM:
    default:
      return (ERROR|DONE|GEN_FAIL);
    }
}

int
sb_write (req_hdr)
rw_hdr *req_hdr;
{
  int dev=SB_DSP_NUM;

  if (!status.alive)
    return (DONE|ERROR|GEN_FAIL);


  switch (dev)
    {
    case SB_DSP_NUM:
      return (dsp_write (req_hdr));

#ifdef FM_SUPPORT
    case SB_FM_NUM:
      return (fm_write (uio));
#endif


#ifdef MIDI_SUPPORT
    case SB_MIDI_NUM:
      return (midi_write (uio));
#endif

    case SB_CMS_NUM:
    default:
      return (DONE|ERROR|GEN_FAIL);
    }
}

/*
 * Send one byte to the named Sound Blaster register.
 * The SBDK recommends 3.3 microsecs after an address write,
 * and 23 after a data write.  What they don't tell you is that
 * you also can't READ from the ports too soon, or crash! (Same timing?)
 * Anyway, 10 usecs is as close as we can get, so..
 *
 * NOTE:  This function is wicked.  It ties up the entire machine for
 * over forty microseconds.  This is unacceptable, but I'm not sure how
 * to make it better.  Look for a re-write in future versions.
 * Does 30 microsecs merit a full timeout() proceedure?
 */
static void
sb_sendb (unsigned int select_addr, BYTE reg,
	  unsigned int data_addr, BYTE value)
{
  outb (select_addr, reg);
  tenmicrosec();
  outb (data_addr, value);
  tenmicrosec();
  tenmicrosec();
  tenmicrosec();
}

/*
 * This waits for an interrupt on the named Unit.
 * The 'patience' is how long to wait (in cycles) before a timeout
 *
 * Always have interrupts masked while calling! (like w/ splhigh())
 */
static int
wait_for_intr (unsigned long patience)
{
  int sem = 0;
  int result;
  int unit=DSP_UNIT;

  /* value of following not used, but needed to let sb_intr know it
     should unblock this part of driver */
  status.wake[unit] = &dsp.semaphore; /* maybe interrupt cant use stack var? */

  if (patience) {
    if (block((unsigned long)status.block_id[unit], (unsigned long)patience))
    {
      if (unit == DSP_UNIT)
	dsp.error = EIO;
      result = FAIL;
    }
  else
    {
      result = GOOD;
    }
  }

  return (result);
}


/*
 * DSP functions.
 */

static int
dsp_ioctl (req_hdr)
ioctl_hdr *req_hdr;
{
  _32bits user_buffer;

  /* point to user data */
  user_buffer = req_hdr->dbuffer;

  switch (req_hdr->funct_cod)
    {
    case DSP_IOCTL_RESET:
      if (dsp_reset () == GOOD)
	return (ESUCCESS);
      else
	return (EIO);
    case DSP_IOCTL_SPEED:
      if (verify_acc( user_buffer, 2, 0)==0)
          return (dsp_set_speed ((unsigned int *)user_buffer.fptr));
      else
	  return(EIO);
    case DSP_IOCTL_VOICE:
      if (verify_acc( user_buffer, 1, 0)==0)
	return (dsp_set_voice (*(FLAG*)user_buffer.fptr));
      else
	return(EIO);
    case DSP_IOCTL_FLUSH:
      return (dsp_flush_dac ((void far *) req_hdr));
    case DSP_IOCTL_COMPRESS:
      if (verify_acc( user_buffer, 1, 0)==0)
	return (dsp_set_compression (*(BYTE *) user_buffer.fptr));
      else
	return(EIO);
    case DSP_IOCTL_STEREO:
      if (verify_acc( user_buffer, 1, 0)==0)
	return (dsp_set_stereo (*(FLAG *) user_buffer.fptr));
      else
	return(EIO);
    case DSP_IOCTL_BUFSIZE:
      if (verify_acc( user_buffer, 2, 0)==0)
	return (dsp_set_bufsize ((unsigned int *) user_buffer.fptr));
      else
	return(EIO);
    case DSP_IOCTL_OVERRUN_ERRNO:
      if (verify_acc( user_buffer, 2, 0)==0)
	{
	  dsp.overrun_errno = (*(int *)user_buffer.fptr);
	  return (ESUCCESS);
	}
      else
	return(EIO);
    default:
      return (EINVAL);
    }
}


/*
 * Reset the DSP chip, and initialize all DSP variables back
 * to square one.  This can be done at any time to abort
 * a transfer and break out of locked modes. (Like MIDI UART mode!)
 * Note that resetting the DSP puts the speed back to 8196, but
 * it shouldn't matter because we set the speed in dsp_open.
 * Keep this in mind, though, if you use DSP_IOCTL_RESET from
 * inside a program.
 */
static int
dsp_reset (void)
{
  int i;

  dsp.used[0] = dsp.used[1] = 0; /* This is only for write; see dsp_read() */
  dsp.full[0] = dsp.full[1] = FALSE;
  dsp.hi = dsp.active = 0;
  dsp.state = NONE_READY;
  dsp.error = ESUCCESS;
  dsp.cont_xfer = NULL;
  dsp.last_xfer_size = 01;
  dsp_set_voice(OFF);   /* have speaker OFF by default */
  status.wake[DSP_UNIT] = NULL;

  /*
   * This is how you reset the DSP, according to the SBDK:
   * Send 0x01 to DSP_RESET (0x226) and wait for three microseconds.
   * Then send a 0x00 to the same port.
   * Poll until DSP_RDAVAIL's most significant bit is set, indicating
   * data ready, then read a byte from DSP_RDDATA.  It should be 0xAA.
   * Allow 100 microseconds for the reset.
   */
  tenmicrosec();		/* Lets things settle down. (necessary?) */
  outb (DSP_RESET, 0x01);
  tenmicrosec();
  outb (DSP_RESET, 0x00);

  dsp.error = EIO;
  for (i = DSP_LOOP_MAX; i; i--)
    {
      tenmicrosec();
      if ((inb (DSP_RDAVAIL) & DSP_DATA_AVAIL)
	  && ((inb (DSP_RDDATA) & 0xFF) == DSP_READY))
	{
	  dsp.error = ESUCCESS;
	  break;
	}
    }

  if (dsp.error != ESUCCESS)
    return (FAIL);
  else
    return (GOOD);
}



/* following grabs 2 GDT selectors for use when allocating DMA buffers */
static 
int  GrabGDTSel(void)
{
  _32bits tempptr;

  /* allocate GDT selector */
  tempptr.fptr=(void far *)&dma_selector1;
  if (get_gdt_slots(1,tempptr))
    return(1);

  /* allocate GDT selector for second DMA buffer*/
  tempptr.fptr=(void far *)&dma_selector2;
  if (get_gdt_slots(1,tempptr))
    {
      /* free 1st dma buf GDT */
      free_gdt(dma_selector1);
      return(1);
    }

  /* return, no errors */
  return(0);
}



/* 
 * This finds page-safe buffers for the DSP DMA to use.  A single DMA
 * transfer can never cross a 64K page boundary, so we have to get
 * aligned buffers for DMA.  The current method is wasteful, but
 * allows any buffer size up to the full 64K (which is nice).  We grab
 * 3 * 64K in the static global memory_chunk, and find the first 64K
 * alignment in it for the first buffer.  The second buffer starts 64K
 * later, at the next alignment.  Yeah, it's gross, but it's flexible.
 * I'm certainly open to ideas!  (Using cool kernel memory alloc is tricky
 * and not real portable.)
 */

/*  MSF - This routine calls the DevHelp routine AllocPhys to get enough
 * RAM to handle the necessary DMA buffers 
 * - returns 0 unless there was a problem */

static int
dsp_find_buffers (void)
{
  _32bits startdmaptr,tempptr;
  long l;

  /* get the memory */
  memory_chunk = alloc_big_mem( 3*65536L );
  if (memory_chunk.phys==0)
    return(1);

  /* figure out where 64k boundary is */
  l=(0x10000-(memory_chunk.phys-(memory_chunk.phys & (0xFFFF0000))));
  dsp.phys_buf[0] = memory_chunk.phys + l;
  dsp.phys_buf[1] = dsp.phys_buf[0]+0x10000;

  /* Map the GDT selector to physical address of first DMA buffer */
  tempptr.phys = dsp.phys_buf[0];
  startdmaptr=phys_to_gdt(tempptr, 0xffff, dma_selector1);
  if (startdmaptr.phys==0L)
    {
      /* Now free physical memory allocated */
      free_mem(memory_chunk);
      return(1);
    }
  else
    dsp.buf[0] = (char far *) startdmaptr.fptr;

  tempptr.phys = dsp.phys_buf[1];
  startdmaptr=phys_to_gdt(tempptr, 0xffff, dma_selector2);
  if (startdmaptr.phys==0L)
    {
      /* Free physical memory allocated */
      free_mem(memory_chunk);
      return(1);
    }
  else
    dsp.buf[1] = (char far *) startdmaptr.fptr;

  /* return, no errors */
  return(0);
}

/*
 * Start a DMA transfer to/from the DSP.
 * This one needs more explaining than I would like to put in comments,
 * so look at the accompanying documentation, which, of course, hasn't
 * been written yet. :-)
 *
 * This function takes one argument, 'dir' which is either PLAY or
 * RECORD, and starts a DMA transfer of as many bytes as are in
 * dsp.buf[dsp.active].  This allows for partial-buffers.
 *
 * Always call this with interrupts masked.
 */
static void
dsp_dma_start (int dir)
{
  int count;

  count = dsp.used[dsp.active] - 1;

  /* Prepare the DMAC.  See sb_regs for defs and more info. */
  outb (DMA_MASK_REG, DMA_MASK);
  outb (DMA_CLEAR, 0);
  outb (DMA_MODE, (dir == RECORD) ? DMA_MODE_READ : DMA_MODE_WRITE);
  outb (DMA_PAGE, (BYTE)((dsp.phys_buf[dsp.active] & 0xff0000) >> 16));/*Page*/
  outb (DMA_ADDRESS, (BYTE)(dsp.phys_buf[dsp.active] & 0x00ff));
                                                         /* LSB of address */
  outb (DMA_ADDRESS, (BYTE)((dsp.phys_buf[dsp.active] & 0xff00) >> 8));
  outb (DMA_COUNT, (BYTE)(count & 0x00ff));
  outb (DMA_COUNT, (BYTE)((count & 0xff00) >> 8));
  outb (DMA_MASK_REG, DMA_UNMASK);

  /*
   * The DMAC is ready, now send the commands to the DSP.
   * Notice that there are two entirely different operations for
   * Low and High speed DSP.  With HS, You only have to send the
   * byte-count when it changes, and that requires an extra command
   * (Checking if it's a new size is quicker than always sending it.)
   */
  if (dsp.hispeed)
    {
      if (count != dsp.last_xfer_size)
	{
	  dsp_command (HIGH_SPEED_SIZE);
	  dsp_command ((BYTE)(count & 0x00ff));
	  dsp_command ((BYTE)((count & 0xff00) >> 8));
	}
      dsp_command (dsp.start_command); /* GO! */
    }
  else				/* Low Speed transfer */
    {
      dsp_command (dsp.start_command);
      dsp_command ((BYTE)count & 0x00ff);
      dsp_command ((BYTE)((count & 0xff00) >> 8)); /* GO! */
    }

  /* This sets up the function to call at the next interrupt: */
  dsp.cont_xfer = (dir == RECORD) ? dsp_next_read : dsp_next_write;
}

/*
 * This is basically the interrupt handler for Playback DMA interrupts.
 * Our first priority is to get the other buffer playing, and worry
 * about the rest after.
 */
void far
dsp_next_write (void)
{

  inb (DSP_RDAVAIL);		/* ack interrupt */

  dsp.full[dsp.active] = FALSE; /* Just-played buffer is not full */
  dsp.active ^= 1;
  if (dsp.full[dsp.active])
    {
      dsp_dma_start (PLAY);
    }
  else
    {
      dsp.cont_xfer = NULL;
    }
}

/*
 * This is similar to dsp_next_write(), but for Sampling instead of playback.
 */
void far
dsp_next_read (void)
{

  inb (DSP_RDAVAIL);		/* ack interrupt */
  /* The active buffer is currently full of samples */
  dsp.full[dsp.active] = TRUE;
  dsp.used[dsp.active] = 0;

  /* Flop to the next buffer and fill it up too, unless it's already full */
  dsp.active ^= 1;
  if (dsp.full[dsp.active])
    {
      /*
       * An overrun occurs when we fill up two buffers faster than the
       * user is reading them.  Lossage has to occur.  This may or
       * may not be bad.  For recording, it's bad.  For real-time
       * FFTs and such, it's not a real big deal.
       */
      dsp.error = dsp.overrun_errno;
      dsp.cont_xfer = NULL;
    }
  else
    dsp_dma_start (RECORD);

}

/*
 * This is the main recording function.  Program flow is tricky with
 * all the double-buffering and interrupts, but such are drivers.
 * To summarize, it starts filling the first buffer when the user
 * requests the first read().  (Filling on the open() call would be silly.)
 * When one buffer is done filling, we fill the other one while copying
 * the fresh data to the user.
 * If the user doesn't read fast enough for that DSP speed, we have an
 * overrun.  See above concerning ERROR_ON_OVERRUN.
 */
static int
dsp_read (req_hdr)
rw_hdr *req_hdr;
{
  unsigned bytecount, hunk_size;
  _32bits user_buffer, tempptr;

  /* convert physical address of user buffer to virtual */
  tempptr.phys = req_hdr->xfer_address;
  user_buffer = phys_to_ldt(tempptr,req_hdr->count);


  /* see if everything is ready for sampling */
  if (dsp.state != READ_READY)
    {
      /* be sure speaker is off or we wont get anything */
      if (dsp.speaker_on) dsp_set_voice(OFF);
      dsp_set_speed(&dsp.speed);
      dsp.state = READ_READY;
      dsp.start_command =(FLAG) (dsp.hispeed ? HS_ADC_8 : ADC_8);

      dsp.used[0] = dsp.used[1] = dsp.bufsize;	/* Start with both empty */
      dsp.full[0] = dsp.full[1] = FALSE;
      dsp.hi = dsp.active = 0;
      dsp.error = ESUCCESS;
      dsp.last_xfer_size = -1;
      dsp_dma_start (RECORD);
    }

  /* read what they want */
  bytecount = 0;
  
  /* While there is still data to read, and data in this chunk.. */
  while ( bytecount < req_hdr->count)
    {
      if (dsp.error != ESUCCESS)
        {
	  free_virt(user_buffer._segadr.segment);
	  return (ERROR|DONE|GEN_FAIL);
	}
      
      while (dsp.full[dsp.hi] == FALSE)
	{
	  status.block_id[DSP_UNIT]=(long) req_hdr;
	  if (wait_for_intr (dsp.timeout)==FAIL)
	    {
	      free_virt(user_buffer._segadr.segment);
	      return(ERROR|DONE|GEN_FAIL);
	    }
	}
      
      /* Now we give a piece of the buffer to the user */
      hunk_size = MIN((unsigned int)(req_hdr->count - bytecount),
		      (unsigned int)(dsp.bufsize - dsp.used[dsp.hi]));


/* memove is a MSC library routine to move memory around, calling method is
 *
 *     memmove( dptr, sptr, size )
 *
 * where dptr points to destination, sptr to source and size is # bytes
 */
      memmove( (unsigned char *)user_buffer.fptr+bytecount, 
	       dsp.buf[dsp.hi]+dsp.used[dsp.hi], 
	       hunk_size);
      dsp.used[dsp.hi] += hunk_size;
      
      if (dsp.used[dsp.hi] == dsp.bufsize)
	{
	  dsp.full[dsp.hi] = FALSE;
	  dsp.hi ^= 1;
	  dsp.used[dsp.hi] = 0;
	  if (dsp.cont_xfer == NULL) /* we had an overrun recently */
	    dsp_dma_start(RECORD);
	}
      bytecount += hunk_size;
    }			/* While there are bytes left in chunk */
 
  free_virt(user_buffer._segadr.segment);
  return (DONE);
}

/* 
 * Main function for DSP sampling.
 * No such thing as an overrun here, but if the user writes too
 * slowly, we have to restart the DSP buffer ping-pong manually.
 * There will then be gaps, of course.
 */
static int
dsp_write (req_hdr)
rw_hdr *req_hdr;
{
  unsigned int hunk_size;
  unsigned int bytecount;
  _32bits user_buffer, tempptr;

  /* convert physical address of user buffer to virtual */
  tempptr.phys = req_hdr->xfer_address;
  user_buffer = phys_to_ldt(tempptr,req_hdr->count);

  if (dsp.state != WRITE_READY)
    {
      dsp_reset();

      /* be sure speaker is on or we wont get anything */
      if (!dsp.speaker_on) dsp_set_voice(ON);

      dsp.state=WRITE_READY;
      dsp_set_speed(&dsp.speed);

      /*
       * If this is the first write() operation for this open(),
       * then figure out the DSP command to use.
       * Have to check if it is High Speed, or one of the compressed modes.
       * If this is the first block of a Compressed sound file,
       * then we have to set the "Reference Bit" in the dsp.command for the
       * first block transfer.
       */
      
      if (dsp.hispeed)
	dsp.start_command = HS_DAC_8;
      else
	{
	  dsp.start_command = dsp.compression;
	  if (dsp.compression == ADPCM_4
	      || dsp.compression == ADPCM_2_6
	      || dsp.compression == ADPCM_2)
	    dsp.start_command |= 1;
	}
    }

  bytecount = 0;

  /* While there is still data to write, and data in this chunk.. */
  while ( bytecount < req_hdr->count )
	{
	  if (dsp.error != ESUCCESS) /* Shouldn't happen */
	    {
	      free_virt(user_buffer._segadr.segment);
	      return (ERROR|DONE|GEN_FAIL);
	    }

	  hunk_size = MIN((unsigned int)(req_hdr->count - bytecount),
			  (unsigned int)(dsp.bufsize - dsp.used[dsp.hi]));
	  memmove(dsp.buf[dsp.hi]+dsp.used[dsp.hi],
		  (unsigned char *)user_buffer.fptr+bytecount,
		  hunk_size);

	  dsp.used[dsp.hi] += hunk_size;

	  if (dsp.used[dsp.hi] == dsp.bufsize)
	    {
	      dsp.full[dsp.hi] = TRUE;

	      /*
	       * This is true when there are no DMA's currently
	       * active.  This is either the first block, or the
	       * user is slow about writing.  Start the chain reaction.
	       */
	      if (dsp.cont_xfer == NULL)
		{
		  dsp.active = dsp.hi;
		  dsp_dma_start (PLAY);
		  if (!dsp.hispeed)
		    dsp.start_command &= ~1; /* Clear reference bit */
/* NOT SURE HERE*/ /* status.wake[DSP_UNIT] = &dsp.semaphore;*/
		}
	      
	      /* If the OTHER buffer is playing, wait for it to finish. */
	      if (dsp.active == dsp.hi ^ 1)
		{
		  status.block_id[DSP_UNIT]=(long)req_hdr;

		  if (wait_for_intr (dsp.timeout)==FAIL)
		    {
		      free_virt(user_buffer._segadr.segment);
		      return(ERROR|DONE|GEN_FAIL);
		    }
		}
	      dsp.hi ^= 1;	/* Switch to other buffer */
	      dsp.used[dsp.hi] = 0; /* Mark it as empty */
	    }			/* if filled hi buffer */
	  bytecount += hunk_size;
	}			/* While there are bytes left in chunk */

  /* free up user pointer */
  free_virt(user_buffer._segadr.segment);
  
  return (DONE);
}


/* a little procedure to call from strategy that is used to flush
 * output buffer 
*/
int sb_flush(req_hdr)
void far *req_hdr;
{
  if (dsp_flush_dac(req_hdr)!=ESUCCESS)
    return(DONE|ERROR|GEN_FAIL);
  else
    return(DONE);
}

/*
 * Play any bytes in the last waiting write() buffer and wait
 * for all buffers to finish transferring.
 * An even number of bytes is forced to keep the stereo channels
 * straight.  I don't think you'll miss one sample.
 */
static int
dsp_flush_dac (req_hdr)
void far * req_hdr;
{
  
  if (dsp.used[dsp.hi] != 0)
    {
      if (dsp.in_stereo)
	dsp.used[dsp.hi] &= ~1;	/* Have to have even number of bytes. */
      dsp.full[dsp.hi] = TRUE; 
      if (dsp.cont_xfer == NULL)
	{
	  dsp.active = dsp.hi;
	  dsp_dma_start (PLAY);
	}
    }

  /* Now wait for any transfers to finish up. */
  while (dsp.cont_xfer)
    {
      status.block_id[DSP_UNIT]=(long)req_hdr;
      if (wait_for_intr (dsp.timeout)==FAIL)
	  return(EIO);

    }
  dsp.used[0] = dsp.used[1] = 0;
  return (ESUCCESS);
}


static unsigned int
dsp_open (void)
{
  int i;

  if (status.dsp_in_use)
    return (ERROR|DONE|GEN_FAIL);

  /* try to grab IRQ, memory for DMA buffers */
  i=SetIRQ(status.irq, (void near *) sb_intr );
  if (i)
    return (ERROR|DONE|GEN_FAIL);

  /* now try to find ram */
  if (dsp_find_buffers())
    return (ERROR|DONE|GEN_FAIL);

  /* everything ok, so continue */
  status.dsp_in_use = TRUE;
  dsp_reset ();			/* Resets card and inits variables */
  dsp.state = NONE_READY;
  dsp.overrun_errno = EIO;

  return (DONE);
}


static int
dsp_close (req_hdr)
void far *req_hdr;
{
  if (status.dsp_in_use)
    {
      if (dsp.state==WRITE_READY)
	dsp_flush_dac( (void far *) req_hdr);
      dsp_reset ();
      status.dsp_in_use = FALSE;
      /* now release memory and IRQ */
      free_mem(memory_chunk);
      if (UnSetIRQ(status.irq))
	return(DONE|ERROR|GEN_FAIL);
      return (DONE);
    }
  else
    return (DONE);		/* Does this ever happen? */
}


/*
 * Set the playback/recording speed of the DSP.
 * This takes a pointer to an integer between DSP_MIN_SPEED
 * and DSP_MAX_SPEED and changes that value to the actual speed
 * you got. (Since the speed is so darn granular.)
 * This also sets the dsp.hispeed flag appropriately.
 * Note that Hi-Speed and compression are mutually exclusive!
 * I also don't check all the different range limits that
 * compression imposes.  Supposedly, the DSP can't play compressed
 * data as fast, but that's your problem.  It will just run slower.
 * Hmmm.. that could cause interrupt timeouts, I suppose.
 */
static int
dsp_set_speed (unsigned int *speed)
{
  BYTE time_constant;

  if (*speed == -1)
    {
      *speed = dsp.speed;
      return (ESUCCESS);
    }
  
  if (*speed < DSP_MIN_SPEED)
    {
      return (EINVAL);
    }


  /* check that speed is below upper allowable speed */
  if (status.sbpro)
    {
      if (*speed > DSP_MAX_SPEED_PRO)
	return (EINVAL);
    }
  else
    {
      if (*speed > DSP_MAX_SPEED_REG)
	return(EINVAL);
    }


  if (*speed > MAX_LOW_SPEED)
    {
      if (dsp.compression != PCM_8)
	return (EINVAL);
      time_constant = (BYTE) ((65536L - (256000000L / (long)*speed)) >> 8);
      dsp.speed = (unsigned int) (256000000L / (65536L - ((long)time_constant << 8)));
      dsp.hispeed = TRUE;
    }
  else
    {
      time_constant = (BYTE) (256 - (1000000 / *speed));
      dsp.speed = (unsigned int)(1000000L / (256 - time_constant));
      dsp.hispeed = FALSE;
    }

  /* Here is where we actually set the card's speed */
  if (dsp_command (SET_TIME_CONSTANT) == FAIL
      || dsp_command (time_constant) == FAIL)
    return (EIO);
  /*
   * Replace speed with the speed we actually got.
   * Set the DSP timeout to be twice as long as a full
   * buffer should take.
   */
  *speed = dsp.speed;
 dsp.timeout = (unsigned long)(((long)dsp.bufsize * (long)HZ * 2L) / (long)dsp.speed); 
  return (ESUCCESS);
}


/*
 * Turn the DSP output speaker on and off.
 * Argument of zero turns it off, on otherwise
 */
static int
dsp_set_voice (int on)
{
  if (dsp_command ((BYTE)(on ? SPEAKER_ON : SPEAKER_OFF)) == GOOD)
    {
      dsp.speaker_on = (FLAG)(on ?  TRUE : FALSE);
      return (ESUCCESS);
    }
  else
    return (EIO);
}

/*
 * Set the DAC hardware decompression mode.
 * No compression allowed for Hi-Speed mode, of course.
 */
static int
dsp_set_compression (int mode)
{
  switch (mode)
    {
    case ADPCM_4:
    case ADPCM_2_6:
    case ADPCM_2:
      if (dsp.hispeed)
	return (EINVAL);      /* Fall through.. */
    case PCM_8:
      dsp.compression = (BYTE)mode;
      return (ESUCCESS);
      
    default:
      return (EINVAL);
    }
}

/*
 * Send a command byte to the DSP port.
 * First poll the DSP_STATUS port until the BUSY bit clears,
 * then send the byte to the DSP_COMMAND port.
 */
static int
dsp_command (BYTE val)
{
  int i;

  for (i = DSP_LOOP_MAX; i; i--)
    {
      if ((inb (DSP_STATUS) & DSP_BUSY) == 0)
	{
	  outb(DSP_COMMAND, val);
	  return (GOOD);
	}
      tenmicrosec ();
    }
  return (FAIL);
}

/*
 * This turns stereo playback/recording on and off.
 * For playback, it seems to only be a bit in the mixer.
 * I don't know the secret to stereo sampling, so this may
 * need reworking.  If YOU know how to sample in stereo, send Email!
 * Maybe this should be a Mixer parameter, but I really don't think so.
 * It's a DSP Thing, isn't it?  Hmm..
 */
static int
dsp_set_stereo (FLAG select)
{
  dsp.in_stereo = (FLAG)(!!select);
  /* Have to preserve DNFI bit from OUT_FILTER */
  mixer_send (CHANNELS, ((mixer_read_reg (OUT_FILTER) & ~STEREO_DAC)
			 | (dsp.in_stereo ? STEREO_DAC : MONO_DAC)));
  return (ESUCCESS);
}



/*
 * Sets the DSP buffer size.
 * Smaller sizes make the DSP more responsive,
 * but require more CPU a dsp.bufsize;
 * Note that the size must be an even value for stereo to work.
 * We just reject odd sizes.
 * -1 is magic, it is replaced with the current buffer size .
 */
static int
dsp_set_bufsize( unsigned int *newsize )
{
  if ((*(int *)newsize)==-1)
    {
      *newsize = dsp.bufsize;
      return (ESUCCESS);
    }

  if (*newsize < DSP_MIN_BUFSIZE || *newsize > DSP_MAX_BUFSIZE
      || *newsize & 1)
      {
      return (EINVAL);
      }

  dsp.bufsize = *newsize;

  /* compute new timeout */
 dsp.timeout = (unsigned long)(((long)dsp.bufsize * (long)HZ * 2L) / (long)dsp.speed); 

  return (ESUCCESS);
}





#ifdef FM_SUPPORT
/*
 * FM support functions
 */
static int
fm_ioctl (int cmd, caddr_t arg)
{
  switch (cmd)
    {
    case FM_IOCTL_RESET:
      return (fm_reset ());
    case FM_IOCTL_PLAY_NOTE:
      return (fm_play_note ((struct sb_fm_note*) arg));
    case FM_IOCTL_SET_PARAMS:
      return (fm_set_params ((struct sb_fm_params*) arg));
    default:
      return (EINVAL);
    }
}

static int
fm_open (int flags)
{
  if (status.fm_in_use)
    return (EBUSY);

  fm_reset();
  status.fm_in_use = 1;
  return (ESUCCESS);
}

static int
fm_close (void)
{
  if (status.fm_in_use)
    {
      status.fm_in_use = 0;
      return (fm_reset ());
    }
  else
    return (ESRCH);
}

/*
 * This really is the only way to reset the FM chips.
 * Just write zeros everywhere.
 */
static int
fm_reset (void)
{
  int reg;
  for (reg = 1; reg <= 0xf5; reg++)
    fm_send (BOTH, reg, 0);

  return (ESUCCESS);;
}

/*
 * Send one byte to the FM chips.  'channel' specifies which
 * of the two SB Pro FM chip-sets to use.
 * SB (non-Pro) and Adlib owners should set both the Left and
 * Right addresses to the same (and only) FM set.
 * See sb_regs.h for more info.
 */
static int
fm_send (int channel, unsigned char reg, unsigned char val)
{
#ifdef FULL_DEBUG
  printf ("(%d) %02x: %02x\n", channel, reg, val);
#endif
  switch (channel)
    {
    case LEFT:
      sb_sendb (FM_ADDR_L, reg, FM_DATA_L, val);
      break;
    case RIGHT:
      sb_sendb (FM_ADDR_R, reg, FM_DATA_R, val);
      break;
    case BOTH:
      sb_sendb (FM_ADDR_B, reg, FM_DATA_B, val);
      break;
    default:
      printf ("sb: Hey, fm_send to which channel?!\n");
      return (FAIL);
    }
  return (GOOD);
}

/*
 * Play one FM note, as defined by the sb_fm_note structure.
 * See sblast.h for info on the fields.
 * Now, these functions don't do any validation of the parameters,
 * but it just uses the lowest bits, so you can't hurt anything
 * with garbage.  Checking all those fields would take a while.
 */
static int
fm_play_note (struct sb_fm_note *n)
{
  int op, voice, ch;
  
  op = n->operator;		/* Only operator 0 or 1 allowed */
  if (op & ~1)
    return (EINVAL);

  voice = n->voice;		/* Voices numbered 0 through 8 */
  if (voice < 0 || voice > 8)
    return (EINVAL);

  ch = n->channel;		/* LEFT, RIGHT, or BOTH */
  if (ch != LEFT && ch != RIGHT && ch != BOTH)
    return (EINVAL);

  /*
   * See sb_regs.h for a detailed explanation of each of these
   * registers and their bit fields.  Here it is, packed dense.
   */
  fm_send (ch, MODULATION(voice, op),
           F(n->am) << 7
           | F(n->vibrato) << 6
           | F(n->do_sustain) << 5
           | F(n->kbd_scale) << 4
           | B4(n->harmonic));
  fm_send (ch, LEVEL(voice, op),
           B2(n->scale_level) << 6
           | B6(~n->volume));
  fm_send (ch, ENV_AD(voice, op),
           B4(n->attack) << 4
           | B4(n->decay));
  fm_send (ch, ENV_SR(voice, op),
           B4(~n->sustain) << 4 /* Make sustain level intuitive */
           | B4(n->release));
  fm_send (ch, FNUM_LO(voice), B8(n->fnum));
  fm_send (ch, FEED_ALG(voice),
           B3(n->feedback) << 1
           | F(n->indep_op));
  fm_send (ch, WAVEFORM(voice, op), B2(n->waveform));
  fm_send (ch, OCT_SET(voice),
           F(n->key_on) << 5
           | B3(n->octave) << 2
           | B2(n->fnum >> 8));
  return (ESUCCESS);
}

static int
fm_set_params (struct sb_fm_params *p)
{
  if (p->channel != LEFT && p->channel != RIGHT && p->channel != BOTH)
    return (EINVAL);
  
  fm_send (p->channel, DEPTHS,
           F(p->am_depth) << 7
           | F(p->vib_depth) << 6
           | F(p->rhythm) << 5
           | F(p->bass) << 4
           | F(p->snare) << 3
           | F(p->tomtom) << 2
           | F(p->cymbal) << 1
           | F(p->hihat));
  fm_send (p->channel, WAVE_CTL, F(p->wave_ctl) << 5);
  fm_send (p->channel, CSM_KEYSPL, F(p->speech) << 7
	   | F(p->kbd_split) << 6);
  return (ESUCCESS);
}

/*
 * This is to write a stream of timed FM notes and parameters
 * to the FM device.  I'm not sure how useful it will be, since
 * it's totally non-standard.  Most FM should stick to the ioctls.
 * See the docs for an explanation of the expected data format.
 * Consider this experimental.
 */
static int
fm_write (struct uio *uio)
{
  int pause, bytecount, chunk;
  char type;
  char *chunk_addr;
  struct sb_fm_note note;
  struct sb_fm_params params;

  for (chunk = 0; chunk < uio->uio_iovcnt; chunk++)
    {
      chunk_addr = uio->uio_iov[chunk].iov_base;
      bytecount = 0;
      while (uio->uio_resid
	     && bytecount < uio->uio_iov[chunk].iov_len)
	{
	  /* First read the one-character event type */
	  if (copyin (chunk_addr + bytecount, &type, sizeof (type)))
	    return (ENOTTY);
	  bytecount += sizeof (type);

	  switch (type)
	    {
	    case 'd':		/* Delay */
	    case 'D':
	      DPRINTF (("FM event: delay\n"));
	      if (copyin (chunk_addr + bytecount, &pause, sizeof (pause)))
		return (ENOTTY);
	      bytecount += sizeof (pause);
	      fm_delay (pause);
	      break;

	    case 'n':		/* Note */
	    case 'N':
	      DPRINTF (("FM event: note\n"));
	      if (copyin (chunk_addr + bytecount, &note, sizeof (note)))
		return (ENOTTY);
	      bytecount += sizeof (note);

	      if (fm_play_note (&note) != ESUCCESS)
		return (FAIL);
	      break;

	    case 'p':		/* Params */
	    case 'P':
	      DPRINTF (("FM event: params\n"));
	      if (copyin (chunk_addr + bytecount, &params, sizeof (params)))
		return (ENOTTY);
	      bytecount += sizeof (params);

	      if (fm_set_params (&params) != ESUCCESS)
		return (FAIL);
	      break;

	    default:
	      printf ("sb: Unrecognized FM event type: %c\n", type);
	      return (FAIL);	/* FAIL: Bad event type */
	    }
	}			/* while data left to write */
      uio->uio_resid -= bytecount;
    }				/* for each chunk */
  return (ESUCCESS);
}

/*
 * I am not happy with this function!
 * The FM chips are supposed to give an interrupt when a timer
 * overflows, and they DON'T as far as I can tell.  Without
 * the interrupt, the timers are useless to Unix, since polling
 * is altogether unreasonable.
 *
 * I am still looking for information on this timer facility, so
 * if you know more about how to use it, and specifically how to
 * make it generate an interrupt, I'd love to hear from you.
 * I'm beginning to think that they can't trigger a bus interrupt,
 * and all the DOS software just polls it.
 * If this is the case, expect this function to disappear in future
 * releases.
 */
static int
fm_delay (int pause)
{
  int ticks;
  
  DPRINTF (("fm_delay() pausing for %d microsecs... ", pause));

#ifndef USE_BROKEN_FM_TIMER	/**** Kludge! ****/
  wait_for_intr (SB_FM_NUM, HZ * pause / 1000000, INTR_PRIORITY); 
#else

  fm_send (TIMER_CHANNEL, TIMER_CTL, FM_TIMER_MASK1 | FM_TIMER_MASK2);
  fm_send (TIMER_CHANNEL, TIMER_CTL, FM_TIMER_RESET);
  while (pause >= 20000)	/* The 80us timer can do 20,400 usecs */
    {
      ticks = MIN((pause / 320), 255);
      fm_send (TIMER_CHANNEL, TIMER2, (256 - ticks));
      printf ("%d ticks (320 us)\n", ticks);
      fm_send (TIMER_CHANNEL, TIMER_CTL, (FM_TIMER_MASK1 | FM_TIMER_START2));
      wait_for_intr (SB_FM_NUM, TIMEOUT, INTR_PRIORITY);
      fm_send (TIMER_CHANNEL, TIMER_CTL, FM_TIMER_MASK1 | FM_TIMER_MASK2);
      fm_send (TIMER_CHANNEL, TIMER_CTL, FM_TIMER_RESET);
      pause -= ticks * 320;
    }

  while (pause >= 80)
    {
      ticks = MIN((pause / 80), 255);
      fm_send (TIMER_CHANNEL, TIMER1, (256 - ticks));
      printf ("%d ticks (80 us)\n", ticks);
      fm_send (TIMER_CHANNEL, TIMER_CTL, (FM_TIMER_START1 | FM_TIMER_MASK2));
      wait_for_intr (SB_FM_NUM, TIMEOUT, INTR_PRIORITY);
      fm_send (TIMER_CHANNEL, TIMER_CTL, FM_TIMER_MASK1 | FM_TIMER_MASK2);
      fm_send (TIMER_CHANNEL, TIMER_CTL, FM_TIMER_RESET);
      pause -= ticks * 80;
    }
  DPRINTF (("done.\n"));
#endif
  return (pause);		/* Return unspent time (< 80 usecs) */
}

#endif




/*
 * Mixer functions
 */
static int
mixer_ioctl (req_hdr)
ioctl_hdr *req_hdr;
{
  _32bits user_buffer;

  /* point to user data */
  user_buffer = req_hdr->dbuffer;

  switch (req_hdr->funct_cod)
    {
    case DSP_IOCTL_STEREO:	/* Mixer can control DSP stereo */
      return (dsp_ioctl (req_hdr));
    case MIXER_IOCTL_SET_LEVELS:
      if (verify_acc( user_buffer, sizeof(struct sb_mixer_levels), 0)==0)
	return (mixer_set_levels ((struct sb_mixer_levels*) user_buffer.fptr));
      else
	return(EIO);
    case MIXER_IOCTL_SET_PARAMS:
      if (verify_acc( user_buffer, sizeof(struct sb_mixer_params), 0)==0)
	return (mixer_set_params ((struct sb_mixer_params*) user_buffer.fptr));
      else
	return(EIO);
    case MIXER_IOCTL_READ_LEVELS:
      if (verify_acc( user_buffer, sizeof(struct sb_mixer_levels), 0)==0)
	return (mixer_get_levels ((struct sb_mixer_levels*) user_buffer.fptr));
      else
	return(EIO);
    case MIXER_IOCTL_READ_PARAMS:
      if (verify_acc( user_buffer, sizeof(struct sb_mixer_params), 0)==0)
	return (mixer_get_params ((struct sb_mixer_params*) user_buffer.fptr));
      else
	return(EIO);
    case MIXER_IOCTL_RESET:
      return (mixer_reset ());
    default:
      return (EINVAL);
    }
}



/*
 * Sets mixer volume levels.
 * All levels except mic are 0 to 15, mic is 7.  See sbinfo.doc
 * for details on granularity and such.
 * Basically, the mixer forces the lowest bit high, effectively
 * reducing the possible settings by one half.  Yes, that's right,
 * volume levels have 8 settings, and microphone has four.  Sucks.
 */
static int
mixer_set_levels (struct sb_mixer_levels *l)
{
  if (l->master.l & ~0xF || l->master.r & ~0xF
      || l->line.l & ~0xF || l->line.r & ~0xF
      || l->voc.l & ~0xF || l->voc.r & ~0xF
      || l->fm.l & ~0xF || l->fm.r & ~0xF
      || l->cd.l & ~0xF || l->cd.r & ~0xF
      || l->mic & ~0x7)
    return (EINVAL);

  mixer_send (VOL_MASTER, (l->master.l << 4) | l->master.r);
  mixer_send (VOL_LINE, (l->line.l << 4) | l->line.r);
  mixer_send (VOL_VOC, (l->voc.l << 4) | l->voc.r);
  mixer_send (VOL_FM, (l->fm.l << 4) | l->fm.r);
  mixer_send (VOL_CD, (l->cd.l << 4) | l->cd.r);
  mixer_send (VOL_MIC, l->mic);
  return (ESUCCESS);
}

/*
 * This sets aspects of the Mixer that are not volume levels.
 * (Recording source, filter level, I/O filtering, and stereo.)
 */
static int
mixer_set_params (struct sb_mixer_params *p)
{
  if (p->record_source != SRC_MIC
      && p->record_source != SRC_CD
      && p->record_source != SRC_LINE)
    return (EINVAL);

  /*
   * I'm not sure if this is The Right Thing.  Should stereo
   * be entirely under control of DSP?  I like being able to toggle
   * it while a sound is playing, so I do this... because I can.
   */
  dsp.in_stereo = (FLAG)(!!p->dsp_stereo);

  mixer_send (RECORD_SRC, (p->record_source
			   | (p->hifreq_filter ? FREQ_HI : FREQ_LOW)
			   | (p->filter_input ? FILT_ON : FILT_OFF)));

  mixer_send (OUT_FILTER, (BYTE)((dsp.in_stereo ? STEREO_DAC : MONO_DAC)
			   | (p->filter_output ? FILT_ON : FILT_OFF)));
  return (ESUCCESS);
}

/*
 * Read the current mixer level settings into the user's struct.
 */
static int
mixer_get_levels (struct sb_mixer_levels *l)
{
  BYTE val;

  val = mixer_read_reg (VOL_MASTER); /* Master */
  l->master.l = B4(val >> 4);
  l->master.r = B4(val);

  val = mixer_read_reg (VOL_LINE); /* FM */
  l->line.l = B4(val >> 4);
  l->line.r = B4(val);
  
  val = mixer_read_reg (VOL_VOC); /* DAC */
  l->voc.l = B4(val >> 4);
  l->voc.r = B4(val);

  val = mixer_read_reg (VOL_FM); /* FM */
  l->fm.l = B4(val >> 4);
  l->fm.r = B4(val);
  
  val = mixer_read_reg (VOL_CD); /* CD */
  l->cd.l = B4(val >> 4);
  l->cd.r = B4(val);

  val = mixer_read_reg (VOL_MIC); /* Microphone */
  l->mic = B3(val);

  return (ESUCCESS);
}

/*
 * Read the current mixer parameters into the user's struct.
 */
static int
mixer_get_params (struct sb_mixer_params *params)
{
  BYTE val;

  val = mixer_read_reg (RECORD_SRC);
  params->record_source = val & 0x07;
  params->hifreq_filter = (FLAG)(!!(val & FREQ_HI));
  params->filter_input = (FLAG)((val & FILT_OFF) ? OFF : ON);
  params->filter_output = (FLAG)((mixer_read_reg (OUT_FILTER) & FILT_OFF) ? OFF : ON);
  params->dsp_stereo = dsp.in_stereo;
  return (ESUCCESS);
}


/*
 * This is supposed to reset the mixer.
 * Technically, a reset is performed by sending a byte to the MIXER_RESET
 * register, but I don't like the default power-up settings, so I use
 * these.  Adjust to taste, and you have your own personalized mixer_reset
 * ioctl.
 */
static int
mixer_reset (void)
{
  struct sb_mixer_levels l;
  struct sb_mixer_params p;

  p.filter_input  = OFF;
  p.filter_output = OFF;
  p.hifreq_filter = TRUE;
  p.record_source = SRC_LINE;

  l.cd.l = l.cd.r = 1;
  l.mic = 1;
  l.master.l = l.master.r = 11;
  l.line.l = l.line.r = 15;
  l.fm.l = l.fm.r = 15;
  l.voc.l = l.voc.r = 15;

  if (mixer_set_levels (&l) == ESUCCESS
      && mixer_set_params (&p) == ESUCCESS)
    return (ESUCCESS);
  else
    return (EIO);
}

/*
 * Send a byte 'val' to the Mixer register 'reg'.
 */
static void
mixer_send (BYTE reg, BYTE val)
{
  sb_sendb (MIXER_ADDR, reg, MIXER_DATA, val);
}

/*
 * Returns the contents of the mixer register 'reg'.
 */
static BYTE
mixer_read_reg (BYTE reg)
{
  outb (MIXER_ADDR, reg);
  tenmicrosec();		/* To make sure nobody reads too soon */
  return ((BYTE)(inb (MIXER_DATA)));
}



#ifdef MIDI_SUPPORTED
/*
 * MIDI functions
 */
static int
midi_open (int flags)
{
  if (status.dsp_in_use)
    return (EBUSY);

  status.dsp_in_use = TRUE;
  dsp_reset ();			/* Resets card and inits variables */

  /*
   * This DSP command requests interrupts on MIDI bytes, but allows
   * writing to the MIDI port at the same time.  This is only
   * available in SB DSP's v2.0 or greater.
   * Any byte sent to the DSP while in this mode will be sent out the
   * MIDI OUT lines.  The only way to exit this mode is to reset the DSP.
   */
  dsp_command (MIDI_UART_READ);
  return (ESUCCESS);
}

static int
midi_close ()
{
  if (status.dsp_in_use)
    {
      status.dsp_in_use = FALSE;
      if (dsp_reset() == GOOD)	/* Exits MIDI UART mode */
	return (ESUCCESS);
      else
	return (EIO);
    }
  else
    return (ESRCH);
}

static int
midi_read (struct uio *uio)
{
  int bytecount, s, chunk;
  BYTE data;

  s = splhigh ();
  /* For each uio_iov[] entry... */
  for (chunk = 0; chunk < uio->uio_iovcnt; chunk++)
    {
      bytecount = 0;

      /* While there is still data to read, and data in this chunk.. */
      while (uio->uio_resid
	     && bytecount < uio->uio_iov[chunk].iov_len)
	{
	  while (!(inb (DSP_RDAVAIL) & DSP_DATA_AVAIL))
	    {
	      if (dsp.dont_block) /* Maybe they don't want to wait.. */
		{
		  splx (s);
		  return (ESUCCESS);
		}
	      wait_for_intr (DSP_UNIT, 0, PSLEP); /* Maybe they do. */
	    }
	  /*
	   * In any case, data is ready for reading by this point.
	   */
	  DPRINTF (("Getting one MIDI byte.."));
	  data = inb (DSP_RDDATA);
	  if (copyout (&data, uio->uio_iov[chunk].iov_base + bytecount, 1))
	    {
	      printf ("sb: Bad copyout()!\n");
	      return (ENOTTY);
	    }
	  bytecount ++;
	  uio->uio_resid --;
	}
    }			/* for each chunk */
  splx (s);
  return (ESUCCESS);
}

static int
midi_write (struct uio *uio)
{
  int bytecount, chunk, s;
  char *chunk_addr;
  BYTE data;

  for (chunk = 0; chunk < uio->uio_iovcnt; chunk++)
    {
      chunk_addr = uio->uio_iov[chunk].iov_base;
      bytecount = 0;
      while (uio->uio_resid
	     && bytecount < uio->uio_iov[chunk].iov_len)
	{
	  if (copyin (chunk_addr + bytecount, &data, sizeof (data)))
	    return (ENOTTY);
	  bytecount++;
	  s = splhigh ();
	  /*
	   * In MIDI UART mode, data bytes are sent as DSP commands.
	   */
	  if (dsp_command (data) == FAIL)
	    {
	      splx (s);
	      return (EIO);
	    }
	  splx (s);
	}			/* while data left to write */
      uio->uio_resid -= bytecount;
    }				/* for each chunk */
  return (ESUCCESS);
}

static int
midi_ioctl (int cmd, caddr_t arg)
{
  switch (cmd)
    {
      /*
       * Shoot.  I can't figure this out, and I don't have any examples.
       * This is supposed to support the fcntl() call.
       * The only flags now are FNDELAY for non-blocking reads.
       * You can currently open the midi device with this flag, but
       * you can't set it with fcntl().
       * Stay tuned...
       */
#if 0				/* sorry */
    case TIOCGETP:
      arg->sg_flags = dsp.dont_block;
      break;
    case TIOCSETP:
      dsp.dont_block = arg->sg_flags & FNDELAY;
      break;
#endif
    default:
      return (EINVAL);
    }
  return (ESUCCESS);
}
#endif 

/* ---- Emacs kernel recompile command ----
Local Variables:
compile-command: "/bin/sh -c \"cd ../../..; kmake\""
End:
*/
