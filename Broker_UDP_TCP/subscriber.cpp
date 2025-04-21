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
#include "common.hpp"
#include "helpers.hpp"
	
void run_client(int sockfd) {
	char buf[70] = {0};
	
	struct pollfd poll_fd[2];
	poll_fd[0] = (struct pollfd) {sockfd, POLLIN, 0};
	poll_fd[1] = (struct pollfd) {STDIN_FILENO, POLLIN, 0};

	int rc;
	while (1) {
		rc = poll(poll_fd, 2, -1);
		DIE(rc < 0, "poll() failed");

		if (poll_fd[1].revents & POLLIN) {
			fgets(buf, sizeof(buf), stdin);
			/* Comanda a clientului de la tastatura */

			char *tok = strtok(buf, "\n ");
			if (!tok)
				continue;

			bool subscribe;
			if (!strcmp(tok, "subscribe")) {
				subscribe = true;
			} else if (!strcmp(tok, "unsubscribe")) {
				subscribe = false;
			} else if (!strcmp(tok, "exit")) {
				/* trebuie sa inchid acest client */
				for (int i = 0; i < 2; ++i)
					if (close(poll_fd[i].fd) < 0)
						DIE(1, "close() failed\n");
				return;
			} else {
				continue;
			}


			tok = strtok(nullptr, "\n ");
			if (!tok) continue;
			if (strtok(nullptr, "\n ")) continue; // should be null

			if (subscribe) {
				printf("Subscribed to topic %s\n", tok);
			} else {
				printf("Unsubscribed from topic %s\n", tok);
			}
			/* topic is now in tok */
			char sent_packet_message[TOPIC_SIZE + 2] = {0};
			sent_packet_message[0] = subscribe;
			int tok_len = strlen(tok);
			memcpy(sent_packet_message + 1, tok, tok_len);
			tok_len += 2;
			send_all(sockfd, &tok_len, sizeof(int));
			send_all(sockfd, sent_packet_message, tok_len);
		} else if (poll_fd[0].revents & POLLIN) {
			// in_addr udp_client_ip;
			// uint16_t udp_client_port;
			int topic_len;
			char data_type;
			int content_len;
			char topic[TOPIC_SIZE + 1 + sizeof(data_type) + sizeof(content_len)] = {0};
			char content[CONTENT_SIZE + 1] = {0};
			// recv_all(sockfd, &udp_client_ip, sizeof(udp_client_ip));
			// recv_all(sockfd, &udp_client_port, sizeof(udp_client_port));
			rc = recv_all(sockfd, &topic_len, sizeof(int));
			if (rc <= 0)
				break; // server closed
			recv_all(sockfd, topic, topic_len);
			
			int len = strlen(topic);
			memcpy(&data_type, topic + len + 1, DATA_SIZE);
			memcpy(&content_len, topic + len + 1 + DATA_SIZE, sizeof(int));
			recv_all(sockfd, content, content_len);

			// const char *ip_client_udp = inet_ntoa(udp_client_ip);
			// const uint16_t port_udp = ntohs(udp_client_port);

			// printf("%s:%hu - %s - ", ip_client_udp, port_udp, topic);
			printf("%s - ", topic);

			switch (data_type)
			{
			case 0: {
				int64_t res_int;
				memcpy(&res_int, content, sizeof(res_int));
				printf("%s - %ld\n", "INT", res_int);
				break;
			} case 1: {
				float res_short;
				memcpy(&res_short, content, sizeof(res_short));
				printf("%s - %.2f\n", "SHORT_REAL", res_short);
				break;
			} case 2: {
				float res_float;
				uint8_t neg_pow;
				memcpy(&res_float, content, sizeof(res_float));
				memcpy(&neg_pow, content + sizeof(res_float), sizeof(neg_pow));
				char format[32] = {0};
				res_float /= pow(10, neg_pow);
				snprintf(format, sizeof(format), "FLOAT - %%.%df\n", neg_pow);
				printf(format, res_float);
				break;
			} case 3: {
				printf("%s - %s\n", "STRING", content);
				break;
			}
			default:
				break;
			}
		}
	}	
}

int main(int argc, char *argv[]) {
	if (argc != 4) {
		fprintf(stderr, "Usage: %s <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n", argv[0]);
		return -1;
	}

	int id_client_len = strlen(argv[1]);
	if (id_client_len > 10) {
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
	if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0) {
		perror("Disabling Nagle's algorithm failed\n");
	}

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

	int ok;
	recv_all(sockfd, &ok, sizeof(int));
	if (ok) {
		run_client(sockfd);
		return 0;
	}

	rc = close(sockfd);
	DIE(rc < 0, "close() failed\n");
	return 0;
}
