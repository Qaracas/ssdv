INC=/usr/include
COP=-fPIC -shared -DHAVE_CONFIG_H -c -O -g
CIN=-I$(INC)
CC=gcc
EXT=leeflujo

UNAME := $(shell uname)

ifeq ($(UNAME), CYGWIN_NT-10.0)
LIB=$(EXT).dll
endif
ifeq ($(UNAME), Linux)
LIB=$(EXT).so
endif

$(EXT): $(EXT).o
	$(CC) -shared -o $(LIB) $(EXT).o

%.o:
	$(CC) $(COP) $(CIN) $(EXT).c

limpia:
	rm -rf *.dll *.o