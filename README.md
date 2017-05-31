# libdefsEval
Debug utility for finding missing files, directories, links, includes, and references used in lib.defs files for the OpenAccess file format.

This application is built using two boost libraries: boost/program_options and boost/filesystem.  This is an attempt to allow other operating systems other than Linux to take advantage of this application in the future.

Tested Requirements:  (You might be able to earlier versions, but this might require some alterations of code.) 
  Linux RHEL6
  Boost version 1_64_0 with header files
  
```sh
$ locate program_options.hpp
/usr/local/boost_1_64_0/boost/program_options.hpp
$ locate locate filesystem/operations.hpp
/usr/local/boost_1_64_0/boost/filesystem/operations.hpp
```
If these two files are not found on your system, please go to http://boost.org to get them.

To get started, here are the initial steps you will need to run.
```sh
$ git clone https://github.com/EDDRSoftware/libdefsEval.git
$ cd libdefsEval
$ locate program_options.so
/usr/local/boost_1_64_0/stage/lib/libboost_program_options.so
$
```

Use your favorite editor and edit the lines pertaining to BOOST.  These should start around line 34.
The original looks like this:

```sh
BOOST_ROOT = /usr/local/boost_1_64_0
BOOST_LIB  = $(BOOST_ROOT)/stage/lib
BOOST_INC  = $(BOOST_ROOT)/boost
```

Note that the BOOST_ROOT should point to your version of boost.  The locate statement from above should help you decipher where your BOOST_ROOT should point.  If you have more than one version of the library listed from the locate statement, I would suggest using the newest version.  This might require you to add or update a LD_LIBRARY_PATH environment variable to your system.

Once you have updated the BOOST_ROOT path in the Makefile, you should be able to run make.

```sh
$ make
g++ -o ./libdefseval main.cpp -I/usr/local/boost_1_64_0 -I/usr/local/boost_1_64_0/boost -L/usr/local/boost_1_64_0/stage/lib -lboost_program_options -lboost_filesystem -lboost_system
$ ls libdefseval
libdefseval
```

Congratulations!  You have successfully built the application.

To run a test on the testData directory, just run...

```sh
$ make test2
LD_LIBRARY_PATH=/usr/local/boost_1_64_0/stage/lib ./libdefseval --def ./testData/lib.defs --libs --cells --views
def: ./testData/lib.defs
DEFINE: /home/username/Development/libdefsEval_orig/testData/lib.defs:2 "DEFINE my_lib $PWD/testData/library"
libPath: /home/username/Development/libdefsEval_orig/testData/library
        libName: my_lib
                cellName: cell1
                        viewName: layout
                        viewName: schematic
                cellName: cell2
                cellName: cell3
ERROR: Recursion in file => "/home/username/Development/libdefsEval_orig/testData/lib.defs" includes file => "/home/username/Development/libdefsEval_orig/testData/lib.defs" which references itself.
INVALID LINE: /home/username/Development/libdefsEval_orig/testData/lib.defs:4 => DEFINE test #This line is treated as an error to help debug.
ERROR: No such file or directory "/home/username/Development/libdefsEval_orig/testData/here"
INVALID LINE: "/home/username/Development/libdefsEval_orig/testData/lib.defs":5 => DEFINE missing_lib $PWD/testData/here
DEFINE: /home/username/Development/libdefsEval_orig/testData/lib.defs:6 "DEFINE dir_link_lib ./directory_link"
libPath: /home/username/Development/libdefsEval_orig/testData/library
        libName: dir_link_lib
                cellName: cell1
                        viewName: layout
                        viewName: schematic
                cellName: cell2
                cellName: cell3
INVALID LINE: /home/username/Development/libdefsEval_orig/testData/lib.defs:7 => INCLUDE #This line is treaded as an error to help debug.
ERROR: Recursion in file => "/home/username/Development/libdefsEval_orig/testData/lib.defs" includes file => "/home/username/Development/libdefsEval_orig/testData/lib.defs" which references itself.
DEFINE: /home/username/Development/libdefsEval_orig/testData/new_dir/symbol.inc:1 "DEFINE test_lib ../library"
libPath: /home/username/Development/libdefsEval_orig/testData/library
        libName: test_lib
                cellName: cell1
                        viewName: layout
                        viewName: schematic
                cellName: cell2
                cellName: cell3
ERROR: Path "/home/username/Development/libdefsEval_orig/testData/./broken_link" contains a broken symlink.
INVALID LINE: "/home/username/Development/libdefsEval_orig/testData/lib.defs":10 => DEFINE broken_lib ./broken_link
$ 
```
Notice that the ‘test2’ shows plenty of examples of failures. This is intended for the test. Also, note the first line of output has set the LD_LIBRARY_PATH to include the boost libraries needed during execution. LD_LIBRARY_PATH=/usr/local/boost_1_64_0/stage/lib You can also set these permanently by adding the line below to your ~/.bashrc file. Keep in mind that the path should be relevant to the location of the boost libraries on your machine.

```sh
export LD_LIBRARY_PATH=/usr/local/boost_1_64_0/stage/lib
```
