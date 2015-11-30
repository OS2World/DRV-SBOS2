all: mixertool.exe

mixertool.res:        mixertool.rc 
	rc -r -i \emx\include mixertool.rc

mixertool:            mixertool.o 
	gcc -o mixertool mixertool.o -los2

mixertool.exe:        mixertool mixertool.res
	emxbind -bpq -rmixertool.res /emx/bin/emxl mixertool
