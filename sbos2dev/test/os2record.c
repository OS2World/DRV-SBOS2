/* MSF - to compile use gcc -o os2record.exe os2record.c -los2 */

#include <stdio.h>
#include <os2.h>
#include "sblast_user.h"
unsigned char buf[20*1024];


main()


{
    HFILE inhandle, outhandle;
    ULONG action, status, speed;
    ULONG numread, toread, bytecount, datlen, parlen;
    ULONG i, maxreadsize, numtoread;

    printf("Opening SBDSP device driver\n");
    status = DosOpen( "SBDSP$", &outhandle, &action, 0,
                          FILE_NORMAL, FILE_OPEN,
   OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYNONE | OPEN_FLAGS_WRITE_THROUGH,
			  NULL );

    printf("Result of DosOpen = %d\n",status);
    printf("Action taken      = %d\n",action);
    printf("File handle       = %d\n",outhandle);

    printf("\n Now opening sample.voc\n");
    status = DosOpen( "sample.voc", &inhandle, &action, 0,
                          FILE_NORMAL, FILE_TRUNCATE | FILE_CREATE,
              OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYNONE,
			  NULL );

    printf("Result of DosOpen = %d\n",status);
    printf("Action taken      = %d\n",action);
    printf("File handle       = %d\n",inhandle);

    maxreadsize=20000;
    printf("Enter amount of bytes to read : ");
    scanf("%ld", &toread);

/* note  - need emx-0.8e for following to work */

    printf("Enter speed for recording: ");
    scanf("%u", &speed);

    /* set speed */
    datlen=2;
    parlen=0;
    status=DosDevIOCtl(outhandle, DSP_CAT, DSP_IOCTL_SPEED,
                    NULL, 0, &parlen,
                    &speed, 2, &datlen);
    printf("Status of setting speed to %u is %u\n",speed,status);

    /* now read some bytes from device */
    bytecount=0;
  do{
    if ((toread-bytecount)<maxreadsize)
      numtoread=toread-bytecount;
    else
      numtoread=maxreadsize;


    printf("%d %d %d\n",bytecount,toread,numtoread);

    status = DosRead(outhandle, &buf,numtoread, &numread);

    printf("%d\n",numread);

    if (numread==0) break;
    
    /* now write some bytes to device */
    i=numread;
    status = DosWrite(inhandle, &buf, i, &numread);
    printf("%d\n",numread);
    bytecount += numread;


  }while(bytecount<toread);



    status = DosClose(inhandle);
    printf("Result of DosClose = %d\n",status);
    status = DosClose(outhandle);
    printf("Result of DosClose = %d\n",status);
  }


