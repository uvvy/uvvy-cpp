MettaNode grabber prototype
===========================

[![Join the chat at https://gitter.im/metta-systems/uvvy](https://badges.gitter.im/metta-systems/uvvy.svg)](https://gitter.im/metta-systems/uvvy?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

MettaNode is a tool for fully decentralized communications - grab data you like and store it forever, share it with your friends, start chats, voice and video calls, form groups by interest, transparently keep all your notes between all of your devices; all based on a simple ideas of [UIA](http://pdos.csail.mit.edu/uia/). It is still young and only base transport protocol is done, work now continues on overlay routing network.

Final target is to have a bunch of clients for desktop and mobile platforms (Win, Mac, Linux, Android, iOS) as well as own operating system implementation ([Metta](https://github.com/berkus/metta/wiki)) running together.

Progress updates in [TODO](TODO).

Dependencies
============

* C++14 (right now, only Clang and libc++)
* [Qt5](http://qt-project.org/) (QtCore, QtNetwork; QtXml for UPnP; QtGui for demo apps)
* [cmake](http://cmake.org/)
* [boost](http://boost.org/)

aptly
```
 $ apt-get install git cmake clang libboost-test1.50-dev libqt4-dev libssl-dev libasound2-dev
```

Included in this repository:

* [opus](http://opus-codec.org/)
* [rtaudio](http://www.music.mcgill.ca/~gary/rtaudio/)
* [miniupnpc](https://github.com/miniupnp/miniupnp/tree/master/miniupnpc)
* [libnatpmp](http://thebends.googlecode.com/svn/trunk/nat/pmp)

Typical config command
======================
```
cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -G Ninja \
 -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug
```

CI status
=========
[![Build Status](https://secure.travis-ci.org/berkus/mettanode.png)](http://travis-ci.org/berkus/mettanode)
[![Bitdeli Badge](https://d2weczhvl823v0.cloudfront.net/berkus/mettanode/trend.png)](https://bitdeli.com/free "Bitdeli Badge")

Authors
=======
Design and development:
[Stanislav Karchebny](http://exocortex.madfire.net)

Code contributions:
Bogdan Lytvynovsky

Original SST and UIA development:
[Bryan Ford](http://www.brynosaurus.com)

