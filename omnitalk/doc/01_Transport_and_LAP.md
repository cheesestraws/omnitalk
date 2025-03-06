# Transports and LAPs and Ports

## Introduction

In a traditional AppleTalk router, the dominant abstraction for network interfaces is called a 'Port'.  A port is somewhat equivalent to what IP routers would call something like a layer 3 interface: it is an abstraction that deals with the stuff that glues layers 3, 2 and sometimes 1 together.  A port is responsible for doing its own address acquisition, for managing address collisions, for managing hardware addresses, and for putting frames onto the wire.

OmniTalk splits the port abstraction into two parts, the Transport and the LAP.  The transport is responsible for actually sending and receiving frames, and does nothing else.  The LAP is responsible for managing the relationship between layer 3 and layer 2, and generating the frames to send.

This split is useful because it is no longer the case that a single LAP has a single layer 2 implementation.  There is encapsulation in play; LLAP is used not just for LocalTalk, but for LToUDP (and LToE, which maybe one day we will support) as well, and ELAP is used both for real Ethernet but also for WiFi (which maybe one day we will support) and for Basilisk II's tunnelled ethernet support.  So in the current OmniTalk codebase, there are transports for LToUDP, LocalTalk via Tashtalk, Ethernet and B2 Ethernet Tunneling.

This allows, for example, an ELAP implementation to be written that is entirely agnostic with regards to whether it is running in UDP tunnelling or directly on Ethernet, and the specifics for each layer 2 or virtual layer 2 are dealt with entirely in the transport.  In the initial release of OmniTalk there will be two LAP implementations, one for LLAP (mostly functional at time of writing) and one for ELAP (totally missing at time of writing).

## Transports

A transport is represented by a transport_t.  Generally, transports are singletons; you would not expect, for example, to have multiple Ethernet transports (unless you had multiple Ethernet MACs, which currently we do not).  A transport has two queues, an inbound queue, where it sticks frames that are inbound to the router, and an outbound queue, where it picks up traffic that is outbound from the router.

Note that the terms 'inbound' and 'outbound' are always used relative to the router as a whole and not to the component of it currently being considered.

The transport's job is limited to receiving frames from the hardware or encapsulation and passing them to the LAP, and receiving packets from the LAP and sending them onwards either to the hardware or through the encapsulation.  

Transports live under the net/ hierarchy, each in its own subdirectory.

## LAPs

A LAP is represented by a lap_t.  Generally, LAPs are multiply instantiable; for example, one can have an LLAP LAP for LocalTalk and an LLAP LAP for LToUDP, and they'll be completely independent.  Similarly, one could have an ELAP LAP for Ethernet and an ELAP LAP for WiFi, and the two would have separate AARP tables, separate state, and would generally be independent.

LAPs are responsible for keeping the L2 to L3 glue going.  They're responsible for address acquisition and maintenance, for discovering network numbers, for maintaining L3 to L2 address mappings where appropriate (by which I mean AARP, at this point).  They are not responsible for maintaining their own routing tables; that is done centrally by the router control plane.

LAPs live under the lap/ hierarchy, each in its own subdirectory.

## "Ports"

There is no named abstraction for a port in the code, because a port is just a coupled pair of LAP and Transport.  These are constructed somewhat on the fly, by passing a reference to a transport into the constructor function for a LAP.  These port startup invocations live in start_net() in net/net.c.
