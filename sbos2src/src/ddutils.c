/*****************************************************************************/
/* General Device Driver Utilities                                           */
/*****************************************************************************/
/* This file contains a bunch of general utility functions all DDs need.     */
/* There are things like change CPU mode, get the current time, yield the    */
/* CPU, block and unblock threads, generate the Dreaded Internal Error msg,  */
/* send EOI, and some request packet queueing functions.                     */
/*****************************************************************************/
/*                                                                           */
/*  void to_prot_mode(void)                                                  */
/*                                                                           */
/*  This function changes the CPU mode from REAL to PROTECT, if it was in    */
/*  REAL mode to begin with.  It also sets a global flag indicating that     */
/*  what mode the CPU was in when entered.  This lets the to_real_mode()     */
/*  function know when to change it back.                                    */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*  void point_to_global(void)                                               */
/*                                                                           */
/*  This function sets up a pointer to the System Global Information         */
/*  segment.  This pointer is GDT based, useable after INIT is done.         */
/*                                                                           */
/*  The basic operation is doen via a call to GetDOSVar DevHlp function with */
/*  a variable number of 1 - SysINFOSeg.  This returns a pointer to a 2-byte */
/*  value.  This value is the selector of the Global Info.  This selector is */
/*  then made into a pointer.                                                */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*  unsigned long curr_time()                                                */
/*                                                                           */
/*  This function returns the current Milliseconds_Since_IPL value from the  */
/*  Global Information Segment.  It is useable at any time except INIT time. */
/*  It cannot be called until AFTER point_to_global() has been called.       */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*  void far yield()                                                         */
/*                                                                           */
/*  This function yields the CPU.  It does it by using the BLOCK DevHlp call */
/*  rather than the YIELD call because the YIELD may not YIELD.  The BLOCK   */
/*  will always work.  It BLOCKs for 32 mSec.  It generates a Block-id from  */
/*  the current time.                                                        */
/*                                                                           */
/*  When it returns, the CPU has been yielded and other tasks have been      */
/*  dispatched.                                                              */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*  int far block(block_id, timeout)                                         */
/*  unsigned long block_id;                                                  */
/*  unsigned long timeout;                                                   */
/*                                                                           */
/*  This function blocks the current thread for up to timeout mSec.  It uses */
/*  the BLOCK DevHlp call.  The block is non-interruptable.                  */
/*  Returns 0 is unblocked ok, 1 if we timed out                             */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*  void far unblock(block_id)                                               */
/*  unsigned long block_id;                                                  */
/*                                                                           */
/*  This function unblocks a thread that is blocked with the given block-id. */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*  void far internal_error(msg_num)                                         */
/*  unsigned msg_num;                                                        */
/*                                                                           */
/*  This function generates the Dreaded Internal Error Detected message with */
/*  your own message text added.  The msg_num is an index into the err_msgs  */
/*  array defined above.  So far, only 1 message is defined.  Calling this   */
/*  function WILL lock up the machine so that only a power off will clear    */
/*  it.  Use only in dire emergencies.                                       */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*  void far push_req_pkt(queue,req)                                         */
/*  _32bits     *queue;                                                      */
/*  reqhdr_type *req;                                                        */
/*                                                                           */
/*  This function adds a request packet to the end of a queue.  It uses the  */
/*  PushReqPacket DevHlp call.                                               */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*  void var pull_req_pkt(queue,req)                                         */
/*  _32bits     *queue;                                                      */
/*  reqhdr_type *req;                                                        */
/*                                                                           */
/*  This function retrieves the next request packet from a queue.  It uses   */
/*  the PullReqPacket DevHlp call.  It stores the pointer to the request     */
/*  packet in req.                                                           */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/* void far dev_done(req,status)                                             */
/* reqhdr_type *req;                                                         */
/* word status;                                                              */
/*                                                                           */
/* dev_done - Set the status in the request packet, set the done bit and     */
/*            unblock the process.                                           */
/*                                                                           */
/* Only return code is SUCCESS.  This is the only return code from the       */
/* devhlp function that is called.                                           */
/*                                                                           */
/*****************************************************************************/
#include <string.h>
#include "demo.h"

static struct global_info_seg far * near g_info;

static byte * near err_msgs[] = {"$DEMO - Big Time ERROR" };
static byte near test_whether_ds_needed;

boolean near to_prot_mode(void)
{

  /* If we are in REAL mode  */
  if (real_mode() == TRUE) {

     /* Change to PROT mode */
     to_prot_moda();

     /* And say we did it */
     return(TRUE);
     }

  /* Otherwise, say we didn't */
  return(FALSE);
}

