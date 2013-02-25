#!/bin/bash

cmake -G "Unix Makefiles" -D CMAKE_BUILD_TYPE:STRING=$1 -D CMAKE_INSTALL_PREFIX:STRING=/home/dav/xdg ../
make && make install
cd ../
