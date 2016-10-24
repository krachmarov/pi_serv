CC = gcc
CFLAGS = -pthread -g -Wall
SRCDIR = src/
INCLUDEDIR = include/

default: server

INCLUDEFILES = $(INCLUDEDIR)server.h

server: $(INCLUDEFILES)
	$(CC) $(CFLAGS) -o $(SRCDIR)server $(SRCDIR)main.c

clean:
	$(RM) $(SRCDIR)server
