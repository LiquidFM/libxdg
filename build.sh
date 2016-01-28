#!/bin/bash

mkdir -p build install && cd build
cmake -G "Unix Makefiles" -D CMAKE_BUILD_TYPE:STRING=$1 -D CMAKE_INSTALL_PREFIX:STRING=../install ..
make && make doc && make install
cd ..
