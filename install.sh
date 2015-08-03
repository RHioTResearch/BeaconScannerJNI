#!/usr/bin/env bash

if [ ! -e Debug ]; then
    mkdir Debug
fi
cd Debug
cmake .. -DCMAKE_BUILD_TYPE=Debug ..
cd ..
cmake --build Debug --target install
