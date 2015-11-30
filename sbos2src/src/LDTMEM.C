/*****************************************************************************/
/* Routines for managing LDT accessed memory                                 */
/*                                                                           */
/* char *phys_to_ldt(loc,count)                                              */
/* long loc;                                                                 */
/* unsigned count;                                                           */
/*                                                                           */
/* Creates a selector in the current LDT from the physical address (loc) and */
/* the size of the block.  Returns the selector as the function value.  If   */
/* The creation fails, the return pointer is 0000:0000.                      */
/*                                                                           */
/* ------------------------------------------------------------------------- */
/*                                                                           */
/* int free_virt(selector)                                                   */
/* unsigned selector;                                                        */
/*                                                                           */
/* Gives the LDT selector back to OS/2 for later use.  Returns 0 if          */
/* successful, -1 if the return was rejected by OS/2.                        */
/*                                                                           */
/*****************************************************************************/
#include "demo.h"

int far free_virt(selctr)
unsigned selctr;
{
  union cpu_regs in_regs,out_regs;
  union cpu_regs *ptr;

  ptr = &in_regs;                            /* Make sure ES = DS            */
  ptr->W.AX = selctr;
  in_regs.W.AX = selctr;                     /* AX = Segment to give back    */
  in_regs.B.DH = 2;                          /* Say we are giving it back    */
  in_regs.B.DL = devhlp_PhysToUVirt;         /* DevHlp command               */
  in_regs.W.es_valid = FALSE;                /* Use current ES and DS regs   */
  in_regs.W.ds_valid = FALSE;
  dev_help(&in_regs,&out_regs);
                                             /* If failure                   */
  if ((out_regs.W.flags & 0x0001) != 0) {    /*    Return -1                 */
     return(-1);
     }                                       /* Else                         */
  return(0);                                 /*    Return 0                  */
}


_32bits far phys_to_ldt1(loc,count)
_32bits loc;
unsigned count;
{
  union cpu_regs in_regs,out_regs;
  _32bits temp;

  in_regs.W.AX = loc._2words.high;           /* AX:BX points to phys address */
  in_regs.W.BX = loc._2words.low;            /* AX:BX points to phys address */
  in_regs.W.CX = count;                      /* CX has the size              */
  in_regs.B.DH = 1;                          /* Allocate above the 1M line   */
  in_regs.B.DL = devhlp_PhysToUVirt;
  in_regs.W.es_valid = FALSE;                /* struct ES not valid          */
  in_regs.W.ds_valid = FALSE;                /* struct DS not valid          */
  dev_help1(&in_regs,&out_regs);             /* Call DevHlp1                 */
  if ((out_regs.W.flags & 0x0001) != 0) {    /* If failure                   */
     temp.phys = 0;
     return(temp);                           /*    Return 0000:0000          */
     }
  temp._2words.high = out_regs.W.ES;         /* Else, return pointer         */
  temp._2words.low  = out_regs.W.BX;
  return(temp);
}


_32bits far phys_to_ldt(loc,count)
_32bits loc;
unsigned count;
{
  union cpu_regs in_regs,out_regs;
  _32bits temp;

  in_regs.W.AX = loc._2words.high;           /* AX:BX points to phys address */
  in_regs.W.BX = loc._2words.low;            /* AX:BX points to phys address */
  in_regs.W.CX = count;                      /* CX has the size              */
  in_regs.B.DH = 1;                          /* Allocate above the 1M line   */
  in_regs.B.DL = devhlp_PhysToUVirt;
  in_regs.W.es_valid = FALSE;                /* ES not valid                 */
  in_regs.W.ds_valid = FALSE;                /* DS not valid                 */
  dev_help(&in_regs,&out_regs);              /* Call DevHlp                  */
  if ((out_regs.W.flags & 0x0001) != 0) {    /* If failure                   */
     temp.phys = 0;
     return(temp);                           /*    Return 0000:0000          */
     }
  temp._2words.high = out_regs.W.ES;         /* Else, return pointer         */
  temp._2words.low  = out_regs.W.BX;
  return(temp);
}

