              PAGE      60,131
              TITLE     GENERIC Device Driver Assembler Level Utilities
;
              .386P     ;    We amy want to use some 386 instructions
;
;ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
;³                  CONSTANT DEFINITIONS                                     ³
;ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
;
TRUE          equ       1      ; Boolean values
FALSE         equ       0
REAL_TO_PROT  equ       02fh   ; This is the DevHlp cmd to go to PROTECT mode
PROT_TO_REAL  equ       030h   ; This is the DevHlp cmd to go to REAL mode
;
              PAGE
;
;
;ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
;³                  CODE SEGMENT                                             ³
;ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
;
MAINSEG       SEGMENT   WORD PUBLIC 'CODE' USE16
;
              ASSUME    CS:MAINSEG
              public    _in_port
              public    _out_port
              public    _enable_irpt
              public    _disable_irpt
              public    _real_mode
              public    _to_real_moda
              public    _to_prot_moda
              extrn     _dev_hlpCS:dword
;
;
;ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
;³                                                                           ³
;³  in_port - Get a byte from an input port                                  ³
;³                                                                           ³
;³  Syntax - word far in_port(word)                                          ³
;³                                                                           ³
;ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
;
_in_port      proc      far
              push      bp
              mov       bp,sp
;
; Get the address of the port
;
              mov       dx,[bp+6]
;
; Do the input
;
              in        al,dx
;
; Clear the high half of the integer
;
              xor       ah,ah
;
; and back we go, with the value in AX (MS integer return convention)
;
              pop       bp
              ret
;
_in_port      endp
;
;ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
;³                                                                           ³
;³  out_port - Write a byte to an output port                                ³
;³                                                                           ³
;³  Syntax - void far out_port(word, word)                                   ³
;³                             port  data                                    ³
;³                                                                           ³
;ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
;
_out_port     proc      far
              push      bp
              mov       bp,sp
;
; Get the address of the port
;
              mov       dx,[bp+6]
;
; and the data to output
;
              mov       ax,[bp+8]
;
; Output it (surprise!)
;
              out       dx,al
;
; All done
;
              pop       bp
              ret
;
_out_port     endp
              PAGE
;
;
;*****************************************************************************
;*                                                                           *
;*  enable_irpt() - Function to Enable CPU interrupts                        *
;*                                                                           *
;*  Syntax - void far enable_irpt(void)                                      *
;*                                                                           *
;*****************************************************************************
;
_enable_irpt  proc      far
              sti
              ret
;
_enable_irpt  endp
;
;
;*****************************************************************************
;*                                                                           *
;*  Disable_irpt() - Function to Disable CPU interrupts                      *
;*                                                                           *
;*  Syntax - void far enable_irpt(void)                                      *
;*                                                                           *
;*****************************************************************************
;
_disable_irpt proc      far
              cli
              ret
;
_disable_irpt endp
              PAGE
;
;
;ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
;³                                                                           ³
;³  real_mode() Function to determine the CPU state                          ³
;³                                                                           ³
;³  Syntax - boolean far real_mode();                                        ³
;³                                                                           ³
;³  Returns TRUE if the CPU is in real mode, FALSE if in protect mode.       ³
;³                                                                           ³
;³  NOTE: Do not rewrite this function to use DevHlp RealToProt as this      ³
;³        function is called before the devhlp entry point is stored.        ³
;³        It would be OK to change this function to return the MSW and       ³
;³        analyze it in C in to_prot_mode.  Just don't call devhlp to        ³
;³        determine if the CPU is in real mode or not.                       ³
;³                                                                           ³
;ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
;
_real_mode    proc      near
;
; Get the machine status word in AX
;
              smsw      ax
;
; Low order bit is Protect Enable.  1 = Protect mode, 0 = Real mode.
;
              and       ax,1
              jz        rm0100
              mov       ax,FALSE
              jmp       rm0200
rm0100:       mov       ax,TRUE
;
; Return to caller
;
rm0200:
              ret
;
_real_mode    endp
              PAGE
;
;
;ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
;³                                                                           ³
;³  to_real_moda - Go to real mode                                           ³
;³                                                                           ³
;³  Syntax - void far to_real_moda(void)                                     ³
;³                                                                           ³
;ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
;
_to_real_moda proc      near
;
; Just load DL with the command and call dev_hlp.  This is because the return
; structure pointer is no longer valid after the mode switch.  But, that
; really doesn't matter because we don't care about the register values anyway.
;
              mov       dl,PROT_TO_REAL
              call      cs:_dev_hlpCS
              ret
;
_to_real_moda endp
;
;
;ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
;³                                                                           ³
;³  to_prot_moda - Go to protect mode                                        ³
;³                                                                           ³
;³  Syntax - void far to_prot_moda(void)                                     ³
;³                                                                           ³
;ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
;
_to_prot_moda proc      near
;
; Same as above.  Just point and shoot.  Forget about the return register
; values.  We are not interested.
;
              mov       dl,REAL_TO_PROT
              call      cs:_dev_hlpCS
              ret
;
_to_prot_moda endp
              PAGE
MAINSEG       ENDS
;
              END
