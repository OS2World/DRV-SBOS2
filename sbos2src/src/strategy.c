/***************************************************************************/
/* strategy_dsp(req_hdr)                                                   */
/*  Strategy function. Called by assembler portion of strategy entry point */
/*  of DD, this is functionally the strategy entry point.  It looks at the */
/*  command portion of the request packet an calls the proper function.    */
/*                                                                         */
/*  There is only 1 command that really does anything - INIT.  Others      */
/*  don't but say we did.  Finally, the rest Return BAD_CMD.               */
/*                                                                         */
/*  All commands return a value that will be ORed with the DONE bit and    */
/*  set into the request packet's STATUS field via DEV_DONE.               */
/*                                                                         */
/***************************************************************************/
#include "demo.h"

/* prototypes for sb_drive routines and others */
int sb_open(int);
int sb_close(int, void far *);
int sb_read(struct rw_hdr_def *);
int sb_write(struct rw_hdr_def *);
int sb_probe( int );
int sb_attach( int, int);
int sb_flush( void far *);
int sb_ioctl(int, ioctl_hdr *);

/* Device Numbers numbers */
/* MSF - May use some variant of following if DSP and MIXER stuff ends up
   in same driver */
#define SB_CMS_NUM  	0	/* NOT USED */
#define SB_FM_NUM    	1	/* NOT USED */
#define SB_DSP_NUM    	2	/* sbdsp$   DSP DAC/ADC */
#define SB_MIDI_NUM   	3	/* NOT USED */
#define SB_MIXER_NUM  	4	/* sbmix$   Non-exclusive-use Mixer */
#define SB_BIGGEST_MINOR 4	/* Highest minor number used. */


int near strategy_dsp(req_hdr)
struct reqhdr *req_hdr;
{
  word rc;

  /* Check to see if the command is outside the range of valid commands */
  if (req_hdr->rh_cmd > MAXCMD) { rc = bad_cmd(); }
  else {

  /* Different commands call for different functions */
     switch (req_hdr->rh_cmd) {
        case  0 :
                  rc = init_dsp((struct init_hdr_in *)req_hdr);    /* Init */
                  break;
        case  1 :
                  rc = bad_cmd();            /* Media Check */
                  break;
        case  2 :
                  rc = bad_cmd();            /* Build BPB */
                  break;
        case  3 :
                  rc = bad_cmd();            /* Reserved */
                  break;
        case  4 :
                  rc = sb_read((rw_hdr *)req_hdr); /* Read */
                  break;
        case  5 :
                  rc = bad_cmd();            /* Non-destructive Read */
                  break;
        case  6 :
                  rc = bad_cmd();            /* Input Status */
                  break;
        case  7 :
                  rc = bad_cmd();            /* Input Flush */
                  break;
        case  8 :
                  rc = sb_write((rw_hdr *)req_hdr);    /* Write */
                  break;
        case  9 :
                  rc = bad_cmd();            /* Write with Verify */
                  break;
        case 10 :
                  rc = bad_cmd();            /* Output Status */
                  break;
        case 11 :
                  rc = sb_flush((void far *)req_hdr);   /* Output Flush */
                  break;
        case 12 :
                  rc = bad_cmd();            /* Reserved */
                  break;
        case 13 :
                  rc=sb_open(SB_DSP_NUM);      /* Open */
                  break;
        case 14 :
                  rc = sb_close(SB_DSP_NUM, (void far *) req_hdr); /* Close */
                  break;
        case 15 :
                  rc = bad_cmd();            /* Removable Media */
                  break;
        case 16 :
                  rc = sb_ioctl(SB_DSP_NUM, (ioctl_hdr *) req_hdr); /* IOCtl */
                  break;
        case 17 :
                  rc = bad_cmd();            /* Reset media */
                  break;
        case 18 :
                  rc = bad_cmd();            /* Get Logical Drive Map */
                  break;
        case 19 :
                  rc = bad_cmd();            /* Set Logical Drive Map */
                  break;
        case 20 :
                  rc = 0;                    /* De-install */
                  break;
        case 21 :
                  rc = bad_cmd();            /* Port Access */
                  break;
        case 22 :
                  rc = bad_cmd();            /* Partitionable Fixed Disks */
                  break;
        case 23 :
                  rc = bad_cmd();            /* Get Logical Unit Map */
                  break;
        case 24 :
                  rc = bad_cmd();            /* Reserved */
                  break;
        case 25 :
                  rc = bad_cmd();            /* Reserved */
                  break;
	case 26 :
                  rc = bad_cmd();            /* Reserved */
                  break;

        case 27 :
                  rc = bad_cmd();            /* Reserved */
                  break;

	case 28 :                            /* SHUTDOWN */
	          rc = 0;
		  break;

	  default :                          /* Just in case */
                  rc = bad_cmd();
                  break;
     } /* switch */
   } /* else */

