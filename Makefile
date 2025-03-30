# Makefile for CUinSPACE Simulated Flight
COMPILE = gcc -g -Wall -Wextra -pthread

TARGET = p2

#files to compile
OBJS = main event manager resource system

# Default target: build the executable
all: $(TARGET)

# Linking the object files to create the executable
$(TARGET): $(OBJS)
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
    rm -f $(OBJS) $(TARGET)