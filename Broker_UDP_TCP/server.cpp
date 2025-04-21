#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/tcp.h>

#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "utils.hpp"

using std::cout;
using std::unordered_map;
using std::vector, std::string;

void log_tcp_client(vector<pollfd>& pollfds, const int tcp_fd,
					unordered_map<string, subscriber_t>& subscribers,
					unordered_map<int, string>& fd_to_id) {
	int rc;
	sockaddr_in tcp_cli_addr;
	socklen_t len = sizeof(sockaddr_in);
	const int newsockfd = accept(tcp_fd, (sockaddr *) &tcp_cli_addr, &len);
	DIE(newsockfd < 0, "socket() failed\n");
	
	const int enable = 1;
	if (setsockopt(newsockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed for tcp client\n");

	char id[ID_SIZE] = {0};
	recv_all(newsockfd, id, ID_SIZE);

	const string id_str(id);

	/* Added a new subscriber */
	if (subscribers.find(id_str) == subscribers.end()) {
		subscribers.emplace(id_str, newsockfd);
		int ok = 1;
		send_all(newsockfd, &ok, sizeof(int));
		printf("New client %s connected from %s:%hu.\n", id,
			inet_ntoa(tcp_cli_addr.sin_addr), ntohs(tcp_cli_addr.sin_port));
		pollfds.push_back({newsockfd, POLLIN, 0});
		fd_to_id.emplace(newsockfd, id_str);
		return;
	}

	subscriber_t& subscriber = subscribers.at(id_str);
	if (!subscriber.connected) {
		int ok = 1;
		send_all(newsockfd, &ok, sizeof(int));
		subscriber.connected = true;
		printf("New client %s connected from %s:%hu.\n", id,
			inet_ntoa(tcp_cli_addr.sin_addr), ntohs(tcp_cli_addr.sin_port));
		pollfds.push_back({newsockfd, POLLIN, 0});
		return;
	}

	int ok = 0;
	send_all(newsockfd, &ok, sizeof(int));
	printf("Client %s already connected.\n", id);
	rc = close(newsockfd);
	DIE(rc < 0, "close() failed\n");
}

bool match(const vector<string>& s, const vector<string>& p) {
	int m = s.size(), n = p.size();
	vector<vector<bool>> dp(m + 1, vector<bool>(n + 1));
	dp[0][0] = true;

	for (int i = 1; i <= m; ++i) {
		for (int j = 1; j <= n; ++j) {
			if (p[j - 1] == "*") {
				/* iau caracterele din trecut sau matchuiesc cu asta curent */
				dp[i][j] = dp[i - 1][j] || dp[i - 1][j - 1];
			} else if (p[j - 1] == "+" || s[i - 1] == p[j - 1]) {
				dp[i][j] = dp[i - 1][j - 1];
			}
		}
	}

	return dp[m][n];
}

vector<string> process_topic(const char* str) {
	int len = strlen(str);
	char *s = new char[len + 1];
	memcpy(s, str, len + 1);

	char *token = strtok(s, "/");
	vector<string> res;
	while (token != nullptr) {
		res.push_back(token);
		token = strtok(nullptr, "/");
	}

	delete[] s;
	return res;
}

int
get_content_length(char data_type, const char *content) {
	switch (data_type) {
		case 0: return sizeof(int64_t);
		case 1: return sizeof(float);
		case 2: return sizeof(float) + sizeof(uint8_t);
		case 3: return strlen(content);
	}
	return 0;
}

void 
receive_udp(const int udp_fd, unordered_map<string, subscriber_t>& subscribers) {
	char buf[UDP_MAXSIZE] = {0};
	sockaddr_in sender;
	memset(&sender, 0, sizeof(sender));
	socklen_t sender_len = sizeof(sender);
	recvfrom(udp_fd, buf, sizeof(buf), 0, (sockaddr *)&sender, &sender_len);

	char topic[TOPIC_SIZE + 1 + DATA_SIZE + sizeof(int)] = {0};
	memcpy(topic, buf, TOPIC_SIZE);
	char data_type;
	memcpy(&data_type, buf + TOPIC_SIZE, 1);
	char content[CONTENT_SIZE + 1] = {0};

	switch (data_type) {
	case 0: {
		uint8_t sign_int;
		uint32_t integer;
		memcpy(&sign_int, buf + TOPIC_SIZE + 1, sizeof(sign_int));
		memcpy(&integer, buf + TOPIC_SIZE + 1 + sizeof(sign_int), sizeof(integer));
		integer = ntohl(integer);
		int64_t res_int = integer;
		if (sign_int)
			res_int *= -1;
		memcpy(content, &res_int, sizeof(res_int));
		break;
	} case 1: {
		uint16_t short_real;
		memcpy(&short_real, buf + TOPIC_SIZE + 1, sizeof(short_real));

		short_real = ntohs(short_real);

		float res_short = static_cast<float>(short_real) / 100;
		memcpy(content, &res_short, sizeof(res_short));
		break;
	} case 2: {
		uint8_t sign_float, neg_pow;
		uint32_t modulo;

		memcpy(&sign_float, buf + TOPIC_SIZE + 1, sizeof(sign_float));
		memcpy(&modulo, buf + TOPIC_SIZE + 2, sizeof(modulo));
		memcpy(&neg_pow, buf + TOPIC_SIZE + 2 + sizeof(modulo), sizeof(neg_pow));
		modulo = ntohl(modulo);

		float res_float = static_cast<float>(modulo);
		if (sign_float)
			res_float *= -1;

		memcpy(content, &res_float, sizeof(res_float));
		memcpy(content + sizeof(res_float), &neg_pow, sizeof(neg_pow));
		break;
	} case 3: {
		memcpy(content, buf + TOPIC_SIZE + 1, CONTENT_SIZE);
		break;
	} default:
		break;
	}

	int content_len = get_content_length(data_type, content);
	int topic_len = strlen(topic);
	int topic_size = topic_len + 1 + sizeof(data_type) + sizeof(int);
	memcpy(topic + topic_len + 1, &data_type, DATA_SIZE);
	memcpy(topic + topic_len + 1 + DATA_SIZE, &content_len, sizeof(int));

	vector<string> topic_vec = process_topic(topic);
	for (const auto& [id, sub] : subscribers) {
		if (!sub.connected) continue;
		for (const auto& sub_topic : sub.topics) {
			if (match(topic_vec, process_topic(sub_topic.c_str()))) {
				// send_all(sub.fd, &sender.sin_addr, sizeof(sender.sin_addr));
				// send_all(sub.fd, &sender.sin_port, sizeof(sender.sin_port));
				send_all(sub.fd, &topic_size, sizeof(int));
				send_all(sub.fd, topic, topic_size);
				send_all(sub.fd, content, content_len);
			}
		}
	}
}

void run_server(const int tcp_fd, const int udp_fd) {
	int rc;
	unordered_map<string, subscriber_t> id_subscribers; /* map of ID -> subscriber */
	unordered_map<int, string> fd_to_id; /* map of fd to ID */
	vector<pollfd> pollfds(3);
	pollfds[0] = (struct pollfd) {tcp_fd, POLLIN, 0};
	pollfds[1] = (struct pollfd) {udp_fd, POLLIN, 0};
	pollfds[2] = (struct pollfd) {STDIN_FILENO, POLLIN, 0};

	rc = listen(tcp_fd, MAX_CONNECTIONS);
	DIE(rc < 0, "listen() failed\n");

	while (true) {
		rc = poll(pollfds.data(), pollfds.size(), 0);
		DIE(rc < 0, "poll() failed\n");

		if (pollfds[0].revents & POLLIN) {
			log_tcp_client(pollfds, tcp_fd, id_subscribers, fd_to_id);
		} else if (pollfds[1].revents & POLLIN) {
			/* UDP Client updates on topics */
			receive_udp(udp_fd, id_subscribers);
		} else if (pollfds[2].revents & POLLIN) {
			/* mesaj tastatura - poate fi exit */
			char buf[5]; // buffer for exit
			fgets(buf, sizeof(buf), stdin);
			if (!strcmp(buf, "exit")) {
				/* Deconecteaza toti clientii */
				for (auto& pollfd : pollfds)
					close(pollfd.fd);
				return;
			}
		} else {
			char received_message[TOPIC_SIZE + 2] = {0};
			for (unsigned i = 3; i < pollfds.size(); ++i) {
				/* Unul din clientii TCP - cerere de subscribe sau unsubscribe */
				if (pollfds[i].revents & POLLIN) {
					int recv_size;
					rc = recv_all(pollfds[i].fd, &recv_size, sizeof(int));
					if (rc == 0) {
						string& id = fd_to_id[pollfds[i].fd];
						subscriber_t& sub = id_subscribers.at(id);
						// sub.topics.clear(); // se sterg topicele urmarite
						sub.connected = false;

						pollfds.erase(pollfds.begin() + i);
						rc = close(pollfds[i].fd);
						DIE(rc < 0, "close() failed\n");
						printf("Client %s disconnected.\n", id.c_str());
						--i;
						continue;
					}
					recv_all(pollfds[i].fd, received_message, recv_size);
					const string& id = fd_to_id[pollfds[i].fd];
					subscriber_t& sub = id_subscribers.at(id);
					/* Subscribe */
					if (received_message[0] == 1) {
						sub.topics.insert(received_message + 1);
					/* Unsubscribe */
					} else {
						sub.topics.erase(received_message + 1);
					}
				}
			}
		}
	}
}


int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <PORT>\n", argv[0]);
		return -1;
	}

	/* disable buffering */
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	uint16_t port;
	int rc = sscanf(argv[1], "%hu", &port);
	DIE(rc != 1, "Given port is invalid\n");

	const int tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_fd < 0, "socket() failed\n");

	const int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(udp_fd < 0, "socket() failed\n");

	sockaddr_in server_address;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	memset(server_address.sin_zero, 0, sizeof(server_address.sin_zero));

	/**
	 * Make the address of the server reusable in case of frequent runs.
	 * https://stackoverflow.com/questions/3229860/what-is-the-meaning-of-so-reuseaddr-setsockopt-option-linux
	 */
	const int enable = 1;
	if (setsockopt(tcp_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed\n");

	if (setsockopt(tcp_fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0) {
		perror("Disabling Nagle's algorithm failed\n");
	}

	rc = bind(tcp_fd, (sockaddr *)&server_address, sizeof(sockaddr_in));
	DIE(rc < 0, "bind() failed\n");

	rc = bind(udp_fd, (sockaddr *)&server_address, sizeof(sockaddr_in));
	DIE(rc < 0, "bind() failed\n");

	/* server closes the fds */
	run_server(tcp_fd, udp_fd);
	return 0;
}
