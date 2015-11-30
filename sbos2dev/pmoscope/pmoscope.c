#define INCL_GPILOGCOLORTABLE
#define INCL_WINFRAMEMGR
#define INCL_WINSYS
#define INCL_WINSCROLLBARS
#define INCL_GPI
#define INCL_WINDIALOGS

#define INCL_DOSSEMAPHORES
#include <os2.h>
#include "sblast_user.h"
#include "pmoscope.h"

HEV  hevDisplayData = 0;
HWND hwndFrame;
HAB       hab;
QMSG      qmsg;
HMQ       hmq;
HWND      hwnd;

#pragma linkage (MainWndProc, system)         /* for ICC */


void Error_and_exit( char *, int);


#define MAX_BUFFER_SIZE 4096

int    npts;
POINTL pnts[MAX_BUFFER_SIZE];
POINTL opnts[MAX_BUFFER_SIZE];

int xwidth, yheight;
int threadid;

int sample_speed;
int sample_size;

HFILE audiohandle=0;


/* this procedure produces 1 new point per second, then sends a WM_PAINT */
/* to make screen refresh                                                */
void create_points(void)
{

  int x, i, first;
  RECTL rct;
  ULONG postcnt, numread;
  APIRET rc;
  unsigned char buf[MAX_BUFFER_SIZE];


  npts=0;
  first=1;

  for(;1;)
    {
      /* Wait for display semaphore, one sec timeout */
      rc=DosWaitEventSem( hevDisplayData, 1000);

      if (rc==0)
	{      
	  /* read in new points */
	  DosRead(audiohandle, &buf, sample_size, &numread);
	  for (i=0; i<sample_size; i++)
	    {
	      opnts[i].x=pnts[i].x;
	      opnts[i].y=pnts[i].y;
	      pnts[i].x = (i*xwidth)/sample_size;
	      x=(buf[i]*yheight)/256;
	      pnts[i].y = x;
	      if (first)
		{
		  opnts[i].x=pnts[i].x;
		  opnts[i].y=pnts[i].y;
		}
	    }
	  first=0;
	  npts=sample_size;
	  DosResetEventSem( hevDisplayData, &postcnt);
	  WinInvalidateRect(hwnd, NULL, FALSE);
	}
    }
}





static MRESULT MainWndProc(hwnd, msg, mp1, mp2)
     HWND          hwnd;
     ULONG        msg;
     MPARAM        mp1;
     MPARAM        mp2;
{
  RECTL      rectl, txtrectl;
  HPS        hps;
  POINTL     pnt;
  int        i;


  switch(msg)
    {
    case WM_ERASEBACKGROUND:
      return( (MRESULT) TRUE);
      break;

    case WM_SIZE:
      /* need to resize array and set global vars */
      for (i=0; i<npts; i++)
	{
	  pnts[i].x = (pnts[i].x*SHORT1FROMMP(mp2))/SHORT1FROMMP(mp1);
	  pnts[i].y = (pnts[i].y*SHORT2FROMMP(mp2))/SHORT2FROMMP(mp1);
	  xwidth  = SHORT1FROMMP(mp2);
	  yheight = SHORT2FROMMP(mp2);
	}
      break;


    case WM_PAINT:
      hps=WinBeginPaint(hwnd, NULL, (PRECTL) &rectl);


      if (npts>1)
	{
	  GpiSetColor(hps,CLR_BACKGROUND);
	  GpiMove(hps, &opnts[0]);
	  GpiPolyLine( hps, npts-1, &opnts[1]); 
	  GpiSetColor(hps,CLR_RED);
	  GpiMove(hps, &pnts[0]);
	  GpiPolyLine( hps, npts-1, &pnts[1]); 
	}

      
      /* let create_points know its ok to go on */
      DosPostEventSem( hevDisplayData );

      (void)WinEndPaint(hps);
      break;

    default:
      return(WinDefWindowProc(hwnd, msg,mp1,mp2));
      break;
    }
  return(0L);
}








/* this setups up audio stuff */
void init_audio( void )
{
  ULONG status, action, datlen, parlen;
  USHORT speed;

  /* first try to open sound blaster */
  status = DosOpen( "SBDSP$", &audiohandle, &action, 0,
		   FILE_NORMAL, FILE_OPEN,
   OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYNONE | OPEN_FLAGS_WRITE_THROUGH,
			  NULL );

  if (status != 0)
    Error_and_exit("Error opening sound device SBDSP$, exiting.",TRUE);


  /* Now setup speed, make sure we wont get errors on overflow */
  datlen=2;
  parlen=0;
  speed=(USHORT) sample_speed;
  status=DosDevIOCtl(audiohandle, DSP_CAT, DSP_IOCTL_SPEED,
                    NULL, 0, &parlen,
                    &speed, datlen, &datlen);
  if (status != 0)
    Error_and_exit("Error setting sampling speed, exiting...", TRUE);

  /* now turn off overflow error */
  datlen=2;
  parlen=0;
  speed=0;
  status=DosDevIOCtl(audiohandle, DSP_CAT, DSP_IOCTL_OVERRUN_ERRNO,
                    NULL, 0, &parlen,
                    &speed, datlen, &datlen);
  if (status != 0)
    Error_and_exit("Error setting overflow error, exiting...",TRUE);

  /* now set buffer length to 512 */
  datlen=2;
  parlen=0;
  speed=(USHORT) sample_size;
  status=DosDevIOCtl(audiohandle, DSP_CAT, DSP_IOCTL_BUFSIZE,
                    NULL, 0, &parlen,
                    &speed, datlen, &datlen);
  if (status != 0)
    Error_and_exit( "Error setting DMA buffer size, exiting...", TRUE);
  
}





