#!/usr/bin/env sh
# Build a c++11 libc++ based boost libraries we need for linux.
# Based on brew recipe.

set -x

wget -O boost_1_54_0.tar.bz2 http://sourceforge.net/projects/boost/files/boost/1.54.0/boost_1_54_0.tar.bz2/download
tar xjf boost_1_54_0.tar.bz2
cd boost_1_54_0
cat <<EOF > user-config.jam
using clang-linux : : /usr/bin/clang++-3.3 ;
EOF
./bootstrap.sh --prefix=/usr --libdir=/usr/lib64 \
	--with-toolset=clang \
	--with-libraries=system,date_time,serialization,program_options,test \
	--without-icu
sudo ./b2 --prefix=/usr --libdir=/usr/lib64 -d2 -j2 --layout=system \
	--user-config=user-config.jam threading=multi install toolset=clang \
	cxxflags=-std=c++11 cxxflags=-stdlib=libc++ cxxflags=-fPIC cxxflags=-m64 \
	linkflags=-stdlib=libc++ linkflags=-m64
