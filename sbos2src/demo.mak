#make file for Demo DD
src=.\src
lst=.\lst
obj=.\obj
msg=.\msg
lib=.
h=.\src
optmain=/Fc$(lst)\$*.cod /c /NTMAINSEG /W3 /Gs /Alfw /Od /Zp /Fo$(obj)\$*.obj >$(msg)\$*.msg
optsize=/Fc$(lst)\$*.cod /c /NTMAINSEG /W3 /Gs /Alfw /Os /Zp /Fo$(obj)\$*.obj >$(msg)\$*.msg
optkeep=/Fc$(lst)\$*.cod /c /NTKEEPSEG /W3 /Gs /Alfw /Od /Zp /Fo$(obj)\$*.obj >$(msg)\$*.msg
optinit=/Fc$(lst)\$*.cod /c /NTINITSEG /W3 /Gs /Alfw /Od /Zp /Fo$(obj)\$*.obj >$(msg)\$*.msg
optdriv=/Fc$(lst)\$*.cod /c /NTMAINSEG /W3 /Gs /Alfw /Od /Zp /Fo$(obj)\$*.obj >$(msg)\$*.msg
hfiles=$(h)\externs.h $(h)\const.h $(h)\struc.h $(h)\demo.h


$(obj)\asmutils.obj : $(src)\asmutils.asm
   masm $(src)\$*.asm,$(obj)\$*.obj,$(lst)\$*.lst; > $(msg)\$*.msg
   lib $(lib)\demo-+$(obj)\$*;

$(obj)\brkpoint.obj : $(src)\brkpoint.asm
   masm $(src)\$*,$(obj)\$*.obj,$(lst)\$*.lst; > $(msg)\$*.msg
   lib $(lib)\demo-+$(obj)\$*;

$(obj)\devhlp.obj : $(src)\devhlp.asm
   masm $(src)\$*,$(obj)\$*.obj,$(lst)\$*.lst; > $(msg)\$*.msg
   lib $(lib)\demo-+$(obj)\$*;

$(obj)\sbos2.obj : $(src)\sbos2.asm
   masm $(src)\sbos2,$(obj)\sbos2.obj,$(lst)\sbos2.lst; > $(msg)\$*.msg

$(obj)\gdtmem.obj : $(src)\gdtmem.c $(hfiles)
   cl $(optkeep) $(src)\$*.c
   lib $(lib)\demo-+$(obj)\$*;

$(obj)\lock.obj : $(src)\lock.c $(hfiles)
   cl $(optkeep) $(src)\$*.c
   lib $(lib)\demo-+$(obj)\$*;

$(obj)\ldtmem.obj : $(src)\ldtmem.c $(hfiles)
   cl $(optkeep) $(src)\$*.c
   lib $(lib)\demo-+$(obj)\$*;

$(obj)\init.obj : $(src)\init.c $(hfiles)
   cl $(optinit) $(src)\$*.c
   lib $(lib)\demo-+$(obj)\$*;

$(obj)\badcmd.obj : $(src)\badcmd.c $(hfiles)
   cl $(optkeep) $(src)\$*.c
   lib $(lib)\demo-+$(obj)\$*;

$(obj)\strategy.obj : $(src)\strategy.c $(hfiles)
   cl $(optmain) $(src)\$*.c
   lib $(lib)\demo-+$(obj)\$*;

$(obj)\ddutils.obj : $(src)\ddutils.c $(hfiles)
   cl $(optmain) $(src)\$*.c
   lib $(lib)\demo-+$(obj)\$*;

$(obj)\sb_drive.obj : $(src)\sb_drive.c $(hfiles) $(src)\sblast.h $(src)\sb_regs.h
   cl $(optdriv) $(src)\$*.c
   lib $(lib)\demo-+$(obj)\$*;


sbos2.sys : $(lib)\demo.lib $(obj)\sbos2.obj
   link @demo.arf > $(msg)\link.lst


$(lib)\demo.lib : demo.arf                        \
                            $(obj)\ASMUTILS.OBJ  \
                            $(obj)\BADCMD.OBJ    \
                            $(obj)\brkpoint.obj  \
                            $(obj)\DDUTILS.OBJ   \
                            $(obj)\DEVHLP.OBJ    \
                            $(obj)\GDTMEM.OBJ    \
                            $(obj)\INIT.OBJ      \
                            $(obj)\LOCK.OBJ      \
                            $(obj)\LDTMEM.OBJ    \
                            $(obj)\sbos2.OBJ      \
                            $(obj)\STRATEGY.OBJ \
	                    $(obj)\sb_drive.obj

