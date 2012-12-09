Libraries for NAT traversal and hole punching
=============================================

* miniupnpc from https://github.com/miniupnp/miniupnp/tree/master/miniupnpc
* libnatpmp from http://thebends.googlecode.com/svn/trunk/nat/pmp
* upnp based on upnp implementation in ktorrent http://ktorrent.sourceforce.net


- http://pupnp.sourceforge.net (BSD License) latest update Apr 2012, first code 2005
- http://www.keymolen.com/ssdp.html C++ SSDP library, non-OSS
- https://code.google.com/p/upnpx/ objc/C++ upnp and ssdp lib, BSD License, based on pupnp, abandoned Nov 2012

Notes
=====

- NAT relaying (using accessible server S to forward packets between A and B) - TURN protocol
- Connection reversal (via server S, but while only one of A, B is behind NAT)
- UDP Hole punching (via server S)
- hairpin (loopback) translation?
