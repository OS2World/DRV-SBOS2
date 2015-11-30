/*****************************************************************************/
/* init_dsp(req_pkt)                                                         */
/*                                                                           */
/* Called by Strategy, this function executes the INIT command from OS/2.    */
/*                                                                           */
/* Store devhlp entry point for use later by dev_help                        */
/* Display the information about this DD (version, copywrite, etc)           */
/* Do whatever initialization your particular DD needs                       */
/* Set the endpoints of the default CODE and DATA segments                   */
/* At any of these steps, if an error is detected (some steps don't do any   */
/* error checking because there is none to do), the INIT ends, and the DD    */
/* returns a FAIL error.  In essence, it fails to install.                   */
/*                                                                           */
/* Finally, it sets up a pointer to Global Information Segment.  This has    */
/* the Milliseconds since IPL value and the current PID.  These are useful   */
/* things to know.                                                           */
/*                                                                           */
/* There is a bit of razzle-dazzle in this function.  At the very beginning, */
/* the DevHlp entry point must be stored in a place where the DevHlp caller  */
/* can get to it.  Due to the nature of DevHlp calls, the caller needs to    */
/* have it addressable by CS.  To do this, the entry point must be stored in */
/* the CODE segment.  But, the CODE segment is not writable.  Therefore, an  */
/* alias selector (Data, Writeable) must be created.  Another problem is     */
/* that to do this uses for calls to DevHlp. So, the basic steps are:        */
/* 1. Store the DevHlp entry point in a temporary location in the Data Seg   */
/* 2. Get the physical address of the DevHlp storage location in the Code    */
/*    segment.  This uses the temporary DevHlp caller that uses the DevHlp   */
/*    entry point stored in the DATA segment                                 */
/* 3. Make an LDT based pointer to the physical address gotten in step 2.    */
/*    This two uses the temporary DevHlp caller                              */
/* 4. Store the DevHlp entry point in the address gotten in step 3.  From    */
/*    this point on, we can use the CS based DevHlp caller.                  */
/* 5. Free the LDT selector slot used in steps 2 and 3.                      */
/*                                                                           */
/* The reason for using CS as the segment pointer to the DevHlp entry point  */
/* is that CS is the only segment always available.  Some DevHlp calls use   */
/* DS and others use ES, the SS and CS are the only ones left.  I chose to   */
/* pay the price at INIT time and store the entry in CS, and not always      */
/* pass it in the stack (which, by the way is a perfectly valid way to do    */
/* it).                                                                      */
/*                                                                           */
/*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "demo.h"

/* prototypes for sb_drive routines and others */
int sb_open(void);
int sb_close(void);
int sb_read(struct rw_hdr_def *);
int sb_write(struct rw_hdr_def *);
int sb_probe( int, int );
int sb_attach( int, int, int, int);



char near zero_msg[]={"0"};
char near newline_msg[]={"\r\n"};
char near mymessage[]={"SB OS/2 Device Driver Version 0.91 loaded.\n\rBy Michael Fulbright (1992)\n\r\n"};
char near bad_probe_msg[]= {"ERROR- SB NOT FOUND! Driver not loaded\r\n"};
char near good_probe_msg[]={"SB FOUND! Driver installation proceeding\r\n"};
char near bad_attach_msg[]={"ERROR- Unable to attach to SB. Driver not loaded\r\n"};
char near PAS16_msg[]={"PAS16 assumed present - cannot test, continuing initialization...\n\r"};
char near good_attach_msg[]={"SB ATTACHED. Driver initialization complete.\r\n"};


/* if init_dsp fails then this will be left with a TRUE value, which lets
 * init_mixer know it shoud fail as well */
static int near init_failed=TRUE;
static int near issbpro;
static int near quiet;  /* install quietly ? */

extern unsigned far   last_c;
extern _32bits  near  devhlp;
extern unsigned far  *dev_hlpCS();
extern unsigned near  last_d;
       boolean  near  initialized = FALSE;

