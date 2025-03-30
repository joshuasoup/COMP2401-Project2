# Makefile for CUinSPACE Simulated Flight
COMPILE = gcc -g -Wall -Wextra -pthread

#files to compile
OBJS = main.o event.o manager.o resource.o system.o

# Default target: build the executable
all: main event manager resource system
	$(COMPILE) -o p2 $(OBJS)

# Compile each source file into an object file explicitly
main: main.c defs.h
	$(COMPILE) -c main.c

event: event.c defs.h
	$(COMPILE) -c event.c

manager: manager.c defs.h
	$(COMPILE) -c manager.c 

resource: resource.c defs.h
	$(COMPILE) -c resource.c

system: system.c defs.h
	$(COMPILE) -c system.c

# Clean target to remove object files and the executable
clean:
	rm -f $(OBJS) p2