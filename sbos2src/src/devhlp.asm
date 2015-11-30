              PAGE      60,131
              TITLE     GENERIC Device Driver devhlp interface
;
;ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
;³                                                                           ³
;³ GENERIC Device Driver DEVHLP Interface                                    ³
;³                                                                           ³
;³ _dev_help  - calls OS/2 DevHlp function by doing a far call to the value  ³
;³              stored in dev_hlpCS.  This is the one to use generally.      ³
;³              The general operation is to copy all the parameters passed   ³
;³              in the input structure by the caller onto the stack and      ³
;³              then pop them into the registers.  Then, dev_hlp is called   ³
;³              by referring to the one in our CODE segment.  This lets us   ³
;³              use DS and ES to hold all sorts of values.  After we         ³
;³              (hopefully) get back from dev_hlp, we store the register and ³
;³              flag values in the output structure and return to the caller ³
;³                                                                           ³
;³ _dev_help1 - Same as above, except it uses the value of del_hlp stored in ³
;³              our data segment and points to it with ES.  This one is      ³
;³              intended to be used only until dev_hlpCS has the right value,³
;³              very soon after INIT starts.  It is used by those functions  ³
;³              to get the right value in dev_hlpCS.  It is in a segment that³
;³              is discarded after INIT is done, so it is not accessable by  ³
;³              other functions.                                             ³
;³                                                                           ³
;ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
;
              .286P                        ;    Must use 286 mode if OS/2
              PAGE
;ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
;³                  STRUCTURE DEFINITIONS                                    ³
;ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
;
regs          struc
r_ax          dw        ?                  ; AX value
r_bx          dw        ?                  ; BX
r_cx          dw        ?                  ; CX
r_dx          dw        ?                  ; DX
r_si          dw        ?                  ; SI
r_di          dw        ?                  ; DI
r_es          dw        ?                  ; ES
r_ds          dw        ?                  ; DS
r_cs          dw        ?                  ; CS
r_flags       dw        ?                  ; FLAGS
r_es_valid    db        ?                  ; Indicator if ES is valid
r_ds_valid    db        ?                  ; Indicator if DS is valid
regs          ends
;
;ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
;³                  CONSTANT DEFINITIONS                                     ³
;ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
;
TRUE          equ       1      ; Boolean values
FALSE         equ       0
;
;
NULL          SEGMENT   WORD PUBLIC 'BEGDATA'
              extrn _devhlp:dword
NULL          ENDS
              PAGE
;
;ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
;³                  CODE SEGMENT                                             ³
;ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
;
MAINSEG       SEGMENT   WORD PUBLIC 'CODE'
;
              ASSUME    CS:MAINSEG
              public    _dev_help
              public    _dev_hlpCS
;
;ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
;³                                                                           ³
;³  dev_help    - function to call OS/2 DevHlp facility.                     ³
;³                                                                           ³
;³  struct regs {                                                            ³
;³         unsigned AX,BX,CX,DX,SI,DI,ES,flags;                              ³
;³         boolean es_valid;                                                 ³
;³         boolean ds_valid;                                                 ³
;³         }                                                                 ³
;³                                                                           ³
;³  Syntax - unsigned dev_helper(far ptr *in_regs, far ptr *out_regs);       ³
;³                                                                           ³
;ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
;
_dev_hlpCS    dd        0
_dev_help     proc      far
;
; Point to current frame
; This gets BP pointing generally at the parameters passed to us by the caller
;
              push      bp
              mov       bp,sp
;
; Save entry regs - we are gonna step on them hard
;
              push      ax
              push      bx
              push      cx
              push      dx
              push      si
              push      di
              push      es
              pushf
              push      ds
;
; Load stack with values to go into registers
;
; First one is DS.  If the ds_valid flag in the in_regs struc passed by the
; caller is TRUE, use the value in the in_regs struc.  If it is FALSE, use
; the current value of DS.
;
              les       bx,[bp+6]
              cmp       es:[bx].r_ds_valid,TRUE
              jnz       devh1001
              mov       ax,es:[bx].r_ds
              push      ax
              jmp       devh2001
devh1001:
              push      ds
;
devh2001:
;
; Next is ES.  Same bit of logic with the es_valid flag, use the struc value
; if es_valid is TRUE, otherwise, use the value in the ES register.
;
              les       bx,[bp+6]
              cmp       es:[bx].r_es_valid,TRUE
              jnz       devh3001
              mov       ax,es:[bx].r_es
              push      ax
              jmp       devh4001
devh3001:
              push      es
;
devh4001:
;
; Now for the rest of the registers.  They are removed from the struc and
; pushed onto the stack in the following order: AX, BX, CX, DX, SI, and DI
;
              mov       ax,es:[bx].r_ax
              push      ax
              mov       ax,es:[bx].r_bx
              push      ax
              mov       ax,es:[bx].r_cx
              push      ax
              mov       ax,es:[bx].r_dx
              push      ax
              mov       ax,es:[bx].r_si
              push      ax
              mov       ax,es:[bx].r_di
              push      ax
