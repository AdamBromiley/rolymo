.SUFFIXES:
.SUFFIXES: .c .h .o




# Output binary
_BIN = mandelbrot
BDIR = .
BIN = $(BDIR)/$(_BIN)

# Source code
_SRC = colour.c log.c main.c parameters.c parser.c
SDIR = src
SRC = $(patsubst %,$(SDIR)/%,$(_SRC))

# Header files
_DEPS = colour.h log.h parameters.h parser.h
HDIR = include
DEPS = $(patsubst %,$(HDIR)/%,$(_DEPS))

# Object files
_OBJS = colour.o log.o main.o parameters.o parser.o
ODIR = obj
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))




# Include directories
_IDIRS = include
IDIRS = $(patsubst %,-I%,$(_IDIRS))

# Libraries to be linked with `-l`
_LDLIBS = m
LDLIBS = $(patsubst %,-l%,$(_LDLIBS))




# Compiler name
CC = gcc

# Compiler options
CFLAGS = $(IDIRS) -g -std=c99 -pedantic \
	-Wall -Wextra -Wcast-align -Wcast-qual -Wdisabled-optimization -Wformat=2 \
	-Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs \
	-Wredundant-decls -Wshadow -Wsign-conversion -Wstrict-overflow=5 \
	-Wswitch-default -Wundef


# Linker name
LD = gcc

# Linker options
LDFLAGS = $(LDLIBS)




.PHONY: all
all: $(BIN)




# Compile source into object files
$(OBJS): $(ODIR)/%.o: $(SDIR)/%.c
	@ mkdir -p $(ODIR)
	$(CC) -c $< $(CFLAGS) -o $@

# Link object files into executable
$(BIN): $(OBJS)
	@ mkdir -p var
	@ mkdir -p $(BDIR)
	$(LD) $(OBJS) $(LDFLAGS) -o $(BIN)




.PHONY: clean
# Remove object files and binary
clean:
	rm -f $(OBJS) $(BIN)