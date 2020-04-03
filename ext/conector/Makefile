INC = /usr/include
COP = -fPIC -shared -DHAVE_CONFIG_H -c -O -g
CIN = -I$(INC)
CC  = gcc -Wall -Wextra -pedantic
EXT = conector

DLB = ../lib

UNAME := $(shell uname)

ifeq ($(UNAME), CYGWIN_NT-10.0)
SUFLIB = .dll
endif
ifeq ($(UNAME), Linux)
SUFLIB = .so
endif

$(EXT): objetos
	$(CC) -shared -o $(EXT)$(SUFLIB) $(EXT).o
	mkdir -p $(DLB)
	cp $(EXT)$(SUFLIB) $(DLB)

objetos:
	$(CC) $(COP) $(CIN) $(EXT).c

limpia:
	rm -rf *$(SUFLIB) *.o $(DLB)
