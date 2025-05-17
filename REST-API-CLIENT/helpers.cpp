#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>


#include <cstring>
#include <string>

#include "helpers.hpp"

static const std::string HEADER_TERMINATOR = "\r\n\r\n";
static const std::string CONTENT_LENGTH = "Content-Length: ";

void error(const char *msg)
{
	perror(msg);
	exit(0);
}

void compute_message(char *message, const char *line)
{
	strcat(message, line);
	strcat(message, "\r\n");
}

int open_connection(const char *host_ip, int portno, int ip_type, int socket_type, int flag)
{
	sockaddr_in serv_addr;
	int sockfd = socket(ip_type, socket_type, flag);
	if (sockfd < 0)
		error("ERROR opening socket");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = ip_type;
	serv_addr.sin_port = htons(portno);
	inet_aton(host_ip, &serv_addr.sin_addr);

	/* connect the socket */
	if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR connecting");

	return sockfd;
}

void close_connection(int sockfd)
{
	close(sockfd);
}

void send_to_server(int sockfd, char *message)
{
	int bytes, sent = 0;
	int total = strlen(message);

	do
	{
		bytes = write(sockfd, message + sent, total - sent);
		if (bytes < 0) {
			error("ERROR writing message to socket");
		}

		if (bytes == 0) {
			break;
		}

		sent += bytes;
	} while (sent < total);
}

long unsigned int buffer_find_insensitive(const std::string& buffer, const std::string& data) {
	if (data.size() > buffer.size()) return std::string::npos;

	size_t last_pos = buffer.size() - data.size() + 1;
	for (size_t i = 0; i < last_pos; ++i) {
		size_t j = 0;
		for (j = 0; j < data.size(); ++j) {
			if (tolower(buffer[i + j]) != tolower(data[j])) {
				break;
			}
		}
		if (j == data.size())
			return i;
	}
	return std::string::npos;
}

std::string receive_from_server(int sockfd)
{
	char response[BUFLEN];
	
	std::string buffer = "";
	long unsigned int header_end = 0;
	int content_length = 0;

	do {
		int bytes = read(sockfd, response, BUFLEN);

		if (bytes < 0){
			error("ERROR reading response from socket");
		}

		if (bytes == 0) {
			break;
		}

		buffer.append(response, bytes);
		header_end = buffer.find(HEADER_TERMINATOR);

		if (header_end != std::string::npos && header_end >= 0) {
			header_end += HEADER_TERMINATOR.length();

			int content_length_start = buffer_find_insensitive(buffer, CONTENT_LENGTH);
			
			if (content_length_start < 0) {
				continue;           
			}

			content_length_start += CONTENT_LENGTH.size();
			content_length = strtol(buffer.data() + content_length_start, NULL, 10);
			break;
		}
	} while (1);
	size_t total = content_length + (size_t) header_end;
	
	while (buffer.size() < total) {
		int bytes = read(sockfd, response, BUFLEN);

		if (bytes < 0) {
			error("ERROR reading response from socket");
		}

		if (bytes == 0) {
			break;
		}

		buffer.append(response, bytes);
	}
	buffer += "";
	return buffer;
}
