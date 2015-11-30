all: pmoscope.exe

pmoscope.res:        pmoscope.rc pmoscope.ico
	rc -r -i \emx\include pmoscope.rc

pmoscope.o: pmoscope.c
	gcc -g -Zmt -O2 -c pmoscope.c


pmoscope:            pmoscope.o 
	gcc -g -Zmt -o pmoscope pmoscope.o -los2

pmoscope.exe:        pmoscope pmoscope.res
	emxbind -bpq -rpmoscope.res /emx/bin/emxl pmoscope


