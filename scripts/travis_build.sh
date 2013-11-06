#!/usr/bin/env sh

set -x

mkdir -p _build_
cd _build_
CC=/usr/bin/clang CXX=/usr/bin/clang++ cmake -DTRAVIS_CI=YES -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=YES -G "Ninja" -DCMAKE_INSTALL_PREFIX=`pwd`/dist/mettanode -DBOOST_LIBRARYDIR=/usr/lib64 .. || exit 1
ninja -j1 install || exit 1
ctest -V || exit 1