void far point_to_global()
{
  _32bits          pointer;     /* A scratch 32 bit value                   */
  union cpu_regs   in_regs;     /* Structure to hand to DevHlp caller       */
  union cpu_regs   out_regs;    /* Another one for return values            */
  unsigned        *intermd_ptr; /* Pointer to the Global Info Seg selector  */

  /* do a GetDOSVar - Global Info Segment - Get a pointer to the selector */
  in_regs.W.AX = 1;
  in_regs.W.DX = devhlp_GetDOSVar;
  in_regs.W.es_valid = FALSE;
  in_regs.W.ds_valid = FALSE;
  dev_help(&in_regs,&out_regs);

  /* AX:BX points to the selector of the Global Info Segment */
  pointer._2words.high = out_regs.W.AX;
  pointer._2words.low = out_regs.W.BX;
  intermd_ptr = (unsigned *)pointer.fptr;

  /* Make a pointer to the Global Info Segment from that selector :0 */
  pointer._2words.high = *intermd_ptr;
  pointer._2words.low = 0;
  g_info = (struct global_info_seg *)pointer.fptr;

}

unsigned long far curr_time()
{

  /* Get the time and return it */
  return(g_info->time_stuff.msec_since_IPL);
}

void far yield()
{
  _32bits temp;
  union cpu_regs in_regs;
  union cpu_regs out_regs;

  enable_irpt();                     /* Let irpts through while we sleep     */
  temp.phys = (physaddr)curr_time(); /* The block ID will be the current time*/
  in_regs.W.BX = temp._2words.low;
  in_regs.W.AX = temp._2words.high;
  in_regs.W.DI = 0;                  /* Block for 32 mSec                    */
  in_regs.W.CX = 32;
  in_regs.B.DH = 0;                  /* It is interruptable                  */
  in_regs.B.DL = devhlp_Block;
  in_regs.W.es_valid = FALSE;        /* DS and ES are not used               */
  in_regs.W.ds_valid = FALSE;
  dev_help(&in_regs,&out_regs);      /* Do it                                */
  return;
}

int far block(block_id, timeout)
unsigned long block_id;
unsigned long timeout;
{
  _32bits temp;
  union cpu_regs in_regs;
  union cpu_regs out_regs;

  temp.phys = (physaddr)block_id;    /* Set the Block ID                     */
  in_regs.W.BX = temp._2words.low;
  in_regs.W.AX = temp._2words.high;
  temp.phys = (physaddr)timeout;     /* Set the timeout                      */
  in_regs.W.DI = temp._2words.high;
  in_regs.W.CX = temp._2words.low;
  in_regs.B.DH = 1;                  /* It ain't interruptable               */
  in_regs.B.DL = devhlp_Block;
  in_regs.W.es_valid = FALSE;        /* DS and ES are not used               */
  in_regs.W.ds_valid = FALSE;

  /* MSF - disable interrupts too */
  disable_irpt();
  dev_help(&in_regs,&out_regs);      /* Do it                                */

  /* MSF - figure out why we were unblocked - timeout or someone ran unblock */
  if ((out_regs.W.flags & 0x0001) != 0)
    return(1); /* carry set */
  else
    return(0); /* carry not set */
}

void far unblock(block_id)
unsigned long block_id;
{
  _32bits temp;
  union cpu_regs in_regs;
  union cpu_regs out_regs;


  temp.phys = (physaddr)block_id;   /* Which one to unblock                 */
  in_regs.W.BX = temp._2words.low;
  in_regs.W.AX = temp._2words.high;
  in_regs.B.DL = devhlp_Run;        /* Say we want to wake him up           */
  in_regs.W.es_valid = FALSE;       /* DS and ES are not used               */
  in_regs.W.ds_valid = FALSE;
  dev_help(&in_regs,&out_regs);     /* Wake it                              */
  return;
}

void far internal_error(msg_num)
word msg_num;
{
  _32bits temp;
  union cpu_regs in_regs;
  union cpu_regs out_regs;

  temp.fptr = (void far *)err_msgs[msg_num];  /* Point at the message        */
  in_regs.W.SI = temp._2words.low;
  in_regs.W.DS = temp._2words.high;
  in_regs.W.DI = strlen(err_msgs[msg_num]);   /* Tell how big it is          */
  in_regs.B.DL = devhlp_InternalError;
  in_regs.W.es_valid = FALSE;                 /* DS is used, ES isn't        */
  in_regs.W.ds_valid = TRUE;
  dev_help(&in_regs,&out_regs);               /* Do it                       */
}

void far EOI(irpt_num)
word irpt_num;
{
  union cpu_regs in_regs;
  union cpu_regs out_regs;

  in_regs.B.AL = (byte)irpt_num;  /* Which IRQ to end                     */
  in_regs.B.DL = devhlp_EOI;
  in_regs.W.es_valid = FALSE;     /* DS and ES are not needed             */
  in_regs.W.ds_valid = FALSE;
  dev_help(&in_regs,&out_regs);

}

