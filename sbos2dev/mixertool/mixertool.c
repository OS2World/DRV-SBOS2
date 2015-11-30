#define INCL_GPILOGCOLORTABLE
#define INCL_WINFRAMEMGR
#define INCL_WINSYS
#define INCL_WINSCROLLBARS

#include <stdio.h>
#include <stdlib.h>
#include <os2.h>
#include "sblast_user.h"
#include "mixertool.h"


HFILE mixer_handle;
struct sb_mixer_levels sblevels;
struct sb_mixer_params sbparams;

#define ID_BUT1 100

HWND hwndFrame;
HWND hwndScroll[9];
HWND hwndButton[NUM_BUTTONS];
HWND hwndPbutton;


QMSG      qmsg;
HMQ       hmq;
HWND      hwnd;
HPS       hps;



HAB  hab;
FLAG srcsel[3];


/* these procedures deal with calculating the position of various scrollbars
 * and their associated text 
 */

/* stuff controlling layout of scrollbars */
#define L_R_SEP 1
#define SB_SEP  2
#define SB_WID  2
#define SB_LEN  5
#define XBORDER  3
#define YBORDER_BOT  2
#define YBORDER_TOP  4

/* store the average width, height of characters */
USHORT fwidth, fheight;
USHORT borderx, bordery;

/* This function returns the position of the lower left corner of
 * the slider specified by slideID.
 *
 * Assumes global variables fwidth, fheight have been set
 */
void GetSliderPos(USHORT slideID, POINTL * pnt)
{
  /* first see if its part of a stereo slider pair */
  if (slideID < SCR_CD)
    {
      /* compute position of left channel slider first */
      pnt->x=(XBORDER+(slideID/2)*(2*SB_WID+SB_SEP+L_R_SEP))*fwidth;
      /* if its the right channel add in left-right separation */
      if (slideID & 1)
	pnt->x += (SB_WID+L_R_SEP)*fwidth;
      pnt->y=fheight*YBORDER_BOT;
    }
  else
    {
      /* compute position for CD slider */
      pnt->x=(XBORDER+(slideID/2)*(2*SB_WID+SB_SEP+L_R_SEP))*fwidth;
      /* if its the mic slider add slider separation */
      if (slideID&1)
	pnt->x += (SB_WID+SB_SEP)*fwidth;
      pnt->y=fheight*YBORDER_BOT;
    }
}



/* these procedures handle the mixer levels */
void StoreMixerLevel( USHORT slideID, USHORT mixlevel, 
		     struct sb_mixer_levels * sblev)
{
  switch (slideID)
    {
    case 0:
      sblev->master.l = (BYTE) (15-mixlevel);
      break;
    case 1:
      sblev->master.r = (BYTE) (15-mixlevel);
      break;
    case 2:
      sblev->voc.l = (BYTE) (15-mixlevel);
      break;
    case 3:
      sblev->voc.r = (BYTE) (15-mixlevel);
      break;
    case 4:
      sblev->fm.l = (BYTE) (15-mixlevel);
      break;
    case 5:
      sblev->fm.r = (BYTE) (15-mixlevel);
      break;
    case 6:
      sblev->line.l = (BYTE) (15-mixlevel);
      break;
    case 7:
      sblev->line.r = (BYTE) (15-mixlevel);
      break;
    case 8:
      sblev->cd.r = sblev->cd.l = (BYTE) (15-mixlevel);
      break;
    case 9:
      sblev->mic = (BYTE) (7-mixlevel);
      break;
    default:
      break;
    }
}


USHORT GetMixerLevel( USHORT slideID, struct sb_mixer_levels sblev)
{
  switch (slideID)
    {
    case 0:
      return 15-sblev.master.l;
      break;
    case 1:
      return 15-sblev.master.r;
      break;
    case 2:
      return 15-sblev.voc.l;
      break;
    case 3:
      return 15-sblev.voc.r;
      break;
    case 4:
      return 15-sblev.fm.l;
      break;
    case 5:
      return 15-sblev.fm.r;
      break;
    case 6:
      return 15-sblev.line.l;
      break;
    case 7:
      return 15-sblev.line.r;
      break;
    case 8:
      return 15-sblev.cd.r;
      break;
    case 9:
      return 7-sblev.mic;
      break;
    default:
      return -1;
      break;
    }
}




