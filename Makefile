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
_SRC = arg_ranges.c array.c colour.c ext_precision.c function.c image.c mandelbrot.c mandelbrot_parameters.c parameters.c
SDIR = src
SRC = $(patsubst %,$(SDIR)/%,$(_SRC))

# Header files
_DEPS = arg_ranges.h array.h colour.h ext_precision.h function.h image.h mandelbrot_parameters.h parameters.h
HDIR = include
DEPS = $(patsubst %,$(HDIR)/%,$(_DEPS))

# Object files
_OBJS = arg_ranges.o array.o colour.o ext_precision.o function.o image.o mandelbrot.o mandelbrot_parameters.o parameters.o
ODIR = obj
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))




# Include directories
_IDIRS = include lib
IDIRS = $(patsubst %,-I%,$(_IDIRS))

# Directories with dynamic libraries in
_LDIRS = libgroot percy
LDIR = lib
LDIRS = $(patsubst %,$(LDIR)/%,$(_LDIRS))

# Library paths
LPATHS = $(patsubst %,-L%,$(LDIRS))
# Runtime library search paths
RPATHS = $(subst $(SPACE),$(COMMA),$(patsubst %,-rpath=%,$(LDIRS)))

# Libraries to be linked with `-l`
_LDLIBS = groot percy m mpc mpfr gmp
LDLIBS = $(patsubst %,-l%,$(_LDLIBS))




# Directories that need Make calls
SUBMAKE = $(LDIR)/libgroot $(LDIR)/percy




# Compiler name
CC = gcc

# Compiler optimisation options
COPT = -flto -Ofast -march=native

# Compiler options
CFLAGS = $(IDIRS) $(COPT) -g -std=c99 -pedantic \
	-Wall -Wextra -Wcast-align -Wcast-qual -Wdisabled-optimization -Wformat=2 \
	-Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs \
	-Wredundant-decls -Wshadow -Wsign-conversion -Wstrict-overflow=5 \
	-Wswitch-default -Wundef


# Linker name
LD = gcc

# Linker optimisation options
LDOPT = -flto -Ofast

# Linker options
LDFLAGS = $(LPATHS) -Wl,$(RPATHS) $(LDLIBS) $(LDOPT) -pthread




.PHONY: all
all: $(BIN)




# Build Make dependencies
.PHONY: build-make
build-make:
	for directory in $(SUBMAKE); do \
		$(MAKE) -C $$directory; \
	done




# Compile source into object files
$(OBJS): $(ODIR)/%.o: $(SDIR)/%.c
	@ mkdir -p $(ODIR)
	$(CC) -c $< $(CFLAGS) -o $@

# Link object files into executable
$(BIN): $(OBJS) build-make
	@ mkdir -p var
	@ mkdir -p $(BDIR)
	$(LD) $(OBJS) $(LDFLAGS) -o $(BIN)




# Clean repository
.PHONY: clean clean-all
# Remove object files and binary
clean:
	rm -f $(OBJS) $(BIN)
# Clean dependencies
clean-all: clean
	for directory in $(SUBMAKE); do \
		$(MAKE) -C $$directory clean; \
	done