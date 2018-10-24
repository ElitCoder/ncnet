#!/bin/bash

# Number of threads
cores=`grep --count ^processor /proc/cpuinfo`

# Clean
./clean.sh

# Make and install
make -j $cores && sudo make install