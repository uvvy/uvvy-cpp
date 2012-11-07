#!/usr/bin/env sh

mkdir -p _build_
cd _build_
cmake -G "Unix Makefiles" .. || exit 1
make -j2 || exit 1

