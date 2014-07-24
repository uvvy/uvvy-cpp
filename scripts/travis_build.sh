#!/usr/bin/env sh
# BUILD_TYPE comes from Travis environment.

set -x

mkdir -p _build_
cd _build_
CC=/usr/bin/clang CXX=/usr/bin/clang++ cmake -DTRAVIS_CI=YES -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DBUILD_TESTING=YES -G "Ninja" -DCMAKE_INSTALL_PREFIX=`pwd`/dist/$BUILD_TYPE/mettanode -DBOOST_LIBRARYDIR=/usr/lib64 .. || exit 1
ninja -j1 install || exit 1

# byte_array, flurry and crypto unit tests suffer from
# ERROR: AddressSanitizer: alloc-dealloc-mismatch (operator new [] vs operator delete)
# This is seemingly a bug in libc++
# Disable asan checks for alloc-dealloc-mismatch until libc++ is sorted out.
ASAN_OPTIONS=alloc_dealloc_mismatch=0 ctest || exit 1
