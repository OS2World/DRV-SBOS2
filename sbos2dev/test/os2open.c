/* MSF - compile with gcc -o os2open.exe os2open.c -los2 */

#include <stdio.h>
#include <os2.h>

main()


{
    HFILE outhandle;
    ULONG action, status;


    printf("Opening SBDSP$ device driver\n");
    status = DosOpen( "SBDSP$", &outhandle, &action, 0,
                          FILE_NORMAL, FILE_OPEN,
   OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYNONE | OPEN_FLAGS_WRITE_THROUGH,
			  NULL );

    printf("Result of DosOpen = %d\n",status);
    printf("Action taken      = %d\n",action);
    printf("File handle       = %d\n",outhandle);

    status = DosClose(outhandle);
    printf("Result of DosClose = %d\n",status);
  }


