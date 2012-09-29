#!/bin/bash

echo "Cleaning the old stuff"
make clean -C Debug 2>&1 | tee >/dev/null
make clean -C Release 2>&1 | tee >/dev/null

echo "build the Project"
make -j16 -C Debug 2>&1 | tee >/dev/null
make -j16 -C Release 2>&1 | tee >/dev/null

echo "Running the Project"
#Debug/ssrg-hyflow-cpp
