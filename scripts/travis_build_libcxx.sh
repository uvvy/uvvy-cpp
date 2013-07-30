#!/usr/bin/env sh

set -x

# Print libsupc++ include paths
echo | g++ -Wp,-v -x c++ - -fsyntax-only

git clone http://llvm.org/git/libcxx.git
mkdir -p libcxx/_build_
cd libcxx/_build_
# LIBCXX_LIBSUPCXX_INCLUDE_PATHS will break with libstdc++/gcc upgrade.
CC=clang-3.3 CXX=clang++-3.3 cmake -G "Ninja" -DLIBCXX_CXX_ABI=libsupc++ -DLIBCXX_LIBSUPCXX_INCLUDE_PATHS="/usr/include/c++/4.6;/usr/include/c++/4.6/x86_64-linux-gnu/" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr .. || exit 1
ninja || exit 1
sudo ninja install || exit 1
