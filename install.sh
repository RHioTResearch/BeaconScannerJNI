#!/usr/bin/env bash

static=0
while getopts s opt
do
  case $opt in
    s) static=1 ;;
    *) exit 1 ;;
  esac
done

if [ ! -e Debug ]; then
    mkdir Debug
fi

if [ $static -gt 0 ]
then
   echo "Using static linking against BlueZ"
   export BluezLinkType="static"
fi

cd Debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
cd ..
cmake --build Debug --target install
