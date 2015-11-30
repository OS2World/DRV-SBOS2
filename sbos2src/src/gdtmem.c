/******************************************************************************/
/* Routines for managing GDT accessed memory                                  */
/*                                                                            */
/* -------------------------------------------------------------------------- */
/*                                                                            */
/* unsigned int get_gdt_slots(count,loc)                                      */
/* int count;                                                                 */
/* char *loc;                                                                 */
/*                                                                            */
/* This function will allocate count GDT selectors and store the selector     */
/* values in the array pointed to by loc.  If successful, it returns 0.  If   */
/* unsuccessful, it returns 1.                                                */
/*                                                                            */
/* -------------------------------------------------------------------------- */
/*                                                                            */
/* char *phys_to_gdt(loc,count,sel)                                           */
/* _32bits loc;                                                              */
/* unsigned count; (0 = 65536)                                                */
/* unsigned sel;                                                              */
/*                                                                            */
/* This function will make a physical block of memory addressable via the GDT.*/
/* It makes the GDT selector sel point to a valid descriptor for the memory   */
/* block at loc of size count.  It returns a valid far pointer to the memory  */
/* if successful, 0000:0000 if failure.                                       */
/*                                                                            */
/* -------------------------------------------------------------------------- */
/*                                                                            */
/* _32bits get_phys_addr(addr)                                               */
/* char *addr;                                                                */
/*                                                                            */
/* This function will return the physical address of the virtual address      */
/* passed as a paramter.  If the address is invalid, it returns 00000000.     */
/*                                                                            */
/* -------------------------------------------------------------------------- */
/*                                                                            */
/* char *alloc_gdt_mem(count,sel)                                             */
/* unsigned count;                                                            */
/* unsigned sel;                                                              */
/*                                                                            */
/* This function will allocate a block of memory from the free pool and make  */
/* it addressable via the GDT.  The block size is specified by count and the  */
/* GDT selector to use is sel.  NOTE: The GDT selector must have been         */
/* previously allocated via get_gdt_slots.  The return value is a valid       */
/* far pointer to the memory block if successful or 0000:0000 if failure.     */
/*                                                                            */
/* -------------------------------------------------------------------------- */
/*                                                                            */
/* char *alloc_mem(count)                                                     */
/* unsigned count;                                                            */
/*                                                                            */
/* This function will allocate a block of memory from the free pool and       */
/* return its physical address.  If the allocate fails, it returns 00000000   */
/*                                                                            */
/* -------------------------------------------------------------------------- */
/*                                                                            */
/* char *alloc_big_mem(count)                                                 */
/* unsigned long count;                                                       */
/*                                                                            */
/* This function will allocate a big block of memory from the free pool and   */
/* return its physical address.  If the allocate fails, it returns 00000000   */
/*                                                                            */
/* -------------------------------------------------------------------------- */
/*                                                                            */
/* int free_mem(address)                                                      */
/* _32bits address;                                                          */
/*                                                                            */
/* This function will give the block of memory starting at address back to    */
/* OS/2.  If the free is successful, the return value is 0.  If it fails,     */
/* the return value is -1.                                                    */
/*                                                                            */
/* -------------------------------------------------------------------------- */
/*                                                                            */
/* int verify_acc(addr,size,acc_type)                                         */
/* unsigned *addr;                                                            */
/* unsigned size;                                                             */
/* unsigned acc_type;                                                         */
/*                                                                            */
/* This function will verify the current LDT access of the segment.  It       */
/* returns -1 if access is denied and 0 OK.                                   */
/*                                                                            */
/******************************************************************************/
#include "demo.h"

word far get_gdt_slots(word count, _32bits loc)
{
  union cpu_regs in_regs,out_regs;

  in_regs.W.ES = loc._segadr.segment;      /* ES:DI point to the GDT slot    */
  in_regs.W.DI = loc._segadr.offset;       /*  array to be filled by OS/2    */
  in_regs.W.CX = count;                    /* How many GDT slots to get      */
  in_regs.B.DL = devhlp_AllocGDTSelector;  /* The DevHlp command             */
  in_regs.W.es_valid = TRUE;               /* Use the struc value of ES      */
  in_regs.W.ds_valid = FALSE;              /* Use the register value of DS   */
  dev_help(&in_regs,&out_regs);            /* GO                             */
  if ((out_regs.W.flags & 0x0001) != 0) {  /* Check for failure              */
     return(1);
     }
  return(0);                               /* Success return path            */
}

_32bits far phys_to_gdt( _32bits loc, word count, word sel)
{
  union cpu_regs in_regs,out_regs;
  _32bits pointer;

  in_regs.W.AX = loc._2words.high;         /* AX:BX hold the phys address    */
  in_regs.W.BX = loc._2words.low;
  in_regs.W.SI = sel;                      /* The selector to use            */
  in_regs.W.CX = count;                    /* The size of the segment (0=64K)*/
  in_regs.B.DL = devhlp_PhysToGDTSelector; /* The DevHlp command             */
  in_regs.W.es_valid = FALSE;              /* Neither DS or ES in the struct */
  in_regs.W.ds_valid = FALSE;              /*   is a valid selector          */
  dev_help(&in_regs,&out_regs);            /* Do it                          */
  if ((out_regs.W.flags & 0x0001) != 0) {  /* Check for failure              */
     pointer.phys = 0L;                    /* Return a NULL pointer if fail  */
     return(pointer);
     }
  pointer._2words.high = sel;              /* Return a valid pointer if OK   */
  pointer._2words.low = 0;
  return(pointer);
}

