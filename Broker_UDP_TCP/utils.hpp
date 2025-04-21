#pragma once

#include <stddef.h>
#include <stdint.h>
#include <netinet/in.h>
#include <unordered_set>
#include <string>

int send_all(int sockfd, void *buff, size_t len);
int recv_all(int sockfd, void *buff, size_t len);

/* Dimensiunea maxima a mesajului */
#define UDP_MAXSIZE 1555

/* Numarul maxim de conexiuni */
#define MAX_CONNECTIONS 32
#define ID_SIZE 11
#define TOPIC_SIZE 50
#define DATA_SIZE 1
#define CONTENT_SIZE 1500
#define SUBSCRIBE_SIZE 70

#include <stdio.h>
#include <stdlib.h>

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

struct subscriber_t {
	bool connected;
	int fd;
	std::unordered_set<std::string> topics;

	subscriber_t(int fd) : fd(fd), connected(true) {
	}
};
