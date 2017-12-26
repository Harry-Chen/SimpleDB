# SimpleDB
A project for the database class, Fall 2017, Tsinghua University

## Dependencies
* `bison` and `flex`
* `CMake` >= 3.0.2

## Environment 
Tested on Linux, including WSL Ubuntu 16.04 and Arch Linux, with latest version of GCC.


## Build without tests
```bash
mkdir build && cd build
cmake .. && make
cd src
./SimpleDB #execute it!
```  
Please do not build in Release or RelWithDebInfo mode, for there will be strange behaviours.


## Build with tests
```bash
mkdir build && cd build
cmake .. -DENABLE_TEST=ON # Unix-Like systems (including WSL)
cmake .. -DENABLE_TEST=ON -Dgtest_disable_pthreads=ON # Windows (even if using MinGW)
make
make test #run the test
```
