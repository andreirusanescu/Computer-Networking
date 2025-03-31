# Copyright Andrei-Marian Rusanescu 323CC 2024-2025

## Constants and structures used
To begin with, I wrote the router in C++ and made use of the `standard template 
library` `queue` and `unordered_map`. I defined myself a structure
`packet_t` whose purpose is to retain the length of the packet that is to
be transmitted in the queuing process, i.e. when the packet cannot be
sent because the destination's MAC address is unknown.

Furthermore, to make the code more readable for future premises, I
used a few constants with suggestive names for the encapsulated protocols
in case of the `ethernet` header and `ICMP` or `ARP` primitives. Due to the
fact that macros and defines are not type checked by the compiler and do not
really have a type I decided to be clear and make my constants typed. They
are also static to be visibile only in the "namespace" of the router.cpp file.

### RTable Class
This Class is `Singleton` because a router has only one routing table. It 
deals with reading the routing table from a file passed as an argument and
implements the `Longest Prefix Match` algorithm using a `Trie`.
The LPM  algorithm is useful to get the best prefix that matches the IP
destination given. It does so by processing each bit of the `ip_dest` in host
order from MSB to LSB. As each bit is being processed, the node is "moving"
down the trie until it reaches a null node which means the longest prefix
has been found. Along this search, each time there is a routing table entry
present in the node, it is saved. The routing table entry is saved at
the end of the prefix in the trie. The insertion of an entry is done in
O(mask_length). For example, if the mask is /24, there is no reason to
process 32 bits of the prefix as only the first 24 (from MSB to LSB) are 
important.

### The main loop
Firstly, the ethernet header is being checked for the protocol: `IP` or
`ARP`.

### IP protocol
The checksum is checked firstly for the IP header. If the checksum fails the
packet is dropped. Then for each interface of the router it is checked if the
coming packet is destined for the router. Each interface is checked due to the
following scenario: what if someone pings the router on another interface may not
be the interface the packet came through. The router treats only the
`ICMP` packets. After these checks if indeed the packet is destined for the router
its icmp checksum is checked and if it is valid, the router sends an `ECHO REPLY` to
the one who sent it.
If the packet was not destined for the router, there are a few situations to be
treated. The `TTL` field of the IP header is checked. If it is greater than 1, it
is just decremented and the checksum recalculated. Otherwise the one who sent
the packet will get a `TIME EXCEEDED` ICMP message. Then the router searches for the
destination of this packet in the `routing table`. If it cannot find the best route
the one who sent this packet is being send a `DESTINATION UNREACHABLE` ICMP message.

### ARP protocol & table
The ARP table is a hashtable or `unordered_map` in C++ and maps `uint32_t` ip addresses
to `uint8_t[6]` MAC addresses. After the best route was found, now the router needs to
know what the next hop is in order to forward the packet further. If the ARP table
doesn't have an entry for this next hop, an `ARP Request` is broadcasted by the router,
asking if anyone knows the MAC address of the destination. The packet that had to be
forwarded is queued for later.
In the case of the ARP table knowing the MAC address of the destination, the packet
is being forwarded to the best_route interface.

The router can process `ARP Requests` and `ARP Replies` too. If the ethernet header
protocol is ARP, it means it is one of the two cases mentioned above.

1. `ARP Reply`
In the case of an ARP Reply, the header of it contains the mac address of the source,
i.e. the one who we sent an ARP Request for. So it means some of the packets that were 
queued can be now forwarded. If they cannot be forwarded, another queue is used as
an auxiliary queue for storing the packets that we do not know the MAC address yet.
In the of this processing of the queue, our main queue is swapped with this auxiliary 
one that may still carry packets to be processed.

2. `ARP Request`
The router can get ARP requests too and when it gets one it saves the IP -> MAC address
entry of the source in the ARP table. It responds only if the router is the target
of this ARP requests with the MAC address of the interface it came through.

In the end, the memory used by the Routing Table is freed.
