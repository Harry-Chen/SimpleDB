# SimpleDB
A project for the database class, Fall 2017, Tsinghua University

## Dependencies
* `bison` and `flex`
* `gcc` >= 5.4.0
* `CMake` >= 3.0.2
* `Doxygen` >= 1.8.0 and `graphviz` >= 2.38.0 (optional for generating documentation)

## Environment 
Tested on Linux, including WSL Ubuntu 16.04 and Arch Linux, with latest version of GCC.  
The code can be built on Windows with MinGW, but not tested.  


## Build without tests  

```bash
mkdir build && cd build
cmake .. && make
cd src # the executive name is 'SimpleDB'
```  
Please do not build in `Release` or `RelWithDebInfo` mode, for there will be strange behaviours.

## Execute
The program always read from `STDIN`. However you can do a redirection to read from file.  
```bash
./SimpleDB # the simple way
./SimpleDB db_name # automatically USE db_name
./SimpleDB db_name init < foo.sql # execute foo.sql in initialization mode
```  
If you are inserting a huge amount of data, *please* be sure to use initialization mode!  
When in initialization mode, all constraints will be ignored when inserting or modifying records in order to increase the speed.


## Build with tests  

```bash
mkdir build && cd build
cmake .. -DENABLE_TEST=ON # Unix-Like systems (including WSL)
cmake .. -DENABLE_TEST=ON -Dgtest_disable_pthreads=ON # Windows (even if using MinGW)
make
make test #run the test
```  
  
## Generate documentation
Just run `doxygen` in the root directory of source, then open `doc/html/index.html`.
