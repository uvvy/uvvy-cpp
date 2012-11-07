#!/usr/bin/env sh

mkdir -p _build_
cd _build_
cmake -G "Ninja" .. || exit 1
ninja || exit 1

