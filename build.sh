# Build with all cores
cores=`grep --count ^processor /proc/cpuinfo`
make -j $cores