unsigned far init_dsp( struct init_hdr_in *req_hdr)
{
   _32bits             *temp;
   _32bits              pointer;
   _32bits              tempaddr;
   char far *s;
   char ch[10];
   struct init_hdr_out *req_hdr_out;
   int SBIRQ, SBaddr;
   int doprobe;

   unsigned int nwritten;

   /* Store the DevHlp entry point in our DS                             */
   /* This one is used only for the next section, which gets the devhlp  */
   /* entry point stored in the DD's CS.  This lets me have 1 devhlper   */
   /* that does not depend on if DS or ES is holding a pointer.  devhelp */
   /* will use CS to access the devhlp entry                             */
   devhlp = req_hdr->pointer1;

   /* Store the DevHlp entry point in our CS */
   /*    Get the phys addr of dev_hlpCS */
   pointer.fptr = (void far *)dev_hlpCS;
   tempaddr = get_phys_addr1(pointer);

   /*    Make an LDT selector point to it */
   pointer = phys_to_ldt1(tempaddr,32);
   temp = (_32bits *)pointer.fptr;

   /*    Store the devhlp entry point in our CS */
   temp->_segadr.segment = req_hdr->pointer1._segadr.segment;
   temp->_segadr.offset  = req_hdr->pointer1._segadr.offset;

   /*    Free the LDT selector slot used */
   pointer.phys = (physaddr)temp;
   free_virt(pointer._segadr.segment);


   /* Set the return state to fail, if it passes, we will fix before exit */
   req_hdr_out = (struct init_hdr_out *)req_hdr;
   req_hdr_out->code_end = 0;
   req_hdr_out->data_end = 0;
   req_hdr_out->data1 = 0;

   /* See if this should be a quiet initialization or not */
   /* first make line from CONFIG.SYS all uppercase, I dont */
   /* trust MSC routine to do this, which appears to allocate memory */
   s=req_hdr->pointer2;
   for (; *s; s++)
     {
       if ( (*s < 123) && (*s > 96))
	 *s = (char) (*s-32);
     }

   s=strstr( req_hdr->pointer2, "QUIET");
   if (s!=NULL)
     quiet=TRUE;
   else
     quiet=FALSE;

   /* Do your initialisation  */
   if (!quiet)
     DosWrite(1, (void far *) mymessage, strlen(mymessage), &nwritten);

   /* these should be set by commandline in config.sys */
   SBaddr = 0x240;
   SBIRQ  = 5;


   /* output string from config.sys */
   if (!quiet)
     {
       DosWrite(1, (void far *) req_hdr->pointer2, strlen(req_hdr->pointer2), 
		&nwritten);
       DosWrite(1, (void far *) newline_msg, 2, &nwritten);
     }

   /* search for IRQ in commandline */
   s=strstr( req_hdr->pointer2, "IRQ");
   if (s!=NULL)
     {
       ch[0]=*(s+3);
       ch[1]=*(s+4);
       ch[2]='\0';
       /* if crazy value then reset to default */
       if ((SBIRQ=atoi(ch))>15)
	 SBIRQ = 5;
  
       /* if they say IRQ2 we really mean IRQ 9 */
       if (SBIRQ==2)
	 SBIRQ=9;
     }

   /* search for ADDR in commandline */
   s=strstr( req_hdr->pointer2, "ADDR" );
   if (s!=NULL)
     {
       ch[0]=*(s+4);
       ch[1]=*(s+5);
       ch[2]=*(s+6);
       ch[3]='\0';
       SBaddr=atoi(ch);
       if (SBaddr==220) SBaddr=0x220;
       else if (SBaddr==240) SBaddr=0x240;
       else SBaddr=0x220;
     }
    

   /* search for 'SBREG' which lets us know to disable SB Pro functions */
   /* assume SBPRO */
   issbpro = TRUE;
   s=strstr( req_hdr->pointer2, "SBREG" );
   if (s!=NULL)
     issbpro=FALSE;

   /* see if the string 'PAS16' exists, meaning we have a SBREG and we want 
    * to skip check for presence of card */
   s=strstr( req_hdr->pointer2, "PAS16" );
   if (s!=NULL)
     {
       issbpro=FALSE;
       doprobe=FALSE;
     }
   else
     doprobe=TRUE;

   

   /* first lets make sure a SB exists */
   if (doprobe)
     {
       if (!sb_probe(SBaddr,doprobe))
	 {
	   if (!quiet)
	     DosWrite(1, (void far *) bad_probe_msg, strlen(bad_probe_msg),
		      &nwritten);
	   return(DONE | ERROR | GEN_FAIL);
	 }
       else
	 {
	   if (!quiet)
	     DosWrite(1, (void far *) good_probe_msg, strlen(good_probe_msg),
		      &nwritten);
	 }
     }
   else
     {
       sb_probe(SBaddr,doprobe);
       
       if (!quiet)
	 DosWrite(1,(void far *) PAS16_msg, sizeof(PAS16_msg)-1, &nwritten);
     }

   /* now lets set everything up (find buffers, grab interupt, etc) */
   if (sb_attach(SBaddr, SBIRQ, issbpro, quiet))
       {
	 if (!quiet)
	   DosWrite(1, (void far *) bad_attach_msg, strlen(bad_attach_msg),
		    &nwritten);
	 return(DONE | ERROR | GEN_FAIL);
       }
   else
     {
       if (!quiet)
	 DosWrite(1, (void far *) good_attach_msg, strlen(good_attach_msg),
		  &nwritten);
     }

   /* Set up so we can use the timer */
   point_to_global();

   /* Set the Code and Data segment sizes in the request header */
   pointer.phys = (physaddr)&last_c;
   req_hdr_out->code_end = pointer._segadr.offset;
   pointer.fptr = (void far *)&last_d;
   req_hdr_out->data_end = pointer._segadr.offset;

   /* Lock down the external segments */
   pointer.fptr = (void far *)strategy_dsp;
   lock(pointer._segadr.segment);

   /* inidicate we were successful */
   init_failed = FALSE;

   return(DONE);

}



char near mixerfailmsg[]={"SB Mixer driver not loaded.\n\r"};
char near smixer_msg[]={"SB Pro Mixer driver loaded.\n\r\n"};

/* this initializes the mixer part */
/* doesnt do much, just returns with successful status */
unsigned far init_mix( struct init_hdr_in *req_hdr)
{
   _32bits              pointer;
   struct init_hdr_out *req_hdr_out;
   unsigned int         nwritten;


   /* if SBDSP$ didnt load, then this shouldnt */
   if (init_failed || issbpro==FALSE)
     {
       req_hdr_out = (struct init_hdr_out *)req_hdr;
       req_hdr_out->code_end = 0;
       req_hdr_out->data_end = 0;
       req_hdr_out->data1 = 0;
       if (!quiet)
	 DosWrite(1, (void far *) mixerfailmsg, strlen(mixerfailmsg), 
		  &nwritten);
       return( DONE | ERROR | GEN_FAIL );
     }

   /* Set the Code and Data segment sizes in the request header */
   req_hdr_out = (struct init_hdr_out *) req_hdr;
   pointer.phys = (physaddr)&last_c;
   req_hdr_out->code_end = pointer._segadr.offset;
   pointer.fptr = (void far *)&last_d;
   req_hdr_out->data_end = pointer._segadr.offset;

   if (!quiet)
     DosWrite(1,(void far *) smixer_msg,strlen(smixer_msg),&nwritten);

   return(DONE);

}


