Jupiter Compiler and Runtime
=====

What is Jupiter?
-----
Jupiter is an multi-purpose, experimental language I've been building.
It uses a unique variation on the Hindley-Milner type inference system, with full support for overloaded methods.
The compiler targets the LLVM IR, for maximum speed and portability.  

Primary Aims
-----
* Simple yet powerful syntax and features for easy and rapid development
* Full type safety without verbose type annotations
* Expressivity through rich operator overloading
* Fast performance

Requirements
-----
Required to build and use the Jupiter compiler:

* Unix-like environment (Unix or Cygwin)
* Clang (currently being developed with `clang++` version 3.4.2)
* LLVM toolchain (`llc` version 3.4.2)

How to Build Jupiter
-----
`make jup` builds the Jupiter to LLVM IR compiler

`make jupc` builds the toolchain for automating the build process

`make runtimelib` builds the runtime library

`make tests` builds Jupiter programs location in `examples/`

Running `make all` will build all of the above.
