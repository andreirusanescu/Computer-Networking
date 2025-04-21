
#include <sys/socket.h>
#include <sys/types.h>

#include "common.hpp"
#include "helpers.hpp"

int recv_all(int sockfd, void *buffer, size_t len) {
	char *buff = static_cast<char *>(buffer);
	int bytes_received = 0;
	int bytes_remaining = len;
	int rc;
	while (bytes_remaining > 0) {
		rc = recv(sockfd, buff + bytes_received, bytes_remaining, 0);
		DIE(rc < 0, "recv() failed\n");
		if (rc == 0)
			break;
		bytes_received += rc;
		bytes_remaining -= rc;
	}
	return bytes_received;
}

int send_all(int sockfd, void *buffer, size_t len) {
	char *buff = static_cast<char *>(buffer);
	int bytes_sent = 0;
	int bytes_remaining = len;
	int rc;
	while (bytes_remaining > 0) {
		rc = send(sockfd, buff + bytes_sent, bytes_remaining, 0);
		DIE(rc < 0, "send() failed\n");
		bytes_sent += rc;
		bytes_remaining -= rc;
	}
	return bytes_sent;
}
