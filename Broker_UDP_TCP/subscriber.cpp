#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/tcp.h>

#include <cstring>
#include <cmath>
#include <array>
#include <functional>

#include "utils.hpp"

static inline
void print_int(const char *content) {
	int64_t res_int;
	memcpy(&res_int, content, sizeof(res_int));
	printf("%s - %ld\n", "INT", res_int);
}

static inline
void print_short(const char *content) {
	float res_short;
	memcpy(&res_short, content, sizeof(res_short));
	printf("%s - %.2f\n", "SHORT_REAL", res_short);
}

static inline
void print_float(const char *content) {
	float res_float;
	uint8_t neg_pow;
	memcpy(&res_float, content, sizeof(res_float));
	memcpy(&neg_pow, content + sizeof(res_float), sizeof(neg_pow));
	char format[32] = {0};
	res_float /= pow(10, neg_pow);
	snprintf(format, sizeof(format), "FLOAT - %%.%df\n", neg_pow);
	printf(format, res_float);
}

static inline
void print_string(const char *content) {
	printf("%s - %s\n", "STRING", content);
}

static void run_client(int sockfd) {
	char buf[SUBSCRIBE_SIZE] = {0};
	/** Could be a switch case
	 *  Prints the content based on the data type */
	std::array<std::function<void(const char *)>, 4> print_data = {
		print_int, print_short, print_float, print_string
	};

	struct pollfd poll_fd[2];
	poll_fd[0] = (struct pollfd) {sockfd, POLLIN, 0};
	poll_fd[1] = (struct pollfd) {STDIN_FILENO, POLLIN, 0};
	int rc;
	while (1) {
		rc = poll(poll_fd, 2, -1);
		DIE(rc < 0, "poll() failed");

		/* STDIN user input */
		if (poll_fd[1].revents & POLLIN) {
			fgets(buf, sizeof(buf), stdin);

			char *tok = strtok(buf, "\n ");
			if (!tok) continue;

			bool subscribe;
			if (!strcmp(tok, "subscribe")) {
				subscribe = true;
			} else if (!strcmp(tok, "unsubscribe")) {
				subscribe = false;
			} else if (!strcmp(tok, "exit")) {
				/* closing this client */
				for (int i = 0; i < 2; ++i) {
					rc = close(poll_fd[i].fd);
					DIE(rc < 0, "close() failed\n");
				}
				return;
			} else {
				continue; /* you have to subscribe to a topic */
			}
			tok = strtok(nullptr, "\n ");
			if (!tok) continue;

			/* should be null -> only two words allowed */
			if (strtok(nullptr, "\n ")) continue;

			if (subscribe)
				printf("Subscribed to topic %s\n", tok);
			else
				printf("Unsubscribed from topic %s\n", tok);

			/* topic is now in tok */
			char sent_packet_message[TOPIC_SIZE + 2] = {0};
			sent_packet_message[0] = subscribe; /* subscribe -> 0 / 1*/
			int tok_len = strlen(tok);
			memcpy(sent_packet_message + 1, tok, tok_len);
			send_all(sockfd, sent_packet_message, sizeof(sent_packet_message));
		} else if (poll_fd[0].revents & POLLIN) {
			in_addr client_ip;
			uint16_t client_port;
			int topic_len;
			char data_type;
			int content_len;
			char ip_port_topicsize[sizeof(client_ip) + sizeof(client_port) + sizeof(topic_len)];
			char topic[TOPIC_SIZE + 1 + DATA_SIZE + sizeof(content_len)] = {0};
			char content[CONTENT_SIZE + 1] = {0};
			rc = recv_all(sockfd, ip_port_topicsize, sizeof(ip_port_topicsize));
			if (rc == 0)
				break; // server closed

			memcpy(&client_ip, ip_port_topicsize, sizeof(in_addr));
			memcpy(&client_port, ip_port_topicsize + sizeof(in_addr), sizeof(uint16_t));
			memcpy(&topic_len, ip_port_topicsize + sizeof(in_addr) + sizeof(uint16_t), sizeof(int));
			recv_all(sockfd, topic, topic_len);

			int len = strlen(topic);
			memcpy(&data_type, topic + len + 1, DATA_SIZE);
			memcpy(&content_len, topic + len + 1 + DATA_SIZE, sizeof(int));
			recv_all(sockfd, content, content_len);
			const char *ip_client_udp = inet_ntoa(client_ip);
			const uint16_t port_udp = ntohs(client_port);
			printf("%s:%hu - ", ip_client_udp, port_udp);
			printf("%s - ", topic);
			print_data[data_type](content);
		}
	}	
}

int main(int argc, char *argv[]) {
	if (argc != 4) {
		fprintf(stderr, "Usage: %s <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n", argv[0]);
		return -1;
	}

	if (strlen(argv[1]) > 10) {
		fprintf(stderr, "Client's ID must have a length of maximum 10 characters\n");
		return -1;
	}

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	uint16_t port;
	int rc = sscanf(argv[3], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");

	const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	const int enable = 1;
	/* Reusable socket */
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed\n");

	/* Disable Nagle's algorithm */
	if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0)
		perror("Disabling Nagle's algorithm failed\n");

	sockaddr_in serv_addr;
	socklen_t socket_len = sizeof(sockaddr_in);

	memset(&serv_addr, 0, socket_len);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
	DIE(rc <= 0, "inet_pton");

	rc = connect(sockfd, (sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(rc < 0, "connect");

	/* Send ID to the server */
	send_all(sockfd, argv[1], ID_SIZE);

	/**
	 * @paragraph
	 * @param ack is like an ACK sent by the server
	 * that tells the client its ID is unique and can run on the server.
	 * If ack == 1 => run_client(), else close socket and exit
	*/
	int ack;
	recv_all(sockfd, &ack, sizeof(int));
	if (ack) {
		/* the socket is closed in run_client() */
		run_client(sockfd);
		return 0;
	}

	rc = close(sockfd);
	DIE(rc < 0, "close() failed\n");
	return 0;
}
