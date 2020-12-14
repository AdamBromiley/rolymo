# Changelog

## 2020-12-14
### Added
- Workers can disconnect and reconnect to the master at any time

## 2020-11-27
### Added
- Distributed computing supported with `-g`, `-G`, and `-p`
- Specify number of bits used in the MPFR/MPC floating-point significands with `--precision`
### Changed
- `--log` merely enables file logging with the default path. Use `--log-file` to specify the path

## 2020-07-14
### Added
- ASCII art output with `-t`
### Changed
- ASCII art output can be outputted to a text file with `-o`

## 2020-07-12
### Changed
- `make mp` will build with multiple-precision floating-point support (as opposed to `make`, without)

## 2020-07-10
### Added
- `-A` option will enable multiple-precision mode (use of multiple precision MPFR/MPC types)

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