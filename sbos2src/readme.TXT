The directory structure is as follows:

src - source for sbos2.sys
msg - compilation messages
lst - code output listings, link map
obj - contains objs


To build sbos2.sys enter 'make demo.mak'. It assumes you are using
Microsoft MSC, MASM, LIB and MAKE.

Notice alot of files are called DEMO.*. These names are of historical
significance only, but changing them to something else would require
some work.


--------------
OTHER FILES
-----------

DEMODD.TXT - document describing how to write device drivers in C (author
             unknown).

