# Build and install
dir=`pwd`
./build.sh && cd build && sudo ninja install
cd $dir
