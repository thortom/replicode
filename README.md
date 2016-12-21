replicode
=========

a constructivist programming language and runtime.

This repo is forked from [https://github.com/sandsmark/replicode](https://github.com/sandsmark/replicode)

Changes so far:

- osx build configuration
- executor & v1.2 examples now copied to build path for consistent runtime path structure
- minor bug fixes

# build

to build cmake and a modern c++ compiler is required:

```bash
    mkdir build
    cd build
    cmake ..
    make
```

# test
An optional step after building is to run the automated tests:

```bash
    make test
```

# run examples

    cd build/executor
    ../replicode

To change the settings, edit `executor/settings.ini` then rebuild. This copies the settings file to the build path where it is referenced by the executable.


## notes

Only the v1.2 examples really function due to syntax changes.
