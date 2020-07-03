# Changelog

## 2020-07-03
### Added
- `-X` option will enable extended-precision mode (use of `long double` types)
### Changed
- Fixed memory allocation bug

## 2020-06-25
### Added
- Centre point & magnification can be specified rather than ranges

## 2020-06-25
### Added
- Memory limit command-line option
- Thread count command-line option

## 2020-06-24
### Changed
- Removed bug with 1-bit image mode - it works now!

## 2020-06-23
### Changed
- Library support changed to being handled through Git Submodules

## 2020-06-22
### Added
- Julia set plotting
- [TODO.md](TODO.md) list
### Removed
- Unsafe pointer cast

## 2020-06-21
### Added
- Multithreading support
- 1-bit image support
- [README.md](README.md) documentation
### Changed
- Memory allocation to only allocate from physical memory pool
- Optimisation to decrease execution time by 50-fold

## 2020-06-20
### Added
- [Makefile](Makefile) to ease installation
- Polymorphism to make project maintenance easier
### Changed
- Complete refactor of codebase
- Ergonomic command-line interface with better argument parsing
- Better logging with timestamps and message severity levels
### Removed
- ASCII art output (temporary)
- Multithreading (temporary)
- Julia set support (temporary)
- 1-bit image support (temporary)