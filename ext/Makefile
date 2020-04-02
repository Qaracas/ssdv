INC=/usr/include
COP=-fPIC -shared -DHAVE_CONFIG_H -c -O -g
CIN=-I$(INC)
CC=gcc -Wall -Wextra -pedantic
EXT=leeflujo conector

UNAME := $(shell uname)

ifeq ($(UNAME), CYGWIN_NT-10.0)
SUFLIB=.dll
endif
ifeq ($(UNAME), Linux)
SUFLIB=.so
endif

$(EXT): objetos
	$(foreach ext,$(EXT), $(CC) -shared -o $(ext)$(SUFLIB) $(ext).o ;)

objetos:
	$(foreach ext,$(EXT), $(CC) $(COP) $(CIN) $(ext).c ;)

limpia:
	rm -rf *$(SUFLIB) *.o
