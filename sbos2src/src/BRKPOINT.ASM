              PAGE      60,131 ;
              TITLE     GENERIC Device Driver Debug Function
;******************************************************************************
;This is a very simple function that executes an INT 3, loads AX with 0 and
;returns to the caller.  Most debuggers attach to INT 3, so this will cause
;the debugger to be activated.  The loading if AX with 0 is setting the return
;value to SUCCESS or 0.
;******************************************************************************
              .286c                         ;    Must use 286 mode if OS/2
;
;
MAINSEG       SEGMENT   WORD PUBLIC 'CODE'
              ASSUME    CS:MAINSEG
              public    _breakpoint
              PAGE
;
;
_breakpoint   proc      far
;
; breakpoint to debugger
;
              int       3
;             nop
;
; Return to OS/2
;
              mov       ax,0
              ret
;
_breakpoint   endp
MAINSEG       ENDS
              END
