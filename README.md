Metta Grabber prototype
=======================

### First prototype requirements

* should be possible to add a device to personal device "cluster", thus allowing it to search the entire cluster and modify (optionally) data in the cluster.
* should be possible to remove a device from cluster, there might be no access to this device anymore (e.g. lost phone)
* devices in the cluster always keep track of status for other devices in the cluster and synchronize with them often.
* synchronized objects - some blobs with associated metadata. blob structure is not defined and is not interesting to the synchronization layer. metadata is a key-value store, with fine-grained synchronization.
* search between devices is performed in metadata index, which devices try to synchronize with priority to other objects.
* if found metadata points to a blob absent in local store, system tries to find the blob on other devices and synchronize it.
* for the first version storage area can be limited to devices within the cluster. in general, blobs could be redundantly and securely stored on any nodes across the network.

### Future routing possibilities

* UIP-like routing (EID's + extended DHT based routing through EID space)

### GUI features

* monitor Files folder for stuff to sync.
* metadata assignments - how? extract as much as possible automatically.
* how to perform search? an ESC-interface sounds intriguing

Dependencies
============

* Qt4 at http://qt-project.org/ (QtCore, QtNetwork; QtXml for UPnP; QtGui for demo apps)
* cmake at http://cmake.org/
* boost at http://boost.org/

aptly
 $ apt-get install git cmake clang libboost-test1.50-dev libqt4-dev libssl-dev libasound2-dev

 With these MettaNode also compiles and runs in Raspberry Pi's raspbian.

Included in this repository:

* sst from http://pdos.csail.mit.edu/uia/sst/
* opus at http://opus-codec.org/
* rtaudio at http://www.music.mcgill.ca/~gary/rtaudio/
* miniupnpc from https://github.com/miniupnp/miniupnp/tree/master/miniupnpc
* libnatpmp from http://thebends.googlecode.com/svn/trunk/nat/pmp

Not yet required

* libqxt at http://libqxt.org/

Typical config command
======================

cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -G Ninja -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug -DCLANG=ON

CI status
=========
[![Build Status](https://secure.travis-ci.org/berkus/mettanode.png)](http://travis-ci.org/berkus/mettanode)

Authors
=======
Design and development:
Stanislav Karchebny <berkus@exquance.com>

Code contributions:
Bogdan Lytvynovsky <>

Original SST and UIA development:
Bryan Ford <>

