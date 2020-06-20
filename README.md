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

```