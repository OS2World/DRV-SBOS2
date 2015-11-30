all: os2open.exe os2record.exe os2play.exe mixer.exe

os2open.exe : os2open.c sblast_user.h
	gcc -o os2open.exe os2open.c -los2

os2record.exe : os2record.c sblast_user.h
	gcc -o os2record.exe os2record.c -los2

os2play.exe : os2play.c sblast_user.h
	gcc -o os2play.exe os2play.c -los2

setmixer.exe : setmixer.c sblast_user.h
	gcc -o setmixer.exe setmixer.c -los2

