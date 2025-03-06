# Memory management and packet buffers.

## Overview

A packet is represented throughout its lifecycle in the OmniTalk code by a packet buffer.  A packet buffer is a value of type packet_t (in mem/buffers.h).  Packet buffers are allocated on the heap by newbuf(), and freed by freebuf().  Do not use free(), as this will leak the actual packet memory.  Buffers containing ethernet frames can be turned into buffer_ts using wrapbuf(), since 	

The point of having a rich packet buffer type is to avoid copying packet data.  Under ESP-IDF, things like sending an Ethernet frame require all the frame data to be in one buffer, including the frame headers.  Ethernet headers are much longer than LocalTalk headers; so if you want to route a DDP packet from an LLAP L2 segment to an ELAP L2 segment without copying the header, we need to make sure that there is enough headroom in the buffer to add an Ethernet layer 2 header in place of the LocalTalk L2 header.

This is made slightly more complicated by the fact that short-form DDP packets have a header that overlaps with the LLAP L2 header; the first three fields of the DDP header are also the three fields of the LLAP L2 header.  So we can't just say that 'the DDP packet begins at a constant offset within the buffer, and the L2 header is before it'.  Fortunately, however, short-form DDP packets can't be routed, and aren't valid on ELAP segments *at all*; so we can largely ignore this case and just pretend that short-form DDP packets have a 3-byte L2 header distinct from the DDP header.

## The pointers in a buffer_t

A buffer_t contains four pointers of importance.  These are:

* `mem_top`: a pointer to the top of the allocated heap memory for the packet.  Unless you are fiddling with memory management, you should ignore this.
* `data`: a pointer to the top of frame data, which is at the start of the L2 header.
* `ddp_data`: a pointer to the start of the L3 packet.  Only valid when `ddp_ready` is set.
* `ddp_payload`: a pointer to the payload of the DDP packet.

Generally, L2 transports and LAPs will look at ->data, control plane gubbins will look at ->ddp_data and ->ddp_payload, and the routing code will look at ->ddp_data.

## The lifetime of a buffer (routing)

A buffer starts its life in a transport.  Either a new buffer_t will be created with the appropriate L2 header headroom and then filled with data from the wire, or an Ethernet buffer from the wire will be wrapped to create a buffer.   This buffer is passed through the transport's inbound queue to the LAP.

The LAP is responsible for taking this frame and turning it into a DDP packet ready for processing.  This means using the helper functions to set up the ddp pointers within the buffer, and determining whether it is addressed to the router or not.  (The LAP is also in charge of address acquisition and other things that are LAP-specific, so it makes sense to do this here.  If the packet is not addressed to the router itself, then it is passed to the router data plane.  At this point the recv_chain is filled in with the transport and LAP instance that the packet travelled through.

The router data plane then looks up the desired DDP next hop for the packet and fills in the send_chain with the appropriate details.  The buffer is then sent over the outbound queue to the LAP responsible for the egress port of the packet.

The LAP's outbound routine then replaces the old L2 header with a new one generated from the DDP address within the packet and the next hop information in the send_chain.  This will move the data pointer to point to the head of the new layer 2 header.  The LAP then sends the buffer to the outbound queue of its corresponding transport.

The transport may, or may not, decide to muck with the L2 header still further; for example, adding the extra L2 header bytes used by LToUDP for the process identifier.  It then sends the packet, starting with the l2 header pointed to by ->data.  It is then the transport's responsibility to free the buffer.

## The lifetime of a buffer (addressed to router).

If a LAP determines that a packet is addressed to the router itself (i.e. is a direct unicast to the port's address or is a broadcast), it will instead be sent over the control plane's inbound queue.  The control plane will pick it up and dispatch it to the correct handler for the protocol involved.

That protocol handler can do one of two things: in simple cases (such as echo), it can re-use the buffer that was used for the incoming packet, construct the outgoing packet within it, and introduce that into the router data plane to be sent.  In more complex cases, it can construct new buffers, one for each packet it needs to respond with, and send those.  If it follows this approach, it is responsible for freeing the incoming buffer.