/* subroutine to update display of mixer params */
/* affects/uses global vars sbparams, srcsel    */

void UpdateMixerParams(void)
{
  USHORT i;
  ULONG  parlen, datlen;

  /* read in current settings */
  parlen=0;
  datlen=sizeof(struct sb_mixer_params);
  DosDevIOCtl(mixer_handle, DSP_CAT, MIXER_IOCTL_READ_PARAMS,
	      NULL, 0, &parlen,
	      &sbparams, datlen, &datlen);

  for (i=0; i<3; i++) 
    srcsel[i]=FALSE;

  switch(sbparams.record_source)
    {
    case SRC_MIC:
      srcsel[2] = TRUE;
      break;
    case SRC_LINE:
      srcsel[1] = TRUE;
      break;
    case SRC_CD:
      srcsel[0] = TRUE;
      break;
    }

  for (i=0; i<3; i++) 
    if (srcsel[i])
      WinSendMsg(hwndButton[i], BM_SETCHECK, MPFROMSHORT(1), NULL);
    else
      WinSendMsg(hwndButton[i], BM_SETCHECK, MPFROMSHORT(0), NULL);

  if (sbparams.hifreq_filter)
    {
      WinSendMsg(hwndButton[3], BM_SETCHECK, MPFROMSHORT(1), NULL);
      WinSendMsg(hwndButton[4], BM_SETCHECK, MPFROMSHORT(0), NULL);
    }
  else
    {
      WinSendMsg(hwndButton[3], BM_SETCHECK, MPFROMSHORT(0), NULL);
      WinSendMsg(hwndButton[4], BM_SETCHECK, MPFROMSHORT(1), NULL);
    }

  if (sbparams.filter_output)
      WinSendMsg(hwndButton[5], BM_SETCHECK, MPFROMSHORT(1), NULL);
  else
      WinSendMsg(hwndButton[5], BM_SETCHECK, MPFROMSHORT(0), NULL);

  if (sbparams.filter_input)
      WinSendMsg(hwndButton[6], BM_SETCHECK, MPFROMSHORT(1), NULL);
  else
      WinSendMsg(hwndButton[6], BM_SETCHECK, MPFROMSHORT(0), NULL);

}

  

/* labels for different sliders */
char *slidenames[]={{"MAS"},{"VOC"},{"FM"},{"LINE"},{"CD"},{"MIC"}};



