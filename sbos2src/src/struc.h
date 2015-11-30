/*****************************************************************************/
/* This file contains all internal structures, unions, and typedefs used by  */
/* the DD.                                                                   */
/*****************************************************************************/
/*****************************************************************************/
/* Definition of a generic 32 bit value.  It can be a far pointer, 2 words   */
/* A segmented address or a 32 bit unsigned integer (probably a phys addr)   */
/*****************************************************************************/
#ifndef NEWSCALARS
   typedef unsigned      word;
   typedef unsigned char byte;
   typedef word          boolean;
   #define NEWSCALARS 1
   #endif

typedef unsigned long physaddr;  /* A physical address is 4 bytes */
typedef void far * farptr;

typedef struct _2words_def {
  unsigned low;
  unsigned high;
  } _2words_type;

typedef struct _segaddr_def {
  unsigned offset;
  unsigned segment;
  } _segadr_type;

typedef union _32bits_def {
   physaddr phys;
   void far * fptr;
   _2words_type _2words;
   _segadr_type _segadr;
   } _32bits;


/*****************************************************************************/
/* Definition of a the various request packets                               */
/*****************************************************************************/
struct reqhdr {
   byte           rh_len;
   byte           rh_unit;
   byte           rh_cmd;
   word           rh_stat;
   byte          *rh_resrvd;
   struct reqhdr *rh_next;
   };

typedef struct reqhdr reqhdr_type;

struct init_hdr_in {
   struct reqhdr  request_hdr;
   byte           data1;
   _32bits        pointer1;
   byte          *pointer2;
   byte           data2;
   };

struct init_hdr_out {
   struct reqhdr  request_hdr;
   byte           data1;
   word           code_end;
   word           data_end;
   byte          *pointer2;
   byte           data2;
   };

struct ioctl_hdr_in {
   struct reqhdr  request_hdr;
   byte           funct_cat;
   byte           funct_cod;
   _32bits        pbuffer;
   _32bits        dbuffer;
   word           file_number;
   word           plen;
   word           dlen;
   };
typedef struct ioctl_hdr_in ioctl_hdr;

struct close_hdr_def {
   byte           rh_len;
   byte           rh_unit;
   byte           rh_cmd;
   word           rh_stat;
   byte          *rh_resrvd;
   struct reqhdr *rh_next;
   word           file_number;
   };
typedef struct close_hdr_def close_hdr;

struct rw_hdr_def {
   byte            rh_len;
   byte            rh_unit;
   byte            rh_cmd;
   word            rh_stat;
   byte           *rh_resrvd;
   struct reqhdr  *rh_next;
   byte            media;
/*   byte           *xfer_address; */
   unsigned long   xfer_address;  /* changed by MSF */
   word            count;
   unsigned long   start;
   word            file_number;
   };
typedef struct rw_hdr_def rw_hdr;

/*****************************************************************************/
/*                                                                           */
/* Structures used by Global Info and Local Info calls                       */
/*                                                                           */
/*****************************************************************************/
struct cur_time {
   unsigned long sec_1970;
   unsigned long msec_since_IPL;
   unsigned char hours;
   unsigned char minutes;
   unsigned char seconds;
   unsigned char hundredths;
   unsigned timezone;
   unsigned timer_interval;
   };

struct cur_date {
   unsigned char day;
   unsigned char month;
   unsigned year;
   };

struct cur_vers {
   unsigned char major;
   unsigned char minor;
   unsigned char revision;
   };

struct cur_stat {
   unsigned char max_sessions;
   unsigned char huge_shift_cnt;
   unsigned char prot_mode;
   unsigned PID_of_last_charin;
   };

struct cur_sched {
   unsigned char dynamic_var;
   unsigned char max_wait;
   unsigned min_timeslice;
   unsigned max_timeslice;
   };

struct global_info_seg {
   struct cur_time time_stuff;
   struct cur_date date_stuff;
   struct cur_vers vers_stuff;
   unsigned char foreground;
   struct cur_stat stat_stuff;
   struct cur_sched sched_stuff;
   unsigned char boot_drive;
   unsigned char trace_flags[4];
   };


/*****************************************************************************/
/*                                                                           */
/* Structures used by dev_helper function interface                          */
/*                                                                           */
/*****************************************************************************/
struct regs {
   unsigned AX;
   unsigned BX;
   unsigned CX;
   unsigned DX;
   unsigned SI;
   unsigned DI;
   unsigned ES;
   unsigned DS;
   unsigned CS;
   unsigned flags;
   char  es_valid;
   char  ds_valid;
   };

struct regs_B {
   unsigned char AL, AH;
   unsigned char BL, BH;
   unsigned char CL, CH;
   unsigned char DL, DH;
   unsigned SI;
   unsigned DI;
   unsigned ES;
   unsigned DS;
   unsigned CS;
   unsigned flags;
   char  es_valid;
   char  ds_valid;
   };

union cpu_regs {
   struct regs W;
   struct regs_B B;
   };


/*****************************************************************************/
/*                                                                           */
/* Structures used by ABIOS interface                                        */
/*                                                                           */
/*****************************************************************************/
typedef struct function_parms_def {
   unsigned req_blk_len;
   unsigned LID;
   unsigned unit;
   unsigned function;
   unsigned resvd1;
   unsigned resvd2;
   unsigned ret_code;
   unsigned time_out;
   } function_parms_type;

typedef struct service_parms_def {
   byte slot_num;        /* 10h */
   byte resvd3;          /* 11h */
   word card_ID;         /* 12h */
   word resvd4;          /* 14h */
   byte *pos_buf;        /* 16h */
   word resvd5;          /* 1Ah */
   word resvd6;          /* 1Ch */
   byte resvd7[40];      /* 1Eh */
   } service_parms_type;

typedef struct lid_service_parms_def {
   byte irpt_level;          /* 10h */
   byte arb_level;           /* 11h */
   unsigned device_id;       /* 12h */
   unsigned unit_count;      /* 14h */
   unsigned flags;           /* 16h */
   unsigned blk_size;        /* 18h */
   unsigned secnd_id;        /* 1Ah */
   unsigned resvd6;          /* 1Ch */
   unsigned resvd7;          /* 1Eh */
   } lid_service_parms_type;

typedef struct req_block_def {
   function_parms_type f_parms;
   service_parms_type  s_parms;
   } req_block_type;

typedef struct lid_block_def {
   function_parms_type     f_parms;
   lid_service_parms_type  s_parms;
   } lid_block_type;


/*****************************************************************************/
/*                                                                           */
/* Structure returned by DOSGetLocalInfo                                     */
/*                                                                           */
/*****************************************************************************/
struct local_info_seg {
   unsigned current_PID;
   unsigned parent_PID;
   unsigned priority;
   unsigned thread_ID;
   unsigned session;
   unsigned unused;
   unsigned keyboard_focus;
   unsigned DOS_mode;
   };

