#pragma once

#include <poll.h>
#include <vector>
#include <string>
#include <unordered_map>

#include "utils.hpp"

/**
 * @brief Logs a tcp client to the server
 * @param pollfds - vector of fds used by the server
 * -> possibly modified by adding another pollfd structure
 * If the ID of the client is unique -> adds its fd to the vector
 * @param tcp_fd -> TCP fd of the server - accepts connections
 * @param subscribers - map between ID's of the subscribers and their
 * corresponding structure `subscriber_t`.
 * @param fd_to_id - map between the fd of the subscriber and its ID
 * -> its purpose is to retrieve the ID of the subscriber when poll
 * fd of it is activated. Then the ID is used to get the whole structure of
 * the client using @param subscribers.
 */
void log_tcp_client(std::vector<pollfd>& pollfds, const int tcp_fd,
					std::unordered_map<std::string, subscriber_t>& subscribers,
					std::unordered_map<int, std::string>& fd_to_id);


/**
 * @brief Matches the topics the TCP client is subscribed to with the
 * topic updated by the UDP clients. Uses dynamic programming.
 * @param s -> servers currently updated topic from UDP
 * @param p -> topics subscribed to by the TCP client, p from pattern because
 * the string p possibly contains "?" and/or "+".
 * @return True - if the strings do match, False otherwise
 */
bool match(const std::vector<std::string>& s, const std::vector<std::string>& p);

/**
 * @brief Preprocessing function for the @fn match
 * Uses @fn strtok to split the @param str after "/"
 * @param str - Topic subscribed to or topic announced by UDP
 * @return vector of split strings
 */
std::vector<std::string> process_topic(const char* str);

/**
 * @brief Receives UDP update on a topic and sends update
 * to the clients subscribed to the topic
 * @param udp_fd - FD of the UDP client
 * @param subscribers - used to iterate through subscribers, not modified 
 */
void receive_udp(const int udp_fd,
const std::unordered_map<std::string, subscriber_t>& subscribers);

/**
 * @brief Main function for the server. Uses tcp_fd to accept
 * new connections from subscribers and udp_fd to receive
 * updates on topics.
 */
void run_server(const int tcp_fd, const int udp_fd);
