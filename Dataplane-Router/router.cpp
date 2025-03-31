#include <queue>
#include <unordered_map>

#include "lib.h"
#include "rtable.h"
#include "protocols.h"

using std::queue;
using std::unordered_map;

/* Packet for the queue */
struct packet_t {
	size_t len;
	char buf[MAX_PACKET_LEN];
};

/* Ethernet encapsulated protocols */
static const uint16_t ETHERTYPE_IP = 0x0800;
static const uint16_t ETHERTYPE_ARP = 0x0806;

/* ICMP */
static const uint8_t NO_CODE = 0;
static const uint8_t ECHO_REPLY = 0;
static const uint8_t DESTINATION_UNREACHABLE = 3;
static const uint8_t ECHO_REQUEST = 8;
static const uint8_t TIME_EXCEEDED = 11;

/* ARP */
static const uint16_t ARP_REQUEST = 1;
static const uint16_t ARP_REPLY = 2;
static const uint16_t HARDWARE_TYPE_ETHERNET = 1;

/* ARP table and queue for ARP Requests */
static unordered_map<uint32_t, uint8_t[6]> arp_table;
static queue<packet_t> packets_queue;

/**
 * @brief Sends ICMP @param packet of @param mtype on the @param interface
 * @param packet - packet to be sent
 * @param mtype - message type of the ICMP
 * @param interface - interface of the router to be sent on
 */
static void
send_icmp(packet_t &packet, size_t interface, uint8_t mtype, uint8_t mcode=NO_CODE) {
	ether_hdr *eth_header = (ether_hdr *)packet.buf;
	ip_hdr *ip_header = (ip_hdr *)(packet.buf + sizeof(ether_hdr));
	icmp_hdr *icmp_header = (icmp_hdr *)(packet.buf + sizeof(ether_hdr) + sizeof(ip_hdr));

	/* Ethernet */
	uint8_t temp[6];
	memcpy(temp, eth_header->ethr_dhost, 6);
	memcpy(eth_header->ethr_dhost, eth_header->ethr_shost, 6);
	memcpy(eth_header->ethr_shost, temp, 6);

	/* IP */
	ip_header->ttl = 64;
	uint32_t tmp = ip_header->dest_addr;
	ip_header->dest_addr = ip_header->source_addr;
	ip_header->source_addr = tmp;

	/* ICMP */
	icmp_header->mtype = mtype;
	icmp_header->mcode = mcode;

	switch (mtype) {
	case ECHO_REPLY:
		ip_header->checksum = 0;
		ip_header->checksum = htons(checksum((uint16_t *)ip_header, sizeof(ip_hdr)));
		icmp_header->check = 0;
		icmp_header->check = htons(checksum((uint16_t *)icmp_header, ntohs(ip_header->tot_len) - sizeof(ip_hdr)));
		send_to_link(packet.len, packet.buf, interface);
		break;
	case DESTINATION_UNREACHABLE:
	case TIME_EXCEEDED:
		ip_header->tot_len = htons(sizeof(ip_hdr) + sizeof(icmp_hdr) + sizeof(ip_hdr) + 8);
		ip_header->proto = IPPROTO_ICMP;
		ip_header->checksum = 0;
		ip_header->checksum = htons(checksum((uint16_t *)ip_header, sizeof(ip_hdr)));

		memcpy(icmp_header + sizeof(icmp_hdr), ip_header, sizeof(ip_hdr));
		memcpy(icmp_header + sizeof(icmp_hdr) + sizeof(ip_hdr), ip_header + sizeof(ip_hdr), 8);

		icmp_header->check = 0;
		icmp_header->check = htons(checksum((uint16_t *)icmp_header, sizeof(icmp_hdr) + sizeof(ip_hdr) + 8));
		send_to_link(sizeof(ether_hdr) + ntohs(ip_header->tot_len), packet.buf, interface);
		break;
	default:
		break;
	}
}

/**
 * @brief Wrapper over the find method of the arp_table.
 * @param given_ip - IP to be looked up in the table
 * @return returns MAC address corresponding to the @param given_ip.
 */
