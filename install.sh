#!/usr/bin/env bash

mkdir Debug
cd Debug
cmake .. -DCMAKE_BUILD_TYPE=Debug ..
cd ..
cmake --build Debug --target scannerJni
