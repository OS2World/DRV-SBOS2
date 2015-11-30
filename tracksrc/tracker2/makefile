# Makefile for Sound Blaster (using BSD-386 device driver)
#
# $Author: steve $
# $Id: Makefile,v 1.2 1992/06/24 06:29:59 steve Exp steve $
# $Revision: 1.2 $

CC = gcc
INSTALL_DIR = $(HOME)/bin
O = .o
CFLAGS = -Zmt
MAIN_OPTS = -O2 -Zmt #Try -O2 if you have gcc 2.x
COPTS = $(MAIN_OPTS) -c
MACHINE = soundblaster
OBJECTS = str32$O $(MACHINE)_audio$O read$O commands$O \
          audio$O automaton$O player$O getopt$O getopt1$O

all: tracker2.exe

tracker2.exe: ${OBJECTS}
	$(CC) -o tracker2.exe ${MAIN_OPTS} ${OBJECTS} -lm -los2
#	strip tracker

str32$O: str32.c defs.h
	$(CC) ${COPTS} str32.c

$(MACHINE)_audio$O: $(MACHINE)_audio.c
	$(CC) ${COPTS} $(MACHINE)_audio.c

audio$O: audio.c
	$(CC) ${COPTS} audio.c

getopt$O: getopt.c getopt.h
	$(CC) ${COPTS} getopt.c

getopt1$O: getopt1.c getopt.h
	$(CC) ${COPTS} getopt1.c

automaton$O: automaton.c defs.h
	$(CC) ${COPTS} automaton.c

player$O: player.c defs.h
	$(CC) ${COPTS} player.c

read$O: read.c  defs.h
	$(CC) ${COPTS} read.c

commands$O: commands.c defs.h
	$(CC) ${COPTS} commands.c

#machine.h: $(MACHINE).h
#	cp $(MACHINE).h machine.h