;
; Now pop the values off the stack and into the proper registers
;
              pop       di
              pop       si
              pop       dx
              pop       cx
              pop       bx
              pop       ax
              pop       es
              pop       ds
;
; call OS/2 DevHlp
;
              call      cs:_dev_hlpCS
;
; Save return values in output structure.  The first thing to do is to save
; DS and BX because that is what we are going to use to point to the struc.
; We save the flags too, 'cause we don't want 'em touched by these operations.
;
              push      ds
              pushf
              push      bx
;
; Now, load DS:BX to point to the output struc given to us by the caller.
;
              lds       bx,[bp+10]
;
; Store AX into the return struc
;
              mov       [bx].r_ax,ax
;
; Next, is BX (use the value in the stack)
;
              pop       ax
              mov       [bx].r_bx,ax
;
; Then the flags, also on the stack
;
              pop       ax
              mov       [bx].r_flags,ax
;
; and DS from the stack.
;
              pop       ax
              mov       [bx].r_ds,ax
;
; ES. But we need to move it to a general register, as we cannot move ES to
; memory directly (the CPU won't do it).  So, we push ES onto the stack and
; pop it into AX, then move AX to the struc.
;
              push      es
              pop       ax
              mov       [bx].r_es,ax
;
; Now the rest of the registers.  We can move them straight from the register
; to the struc as we have not touched them since we returned from DevHlp.
;
              mov       [bx].r_cx,cx
              mov       [bx].r_dx,dx
              mov       [bx].r_di,di
              mov       [bx].r_si,si
;
; And finally, set both es_valid and ds_valid to TRUE cause they are silly
;
              mov       [bx].r_es_valid,TRUE
              mov       [bx].r_ds_valid,TRUE
;
; Restore entry regs
;
              pop       ds
              popf
              pop       es
              pop       di
              pop       si
              pop       dx
              pop       cx
              pop       bx
              pop       ax
;
; Return to caller
;
              pop       bp
              ret
;
_dev_help     endp
MAINSEG       ENDS
;
INITSEG       SEGMENT   WORD PUBLIC 'CODE'
;
              ASSUME    CS:INITSEG
              public    _dev_help1
;
;ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
;³                                                                           ³
;³  dev_help1   - function to call OS/2 DevHlp facility.                     ³
;³                                                                           ³
;³  struct regs {                                                            ³
;³         unsigned AX,BX,CX,DX,SI,DI,ES,flags;                              ³
;³         boolean es_valid;                                                 ³
;³         boolean ds_valid;                                                 ³
;³         }                                                                 ³
;³                                                                           ³
;³  Syntax - unsigned dev_helper(far ptr *in_regs, far ptr *out_regs);       ³
;³                                                                           ³
;³  This one is used at init time until dev_hlpCS is initialized.  After     ³
;³  INIT, this one is gone as it is in a segment that is discarded after     ³
;³  initialization is finished.                                              ³
;³                                                                           ³
;ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
;
_dev_help1    proc      far
;
; Point to current frame
;
              push      bp
              mov       bp,sp
;
; Save entry regs
;
              push      ax
              push      bx
              push      cx
              push      dx
              push      si
              push      di
              push      es
              pushf
              push      ds
              push      ds
;
; Load stack with values to go into registers
;
              les       bx,[bp+6]
              cmp       es:[bx].r_ds_valid,TRUE
              jnz       devh1000
              mov       ax,es:[bx].r_ds
              push      ax
              jmp       devh2000
devh1000:
              push      ds
;
devh2000:
;
              mov       ax,es:[bx].r_ax
              push      ax
              mov       ax,es:[bx].r_bx
              push      ax
              mov       ax,es:[bx].r_cx
              push      ax
              mov       ax,es:[bx].r_dx
              push      ax
              mov       ax,es:[bx].r_si
              push      ax
              mov       ax,es:[bx].r_di
              push      ax
;
; load registers from the stack
;
              pop       di
              pop       si
              pop       dx
              pop       cx
              pop       bx
              pop       ax
              pop       ds
              pop       es
;
; call OS/2 DevHlp
;
              call      es:_devhlp
;
; Save return values in output structure
;
              push      ds
              pushf
              push      bx
              lds       bx,[bp+10]
              mov       [bx].r_ax,ax
              pop       ax
              mov       [bx].r_bx,ax
              pop       ax
              mov       [bx].r_flags,ax
              pop       ax
              mov       [bx].r_ds,ax
              push      es
              pop       ax
              mov       [bx].r_es,ax
              mov       [bx].r_cx,cx
              mov       [bx].r_dx,dx
              mov       [bx].r_di,di
              mov       [bx].r_si,si
              mov       [bx].r_es_valid,TRUE
              mov       [bx].r_ds_valid,TRUE
;
; Restore entry regs
;
              pop       ds
              popf
              pop       es
              pop       di
              pop       si
              pop       dx
              pop       cx
              pop       bx
              pop       ax
;
; Return to caller
;
              pop       bp
              ret
;
_dev_help1    endp
              PAGE
INITSEG       ENDS
;
              END
