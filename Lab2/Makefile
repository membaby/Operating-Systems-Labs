CC=gcc
CFLAGS=-Wall

.PHONY: all clean

all: matMultp

matMultp: script.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f matMultp
