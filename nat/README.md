Libraries for NAT traversal and hole punching
=============================================

* miniupnpc from https://github.com/miniupnp/miniupnp/tree/master/miniupnpc
* libnatpmp from http://thebends.googlecode.com/svn/trunk/nat/pmp

Notes
=====

- NAT relaying (using accessible server S to forward packets between A and B) - TURN protocol
- Connection reversal (via server S, but while only one of A, B is behind NAT)
- UDP Hole punching (via server S)
- hairpin (loopback) translation?
