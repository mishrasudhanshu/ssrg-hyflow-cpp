#!/bin/bash

echo "Cleaning the old stuff"
make clean -C ../Debug 2>&1 | tee >/dev/null

echo "build the Project"
RELEASE=1 make -j16 -C ../Debug 2>&1 | tee >/dev/null

echo "Running the Project"
cd ..
Debug/ssrg-hyflow-cpp
