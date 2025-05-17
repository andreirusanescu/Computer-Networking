#pragma once

#include <string>

/**
 * @brief: Manually Computes a GET request.
 * @param host - ip of the host
 * @param port - port of the host
 * @param url - the url
 * @param cookie - cookie of the session
 * @param auth - jwt auth token
 */
char *compute_get_request(const std::string_view& host, uint16_t port, const std::string_view& url,
						  const std::string& cookie, const std::string& auth);

/**
 * @brief: Manually Computes a POST request.
 * @param host - ip of the host
 * @param port - port of the host
 * @param url - the url
 * @param content_type - the type of the content
 * @param content - the actual content
 * @param cookie - cookie of the session
 * @param auth - jwt auth token
 */
char *compute_post_request(const std::string_view& host, uint16_t port, const std::string_view& url,
						   const std::string_view& content_type, const std::string& content,
						   const std::string& cookie, const std::string& auth);

/**
 * @brief: Manually Computes a DELETE request.
 * @param host - ip of the host
 * @param port - port of the host
 * @param url - the url
 * @param cookie - cookie of the session
 * @param auth - jwt auth token
 */
char *compute_delete_request(const std::string_view& host, uint16_t port, const std::string_view& url,
						     const std::string& cookie, const std::string& auth);

/**
 * @brief: Manually Computes a PUT request.
 * @param host - ip of the host
 * @param port - port of the host
 * @param url - the url
 * @param content_type - the type of the content
 * @param content - the actual content
 * @param cookie - cookie of the session
 * @param auth - jwt auth token
 */
char *compute_put_request(const std::string_view& host, uint16_t port, const std::string_view& url,
						   const std::string_view& content_type, const std::string& content,
						   const std::string& cookie, const std::string& auth);
