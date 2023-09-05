# CprE 487 / 587 ML C++ Framework
Written by: Matthew Dwyer

This repository contains the source code for the C++ ML framework.

## Navigation
TODO

## Building
Get started by running `make help`. This will display a list of make targets:
```shell
487/587 ML Framework Help:
        Usage: make <target> [params]

 Targets:
        build:          Builds the framework (same as 'make')
                        NOTE: header files (.h/.hpp) changes are not detected, please 'clean' first
        rebuild:        Performs a 'clean' then 'build'
        build_debug:    Same as 'build', but with without optimizations and debug information
        redebug:        Performs a 'clean' then 'debug' build
        clean:          Cleans all build artifacts
        update:         Checks for a framework update. If one is found, it is pulled
        submit:         Zips directory for submission
        help:           Shows this help message
```
To build the framework, run `make build`. To run the build binary, run `./build/ml`. This will run some basic checks to ensure that your framework is built correctly.

## Flags
Additionally, there are flags which can change the compilation/operation of the framework in different ways. To use them, add them to your `make` command as such:
```shell
CPPFLAGS="<Flag 1> <Flag 2>" make rebuild
```
For example, to disable timers in the code, you would compile with the following:
```shell
CPPFLAGS="DISABLE_TIMING" make rebuild
```

Below is a list of those flags:
- `DISABLE_TIMING` Functionally disable all timers implemented using the built-in timing functionality.


