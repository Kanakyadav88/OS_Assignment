CC = gcc
CFLAGS = -Wall -g

SOURCES = SimpleShell.c fib.c helloworld.c 
EXECUTABLES = $(SOURCES:.c=)

all: $(EXECUTABLES)

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

run: all
	./SimpleShell

clean:
	rm -f $(EXECUTABLES)

.PHONY: all run clean