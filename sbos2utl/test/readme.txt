These are some simple sample programs.

SETMIXER - allows one to set the volume levels of the SB PRO mixer. Only
           works on the SB PRO.

OS2OPEN  - simple program tries to open the SBDSP$ device, which is created
           by SBOS2.SYS. Run if things dont seem to be working. If you
           get an error message something is wrong.

OS2PLAY  - Plays back file 'sample.out' at the specified speed. 

OS2RECORD - Records from current recording source to file 'sample.out'.

These are primarily for testing that the driver is loaded correctly, although
SETMIXER.EXE is useful for setting volume levels from inside a .CMD file.
