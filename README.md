replicode
=========

a constructivist programming language and runtime.

This repo is forked from [https://github.com/dmacd/replicode](https://github.com/dmacd/replicode)

This project is built with [vscode](https://code.visualstudio.com/), to build cmake and a modern c++ compiler is required.

# setup
sudo apt install build-essential
sudo apt install cmake
sudo apt install libboost-log-dev
git submodule add git://github.com/arsenm/sanitizers-cmake.git externals/sanitizers-cmake (see: https://github.com/arsenm/sanitizers-cmake)

# build
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
```bash
cd ../executor
./../build/replicode
```

To change the settings, edit `executor/settings.ini` then rebuild.

## notes
Only the v1.2 examples really function due to syntax changes.

## code structure
The main program execution for the example happens in executor/main.cpp
- main() // loads the settings.ini and the seed image, starts up r_exec/mem.cpp and then waits for run_time and ends by saving the results to disk
- r_exec/mem.cpp starts up the runReductionCore and the runTimeCore threads which do the magic
- The r_exec/pattern_extractor.cpp and r_exec/g_monitor.cpp push new reduction and time jobs to the job queues