static MRESULT MainWndProc(hwnd, msg, mp1, mp2)
     HWND          hwnd;
     ULONG        msg;
     MPARAM        mp1;
     MPARAM        mp2;
{
  RECTL      rectl, txtrectl;
  HPS        hps;
  USHORT     i, slidepos;
  ULONG      parlen, datlen;
  SWP        winpos;
  POINTL     bleft, tmppnt;
  char       valstr[3];


  switch(msg)
    {
    case WM_ERASEBACKGROUND:
      return( (MRESULT) TRUE);
      break;


    case WM_PAINT:
      hps=WinBeginPaint(hwnd, NULL, (PRECTL) &rectl);

      bleft.x=fwidth*XBORDER-(SB_WID*fwidth)/2;
      bleft.y=0;
      /* loop through scrollbars, inquire position and output values */
      for (i=0; i<=SCR_MIC; i++)
	{
	  slidepos=(USHORT)WinSendMsg(hwndScroll[i],SBM_QUERYPOS,NULL,NULL);
	  if (i!=SCR_MIC)
	    slidepos=15-slidepos;
	  else
	    slidepos=7-slidepos;
	  
	  _ultoa((ULONG)slidepos, valstr, 10);

	  WinQueryWindowPos(hwndScroll[i], &winpos);
	  winpos.x -= borderx;
	  winpos.y -= bordery;
	  txtrectl.yBottom=winpos.y-(LONG)(1.75*fheight);
	  txtrectl.yTop=txtrectl.yBottom+(LONG)(1.25*fheight);
	  txtrectl.xLeft=winpos.x-(LONG)(0.25*winpos.cx);
	  txtrectl.xRight=txtrectl.xLeft+(LONG)(winpos.cx*1.4);

	  /* put a box around it */
	  WinFillRect(hps, &txtrectl, SYSCLR_BACKGROUND);
	  tmppnt.x=txtrectl.xLeft;
	  tmppnt.y=txtrectl.yBottom;
	  GpiMove(hps,&tmppnt);
	  tmppnt.x=txtrectl.xRight;
	  tmppnt.y=txtrectl.yTop;
	  GpiBox(hps,DRO_OUTLINE,&tmppnt,0,0);


	  /* draw some text */
	  WinDrawText(hps, -1, (PCH) valstr, (PRECTL) &txtrectl,
		  (LONG) GpiQueryColorIndex (hps, (ULONG) NULL,
				WinQuerySysColor (HWND_DESKTOP,
				SYSCLR_WINDOWTEXT,  0L)),
		  (LONG) GpiQueryColorIndex (hps, (ULONG) NULL,
				WinQuerySysColor (HWND_DESKTOP,
				SYSCLR_WINDOW, 0L)),
		  DT_CENTER | DT_VCENTER );

	  /* now put a label over slider indicating if Left or Right */
	  if (i<SCR_CD)
	    {
	      txtrectl.yBottom = winpos.y+winpos.cy;
	      txtrectl.yTop    = txtrectl.yBottom+(LONG)(1.25*fheight);
	      if (i & 1)
		valstr[0] = 'R';
	      else
		valstr[0] = 'L';
	      valstr[1] = '\0';

	      WinDrawText(hps, -1, (PCH) valstr, (PRECTL) &txtrectl,
			  (LONG) GpiQueryColorIndex (hps, (ULONG) NULL,
				       WinQuerySysColor (HWND_DESKTOP,
					     SYSCLR_WINDOWTEXT,  0L)),
			  (LONG) GpiQueryColorIndex (hps, (ULONG) NULL,
				       WinQuerySysColor (HWND_DESKTOP,
					     SYSCLR_WINDOW, 0L)),
			  DT_CENTER | DT_VCENTER );

	      /* put labels on too */
	      if (i & 1)
		{
		  txtrectl.xLeft -= (L_R_SEP+SB_WID)*fwidth;
		  txtrectl.yTop  += fheight;
		  txtrectl.yBottom += fheight;
		  WinDrawText(hps, -1, slidenames[i/2], (PRECTL) &txtrectl,
			    (LONG) GpiQueryColorIndex (hps, (ULONG) NULL,
				       WinQuerySysColor (HWND_DESKTOP,
					     SYSCLR_WINDOWTEXT,  0L)),
			    (LONG) GpiQueryColorIndex (hps, (ULONG) NULL,
				       WinQuerySysColor (HWND_DESKTOP,
					     SYSCLR_WINDOW, 0L)),
			    DT_CENTER | DT_VCENTER );
		}

	    }
	  else
	    {
	      /* put labels on CD, MIC controls */
	      txtrectl.yBottom = winpos.y+winpos.cy+fheight;
	      txtrectl.yTop    = txtrectl.yBottom+(LONG)(1.25*fheight);
	      txtrectl.xLeft  -= (LONG)(0.5*fwidth);
	      txtrectl.xRight += (LONG)(0.5*fwidth);
	      if (i==SCR_CD)
		WinDrawText(hps, -1, "CD", (PRECTL) &txtrectl,
			    (LONG) GpiQueryColorIndex (hps, (ULONG) NULL,
				       WinQuerySysColor (HWND_DESKTOP,
					     SYSCLR_WINDOWTEXT,  0L)),
			    (LONG) GpiQueryColorIndex (hps, (ULONG) NULL,
				       WinQuerySysColor (HWND_DESKTOP,
					     SYSCLR_WINDOW, 0L)),
			    DT_CENTER | DT_VCENTER );
	      else
		WinDrawText(hps, -1, "MIC", (PRECTL) &txtrectl,
			    (LONG) GpiQueryColorIndex (hps, (ULONG) NULL,
				       WinQuerySysColor (HWND_DESKTOP,
					     SYSCLR_WINDOWTEXT,  0L)),
			    (LONG) GpiQueryColorIndex (hps, (ULONG) NULL,
				       WinQuerySysColor (HWND_DESKTOP,
					     SYSCLR_WINDOW, 0L)),
			    DT_CENTER | DT_VCENTER );

	    }		

	  
	}      


      (void)WinEndPaint(hps);
      break;


    /* this will handle case if they press the 'refresh' button */
    case WM_COMMAND:
      /* read values from SB Pro mixer, update display */
      parlen=0;
      datlen=sizeof(struct sb_mixer_levels);
      DosDevIOCtl(mixer_handle, DSP_CAT, MIXER_IOCTL_READ_LEVELS,
		  NULL, 0, &parlen,
		  &sblevels, datlen, &datlen);


      /* update all sliders */
      for (i=0;i<=SCR_MIC;i++)
	WinSendMsg(hwndScroll[i], SBM_SETPOS, 
		   MPFROM2SHORT(GetMixerLevel(i,sblevels),0),NULL);

      /* refresh all labels for sliders */
      for (i=0;i<=SCR_MIC;i++)
	{
	  WinQueryWindowPos(hwndScroll[i], &winpos);
	  winpos.x -= borderx;
	  winpos.y -= bordery;
	  rectl.yBottom=winpos.y-(LONG)(1.75*fheight);
	  rectl.yTop=rectl.yBottom+(LONG)(1.25*fheight);
	  rectl.xLeft=winpos.x-(LONG)(0.25*winpos.cx);
	  rectl.xRight=rectl.xLeft+(LONG)(winpos.cx*1.4);
	  WinInvalidateRect(hwnd, &rectl, FALSE);
	}


      /* now handle parameters */
      UpdateMixerParams();
      break;

    /* this will handle button messages */
    case WM_CONTROL:
      /* figure out which button */
      i=SHORT1FROMMP(mp1);
      switch(i)
	{
	  USHORT butnum;

	case ID_BUTSRCMIC:
	case ID_BUTSRCCD:
	case ID_BUTSRCLINE:
	  /* see if they clicked on the button */
	  if (SHORT2FROMMP(mp1)==BN_CLICKED ||
	      SHORT2FROMMP(mp1)==BN_DBLCLICKED)
	    {
	      /* inquire current state, then invert */
	      butnum = i-100;
	      if (srcsel[butnum]==FALSE)
		{
		  for (i=0; i<3; i++)
		    if (i!=butnum)
		      {
			if (srcsel[i])
			  WinSendMsg(hwndButton[i], 
				     BM_SETCHECK,MPFROMSHORT(0),NULL);
			srcsel[i]=FALSE;
		      }
		    else
		      {
			WinSendMsg(hwndButton[i], 
				   BM_SETCHECK,MPFROMSHORT(1),NULL);
			srcsel[i]=TRUE;
		      }
		  
		  if (srcsel[0])
		    sbparams.record_source=SRC_CD;
		  else if (srcsel[1])
		    sbparams.record_source=SRC_LINE;
		  else 
		    sbparams.record_source=SRC_MIC;

		  parlen=0;
		  datlen=sizeof(struct sb_mixer_params);
		  DosDevIOCtl(mixer_handle, DSP_CAT, MIXER_IOCTL_SET_PARAMS,
			      NULL, 0, &parlen,
			      &sbparams, datlen, &datlen);
		}
	    }
	
	  break;
	  
	case ID_BUTFILTHI:
	case ID_BUTFILTLO:
	  /* see if they clicked on the button */
	  if (SHORT2FROMMP(mp1)==BN_CLICKED ||
	      SHORT2FROMMP(mp1)==BN_DBLCLICKED)
	    {
	      if (i==ID_BUTFILTHI)
		{
		  if (sbparams.hifreq_filter==FALSE)
		    {
		      sbparams.hifreq_filter=TRUE;
		      WinSendMsg(hwndButton[3], 
				 BM_SETCHECK, MPFROMSHORT(1), NULL);
		      WinSendMsg(hwndButton[4], 
				 BM_SETCHECK, MPFROMSHORT(0), NULL);
		    }
		}
	      else
		{
		  if (sbparams.hifreq_filter==TRUE)
		    {
		      sbparams.hifreq_filter=FALSE;
		      WinSendMsg(hwndButton[3], 
				 BM_SETCHECK, MPFROMSHORT(0), NULL);
		      WinSendMsg(hwndButton[4], 
				 BM_SETCHECK, MPFROMSHORT(1), NULL);
		    }
		}
	      parlen=0;
	      datlen=sizeof(struct sb_mixer_params);
	      DosDevIOCtl(mixer_handle, DSP_CAT, MIXER_IOCTL_SET_PARAMS,
			  NULL, 0, &parlen,
			  &sbparams, datlen, &datlen);
	    }
	  break;
	  
	case ID_BUTFILTOUT:
	  /* see if they clicked on the button */
	  if (SHORT2FROMMP(mp1)==BN_CLICKED ||
	      SHORT2FROMMP(mp1)==BN_DBLCLICKED)
	    {
	      if (WinSendMsg(hwndButton[5], BM_QUERYCHECK, NULL, NULL))
		{
		  sbparams.filter_output = FALSE;
		  WinSendMsg(hwndButton[5], BM_SETCHECK,MPFROMSHORT(0),NULL);
		}
	      else
		{
		  sbparams.filter_output = TRUE;
		  WinSendMsg(hwndButton[5], BM_SETCHECK,MPFROMSHORT(1),NULL);
		}
	      parlen=0;
	      datlen=sizeof(struct sb_mixer_params);
	      DosDevIOCtl(mixer_handle, DSP_CAT, MIXER_IOCTL_SET_PARAMS,
			  NULL, 0, &parlen,
			  &sbparams, datlen, &datlen);
	    }
	  break;
	  
	case ID_BUTFILTIN:
	  /* see if they clicked on the button */
	  if (SHORT2FROMMP(mp1)==BN_CLICKED ||
	      SHORT2FROMMP(mp1)==BN_DBLCLICKED)
	    {
	      if (WinSendMsg(hwndButton[6], BM_QUERYCHECK, NULL, NULL))
		{
		  sbparams.filter_input=FALSE;
		  WinSendMsg(hwndButton[6], BM_SETCHECK,MPFROMSHORT(0),NULL);
		}
	      else
		{
		  sbparams.filter_input=TRUE;
		  WinSendMsg(hwndButton[6], BM_SETCHECK,MPFROMSHORT(1),NULL);
		}
	      parlen=0;
	      datlen=sizeof(struct sb_mixer_params);
	      DosDevIOCtl(mixer_handle, DSP_CAT, MIXER_IOCTL_SET_PARAMS,
			  NULL, 0, &parlen,
			  &sbparams, datlen, &datlen);
	    }
	  break;
	  
	default:
	  break;
	}


    /* This will handle a vertical scroll bar */
    case WM_VSCROLL:

      /* see who was responsible */
      i=SHORT1FROMMP(mp1);
      if (i<10)
	{
	  USHORT smax, smin;
	  MRESULT mr;
	  USHORT update=1;

	  /* this is my scroll bar, see what they did */
	  switch(SHORT2FROMMP(mp2))
	    {
	    case SB_LINEUP:
	    case SB_LINEDOWN:

	      slidepos=(USHORT)WinSendMsg(hwndScroll[i], SBM_QUERYPOS,
					      NULL, NULL);
	      mr=WinSendMsg(hwndScroll[i], SBM_QUERYRANGE, NULL, NULL);
	      if (SHORT2FROMMP(mp2)==SB_LINEUP)
		{
		  smin=SHORT1FROMMR(mr);
		  slidepos-=2;
		  if (slidepos < smin) slidepos=smin;
		}
	      else
		{
		  smax=SHORT2FROMMR(mr);
		  slidepos+=2;
		  if (slidepos > smax) slidepos=smax;
		}
	      break;

	    case SB_PAGEUP:
	    case SB_PAGEDOWN:

	      slidepos=(USHORT)WinSendMsg(hwndScroll[i], SBM_QUERYPOS,
					      NULL, NULL);

	      mr=WinSendMsg(hwndScroll[i], SBM_QUERYRANGE, NULL, NULL);
	      if (SHORT2FROMMP(mp2)==SB_PAGEUP)
		{
		  smin=SHORT1FROMMR(mr);
		  slidepos -=4;
		  if (slidepos < smin) slidepos=smin;
		}
	      else
		{
		  smax=SHORT2FROMMR(mr);
		  slidepos +=4;
		  if (slidepos >smax) slidepos=smax;
		}
	      break;

	    case SB_SLIDERPOSITION: 
	    case SB_SLIDERTRACK:

	      slidepos=SHORT1FROMMP(mp2);
	      break;

	    default:
	      update=0;
	      break;
	    }

	  /* see if we should do anything */
	  if (update != 0)
	    {
	      /* send new volume to SBMIX */
	      parlen=0;
	      datlen=sizeof(struct sb_mixer_levels);
	      StoreMixerLevel(i, slidepos, &sblevels);
	      DosDevIOCtl(mixer_handle, DSP_CAT, MIXER_IOCTL_SET_LEVELS,
			  NULL, 0, &parlen,
			  &sblevels, datlen, &datlen);

	      /* read what position was really set to */
	      datlen=sizeof(struct sb_mixer_levels);
	      parlen=0;
	      DosDevIOCtl(mixer_handle, DSP_CAT,MIXER_IOCTL_READ_LEVELS,
				 NULL, 0, &parlen,
				 &sblevels, datlen, &datlen);
	      slidepos=GetMixerLevel(i, sblevels);

	      /* now update scrollbar, text */
	      WinSendMsg(hwndScroll[i], SBM_SETPOS, MPFROM2SHORT(slidepos,0),
			 NULL);
	      /* now redraw the number at bottom of slidebar */
	      WinQueryWindowPos(hwndScroll[i], &winpos);
	      winpos.x -= borderx;
	      winpos.y -= bordery;
	      rectl.yBottom=winpos.y-(LONG)(1.75*fheight);
	      rectl.yTop=rectl.yBottom+(LONG)(1.25*fheight);
	      rectl.xLeft=winpos.x-(LONG)(0.25*winpos.cx);
	      rectl.xRight=rectl.xLeft+(LONG)(winpos.cx*1.4);
	      WinInvalidateRect(hwnd, &rectl, FALSE);
	      break;
	    }
	}
    default:
      return(WinDefWindowProc(hwnd, msg,mp1,mp2));
      break;
    }

  return(0L);
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
		ID_HELPWIN, MB_OK | mbtype );

  if (mixer_handle)
    DosClose(mixer_handle);

  WinDestroyWindow(hwndFrame);
  WinDestroyMsgQueue(hmq);
  WinTerminate(hab);
  exit(0);
}




