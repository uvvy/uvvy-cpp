#!/usr/bin/env sh

set -x

ncpus=$(grep -c processor /proc/cpuinfo|wc -l)
echo "Building on $ncpus processors"

git clone http://llvm.org/git/libcxx.git
mkdir -p libcxx/_build_
cd libcxx/_build_
# LIBCXX_LIBSUPCXX_INCLUDE_PATHS will break with libstdc++/gcc upgrade.
CC=clang CXX=clang++ cmake -G "Unix Makefiles" -DLIBCXX_CXX_ABI=libsupc++ -DLIBCXX_LIBSUPCXX_INCLUDE_PATHS="/usr/include/c++/4.6;/usr/include/c++/4.6/x86_64-linux-gnu/" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr .. || exit 1
make -j$ncpus || exit 1
sudo make install || exit 1
