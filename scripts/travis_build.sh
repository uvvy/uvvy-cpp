#!/usr/bin/env sh

set -x

mkdir -p _build_
cd _build_
cmake -DBUILD_TESTING=YES -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=`pwd`/dist/mettanode .. || exit 1
make -j3 install || exit 1
ctest || exit 1
