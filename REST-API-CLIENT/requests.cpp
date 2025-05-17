#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>

#include "helpers.hpp"
#include "requests.hpp"

#include <string>
#include <vector>

#include "json.hpp"
using json = nlohmann::json;

using std::string, std::string_view;

char *compute_get_request(const string_view& host, uint16_t port, const string_view& url,
						  const string& cookie, const string& auth)
{
	char *message = (char *)calloc(BUFLEN, sizeof(char));
	char *line = (char *)calloc(LINELEN, sizeof(char));
	
	sprintf(line, "GET %s HTTP/1.1", url.data());
	compute_message(message, line);

	memset(line, 0, LINELEN);
	sprintf(line, "Host: %s:%d", host.data(), port);
	compute_message(message, line);

	if (!auth.empty()) {
		string auth_base = "Authorization: Bearer " + auth;
		compute_message(message, auth_base.data());
	}

	if (!cookie.empty()) {
		string cookie_base = "Cookie: " + cookie;
		compute_message(message, cookie_base.data());
	}
	
	compute_message(message, "");
	free(line);
	return message;
}

char *compute_post_request(const string_view& host, uint16_t port, const string_view& url,
						   const string_view& content_type, const string& content,
						   const std::string& cookie, const std::string& auth)
{
	char *message = (char *) calloc(BUFLEN, sizeof(char));
	char *line = (char *) calloc(LINELEN, sizeof(char));
	
	sprintf(line, "POST %s HTTP/1.1", url.data());
	compute_message(message, line);

	sprintf(line, "Host: %s:%d", host.data(), port);
	compute_message(message, line);

	if (!auth.empty()) {
		string auth_base = "Authorization: Bearer " + auth;
		compute_message(message, auth_base.data());
	}

	memset(line, 0, LINELEN);
	sprintf(line, "Content-Type: %s", content_type.data());
	compute_message(message, line);
	
	memset(line, 0, LINELEN);
	sprintf(line, "Content-Length: %lu", content.length());
	compute_message(message, line);

	if (!cookie.empty()) {
		string cookie_base = "Cookie: " + cookie;
		compute_message(message, cookie_base.data());
	}

	compute_message(message, "");

	memset(line, 0, LINELEN);
	compute_message(message, content.c_str());

	free(line);
	return message;
}

char *compute_delete_request(const string_view& host, uint16_t port,
							 const string_view& url, const string& cookie,
							 const string& auth)
{
	char *message = (char *)calloc(BUFLEN, sizeof(char));
	char *line = (char *)calloc(LINELEN, sizeof(char));

	sprintf(line, "DELETE %s HTTP/1.1", url.data());
	compute_message(message, line);

	memset(line, 0, LINELEN);
	sprintf(line, "Host: %s:%d", host.data(), port);
	compute_message(message, line);

	if (!auth.empty()) {
		string auth_base = "Authorization: Bearer " + auth;
		compute_message(message, auth_base.data());
	}

	if (!cookie.empty()) {
		string cookie_base = "Cookie: " + cookie;
		compute_message(message, cookie_base.data());
	}

	compute_message(message, "");
	free(line);
	return message;
}

char *compute_put_request(const string_view& host, uint16_t port, const string_view& url,
						   const string_view& content_type, const string& content,
						   const string& cookie, const std::string& auth)
{
	char *message = (char *) calloc(BUFLEN, sizeof(char));
	char *line = (char *) calloc(LINELEN, sizeof(char));
	
	sprintf(line, "PUT %s HTTP/1.1", url.data());
	compute_message(message, line);

	sprintf(line, "Host: %s:%d", host.data(), port);
	compute_message(message, line);

	if (!auth.empty()) {
		string auth_base = "Authorization: Bearer " + auth;
		compute_message(message, auth_base.data());
	}

	memset(line, 0, LINELEN);
	sprintf(line, "Content-Type: %s", content_type.data());
	compute_message(message, line);
	
	memset(line, 0, LINELEN);
	sprintf(line, "Content-Length: %lu", content.length());
	compute_message(message, line);

	if (!cookie.empty()) {
		string cookie_base = "Cookie: " + cookie;
		compute_message(message, cookie_base.data());
	}

	compute_message(message, "");

	memset(line, 0, LINELEN);
	compute_message(message, content.c_str());

	free(line);
	return message;
}
