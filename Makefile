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
_SRC = arg_ranges.c array.c colour.c connection_handler.c ext_precision.c \
	   function.c getopt_error.c image.c mandelbrot.c mandelbrot_parameters.c \
	   network_ctx.c parameters.c process_args.c process_options.c \
	   program_ctx.c request_handler.c serialise.c stack.c
SDIR = src
SRC = $(patsubst %,$(SDIR)/%,$(_SRC))

# Header files
_DEPS = arg_ranges.h array.h colour.h connection_handler.h ext_precision.h \
	    function.h getopt_error.h image.h mandelbrot_parameters.h \
		network_ctx.h parameters.h process_args.h process_options.h \
		program_ctx.h request_handler.h serialise.h stack.h
HDIR = include
DEPS = $(patsubst %,$(HDIR)/%,$(_DEPS))

# Object files
_OBJS = arg_ranges.o array.o colour.o connection_handler.o ext_precision.o \
	    function.o getopt_error.o image.o mandelbrot.o \
		mandelbrot_parameters.o network_ctx.o parameters.o process_args.o \
		process_options.o program_ctx.o request_handler.o serialise.o stack.o
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
_LDLIBS = groot percy m
LDLIBS = $(patsubst %,-l%,$(_LDLIBS))

# multiple-precision libraries to be linked with `-l`
_LDLIBS_MP = mpc mpfr gmp
LDLIBS_MP = $(patsubst %,-l%,$(_LDLIBS_MP))




# Directories that need Make calls
SUBMAKE = $(LDIR)/libgroot $(LDIR)/percy




# Compiler name
CC = gcc

# Compiler optimisation options
COPT = -flto -Ofast -march=native

# Compiler options
CFLAGS = $(IDIRS) $(COPT) -D"_POSIX_C_SOURCE=200809L" -g -std=c99 -pedantic \
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




.PHONY: all mp
# Build with standard-precision
all: $(BIN)
# Build with multiple-precision extension
mp: PERCY_MP = mp
mp: CFLAGS += -D"MP_PREC"
mp: LDFLAGS += $(LDLIBS_MP)
mp: $(BIN)




# Build Make dependencies
.PHONY: build-make build-libgroot build-percy
build-make: build-libgroot build-percy
build-libgroot:
	$(MAKE) -C $(LDIR)/libgroot
build-percy:
	$(MAKE) -C $(LDIR)/percy $(PERCY_MP)




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