int main (argc, argv )
int argc;
char *argv[];
{
  FONTMETRICS fm;

  USHORT i;
  ULONG  status, action, datlen, parlen;
  POINTL bleft;
  RECTL  rect;

  ULONG     flCreate = FCF_TITLEBAR | FCF_BORDER | FCF_SYSMENU |
                       FCF_MINBUTTON | FCF_SHELLPOSITION | FCF_TASKLIST |
		       FCF_ICON;


  char      szTitle[]={"MixerTool"};



  /* before we do anything make sure we can open the mixer */
  status = DosOpen( "SBMIX$", &mixer_handle, &action, 0,
		   FILE_NORMAL, FILE_OPEN,
   OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYNONE | OPEN_FLAGS_WRITE_THROUGH,
			  NULL );

  if (status != 0)
    Error_and_exit("Error opening audio device SBMIX$.\n This Program must be run on a SBPRO.", TRUE);

  /* read current levels */
  datlen=sizeof(struct sb_mixer_levels);
  parlen=0;
  status=DosDevIOCtl(mixer_handle, DSP_CAT, MIXER_IOCTL_READ_LEVELS,
                    NULL, 0, &parlen,
                    &sblevels, datlen, &datlen);
  if (status != 0)
    Error_and_exit("Error reading current mixer levels.", TRUE);

  /* everything ok, so continue */
  hab = WinInitialize(QV_OS2);
  hmq = WinCreateMsgQueue( hab, 0);

  /* register this window */
  if (!WinRegisterClass(hab, szTitle, MainWndProc, 
			CS_SIZEREDRAW, 0L))
    return(0);

  /* create main window */
  hwndFrame = WinCreateStdWindow((HAB)hab,
				 0,
				 (PVOID) &flCreate,
				 (PSZ) szTitle,
				 (PSZ) NULL,
				 0L,
				 (HMODULE) NULL,
				 (USHORT) ID_MAIN,
				 (PHWND) &hwnd);

  /* set title bar */
  WinSetWindowText(WinWindowFromID (hwndFrame, FID_TITLEBAR), (PSZ) szTitle);


  /* now figure out the size of current font */
  hps=WinGetPS(hwnd);
  GpiQueryFontMetrics(hps,(LONG)sizeof(FONTMETRICS), (PFONTMETRICS)&fm);

  /* record some useful info */
  fwidth = (USHORT)(1.2*fm.lAveCharWidth);
  fheight= (USHORT)(fm.lExternalLeading+fm.lMaxBaselineExt);

  WinReleasePS(hps);


  /* now resize window */
  WinSetWindowPos(hwndFrame, NULL, 0, 0,
		  (2*XBORDER+10*SB_WID+5*SB_SEP+4*L_R_SEP)*fwidth,
		  (SB_LEN+YBORDER_TOP+YBORDER_BOT+3)*fheight, 
		  SWP_SIZE | SWP_SHOW);


  /* inquire into size of main window border */
  borderx=(USHORT) WinQuerySysValue(HWND_DESKTOP, SV_CXBORDER);
  bordery=(USHORT) WinQuerySysValue(HWND_DESKTOP, SV_CYBORDER);

  /* now make the scrollbars */
  /* first lets do the stereo scrollbars */
  for (i=0; i<=SCR_MIC; i++)
    {
      GetSliderPos(i, &bleft);
      hwndScroll[i] = WinCreateWindow(hwndFrame, WC_SCROLLBAR, NULL,
			   SBS_VERT | WS_VISIBLE,
			   bleft.x,bleft.y,
			   SB_WID*fwidth,SB_LEN*fheight,
			   hwndFrame, HWND_TOP, i, NULL, NULL);

      if (i!=SCR_MIC)
	{
	  /* Set range from 0 to 15 */
	  WinSendMsg(hwndScroll[i],SBM_SETSCROLLBAR, 
		     MPFROM2SHORT(GetMixerLevel(i, sblevels),0),
		     MPFROM2SHORT(0,14));
	}
      else
	{
	  WinSendMsg(hwndScroll[i],SBM_SETSCROLLBAR, 
		     MPFROM2SHORT(GetMixerLevel(i, sblevels),0),
		     MPFROM2SHORT(0,6));
	}
    }
  /* done with drawing scrollbars */
  
  /* Create buttons */
  hwndButton[0] = WinCreateWindow(hwndFrame, WC_BUTTON, "SRC_CD",
		       WS_VISIBLE | BS_RADIOBUTTON,
		       (LONG)(2*fwidth),
		       (SB_LEN+YBORDER_BOT+2)*fheight+fheight/2,
		       11*fwidth,fheight,
		       hwndFrame, HWND_TOP, ID_BUTSRCCD, NULL, NULL);

  hwndButton[1] = WinCreateWindow(hwndFrame, WC_BUTTON, "SRC_LINE",
		       WS_VISIBLE | BS_RADIOBUTTON,
		       (LONG)(2*fwidth),
		       (SB_LEN+YBORDER_BOT+3)*fheight+fheight/2,
		       13*fwidth,fheight,
		       hwndFrame, HWND_TOP, ID_BUTSRCLINE, NULL, NULL);

  hwndButton[2] = WinCreateWindow(hwndFrame, WC_BUTTON, "SRC_MIC",
		       WS_VISIBLE | BS_RADIOBUTTON,
		       (LONG)(2*fwidth),
		       (SB_LEN+YBORDER_BOT+4)*fheight+fheight/2,
		       12*fwidth,fheight,
		       hwndFrame, HWND_TOP, ID_BUTSRCMIC, NULL, NULL);

  hwndButton[3] = WinCreateWindow(hwndFrame, WC_BUTTON, "FILT_HI",
		       WS_VISIBLE | BS_RADIOBUTTON,
		       (LONG)(15.5*fwidth),
		       (SB_LEN+YBORDER_BOT+4)*fheight+fheight/2,
		       11*fwidth,fheight,
		       hwndFrame, HWND_TOP, ID_BUTFILTHI, NULL, NULL);

  hwndButton[4] = WinCreateWindow(hwndFrame, WC_BUTTON, "FILT_LO",
		       WS_VISIBLE | BS_RADIOBUTTON,
		       (LONG)(15.5*fwidth),
                       (SB_LEN+YBORDER_BOT+3)*fheight+fheight/2,
		       11*fwidth,fheight,
		       hwndFrame, HWND_TOP, ID_BUTFILTLO, NULL, NULL);

  hwndButton[5] = WinCreateWindow(hwndFrame, WC_BUTTON, "FILT_OUT",
		       WS_VISIBLE | BS_RADIOBUTTON,
		       (LONG)(27*fwidth),
                       (SB_LEN+YBORDER_BOT+4)*fheight+fheight/2,
		       12*fwidth,fheight,
		       hwndFrame, HWND_TOP, ID_BUTFILTOUT, NULL, NULL);

  hwndButton[6] = WinCreateWindow(hwndFrame, WC_BUTTON, "FILT_IN",
		       WS_VISIBLE | BS_RADIOBUTTON,
		       (LONG)(27*fwidth),
		       (SB_LEN+YBORDER_BOT+3)*fheight+fheight/2,
		       12*fwidth,fheight,
		       hwndFrame, HWND_TOP, ID_BUTFILTIN, NULL, NULL);

  hwndPbutton = WinCreateWindow(hwndFrame, WC_BUTTON, "Refresh",
		       WS_VISIBLE | BS_PUSHBUTTON,
		       (LONG)(21*fwidth),
		       (SB_LEN+YBORDER_BOT+2)*fheight+fheight/4,
		       12*fwidth,(LONG)(1.25*fheight),
		       hwndFrame, HWND_TOP, ID_BUTREFRESH, NULL, NULL);

  UpdateMixerParams();

  /* show what we've done */
  WinShowWindow(hwndFrame, TRUE);

  while(WinGetMsg(hab, (PQMSG)&qmsg,(HWND)NULL,0,0))
    WinDispatchMsg(hab,(PQMSG)&qmsg);


  /* destroy all sliderbars */
  for (i=0; i<=SCR_MIC;i++)
    WinDestroyWindow(hwndScroll[i]);

  for (i=0; i<NUM_BUTTONS; i++)
    WinDestroyWindow(hwndButton[i]);

  WinDestroyWindow(hwndPbutton);

  WinDestroyWindow(hwndFrame);
  WinDestroyMsgQueue(hmq);
  WinTerminate(hab);
}

