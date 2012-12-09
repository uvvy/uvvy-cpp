#!/usr/bin/env sh

set -x

mkdir -p _build_
cd _build_
cmake -DBUILD_TESTS=YES -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=`pwd`/dist/mettanode .. || exit 1
make -j2 install || exit 1