void Error_and_exit( char * tempstr, int error )
{
  ULONG mbtype;

  if (error)
    mbtype = MB_ERROR;
  else
    mbtype = MB_INFORMATION;

  /* open up informative dialog box */
  WinMessageBox(HWND_DESKTOP, hwndFrame, tempstr, NULL,
		HELP_MSG_WIN, MB_OK | mbtype );

  if (hevDisplayData)
    DosCloseEventSem(hevDisplayData);

  if (audiohandle)
    DosClose(audiohandle);

  WinDestroyWindow(hwndFrame);
  WinDestroyMsgQueue(hmq);
  WinTerminate(hab);
  exit(0);
}




int main(argc, argv)
     int argc;
     char *argv[];
{
  char      szTitle[16];
  char      tempstr[100];
  ULONG     flCreate = FCF_TITLEBAR | FCF_SYSMENU | FCF_SIZEBORDER |
                       FCF_MINMAX | FCF_SHELLPOSITION | FCF_TASKLIST |
		       FCF_ICON;

  RECTL     rectl;

  hab = WinInitialize(QV_OS2);

  hmq = WinCreateMsgQueue( hab, 0);

  (void)WinLoadString(hab, 0L, IDS_TITLE, sizeof(szTitle), (PSZ) szTitle);

  if (!WinRegisterClass(hab, szTitle, MainWndProc, 
			CS_SIZEREDRAW, 0L))
    return(0);
      
  hwndFrame = WinCreateStdWindow((HAB)hab,
				 0L,
				 (PVOID) &flCreate,
				 (PSZ) szTitle,
				 (PSZ) NULL,
				 0L,
				 (HMODULE) NULL,
				 (USHORT) ID_MAIN,
				 (PHWND) &hwnd);


  WinSetWindowText(WinWindowFromID (hwndFrame, FID_TITLEBAR), (PSZ) szTitle);


  /* resize screen to reasonable size */
  WinSetWindowPos(hwndFrame, NULL, 0, 0,
		  300,
		  150,
		  SWP_SIZE | SWP_SHOW);


  WinShowWindow(hwndFrame, TRUE);

  /* get current size */
  /* now figure out the size of current font */
  WinQueryWindowRect(hwnd, &rectl);
  xwidth=rectl.xRight-rectl.xLeft;
  yheight=rectl.yTop-rectl.yBottom;

  /* now scan commandline params */
  if (argc==1)
    {
      sprintf(tempstr, 
	      "Usage:  pmoscope  rate  bufsize\nMax bufsize is %d.\n",
	      MAX_BUFFER_SIZE);
      Error_and_exit(tempstr, FALSE);
    }



  /* parse speed, sample size to use */
  sample_speed=(USHORT) atoi(argv[1]);

  if (argc >= 3)
    sample_size=(USHORT) atoi(argv[2]);
  else
    sample_size = 512;

  /* make sure value for buffer is legal */
  if (sample_size > MAX_BUFFER_SIZE)
    {
      sprintf(tempstr, "Maximum buffer size is %d\n",MAX_BUFFER_SIZE);
      Error_and_exit(tempstr, TRUE);
    }


  /* init SB */
  init_audio();

  /* create event semaphore */
  DosCreateEventSem("\\SEM32\\DISPLAYDATA", &hevDisplayData, 0, FALSE);

  /* start up other thread */
  threadid = _beginthread(create_points, NULL, 0x4000);
  if (threadid == -1)
    {
      sprintf(tempstr, "Error creating sampling thread, exiting...\n");
      Error_and_exit(tempstr, TRUE);
    }


  /* start message loop */
  while(WinGetMsg(hab, (PQMSG)&qmsg,(HWND)NULL,0,0))
    WinDispatchMsg(hab,(PQMSG)&qmsg);

  DosCloseEventSem(hevDisplayData);

  WinDestroyWindow(hwndFrame);
  WinDestroyMsgQueue(hmq);
  WinTerminate(hab);

  DosClose(audiohandle);
}

