#pragma once

#include "json.hpp"

#include <string>

#define LOGIN_ADMIN    "login_admin"
#define LOGOUT_ADMIN   "logout_admin"
#define ADD_USER       "add_user"
#define EXIT           "exit"
#define GET_USERS      "get_users"
#define DELETE_USER    "delete_user"
#define LOGIN		   "login"
#define LOGOUT		   "logout"
#define GET_ACCESS	   "get_access"
#define GET_MOVIES	   "get_movies"
#define GET_MOVIE	   "get_movie"
#define ADD_MOVIE	   "add_movie"
#define UPDATE_MOVIE   "update_movie"
#define DELETE_MOVIE   "delete_movie"
#define GET_COLLECTS   "get_collections"
#define GET_COLLECT    "get_collection"
#define ADD_COLLECT    "add_collection"
#define ADD_MOV_COLL   "add_movie_to_collection"
#define DEL_MOV_COLL   "delete_movie_from_collection"
#define DEL_COLLECT    "delete_collection"
#define STATUS_ERROR   600 // custom error for get_status()

/* Constants */
static const uint16_t port = 8081;
static const std::string_view ip             { "63.32.125.183" };
static const std::string_view user_login     { "/api/v1/tema/user/login" };
static const std::string_view user_logout    { "/api/v1/tema/user/logout" };
static const std::string_view admin_login    { "/api/v1/tema/admin/login" };
static const std::string_view admin_logout   { "/api/v1/tema/admin/logout" };
static const std::string_view add_users      { "/api/v1/tema/admin/users" };
static const std::string_view library_access { "/api/v1/tema/library/access" };
static const std::string_view library_movies { "/api/v1/tema/library/movies" };
static const std::string_view content_type   { "application/json" };
static const std::string_view collections	 { "/api/v1/tema/library/collections" };

class Client {
private:
	/* flag set if someone is currently logged in */
	bool logged = false;

	/* if the current user is admin */
	bool is_admin = false;

	
	/* Cookie for the current session, if not set => "" */
	std::string session;
	
	/* Authentication token - given at get_access() */
	std::string jwt_token;
	
	/* The following members are used as helpers in add_collection
	   and add_movie if the user is adding a collection, to display
	   certain messages depending on the operation, and or
	   retain errors if they occurred. */
	bool adding = false;
	bool ret_movies;
	int collection_id;
	int curr_movie_id;
	nlohmann::json json_helper;

	/**
	 * Checks if the input string has exactly
	 * one word
	 * @return true for 1 word, else false.
	 */
	bool parse_input(std::string& str);

	/**
	 * Sets the cookie (session).
	 */
	void get_cookie(const std::string& str);

	/**
	 * Returns the status of the HTTP request
	 * @param str - the HTTP request
	 * @return the status.
	 */
	int get_status(const std::string& str);

	/**
	 * Prints cred_show to STDOUT and returns
	 * the string read from STDIN
	 * @return the string read.
	 */
	std::string parse_cred(const char *cred_show);

	/**
	 * Returns true if the string contains digits only.
	 */
	bool contains_digits(const std::string& str);

	/**
	 * Checks if the string is a valid double number.
	 */
	bool is_double(const std::string& str);

	/**
	 * Returns the body of the response - i.e. the JSON.
	 */
	std::string get_json(const std::string& response);

public:
	/**
	 * Logins the admin user.
	 */
	void login();

	/**
	 * Adds a user to the admin's ownership.
	 */
	void add_user();

	/**
	 * Returns the admin's users list.
	 */
	void get_users();

	/**
	 * Delets an admin's user.
	 */
	void delete_user();

	/**
	 * Login function for a normal user.
	 */
	void login_user();

	/**
	 * Gets access to library for a user.
	 * Saves JWT token.
	 */
	void get_access();

	/**
	 * Logs out admin.
	 */
	void logout_admin();

	/**
	 * Logs out normal user.
	 */
	void logout();

	/**
	 * Displays all the movies of a user.
	 */
	void get_movies();

	/**
	 * Displays the data of the movie
	 * with a given ID (from STDIN)
	 */
	void get_movie();

	/**
	 * Adds a new movie to a user
	 */
	void add_movie();

	/**
	 * Updates a movie
	 */
	void update_movie();

	/**
	 * Deletes a movie
	 * from a user
	 */
	void delete_movie();

	/**
	 * Shows the user's collections
	 */
	void get_collections();

	/**
	 * Gets the collection with a given ID
	 */
	void get_collection();

	/**
	 * Adds a new collection with
	 * a given title to a user's collections.
	 * Performs multiple HTTP requests based
	 * on how many movies the user wants to
	 * add to the collection.
	 * Adds an empty collection, then gets
	 * the ID of it and then, adds n movies
	 * to it.
	 */
	void add_collection();

	/**
	 * Adds a movie to a collection, based on the ID of
	 * the collection and the ID of the movie. If called from
	 * add_collection(), the ID's are already set.
	 */
	void add_mov_coll();

	/**
	 * Deletes a collection from the user's collections,
	 * based on an ID received from STDIN
	 */
	void del_collection();

	/**
	 * Deletes a movie from the user's collection,
	 * based on the collection ID and the movie ID
	 * received from STDIN.
	 */
	void del_mov_coll();
};
