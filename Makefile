#
# MAKEFILE
#
# make compiles the main executables for the client and server
# make test compiles the test executables
# make clean removes all of the generated object files, executables, and test executables
#

CC=g++
CFLAGS=-c

# header files that are needed to compile with to generate the objects
DEPS = rdt_packet.h rdt_window.h

# object files that are referenced by the others like the window and packet libraries
MAINOBJ = rdt_window.o rdt_packet.o

# Test files (for packet_unit_test.cpp for example, add packet_unit_test to TESTFILE)
TESTFILE = packet_unit_test window_unit_test window_fill_unit_test

all: hello

hello: webserver webclient

webserver: webserver.o $(MAINOBJ)
	$(CC) webserver.o $(MAINOBJ) -o webserver

webclient: webclient.o $(MAINOBJ)
	$(CC) webclient.o $(MAINOBJ) -o webclient


%.o: %.c $(DEPS)
	$(CC) -o $@ $< $(CFLAGS)

%_test: $(MAINOBJ) %_test.o
	$(CC) $(MAINOBJ) $@.o -o $@.testexec

test: $(TESTFILE)

clean:
	rm -rf *.o webserver webclient *.testexec $(TESTFILE)
