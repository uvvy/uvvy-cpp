#!/usr/bin/env sh

git clone git://github.com/martine/ninja.git
cd ninja
git checkout release
./bootstrap.py || exit 1
sudo cp ninja /usr/bin/ || exit 1