  /* set return code */
  if (rc==0) rc=DONE;
  req_hdr->rh_stat = rc; 

  return(SUCCESS);

}



/* this is for the mixer part of SB Pro, not used with SB */
int near strategy_mix(req_hdr)
struct reqhdr *req_hdr;
{
  word rc;

  /* Check to see if the command is outside the range of valid commands */
  if (req_hdr->rh_cmd > MAXCMD) { rc = bad_cmd(); }
  else {

  /* Different commands call for different functions */
     switch (req_hdr->rh_cmd) {
        case  0 : rc = init_mix((struct init_hdr_in *)req_hdr);    /* Init */
                  break;
        case  1 :
                  rc = bad_cmd();            /* Media Check */
                  break;
        case  2 :
                  rc = bad_cmd();            /* Build BPB */
                  break;
        case  3 :
                  rc = bad_cmd();            /* Reserved */
                  break;
        case  4 :
                  rc = bad_cmd();            /* Read */
                  break;
        case  5 :
                  rc = bad_cmd();            /* Non-destructive Read */
                  break;
        case  6 :
                  rc = bad_cmd();            /* Input Status */
                  break;
        case  7 :
                  rc = bad_cmd();            /* Input Flush */
                  break;
        case  8 :
                  rc = bad_cmd();            /* Write */
                  break;
        case  9 :
                  rc = bad_cmd();            /* Write with Verify */
                  break;
        case 10 :
                  rc = bad_cmd();            /* Output Status */
                  break;
        case 11 :
                  rc = bad_cmd();            /* Output Flush */
                  break;
        case 12 :
                  rc = bad_cmd();            /* Reserved */
                  break;
        case 13 :
                  rc=sb_open(SB_MIXER_NUM);                    /* Open */
                  break;
        case 14 :
                  rc = sb_close(SB_MIXER_NUM,(void far *) req_hdr);  /*Close*/
                  break;
        case 15 :
                  rc = bad_cmd();            /* Removable Media */
                  break;
        case 16 :
                  rc = sb_ioctl(SB_MIXER_NUM, (ioctl_hdr *) req_hdr); /*IOCtl*/
                  break;
        case 17 :
                  rc = bad_cmd();            /* Reset media */
                  break;
        case 18 :
                  rc = bad_cmd();            /* Get Logical Drive Map */
                  break;
        case 19 :
                  rc = bad_cmd();            /* Set Logical Drive Map */
                  break;
        case 20 :
                  rc = 0;                    /* De-install */
                  break;
        case 21 :
                  rc = bad_cmd();            /* Port Access */
                  break;
        case 22 :
                  rc = bad_cmd();            /* Partitionable Fixed Disks */
                  break;
        case 23 :
                  rc = bad_cmd();            /* Get Logical Unit Map */
                  break;
        case 24 :
                  rc = bad_cmd();            /* Reserved */
                  break;
        case 25 :
                  rc = bad_cmd();            /* Reserved */
                  break;
        case 26 :
                  rc = bad_cmd();            /* Reserved */
                  break;

        case 27 :
                  rc = bad_cmd();            /* Reserved */
                  break;
	case 28:
            	  rc = 0;                    /* shutdown */
	          break;
        default :                            /* Just in case */
                  rc = bad_cmd();
                  break;
     } /* switch */
   } /* else */


  /* set return code */
  if (rc==0) rc=DONE;
  req_hdr->rh_stat = rc; 

  return(SUCCESS);

}
