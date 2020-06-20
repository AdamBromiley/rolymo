# Mandelbrot Plotter

The Mandelbrot set is the set of numbers $c\in\mathbb{C}$ for which the function $f_{c}\left(x\right)=z^{2}+c$ does not diverge when iterated from $z=0$.

## Features
- Complete control over the plot with command-line arguments
- Output to the NetPBM family of image files - `.pbm`, `.pgm`, and `.ppm`
- ASCII art output to the terminal

## Usage
From the program's root directory, `make` compiles the `mandelbrot` binary.

Run the program with `./mandelbrot`. By default, without any command-line arguments, the program outputs `mandelbrot.pnm` - a 550 px by 500 px, 24-bit colour Mandelbrot set plot.

Help can be provided with `./mandelbrot --help`.

```
Usage: ./mandelbrot [LOG PARAMETERS...] [OUTPUT PARAMETERS...] [-j CONSTANT] [PLOT PARAMETERS...]
       ./mandelbrot --help

A Mandelbrot and Julia set plotter.

Mandatory arguments to long options are mandatory for short options too.
Output parameters:
  -c SCHEME, --colour=SCHEME    Specify colour palette to use
                                [+] Default = 5
                                [+] SCHEME may be:
                                    2  = Black and white
                                    3  = White and black
                                    4  = Greyscale
                                    5  = Rainbow
                                    6  = Vibrant rainbow
                                    7  = Red and white
                                    8  = Fire
                                    9  = Red hot
                                    10 = Matrix
                                [+] Black and white schemes are 1-bit
                                [+] Greyscale schemes are 8-bit
                                [+] Coloured schemes are full 24-bit

  -o FILE                       Output file name
                                [+] Default = 'var/mandelbrot.pnm'
  -r WIDTH,  --width=WIDTH      The width of the image file in pixels
                                [+] If using a 1-bit colour scheme, WIDTH must be a multiple of 8 to allow for
                                    bit-width pixels
  -s HEIGHT, --height=HEIGHT    The height of the image file in pixels
  -t                            Output to stdout using ASCII characters as shading
Plot type:
  -j CONSTANT                   Plot Julia set with specified constant parameter
Plot parameters:
  -m MIN,    --min=MIN          Minimum value to plot
  -M MAX,    --max=MAX          Maximum value to plot
  -i NMAX,   --iterations=NMAX  The maximum number of function iterations before a number is deemed to be within the set
                                [+] Default = 100
                                [+] A larger maximum leads to a preciser plot but increases computation time

  Default parameters:
    Julia Set:
      MIN    = -2 + -2i
      MAX    = 2 + 2i
      WIDTH  = 800
      HEIGHT = 800

    Mandelbrot set:
      MIN    = -2 + -1.25i
      MAX    = 0.75 + 1.25i
      WIDTH  = 550
      HEIGHT = 500

Miscellaneous:
  --log[=FILE]                  Output log to file, with optional file path argument
                                [+] Default = 'var/mandelbrot.log'
                                [+] Option may be used with '-v'
  -l LEVEL,  --log-level=LEVEL  Only log messages more severe than LEVEL
                                [+] Default = 4
                                [+] LEVEL may be:
                                    0  = NONE (log nothing)
                                    1  = FATAL
                                    2  = ERROR
                                    3  = WARNING
                                    4  = INFO
                                    5  = DEBUG
  -v,        --verbose          Redirect log to stderr
             --help             Display this help message and exit

Examples:
  mandelbrot
  mandelbrot -j "0.1 - 0.2e-2i" -o "juliaset.pnm"
  mandelbrot -t
  mandelbrot -i 200 --width=5500 --height=5000 --colour=9

```