_32bits far alloc_gdt_mem( word count, word sel)
{
  _32bits phys_addr;

  /* allocate the memory */
  phys_addr = alloc_mem(count);
  if (phys_addr.phys == 0L) {
     return(phys_addr);
     }

  /* Associate the GDT selector with the memory */
  return(phys_to_gdt(phys_addr,count,sel));

}

_32bits far alloc_mem(word count)
{
  union cpu_regs in_regs,out_regs;
  _32bits pointer;

  /* allocate the memory */
  in_regs.W.AX = 0;
  in_regs.W.BX = count;
  in_regs.B.DH = 0;
  in_regs.B.DL = devhlp_AllocPhys;
  in_regs.W.es_valid = FALSE;
  in_regs.W.ds_valid = FALSE;
  dev_help(&in_regs,&out_regs);
  if ((out_regs.W.flags & 0x0001) != 0) {
     pointer.phys = 0L;
     return(pointer);
     }
  pointer._2words.high = out_regs.W.AX;
  pointer._2words.low = out_regs.W.BX;
  return(pointer);

}

_32bits far alloc_big_mem( unsigned long count)
{
  union cpu_regs in_regs,out_regs;
  _32bits temp;
  _32bits pointer;

  temp.phys = count;

  /* allocate the memory */
  in_regs.W.AX = temp._2words.high;
  in_regs.W.BX = temp._2words.low;
  in_regs.B.DH = 0; /* always allocate above 1 meg line */
  in_regs.B.DL = devhlp_AllocPhys;
  in_regs.W.es_valid = FALSE;
  in_regs.W.ds_valid = FALSE;
  dev_help(&in_regs,&out_regs);
  if ((out_regs.W.flags & 0x0001) != 0) {
     pointer.phys = 0L;
     return(pointer);
     }
  pointer._2words.high = out_regs.W.AX;
  pointer._2words.low = out_regs.W.BX;
  return(pointer);

}

word far free_mem( _32bits address)
{
  union cpu_regs in_regs,out_regs;

  /* allocate the memory */
  in_regs.W.AX = address._2words.high;
  in_regs.W.BX = address._2words.low;
  in_regs.B.DL = devhlp_FreePhys;
  in_regs.W.es_valid = FALSE;
  in_regs.W.ds_valid = FALSE;
  dev_help(&in_regs,&out_regs);
  if ((out_regs.W.flags & 0x0001) != 0) {
     return(-1);
     }
  return(0);

}

_32bits far get_phys_addr( _32bits addr)
{
  union cpu_regs in_regs,out_regs;
  _32bits pointer;

  /* allocate the memory */
  in_regs.W.SI = addr._segadr.offset;
  in_regs.W.DS = addr._segadr.segment;
  in_regs.B.DL = devhlp_VirtToPhys;
  in_regs.W.es_valid = FALSE;
  in_regs.W.ds_valid = TRUE;
  dev_help(&in_regs,&out_regs);
  if ((out_regs.W.flags & 0x0001) != 0) {
     pointer.phys = 0L;
     return(pointer);
     }
  pointer._2words.high = out_regs.W.AX;
  pointer._2words.low = out_regs.W.BX;
  return(pointer);

}

_32bits far get_phys_addr1( _32bits addr)
{
  union cpu_regs in_regs,out_regs;
  _32bits pointer;

  /* allocate the memory */
  in_regs.W.SI = addr._segadr.offset;
  in_regs.W.DS = addr._segadr.segment;
  in_regs.B.DL = devhlp_VirtToPhys;
  in_regs.W.es_valid = FALSE;
  in_regs.W.ds_valid = TRUE;
  dev_help1(&in_regs,&out_regs);
  if ((out_regs.W.flags & 0x0001) != 0) {
     pointer.phys = 0L;
     return(pointer);
     }
  pointer._2words.high = out_regs.W.AX;
  pointer._2words.low = out_regs.W.BX;
  return(pointer);

}

word far verify_acc( _32bits addr, word size, word acc_type)
{
  union cpu_regs in_regs,out_regs;

  /* allocate the memory */
  in_regs.W.CX = size;
  in_regs.W.DI = addr._segadr.offset;
  in_regs.W.AX = addr._segadr.segment;
  in_regs.B.DH = (byte)acc_type;
  in_regs.B.DL = devhlp_VerifyAccess;
  in_regs.W.es_valid = FALSE;
  in_regs.W.ds_valid = FALSE;
  dev_help(&in_regs,&out_regs);
  if ((out_regs.W.flags & 0x0001) != 0) {
     return(-1);
     }
  return(0);

}


word far free_gdt( word sel)
{
  union cpu_regs in_regs,out_regs;

  /* allocate the memory */
  in_regs.W.AX = sel;
  in_regs.B.DL = devhlp_FreeGDTSelector;
  in_regs.W.es_valid = FALSE;
  in_regs.W.ds_valid = FALSE;
  dev_help(&in_regs,&out_regs);
  if ((out_regs.W.flags & 0x0001) != 0) {
     return(-1);
     }
  return(0);

}

