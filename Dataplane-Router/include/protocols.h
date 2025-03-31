#ifndef __PROTOCOLS_H__
#define __PROTOCOLS_H__ 1

#include <unistd.h>
#include <stdint.h>

/* Ethernet ARP packet from RFC 826 */
struct arp_hdr {
	uint16_t hw_type;   /* Format of hardware address */
	uint16_t proto_type;   /* Format of protocol address */
	uint8_t hw_len;    /* Length of hardware address */
	uint8_t proto_len;    /* Length of protocol address */
	uint16_t opcode;    /* ARP opcode (command) */
	uint8_t shwa[6];  /* Sender hardware address */
	uint32_t sprotoa;   /* Sender IP address */
	uint8_t thwa[6];  /* Target hardware address */
	uint32_t tprotoa;   /* Target IP address */
} __attribute__((packed));

/* Ethernet frame header*/
struct  ether_hdr {
		uint8_t  ethr_dhost[6]; // Destination MAC address
		uint8_t  ethr_shost[6]; // Source MAC address
		uint16_t ethr_type;     // Identifier of the encapsulated protocol
};

/* IP Header */
struct ip_hdr {
		uint8_t    ihl:4, ver:4;// we use version = 4
		uint8_t    tos;         // Nu este relevant pentru temă (set pe 0)
		uint16_t   tot_len;     // total length = ipheader + data
		uint16_t   id;          // Nu este relevant pentru temă, (set pe 4)
		uint16_t   frag;        // Nu este relevant pentru temă, (set pe 0)
		uint8_t    ttl;         // Time to Live -> to avoid loops, we will decrement
		uint8_t    proto;       // Identificator al protocolului encapsulat (e.g. ICMP)
		uint16_t   checksum;    // checksum     -> Since we modify TTL,
		uint32_t   source_addr; // Adresa IP sursă  
		uint32_t   dest_addr;   // Adresa IP destinație

		ip_hdr() : ihl(5), ver(4), tos(0), tot_len(0), id(4), frag(0), 
							 ttl(64), proto(0), checksum(0), source_addr(0), dest_addr(0) {
		}
};

/**
 * @brief
 * Destination unreachable => @param mtype = 3, @param mcode = 0
 * Time exceeded           => @param mtype = 11, @param mcode = 0
 * Echo request            => @param mtype = 8, @param mcode = 0
 * Echo reply              => @param mtype = 0, @param mcode = 0
 */
struct icmp_hdr
{
	uint8_t mtype;                /* message type */
	uint8_t mcode;                /* type sub-code */
	uint16_t check;               /* checksum */
	union
	{
		struct
		{
			uint16_t        id;
			uint16_t        seq;
		} echo_t;                        /* echo datagram.  Vom folosi doar acest câmp din union*/
		uint32_t        gateway_addr;        /* Gateway address. Nu este relevant pentru tema */
		struct
		{
			uint16_t        __unused;
			uint16_t        mtu;
		} frag_t;                        /* Nu este relevant pentru tema */
	} un_t;
};

#endif /* __PROTOCOLS_H__ */
