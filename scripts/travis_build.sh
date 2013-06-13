#!/usr/bin/env sh

set -x

ncpus=$(grep -c processor /proc/cpuinfo|wc -l)
echo "Building on $ncpus processors"

mkdir -p _build_
cd _build_
cmake -DTRAVIS_CI=YES -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=YES -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=`pwd`/dist/mettanode .. || exit 1
make -j$ncpus install || exit 1
ctest || exit 1
