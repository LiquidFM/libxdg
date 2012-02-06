#!/bin/bash

cd debug
cmake -G "Unix Makefiles" -D CMAKE_BUILD_TYPE:STRING=$1 ../
make
cd ../
