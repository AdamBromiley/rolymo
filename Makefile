.SUFFIXES:
.SUFFIXES: .c .h .o


COMMA = ,
EMTPY =
SPACE = $(EMPTY) $(EMPTY)




# Output binary
_BIN = mandelbrot
BDIR = .
BIN = $(BDIR)/$(_BIN)

# Source code
_SRC = array.c colour.c function.c image.c mandelbrot.c mandelbrot_parameters.c parameters.c
SDIR = src
SRC = $(patsubst %,$(SDIR)/%,$(_SRC))

# Header files
_DEPS = array.h colour.h function.h image.h mandelbrot_parameters.h parameters.h
HDIR = include
DEPS = $(patsubst %,$(HDIR)/%,$(_DEPS))

# Object files
_OBJS = array.o colour.o function.o image.o mandelbrot.o mandelbrot_parameters.o parameters.o
ODIR = obj
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))




# Include directories
_IDIRS = include lib
IDIRS = $(patsubst %,-I%,$(_IDIRS))

# Directories with dynamic libraries in
_LDIRS = groot percy
LDIR = lib
LDIRS = $(patsubst %,$(LDIR)/%,$(_LDIRS))

# Library paths
LPATHS = $(patsubst %,-L%,$(LDIRS))
# Runtime library search paths
RPATHS = $(subst $(SPACE),$(COMMA),$(patsubst %,-rpath=%,$(LDIRS)))

# Libraries to be linked with `-l`
_LDLIBS = groot percy m
LDLIBS = $(patsubst %,-l%,$(_LDLIBS))




# Directories that need Make calls
SUBMAKE = $(LDIR)/groot $(LDIR)/percy




# Compiler name
CC = gcc

# Compiler optimisation options
COPT = -flto -Ofast -march=native

# Compiler options
CFLAGS = $(IDIRS) -g -std=c99 -pedantic \
	-Wall -Wextra -Wcast-align -Wcast-qual -Wdisabled-optimization -Wformat=2 \
	-Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs \
	-Wredundant-decls -Wshadow -Wsign-conversion -Wstrict-overflow=5 \
	-Wswitch-default -Wundef


# Linker name
LD = gcc

# Linker optimisation options
LDOPT = -flto -Ofast

# Linker options
LDFLAGS = $(LPATHS) -Wl,$(RPATHS) $(LDLIBS) -pthread




.PHONY: all
all: $(BIN)




.PHONY: make-sublibs
# Make all dependencies
make-sublibs:
	for directory in $(SUBMAKE); do \
		$(MAKE) -C $$directory; \
	done


# Compile source into object files
$(OBJS): $(ODIR)/%.o: $(SDIR)/%.c
	@ mkdir -p $(ODIR)
	$(CC) -c $< $(CFLAGS) -o $@

# Link object files into executable
$(BIN): $(OBJS) make-sublibs
	@ mkdir -p var
	@ mkdir -p $(BDIR)
	$(LD) $(OBJS) $(LDFLAGS) -o $(BIN)




.PHONY: clean clean-all
# Remove object files and binary
clean:
	rm -f $(OBJS) $(BIN)
# Clean dependencies
clean-all: clean
	for directory in $(SUBMAKE); do \
		$(MAKE) -C $$directory clean; \
	done