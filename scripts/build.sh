#!/bin/bash

# TODO: Add checks for required libraries:
# libboost-serialization-dev
# libboost-thread-dev
# libpthread
# libzmq
# libtbb

echo "Cleaning the old stuff"
make clean -C Debug | tee >/dev/null
make clean -C Release | tee >/dev/null

echo "build the Project"
make -j16 -C Debug | tee >/dev/null
make -j16 -C Release | tee >/dev/null

echo "Running the Project"
#Debug/ssrg-hyflow-cpp
