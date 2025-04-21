#pragma once

#include <stddef.h>
#include <stdint.h>
#include <netinet/in.h>

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

