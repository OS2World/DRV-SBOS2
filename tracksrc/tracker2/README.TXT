			    TRACKER-SBPRO
                            -------------

README 6/29/92

This package plays Amiga .MOD soundtracker module files on various
hardware platforms.  The bulk of the code was written by Marc Espie
<espie@ens.fr> in Dec of 1991.  Please read "tracker.doc" for his
program notes.

I have modified the code to use the Sound Blaster Pro sound card, as
supported by my SBlast-BSD device driver for BSD Unix.  The
hardware-specific parts of the player are nicely isolated in a
separate file, so that the package should be easily portable to other
platforms or drivers.

The original tracker package included support for Sparc and SGI audio.
Since I do not have one of these, I can not test my modifications on
them, but I would hope that they still work fine.  Minor fixes may be
necessary to make them compile.

As it stands, Tracker supports the full 44K speed capacity of the
Sound Blaster Pro and stereo output.  For more information the free
SBlast-BSD device driver for using a Sound Blaster Pro under BSD Unix
on an i386, please contact me via email.  

Lance Norskog <thinman@netcom.com> has written a fine device driver
for SysV variants of 386 Unix which supports the orginal Sound Blaster
rather than the Pro.  If this better suits your needs, contact him for
more info.  He has also modified this tracker program to work in that
system.

Here is a list of changes I have made to Marc's original Tracker code:

1)  Changed the decompression routines to be much more flexible.
    You may define a new filename suffix to automatically decompress 
    and read by simply adding a line to a structure in str32.c

2)  Rewrote the command line option system to use GNU getopt.
    To the user, this means less playing with environment variables,
    more flexibility, and easier options to remember.  Of course,
    options can still be abbreviated to one character for brevity.

3)  New -help to go with the new options.

4)  -quiet to print nothing to stdout.

5)  -verbose to print a complete run-down of the MOD file contents
    in table form.  This code was already in Marc's code, but
    had to be compiled in with #define SHOW_SEQ 

6)  Eliminated "ntracker" and "otracker" executables and added
    -new and -old options to tracker.  Name-sensitive binaries bug me.

7)  Merged all the header files into "defs.h" and eliminated
    machine.h, which had to be the same for every machine anyway.

Please report any bugs in these areas to shaehnic@ucsd.edu.  Bugs with
the actual .MOD reading and parsing go to Marc, of course.

Enjoy!
-Steve

Steve Haehnichen
shaehnic@ucsd.edu
