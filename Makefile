CC=g++
CFLAGS=-Wall
SOURCES=opcodes.cpp instructions.cpp preproc.cpp main.cpp
EXDIR=bin
EXECUTABLE=sfotasm

all:
	mkdir $(EXDIR) && $(CC) $(SOURCES) $(CFLAGS) -o $(EXDIR)/$(EXECUTABLE)

install:
	install $(EXDIR)/$(EXECUTABLE) /usr/local/bin

uninstall:
	rm /usr/local/bin/$(EXECUTABLE)