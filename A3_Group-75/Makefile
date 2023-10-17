# Makefile for simpScheduler

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall

# Executable name
EXEC = simpScheduler

# Default values for NCPU and TSLICE
NCPU = 4
TSLICE = 100

all: $(EXEC)

$(EXEC): simpScheduler.c
	$(CC) $(CFLAGS) simpScheduler.c -o $(EXEC)

run: $(EXEC)
	./$(EXEC) $(NCPU) $(TSLICE)

clean:
	rm -f $(EXEC)

.PHONY: all run clean