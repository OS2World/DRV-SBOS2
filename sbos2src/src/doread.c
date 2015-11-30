/* This handles DMA to SB */

#include "demo.h"

int far do_read(req_hdr)
struct rw_hdr_def *req_hdr;
{
  _32bits dmaphysaddr, dmaldtptr;
  byte *sptr;
  unsigned int  len, i;

  /* copy data from user buffer into my buffer */
  /* need to convert the physical address passed by the user into */
  /* a virtual address (LDT) that the device driver can use */
  dmaphysaddr.phys = req_hdr->xfer_address;
  len = req_hdr->count;
  dmaldtptr = phys_to_ldt(dmaphysaddr, len);

  /* dmaldtptr now points at user buffer, so fill with #'s */
  for (sptr=(byte far *)dmaldtptr.fptr,i=0; i<len; i++, sptr++)
    *sptr = (byte)i;


  /* release selector mapped to user buffer */
  free_virt(dmaldtptr._segadr.segment);

  return(DONE);
}


