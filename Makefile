# 
# MIT License
# 
# Copyright (c) 2017 EDDR Software, LLC.
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
#
# Changes:
# 2017-05-05: First & Last Name: What you did. 
# 2017-05-31: Kevin Nesmith: Fixed a typo.
#

default: build

TARGET     = ./libdefseval
CXX        = g++
CXX_FILES  = main.cpp
BOOST_ROOT = /usr/local/boost_1_64_0
BOOST_LIB  = $(BOOST_ROOT)/stage/lib
BOOST_INC  = $(BOOST_ROOT)/boost

build:
	$(CXX) -o $(TARGET) $(CXX_FILES) -I$(BOOST_ROOT) -I$(BOOST_INC) -L$(BOOST_LIB) -lboost_program_options -lboost_filesystem -lboost_system

version:
	LD_LIBRARY_PATH=$(BOOST_LIB) $(TARGET) -v

test:
	LD_LIBRARY_PATH=$(BOOST_LIB) $(TARGET) --help

test1:
	LD_LIBRARY_PATH=$(BOOST_LIB) $(TARGET) --def ./testData/lib.defs

test2:
	LD_LIBRARY_PATH=$(BOOST_LIB) $(TARGET) --def ./testData/lib.defs --libs --cells --views

test3:
	LD_LIBRARY_PATH=$(BOOST_LIB) $(TARGET) --def ./testData/lib.defs --libs --cellviews

test4:
	LD_LIBRARY_PATH=$(BOOST_LIB) $(TARGET) --def ./testData/lib.defs --libs --cells --views --comments --emptylines

test5:
	LD_LIBRARY_PATH=$(BOOST_LIB) $(TARGET) --def ./testData/lib.defs --libs --cellviews --comments

test6:
	mkdir -p /tmp/here\ there/cell1
	LD_LIBRARY_PATH=$(BOOST_LIB) $(TARGET) --def ./testData/lib.inc --libs --cells --views --comments

clean:
	rm $(TARGET)

