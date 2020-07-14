# Rolymo Fractal Generator

The Rolymo, multi-threaded, fractal generator efficiently produces stunning image plots of the Mandelbrot set (and other fractals). User control ranges from leaving all settings on their defaults to selecting colour-mapping choices, dimensions, and iteration maximums, amongst other command-line arguments.

### Mandelbrot set
The Mandelbrot set is the set of complex numbers `c` for which the function `f(x) = z^2 + c` does not diverge when iterated from `z = 0`.

### Julia sets
Similarly, Julia sets are the sets of complex numbers `z` for which the function `f(x) = z^2 + c` does not diverge when iterated from `c = constant` (to be specified by the user).

## Features
- Arbitrary-precision floating-point support
- Julia set plotting
- Output to the NetPBM family of image files - `.pbm`, `.pgm`, and `.ppm`
- ASCII art output to the terminal

## Dependencies
The following dependencies must be installed to system **if compiling with** `make mp`:
- The [GNU Multiple Precision Arithmetic Library](https://gmplib.org/) (GMP), version 5.0.0 or later
- The [GNU Multiple Precision Floating-Point Reliable Library](https://www.mpfr.org/) (MPFR), version 3.0.0 or later
- The [GNU Multiple Precision Complex Library](http://www.multiprecision.org/mpc/home.html) (MPC)

## Usage
From the program's root directory, `make` compiles the `mandelbrot` binary. To enable arbitrary-precision support, the aforementioned GNU multiple-precision arithmetic libraries must be install to system. The package is then built with `make mp`.

Run the program with `./mandelbrot`. By default, without any command-line arguments, the program outputs `var/mandelbrot.pnm` - a 550 px by 500 px, 24-bit colour Mandelbrot set plot.

Help can be provided with `./mandelbrot --help`.

```
Usage: ./mandelbrot [LOG PARAMETERS...] [OUTPUT PARAMETERS...] [-j CONSTANT] [PLOT PARAMETERS...]
       ./mandelbrot --help

A Mandelbrot and Julia set plotter.

Mandatory arguments to long options are mandatory for short options too.
Output parameters:
  -c SCHEME, --colour=SCHEME    Specify colour palette to use (default = Rainbow)
                                  SCHEME may be:
                                    0  = ASCII
                                    1  = Black and white
                                    2  = White and black
                                    3  = Greyscale
                                    4  = Rainbow
                                    5  = Vibrant rainbow
                                    6  = Red and white
                                    7  = Fire
                                    8  = Red hot
                                    9  = Matrix
                                  Black and white schemes are 1-bit
                                  Greyscale schemes are 8-bit
                                  Coloured schemes are full 24-bit

  -o FILE                       Output file name (default = 'var/mandelbrot.pnm')
  -r WIDTH,  --width=WIDTH      The width of the image file in pixels
                                  If using a 1-bit colour scheme, WIDTH must be a multiple of 8 to allow  for
                                  bit-width pixels
  -s HEIGHT, --height=HEIGHT    The height of the image file in pixels
  -t                            Output to stdout (or, with -o, text file) using ASCII characters as shading
Plot type:
  -j CONSTANT                   Plot Julia set with specified constant parameter
Plot parameters:
  -m MIN,    --min=MIN          Minimum value to plot
  -M MAX,    --max=MAX          Maximum value to plot
  -i NMAX,   --iterations=NMAX  The maximum number of function iterations before a number is deemed to be within the set
                                  A larger maximum leads to a preciser plot but increases computation time

  Default parameters (standard-precision):
    Julia Set:
      MIN        = -2 + -2i
      MAX        = 2 + 2i
      ITERATIONS = 100
      WIDTH      = 800
      HEIGHT     = 800

    Mandelbrot set:
      MIN        = -2 + -1.25i
      MAX        = 0.75 + 1.25i
      ITERATIONS = 100
      WIDTH      = 550
      HEIGHT     = 500

Optimisation:
  -A [PREC], --arbitrary[=PREC] Enable arbitrary-precision mode
                                  Specify optional number of precision bits (default = 128 bits)
                                  MPFR floating-points will be used for calculations
                                  Precision better than '-X', but will be considerably slower
  -T COUNT,  --threads=COUNT    Use COUNT number of processing threads (default = processor count)
  -X,        --extended         Extend precision (64 bits, compared to standard-precision 53 bits)
                                  The extended floating-point type will be used for calculations
                                  This will increase precision at high zoom but may be slower
  -z MEM,    --memory=MEM       Limit memory usage to MEM megabytes (default = 80% of free RAM)
Log settings:
             --log[=FILE]       Output log to file, with optional file path argument
                                  Optionally, specify path other than default ('var/mandelbrot.log')
                                  Option may be used with '-v'
  -l LEVEL,  --log-level=LEVEL  Only log messages more severe than LEVEL (default = INFO)
                                  LEVEL may be:
                                    0  = NONE (log nothing)
                                    1  = FATAL
                                    2  = ERROR
                                    3  = WARNING
                                    4  = INFO
                                    5  = DEBUG
  -v,        --verbose          Redirect log to stderr

Miscellaneous:
             --help             Display this help message and exit

Examples:
  ./mandelbrot
  ./mandelbrot -j "0.1 - 0.2e-2i" -o "juliaset.pnm"
  ./mandelbrot -t
  ./mandelbrot -i 200 --width=5500 --height=5000 --colour=9

```

## Optimisation
Given that a single run of the program may compute trillions of complex operations, optimisation is an important part of the project. The code has been refactored to improve speed, however readability, maintainability, and modularity must still be prioritised.

### Command-line Arguments
There are two command-line arguments to increase/decrease the amount of resources used by Rolymo:
| Argument         | Description |
| :--------------- | :---------- |
| `-T`/`--threads` |Specify the number of multi-processing threads to be used. Generally, Rolymo utilises 100% of a CPU core, so for maximum performance it is recommended (and default) to set at the number of processing cores on your machine. |
| `-z`/`--memory`  |Use below a specified maximum of memory for the working image array allocation. This value is, by default, specified in `MB`, but can be given with other magnitude prexfixes (i.e. `kB`, `GB`, etc). As a default, Rolymo will use a maximum of 80% of the free *physical* memory on offer. This prevents usage of slow, swap memory and also gives space for other, regular programs, and the OS, to run comfortably. |

### Build Flags
GCC flags (in [Makefile](Makefile) located in the `$COPT` and `$LDOPT` variables) are used to heavily optimise the output code with (mainly) the sacrifice of some floating point rounding precision. The following flags are set by default:

**Compiler:** `-flto -Ofast -march=native`

**Linker:** `-flto -Ofast`

| Flag            | Description                                                                               |
| :-------------- | :---------------------------------------------------------------------------------------- |
| `-flto`         | Perform link-time optimisation                                                            |
| `-Ofast`        | Enable all `-O3` optimisations along with, most impactful for this program, `-ffast-math` |
| `-march=native` | Optimise for the user's machine                                                           |