#pragma once

#include <unordered_set>
#include <string>
#include "common.hpp"

struct subscriber_t {
	bool connected;
	int fd;
	std::unordered_set<std::string> topics;

	subscriber_t(int fd) : fd(fd), connected(true) {
	}
};