void far push_req_pkt(queue,req)
_32bits *queue;
reqhdr_type *req;
{
  _32bits temp;
  union cpu_regs in_regs;
  union cpu_regs out_regs;

  temp.fptr = (void far *)queue;        /* Point at the queue               */
  in_regs.W.SI = temp._segadr.offset;
  temp.fptr = (void far *)req;          /* Point at the Request packet      */
  in_regs.W.ES = temp._segadr.segment;
  in_regs.W.BX = temp._segadr.offset;
  in_regs.B.DL = devhlp_PushReqPacket;
  in_regs.W.es_valid = TRUE;            /* ES is used, DS isn't             */
  in_regs.W.ds_valid = FALSE;
  dev_help(&in_regs,&out_regs);         /* Do it                            */

}

void far pull_part_req_pkt(queue,req)
_32bits *queue;
reqhdr_type *req;
{
  _32bits temp;
  union cpu_regs in_regs;
  union cpu_regs out_regs;

  temp.fptr = (void far *)queue;        /* Point at the queue               */
  in_regs.W.SI = temp._segadr.offset;
  temp.fptr = (void far *)req;          /* Point at the request packet      */
  in_regs.W.ES = temp._segadr.segment;
  in_regs.W.BX = temp._segadr.offset;
  in_regs.B.DL = devhlp_PullParticular;
  in_regs.W.es_valid = TRUE;            /* ES is used, DS isn't             */
  in_regs.W.ds_valid = FALSE;
  dev_help(&in_regs,&out_regs);         /* Do it                            */

}

void far dev_done(req,status)
reqhdr_type *req;
word status;
{
  union cpu_regs in_regs,out_regs;
  _32bits pointer;

  /* Set the status field in the request packet to the requested value */
  req->rh_stat = status;

  pointer.phys = (physaddr)req;
  in_regs.W.BX = pointer._segadr.offset;   /* BX = offset of request packet  */
  in_regs.W.ES = pointer._segadr.segment;  /* ES = segment of request packet */
  in_regs.B.DL = devhlp_DevDone;           /* DL = Dev_done command          */
  in_regs.W.es_valid = TRUE;               /* ES has a valid selector        */
  in_regs.W.ds_valid = FALSE;              /* DS is not a valid selector     */
  dev_help(&in_regs,&out_regs);            /* Do it!                         */
  return;                                  /* We don't care what the return  */
                                           /*    code is because DevHlp does */
                                           /*    not return anything but OK  */
}


int far SetIRQ(irpt_num, intr_address)
word irpt_num;
void near *intr_address;
{
  union cpu_regs in_regs;
  union cpu_regs out_regs;
  _32bits  tempptr;
  int error;

  in_regs.W.AX = (unsigned) intr_address;
  in_regs.W.BX = (word)irpt_num;         /* Which IRQ to grab      */
  in_regs.B.DH = 0;                       /* Dont share IRQ */
  in_regs.B.DL = devhlp_SetIRQ;

  tempptr.fptr = (void far *)(&test_whether_ds_needed);
  in_regs.W.DS = tempptr._segadr.segment;
  in_regs.W.es_valid = FALSE;     /* ES is not needed             */
  in_regs.W.ds_valid = TRUE;
  dev_help(&in_regs,&out_regs);
  if ((out_regs.W.flags & 0x0001) != 0)
    error = 1; /* carry set */
  else
    error = 0; /* carry not set */
  return(error);          /* error code indicated by carry flag */

}

int far UnSetIRQ(irpt_num)
word irpt_num;
{
  union cpu_regs in_regs;
  union cpu_regs out_regs;
  _32bits  tempptr;
  int error;

  in_regs.W.BX = (word)irpt_num;         /* Which IRQ to grab      */
  in_regs.B.DH = 0;                       /* Dont share IRQ */
  in_regs.B.DL = devhlp_UnSetIRQ;

  tempptr.fptr = (void far *)(&test_whether_ds_needed);
  in_regs.W.DS = tempptr._segadr.segment;
  in_regs.W.es_valid = FALSE;     /* ES is not needed             */
  in_regs.W.ds_valid = TRUE;
  dev_help(&in_regs,&out_regs);
  if ((out_regs.W.flags & 0x0001) != 0)
    error = 1; /* carry set */
  else
    error = 0; /* carry not set */
  return(error);          /* error code indicated by carry flag */

}



/* convert unsigned int to a string, assumes buffer big enough */
/* returns length of string */
/* the string is zero terminated, but length DOES NOT include zero at end */
int near powten[]={1,10,100,1000,10000};

int ItoA(int num, char *buf)
{
  int i, j, len;

  /* do easy case */
  if (num==0)
    {
      buf[0]=' ';
      buf[1]='0';
      buf[2]='\0';
      return(2);
    }

 
  /* output sign */  
  if (num<0) 
    {
      buf[0]='-';
      num  = -num;
    }
  else
    buf[0] = ' ';
  

  /* now figure out rest of string. Output right justified string */
  len = 1;
  for (i=4; i>-1; i--)
    {
      j = num/powten[i];
      if ((j)||(len>1)) {
	buf[len] = (char)('0'+j);
	len++;
      }
      num -= j*powten[i];
    }

  /* zero terminate */
  buf[len]='\0';
  return(len);
}

