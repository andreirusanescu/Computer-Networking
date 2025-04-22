#pragma once

#include <stddef.h>
#include <stdint.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

#include <unordered_set>
#include <string>

#define UDP_MAXSIZE 1555
#define MAX_CONNECTIONS 32
#define ID_SIZE 11
#define TOPIC_SIZE 50
#define DATA_SIZE 1
#define CONTENT_SIZE 1500

/* ("UNSUBSCRIBE " = 12) + (TOPIC_SIZE = 50) + '\0 = 63 */
#define SUBSCRIBE_SIZE 63


/*
 * Macro de verificare a erorilor
 * Exemplu:
 * 		int fd = open (file_name , O_RDONLY);
 * 		DIE( fd == -1, "open failed");
 */

#define DIE(assertion, call_description)                                       \
	do {                                                                         \
		if (assertion) {                                                           \
			fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
			perror(call_description);                                                \
			exit(EXIT_FAILURE);                                                      \
		}                                                                          \
	} while (0)

/**
 * @brief Sends len bytes of buff to sockfd
 */
int send_all(int sockfd, void *buff, size_t len);

/**
 * @brief Receives len bytes in buff from sockfd
 */
int recv_all(int sockfd, void *buff, size_t len);

/**
 * @struct
 * Subscriber structure for the TCP clients
 * @param connected - if the client is connected or not
 * @param fd - file descriptor of the TCP connection with
 * the subscriber
 * @param topics - set of topics the client subscribed to
 */
struct subscriber_t {
	bool connected;
	int fd;
	std::unordered_set<std::string> topics;

	/* List initialization constructor */
	subscriber_t(int fd) : fd(fd), connected(true) {
	}
};