static
uint8_t *get_arp_entry(uint32_t given_ip) {
	auto entry = arp_table.find(given_ip);
	return entry != arp_table.end() ? entry->second : NULL;
}

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, 0);
	packet_t packet;

	// Do not modify this line
	init(argv + 2, argc - 2);

	/* Singleton pattern */

	RTable *rtable = RTable::get_instance();
	rtable->read_rtable(argv[1]);

	while (1) {
		size_t interface;

		interface = recv_from_any_link(packet.buf, &packet.len);
		DIE(interface < 0, "recv_from_any_links");

		/* Extract the Ethernet header from the packet. Since protocols are
		 * stacked, the first header is the ethernet header, the next header is
		 * at packet.buf + sizeof(struct ether_header) */
		ether_hdr *eth_header = (ether_hdr *)packet.buf;

		/* IP */
		if (ntohs(eth_header->ethr_type) == ETHERTYPE_IP) {
			ip_hdr *ip_header = (ip_hdr *)(packet.buf + sizeof(ether_hdr));

			/* Check L3 Checksum */
			uint16_t old_checksum = ntohs(ip_header->checksum);
			ip_header->checksum = 0;
			uint16_t check_sum2 = checksum((uint16_t *)ip_header, sizeof(ip_hdr));
			if (old_checksum ^ check_sum2) {
				/* DROP PACKET */
				continue;
			}

			bool sent = false;
			/* Checking if the router is the destination */
			for (int i = 0; i < ROUTER_NUM_INTERFACES; ++i) {
				char *router_ip = get_interface_ip(i);
				in_addr adr;
				inet_pton(AF_INET, router_ip, &adr);

				/* ICMP to router */
				if (ip_header->dest_addr == adr.s_addr) {

					if (ip_header->proto != IPPROTO_ICMP)
						continue;
					/* Respond */
					icmp_hdr *icmp_header = (icmp_hdr *)(packet.buf + sizeof(ether_hdr) + sizeof(ip_hdr));

					if (icmp_header->mtype != ECHO_REQUEST)
						continue;

					uint16_t recvd_icmp_check = icmp_header->check;
					icmp_header->check = 0;

					/* Checksum on everything left of the packet */
					uint16_t test_icmp_check = checksum((uint16_t *)icmp_header, ntohs(ip_header->tot_len) - sizeof(ip_hdr));

					/* Discard */
					if (test_icmp_check ^ ntohs(recvd_icmp_check))
						continue;

					/* Echo reply */
					send_icmp(packet, interface, ECHO_REPLY);
					sent = true;
					break;
				}
			}

			if (sent) continue;

			if (ip_header->ttl > 1) {
				--ip_header->ttl;
				ip_header->checksum = 0;
				ip_header->checksum = htons(checksum((uint16_t *)ip_header, sizeof(ip_hdr)));
			} else {
				send_icmp(packet, interface, TIME_EXCEEDED);
				continue;
			}
			route_table_entry *best_route = rtable->get_best_route(ip_header->dest_addr);
			if (!best_route) {
				/* Host_unreachable */
				send_icmp(packet, interface, DESTINATION_UNREACHABLE);
				continue;
			}

			uint8_t *destination_mac = get_arp_entry(best_route->next_hop);

			/* Create an ARP Request */
			if (!destination_mac) {
				/* Setup the interface for the time when it will be sent */
				get_interface_mac(best_route->interface, eth_header->ethr_shost);

				/* Enqueue the packet */
				packets_queue.push(packet);

				/* Create ARP Request Packet */
				char arp_req[MAX_PACKET_LEN];

				/* Ethernet Header for ARP */
				ether_hdr *arp_eth = (ether_hdr *)arp_req;
				arp_eth->ethr_type = htons(ETHERTYPE_ARP);

				/* Send ARP Req on best_route interface as Broadcast */
				get_interface_mac(best_route->interface, arp_eth->ethr_shost);
				hwaddr_aton("FF:FF:FF:FF:FF:FF", arp_eth->ethr_dhost);

				/* ARP Header */
				arp_hdr *arp_header = (arp_hdr *)(arp_req + sizeof(ether_hdr));
				arp_header->hw_type = htons(HARDWARE_TYPE_ETHERNET);
				arp_header->proto_type = htons(ETHERTYPE_IP);
				arp_header->hw_len = 6;
				arp_header->proto_len = 4;
				arp_header->opcode = htons(ARP_REQUEST);

				get_interface_mac(best_route->interface, arp_header->shwa);
				memset(arp_header->thwa, 0, 6);

				char *ip = get_interface_ip(best_route->interface);
				in_addr adr;
				inet_pton(AF_INET, ip, &adr);
				arp_header->sprotoa = adr.s_addr;
				arp_header->tprotoa = ip_header->dest_addr;

				send_to_link(sizeof(arp_hdr) + sizeof(ether_hdr), arp_req, best_route->interface);
				continue;
			}

			memcpy(eth_header->ethr_dhost, destination_mac, 6);
			get_interface_mac(best_route->interface, eth_header->ethr_shost);
			send_to_link(packet.len, packet.buf, best_route->interface);
			continue;
		}

		/* ARP */
		if (ntohs(eth_header->ethr_type) == ETHERTYPE_ARP) {
			arp_hdr *arp_header = (arp_hdr *)(packet.buf + sizeof(ether_hdr));

			/* ARP Reply */
			if (ntohs(arp_header->opcode) == ARP_REPLY) {
				memcpy(arp_table[arp_header->sprotoa], arp_header->shwa, 6);
				queue<packet_t> pending;
				while (!packets_queue.empty()) {
					packet_t packet1 = packets_queue.front();
					packets_queue.pop();

					ether_hdr *eth_header1 = (ether_hdr *)packet1.buf;
					ip_hdr *ip_header1 = (ip_hdr *)(packet1.buf + sizeof(ether_hdr));

					route_table_entry *best_route1 = rtable->get_best_route(ip_header1->dest_addr);
					uint8_t *destination_mac1 = get_arp_entry(best_route1->next_hop);

					/* If the router knows the MAC */
					if (destination_mac1) {
						memcpy(eth_header1->ethr_dhost, destination_mac1, 6);
						send_to_link(packet1.len, packet1.buf, best_route1->interface);
						/* It doesn't know the MAC yet */
					} else {
						pending.push(packet1);
					}
				}
				std::swap(packets_queue, pending);

				/* ARP Request*/
			} else if (ntohs(arp_header->opcode) == ARP_REQUEST) {
				/* If target MAC is broadcast or the MAC of the interface - check */

				/* Add IP -> MAC of sender to the ARP table */
				if (get_arp_entry(arp_header->sprotoa) == NULL)
					memcpy(arp_table[arp_header->sprotoa], arp_header->shwa, 6);

				/* Building Ethernet header */
				/* Get the MAC of the interface it came from */
				uint8_t arp_src_mac[6];
				get_interface_mac(interface, arp_src_mac);

				uint8_t tmp[6];
				memcpy(tmp, eth_header->ethr_shost, 6);
				memcpy(eth_header->ethr_shost, arp_src_mac, 6);
				memcpy(eth_header->ethr_dhost, tmp, 6);

				/* Get the IP of the interface*/
				char *router_ip = get_interface_ip(interface);
				in_addr router_addr;
				inet_pton(AF_INET, router_ip, &router_addr);

				/* Prepare ARP Reply */
				arp_header->opcode = htons(ARP_REPLY);

				/* MAC Addresses */
				memcpy(tmp, arp_header->shwa, 6);
				memcpy(arp_header->shwa, arp_src_mac, 6);
				memcpy(arp_header->thwa, tmp, 6);

				/* IP Addresses */
				uint32_t ip_tmp = arp_header->sprotoa;
				arp_header->sprotoa = router_addr.s_addr;
				arp_header->tprotoa = ip_tmp;
				send_to_link(sizeof(arp_hdr) + sizeof(ether_hdr), packet.buf, interface);
			}
		}
	}

	/* It doesn't get here, just exits due to the DIE */
	delete rtable;
	return 0;
}

