/*****************************************************************************/
/* This file contains function prototypes for all functions in the DD        */
/*****************************************************************************/

/* Debugger breakpoint function */
extern void far breakpoint(void);

/* OS/2 Request packet command processors */
extern word far bad_cmd(void);
extern word far init_dsp( struct init_hdr_in *);
extern word far init_mix( struct init_hdr_in *);
extern int far dmaout( struct rw_hdr_def *);
extern int far do_read( struct rw_hdr_def *);
extern int near strategy_dsp(struct reqhdr *);
extern int near strategy_mix(struct reqhdr *);

/* Enable and disable interrupts */
extern void far enable_irpt(void);
extern void far disable_irpt(void);

/* Print a message on the display (INIT time only) */
extern unsigned far prt_msg(unsigned);
extern void far make_msg_fname( char *);

/* Get the current Process ID (PID) */
extern unsigned far get_PID(void);

/* Get the next command number */
extern unsigned long far get_cmd_num(void);

/* Port input and output routines   */
extern void far out_port(word,word);  /* Address first, data second */
extern word far in_port(word);

/* Block/Unblock functions   */
extern int  far   block(unsigned long,    /* Block ID                    */
                        unsigned long);   /* Timeout (-1 is forever)     */
extern void far unblock(unsigned long);   /* Block ID                    */

/* Create a GDT descriptor for a physical address                         */
/* Returns 0:0 if unsuccessful                                            */
extern _32bits far phys_to_gdt(_32bits,               /* Physical address */
                               word,                  /* Size of block    */
                               word);                 /* Selector number  */


/* Create an LDT descriptor for a physical address                        */
/* Returns 0:0 if unsuccessful                                            */
extern _32bits far phys_to_ldt(_32bits,               /* Physical address */
                         unsigned);                   /* Size of block    */


/* Create an LDT descriptor for a phys addr (uses far call to devhlp)     */
/* Returns 0:0 if unsuccessful                                            */
extern _32bits far phys_to_ldt1(_32bits,              /* Physical address */
                         unsigned);                   /* Size of block    */


/* Get the physical address of a segment                                  */
/* Returns 0 if unsuccessful                                              */
extern _32bits far get_phys_addr(_32bits );           /*  Virtual address */

/* Get the physical address of a segment    (uses far call to devhlp)     */
/* Returns 0 if unsuccessful                                              */
extern _32bits far get_phys_addr1(_32bits );          /*  Virtual address */


/* Verify application access to a segment                                 */
/* Returns SUCCESS if OK                                                  */
/* Note that this function will cause the application to be stopped with  */
/* a TRAP 000D popup if the verivy fails.                                 */
extern word far verify_acc(_32bits ,             /* Virtual Address       */
                           word,                /* Size                   */
                           word);               /* Access type 0=RD,1=R/W */

/* Allocate a block of memory and create a GDT descriptor for it          */
/* Returns pointer to memory, 0:0 if unsuccessful.  It needs a previously */
/* allocated GDT selector to work with.                                   */
extern _32bits far alloc_gdt_mem(word,                   /* Size of block */
                                 word );                 /* Selector      */


/* free previously allocated GDT selector via get_gdt_slots() */
extern word far free_gdt( word );

/* Return a previously allocated block of memory to OS/2's pool           */
extern word far free_mem(_32bits);                    /* Physical address */


/* Allocate a block of memory and return the physical address, returns    */
/* 0L if unsuccessful                                                     */
extern _32bits far alloc_mem(word);                      /* Size of block */
extern _32bits far alloc_big_mem(unsigned long);         /* Size of block */

/* Free an LDT created by PHYS_TO_LDT or PHYS_TO_LDT1                     */
extern int far free_virt(unsigned);                  /* Selector of block */


/* Allocate GDT slots for use by this DD.  Only valid at INIT time        */
extern word far get_gdt_slots(word,           /* number of slots          */
                              _32bits);       /* Array to store selectors */


/* Go to real or protect mode.                                            */
extern boolean near to_prot_mode(void);
extern void near to_real_mode(void);
extern void near to_prot_moda(void);
extern void near to_real_moda(void);

/* Get the millisec since IPL.                                            */
extern void far point_to_global(void);
extern unsigned long far curr_time(void);

/* Determine the current CPU mode, returns TRUE if currently in REAL mode,*/
/* returns FALSE if currently in PROTECT mode.                            */
extern boolean far real_mode(void);

/* Call the DevHlp function.                                               */
/* dev_help1 uses a far return                                             */
extern unsigned far dev_help ( union cpu_regs *, /* Input register values  */
                           union cpu_regs *);    /* Output register values */

extern unsigned far dev_help1(union cpu_regs *, /* Input register values  */
                          union cpu_regs *);    /* Output register values */

/* Yield the CPU for 32 millisec */
extern void far yield(void);

/* Signal an internal error */
extern void far internal_error(word);           /* Message number         */

/* Signal End of Interrupt to OS/2 */
extern void far EOI(word);                      /* Interrupt number       */

/* Set up interupt handler */
int far SetIRQ(word irpt_num, void near *intr_address);
int far UnSetIRQ(word irpt_num);

/* Request Packet processing */
extern void far push_req_pkt(_32bits *,         /* Pointer to queue       */
                             reqhdr_type *);    /* Request Packet         */
extern void far pull_part_req_pkt(_32bits *,    /* Pointer to queue       */
                             reqhdr_type *);    /* Request Packet         */
extern void far dev_done(reqhdr_type *,         /* Request Packet         */
                         word);                 /* Status To give         */

/* Lock/UnLock functions */
extern word far unlock(unsigned long);
extern unsigned long far lock(word);

/* lib C type functions I've written (since I dont want to call MSC library */
int ItoA( int, char *);


/* API Functions */
unsigned int pascal DosWrite(unsigned int, void *, unsigned int, unsigned int *);
unsigned int pascal DosRead( unsigned int, void *, unsigned int, unsigned int *);
