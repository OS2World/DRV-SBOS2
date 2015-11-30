/****************************************************************************/
/* Routines for locking and unlocking segments                              */
/****************************************************************************/
/* Lock a segment                                                           */
/*                                                                          */
/* long lock(sel)                                                           */
/* unsigned sel; - Selector to segment to lock down                         */
/*                                                                          */
/* Return value - Lock handle of segment, 0 if the lock failed              */
/*                                                                          */
/* This function requests OS/2 lock the segment.  The request is for a      */
/* short term lock.  THis is done because a long term lock causes OS/2 to   */
/* swap something out, degrading performance to an unacceptable level.  The */
/* short term lock doesn't seem to do that.  The function returns the 'lock */
/* handle' that is needed to unlock the segment later.  It returns a lock   */
/* handle of 0 if the lock fails.  It also increments the variable          */
/* lock_count.  This is a useful debug tool as the single most common cause */
/* of the dreaded "Internal Error Detected at.."  message is an error in    */
/* lock/unlock.  Either we are passing an invalid lock handle to UNLOCK or  */
/* we locked down too much without unlocking.  This counter will be very    */
/* high (on the order of 0x180 or more if the latter is the case.           */
/*                                                                          */
/****************************************************************************/
/* Unlock a segment                                                         */
/*                                                                          */
/* unsigned unlock(handle)                                                  */
/* long handle; - Lock handle of segment to unlock                          */
/*                                                                          */
/* Return value - SUCCESS or -1 if the unlock failed                        */
/*                                                                          */
/* This function requests OS/2 unlock a previously locked segment.  If the  */
/* handle passed is 0, the function just returns SUCCESS.  This lets the DD */
/* unlock a segment and store 0 in the place of the lock handle.  Later, if */
/* it tries to unlock the same segment again, the function will recognize   */
/* this fact and act as if the unlock was successful.  This lets the unlock */
/* activity use a small number of common routines.                          */
/*                                                                          */
/* The function also decrements the lock_count variable and sets in_unlock  */
/* to TRUE while the device helper routine is being called.  This is        */
/* another debug facility to help detect when a bad lock handle is passed.  */
/*                                                                          */
/****************************************************************************/
#include <stdio.h>
#include "demo.h"

boolean near in_unlock = FALSE;  /* in_unlock starts out FALSE */
word    near lock_count = 0;     /* lock_count starts out 0    */

unsigned long far lock(sel)
word sel;
{
  physaddr handle;
  union cpu_regs in_regs;
  union cpu_regs out_regs;

  in_regs.W.AX = sel;                              /* AX holds the selector  */
  in_regs.B.BH = 0;                                /* Short term block       */
  in_regs.B.BL = 0;                                /* Block - I'll wait      */
  in_regs.W.es_valid = FALSE;                      /* Use current ES and DS  */
  in_regs.W.ds_valid = FALSE;
  in_regs.B.DL = devhlp_Lock;                      /* Do it                  */
  dev_help(&in_regs,&out_regs);
  if ((out_regs.W.flags & 0x0001) != 0) {          /* If it failed           */
     return(0L);                                   /*    Return 0            */
     }                                             /* Endif                  */
  lock_count++;                                    /* Increment lock count   */
  handle = (out_regs.W.AX * 0x10000) + out_regs.W.BX;  /* Build lock handle  */
  return(handle);                                  /* And return it to caller*/
}

word far unlock(han)
unsigned long han;
{
  union cpu_regs in_regs;
  union cpu_regs out_regs;
  _32bits        handle;


     /* If the lock handle is 0, already unlocked, just say we did it */
     handle.phys = han;
     if (handle.phys == 0L) { return(SUCCESS); }

     in_regs.W.AX = handle._2words.high;       /* AX:BX hold the lock handle */
     in_regs.W.BX = handle._2words.low;
     in_regs.W.es_valid = FALSE;               /* Use current ES and DS      */
     in_regs.W.ds_valid = FALSE;
     in_unlock = TRUE;                         /* Mark that we are in UNLOCK */
     in_regs.B.DL = devhlp_Unlock;             /* Do it                      */
     dev_help(&in_regs,&out_regs);
     in_unlock = FALSE;                        /* Done unlocking             */
     if ((out_regs.W.flags & 0x0001) != 0) {   /* If it failed               */
        return(-1);                            /*     Return -1              */
        }                                      /* Endif
     lock_count--;                             /* Decrement lock count       */

     return(SUCCESS);                          /* Return SUCCESS             */
}

