#!/bin/bash

# Check if the build folder exists
if [ ! -d "build" ]; then
    mkdir build
fi

cd build/

source ~/workspace2/SDK_2022.1/environment-setup-cortexa72-cortexa53-xilinx-linux

make clean

# Run CMake with specified options
cmake ..

# Compile the project using make with 4 parallel jobs
make -j4

# Copy the built binaries and configuration file to the remote server
scp admin root@192.168.2.150:/home/root/carpark/