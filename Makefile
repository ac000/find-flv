CC=gcc
CFLAGS=-Wall -g -std=c99 -O2
LDFLAGS=

find-flv: find-flv.c
	 $(CC) $(CFLAGS) -o find-flv find-flv.c

clean:
	rm -f find-flv
