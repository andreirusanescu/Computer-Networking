#include <unistd.h>     /* read, write, close */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>
#include <arpa/inet.h>
#include "helpers.hpp"
#include "requests.hpp"

#include <iostream>
#include <unordered_map>
#include <string>
#include <cstring>
#include <sstream>

#include "json.hpp"
#include "client.hpp"

using std::string_view, std::string;
using std::istringstream, std::getline;
using std::cin, std::cout, std::endl;
using json = nlohmann::json;

bool Client::parse_input(string& str) {
	/* input stringstream that splits after spaces */
	istringstream is(str);
	string token;
	int cnt = 0;
	while (is >> token) ++cnt;
	return cnt == 1;
}

void Client::get_cookie(const string& str) {
	const std::string key = "Set-Cookie: ";
	size_t start = str.find(key);
	/* Cookie was not found */
	if (start == std::string::npos) return;

	start += key.length();
	size_t end = str.find(';', start);
	if (end == string::npos)
		end = str.length();

	size_t length = end - start;

	session = str.substr(start, length);
}

int Client::get_status(const string& str) {
	const string key = "HTTP/1.1 ";
	size_t start = str.find(key);
	if (start == string::npos) return STATUS_ERROR;

	start += key.length();
	size_t end = start;
	int status = 0;
	while (end < str.length() && isdigit(str[end])) {
		status = status * 10 + (str[end] - '0');
		++end;
	}

	if (end == start) return STATUS_ERROR;

	return status;
}

string Client::parse_cred(const char *cred_show) {
	string cred;
	cout << cred_show;
	cout.flush();
	getline(cin, cred);
	if (!parse_input(cred)) {
		cout << "ERROR: No whitespaces, please" << endl;
		return "";
	}
	return cred;
}

bool Client::contains_digits(const string& str) {
	for (char c : str) {
		if (!isdigit(c)) {
			cout << "ERROR: Should contain only numerical values!" << endl;
			return false;
		}
	}

	return true;
}

bool Client::is_double(const string& str) {
	if (str.empty() || str[0] == '.' || str[str.size() - 1] == '.')
		return false;
	int cnt = 0;
	for (char c : str) {
		if (!isdigit(c)) {
			/* First dot */
			if (c == '.' && cnt == 0) {
				++cnt;
			} else {
				cout << "ERROR: NOT DOUBLE" << endl;
				return false;
			}
		}
	}

	return true;
}

string Client::get_json(const string& response) {
	int start = response.find("{");
	return string(response.begin() + start, response.end());
}

void Client::login() {
	const string& user = parse_cred("username=");
	if (user.empty()) return;
	const string& pw = parse_cred("password=");
	if (pw.empty()) return;
	
	json creds;
	creds["username"] = user;
	creds["password"] = pw;
	
	int sockfd = open_connection(ip.data(), port, AF_INET, SOCK_STREAM, 0);
	string empty;
	char *message = compute_post_request(ip, port, admin_login, content_type,
								   creds.dump(), empty, empty);

	send_to_server(sockfd, message);
	free(message);
	const string& response = receive_from_server(sockfd);
	int status = get_status(response);
	json body = json::parse(get_json(response));
	if (status < 300) {
		get_cookie(response);
		cout << "SUCCESS: Admin autentificat cu succes" << endl;
		is_admin = true;
		logged = true;
	} else {
		cout << "ERROR: " << body["error"].get<string>() << endl;
	}

	close_connection(sockfd);
}

void Client::add_user() {
	if (!is_admin) {
		cout << "ERROR: You have to be admin to add an user!" << endl;
		return;
	}
	
	const string& user = parse_cred("username=");
	if (user.empty()) return;
	const string& pw = parse_cred("password=");
	if (pw.empty()) return;
	
	json creds;
	creds["username"] = user;
	creds["password"] = pw;

	int sockfd = open_connection(ip.data(), port, AF_INET, SOCK_STREAM, 0);

	string empty;
	char *message = compute_post_request(ip, port, add_users, content_type,
										 creds.dump(), session, empty);
	send_to_server(sockfd, message);
	free(message);

	const string& response = receive_from_server(sockfd);

	int status = get_status(response);
	if (status < 300) {
		cout << "SUCCESS: Utilizator adăugat cu succes" << endl;
	} else {
		json body = json::parse(get_json(response));
		cout << "ERROR: " << body["error"].get<string>() << endl;
	}

	close_connection(sockfd);
}

void Client::get_users() {
	if (!is_admin) {
		cout << "ERROR: You have to be admin to view the users!" << endl;
		return;
	}
	int sockfd = open_connection(ip.data(), port, AF_INET, SOCK_STREAM, 0);

	string empty;
	char *message = compute_get_request(ip, port, add_users, session, empty);
	send_to_server(sockfd, message);
	free(message);

	const string& response = receive_from_server(sockfd);
	int status = get_status(response);
	json msg_parsed = json::parse(get_json(response));

	/* Errors have status code under 300 */
	if (status < 300) {
		cout << "SUCCESS: Lista utilizatorilor" << endl;
		const auto& vec = msg_parsed["users"];
		for (const auto& user : vec) {
			cout << '#' << user["id"].get<int>() << ' ' 
				 << user["username"].get<string>() << ':' 
				 << user["password"].get<string>() << '\n';
		}
		cout.flush();
	} else {
		cout << "ERROR: " << msg_parsed["error"].get<string>() << endl;
	}

	close_connection(sockfd);
}

void Client::delete_user() {
	if (!is_admin) {
		cout << "ERROR: You have to be admin to delete a user!" << endl;
		return;
	}
	const string& user = parse_cred("username=");
	if (user.empty()) return;
	
	int sockfd = open_connection(ip.data(), port, AF_INET, SOCK_STREAM, 0);
	const string& full_url = string(add_users.data()) + "/" + user;
	string empty;
	char *message = compute_delete_request(ip, port, full_url,
										   session, empty);
	send_to_server(sockfd, message);
	free(message);

	const string& response = receive_from_server(sockfd);
	int status = get_status(response);
	if (status < 300) {
		cout << "SUCCESS: Utilizator șters" << endl;
	} else {
		json body = json::parse(get_json(response));
		cout << "ERROR: " << body["error"].get<string>() << endl;
	}

	close_connection(sockfd);
}

void Client::login_user() {
	if (logged) {
		cout << "ERROR: You have to log out first!" << endl;
		return;
	}
	
	const string& admin_username = parse_cred("admin_username=");
	if (admin_username.empty()) return;
	const string& username = parse_cred("username=");
	if (username.empty()) return;
	const string& password = parse_cred("password=");
	if (password.empty()) return;
	
	json creds;
	creds["admin_username"] = admin_username;
	creds["username"] = username,
	creds["password"] = password;
	
	int sockfd = open_connection(ip.data(), port, AF_INET, SOCK_STREAM, 0);
	string empty = "";
	char *message = compute_post_request(ip, port, user_login, content_type,
										 creds.dump(), empty, empty);

	send_to_server(sockfd, message);
	free(message);

	const string& response = receive_from_server(sockfd);
	int status = get_status(response);
	if (status < 300) {
		get_cookie(response);
		cout << "SUCCESS: Autentificare reușită" << endl;
		logged = true;
	} else {
		json body = json::parse(get_json(response));
		cout << "ERROR: " << body["error"].get<string>() << endl;
	}

	close_connection(sockfd);
}

void Client::get_access() {
	if (!logged) {
		cout << "ERROR: You are not logged in!" << endl;
		return;
	}
	
	int sockfd = open_connection(ip.data(), port, AF_INET, SOCK_STREAM, 0);
	string empty;
	char *message = compute_get_request(ip, port, library_access, session, empty);
	send_to_server(sockfd, message);
	free(message);

	const string& response = receive_from_server(sockfd);
	int status = get_status(response);
	json body = json::parse(get_json(response));
	if (status < 300) {
		jwt_token = body["token"].get<string>();
		cout << "SUCCESS: Token JWT primit" << endl;
	} else {
		cout << "ERROR: " << body["error"].get<string>() << endl;
	}

	close_connection(sockfd);
}

void Client::logout_admin() {
	if (!is_admin) {
		cout << "ERROR: You are not admin!" << endl;
		return;
	}

	int sockfd = open_connection(ip.data(), port, AF_INET, SOCK_STREAM, 0);
	string empty = "";
	char *message = compute_get_request(ip, port, admin_logout, session, empty);
	send_to_server(sockfd, message);
	free(message);

	const string& response = receive_from_server(sockfd);
	int status = get_status(response);
	if (status < 300) {
		cout << "SUCCESS: Admin delogat" << endl;
		is_admin = false;
		logged = false;
		jwt_token.clear(); // reset authentication token
		session.clear(); // reset cookie
	} else {
		json body = json::parse(get_json(response));
		cout << "ERROR: " << body["error"].get<string>() << endl;
	}

	close_connection(sockfd);
}

void Client::logout() {
	if (!logged) {
		cout << "ERROR: You are not logged!" << endl;
		return;
	}
	if (is_admin) {
		cout << "ERROR: You are admin! Use logout_admin" << endl;
		return;
	}

	int sockfd = open_connection(ip.data(), port, AF_INET, SOCK_STREAM, 0);
	string empty = "";
	char *message = compute_get_request(ip, port, user_logout, session, empty);
	send_to_server(sockfd, message);
	free(message);

	const string& response = receive_from_server(sockfd);
	int status = get_status(response);
	json body = json::parse(get_json(response));
	if (status < 300) {
		cout << "SUCCESS: Utilizator delogat" << endl;
		logged = false;
		jwt_token.clear(); // reset token
		session.clear(); // reset cookie
	} else {
		cout << "ERROR: " << body["error"].get<string>() << endl;
	}

	close_connection(sockfd);
}

void Client::get_movies() {
	int sockfd = open_connection(ip.data(), port, AF_INET, SOCK_STREAM, 0);
	char *message = compute_get_request(ip, port, library_movies, session, jwt_token);
	send_to_server(sockfd, message);
	free(message);

	const string& response = receive_from_server(sockfd);
	int status = get_status(response);
	json body = json::parse(get_json(response));
	if (status < 300) {
		cout << "SUCCESS: Lista filmelor" << '\n';
		for (const auto& movie : body["movies"])
			cout << '#' << movie["id"].get<int>() << ' '
				 << movie["title"].get<string>() << '\n';
		cout.flush();
	} else {
		cout << "ERROR: " << body["error"].get<string>() << endl;
	}

	close_connection(sockfd);
}

void Client::get_movie() {
	const string& id = parse_cred("id=");
	if (id.empty() || !contains_digits(id)) return;

	int sockfd = open_connection(ip.data(), port, AF_INET, SOCK_STREAM, 0);
	
	const string& movie_base = string(library_movies.data()) + "/" + id;
	string empty = "";
	char *message = compute_get_request(ip, port, movie_base, empty, jwt_token);
	send_to_server(sockfd, message);
	free(message);

	const string& response = receive_from_server(sockfd);
	int status = get_status(response);
	json body = json::parse(get_json(response));
	if (status < 300) {
		cout << "SUCCESS: Detalii film" << endl;
		cout << body.dump() << endl;
	} else {
		cout << "ERROR: " << body["error"].get<string>() << endl;
	}

	close_connection(sockfd);
}

void Client::add_movie() {
	cout << "title=";
	cout.flush();
	string title;
	getline(cin, title);
	const string& year = parse_cred("year=");
	if (year.empty() || !contains_digits(year)) return;
	
	cout << "description=";
	cout.flush();
	string description;
	getline(cin, description);
	const string& rating = parse_cred("rating=");
	if (rating.empty() || !is_double(rating)) return;

	json creds;
	creds["title"] = title;
	creds["year"] = stoi(year); // convert to int
	creds["description"] = description;
	creds["rating"] = stod(rating); // convert to double
	
	int sockfd = open_connection(ip.data(), port, AF_INET, SOCK_STREAM, 0);
	string empty = "";
	char *message = compute_post_request(ip, port, library_movies, content_type,
										 creds.dump(), session, jwt_token);
	send_to_server(sockfd, message);
	free(message);

	const string& response = receive_from_server(sockfd);
	int status = get_status(response);
	json body = json::parse(get_json(response));
	if (status < 300)
		cout << "SUCCESS: Film adaugat" << endl;
	else
		cout << "ERROR: " << body["error"].get<string>() << endl;

	close_connection(sockfd);
}

void Client::update_movie() {
	string id = parse_cred("id=");
	if (id.empty() || !contains_digits(id)) return;
	cout << "title=";
	cout.flush();
	string title;
	getline(cin, title);
	const string& year = parse_cred("year=");
	if (year.empty() || !contains_digits(year)) return;

	cout << "description=";
	cout.flush();
	string description;
	getline(cin, description);
	const string& rating = parse_cred("rating=");
	if (rating.empty() || !is_double(rating)) return;

	json creds;
	creds["title"] = title;
	creds["year"] = stoi(year);
	creds["description"] = description;
	creds["rating"] = stod(rating);
	
	int sockfd = open_connection(ip.data(), port, AF_INET, SOCK_STREAM, 0);
	string empty = "";
	const string& full_url = string(library_movies.data()) + "/" + id;
	char *message = compute_put_request(ip, port, full_url, content_type, creds.dump(), session, jwt_token);
	send_to_server(sockfd, message);
	free(message);
	
	const string& response = receive_from_server(sockfd);
	int status = get_status(response);
	json body = json::parse(get_json(response));
	if (status < 300)
		cout << "SUCCESS: Film updatat" << endl;
	else
		cout << "ERROR: " << body["error"].get<string>() << endl;

	close_connection(sockfd);
}

void Client::delete_movie() {
	const string& id = parse_cred("id=");
	if (id.empty() || !contains_digits(id)) return;

	int sockfd = open_connection(ip.data(), port, AF_INET, SOCK_STREAM, 0);
	const string& full_url = string(library_movies.data()) + "/" + id;
	char *message = compute_delete_request(ip, port, full_url, session, jwt_token);
	send_to_server(sockfd, message);
	free(message);

	const string& response = receive_from_server(sockfd);

	int status = get_status(response);
	json body = json::parse(get_json(response));
	if (status < 300) {
		cout << "SUCCESS: Film sters" << endl;
	} else {
		cout << "ERROR: " << body["error"].get<string>() << endl;
	}

	close_connection(sockfd);
}

void Client::get_collections() {
	int sockfd = open_connection(ip.data(), port, AF_INET, SOCK_STREAM, 0);
	char *message = compute_get_request(ip, port, collections, session, jwt_token);
	send_to_server(sockfd, message);
	free(message);

	const string& response = receive_from_server(sockfd);
	int status = get_status(response);
	json body = json::parse(get_json(response));
	if (status < 300) {
		cout << "SUCCESS: Lista colectiilor" << '\n';
		for (const auto& col : body["collections"])
			cout << '#' << col["id"].get<int>() << ": " << col["title"].get<string>() << '\n';
		cout.flush();
	} else {
		cout << "ERROR: " << body["error"].get<string>() << endl;
	}

	close_connection(sockfd);
}

void Client::get_collection() {
	string id;
	if (!adding) {
		id = parse_cred("id=");
		if (id.empty() || !contains_digits(id)) return;
	} else {
		id = std::to_string(collection_id);
	}

	int sockfd = open_connection(ip.data(), port, AF_INET, SOCK_STREAM, 0);
	
	const string& movie_base = string(collections.data()) + "/" + id;
	string empty = "";
	char *message = compute_get_request(ip, port, movie_base, empty, jwt_token);
	send_to_server(sockfd, message);
	free(message);

	const string& response = receive_from_server(sockfd);
	int status = get_status(response);
	json body = json::parse(get_json(response));
	if (status < 300) {
		// If get_collection() is called from add_collection() routine
		if (adding)
			cout << "SUCCESS: Colecție adăugată" << endl;
		else
			cout << "SUCCESS: Detalii colecție" << endl;
		// Using newlines instead of endl to reduce overhead
		cout << "title: " << body["title"].get<string>() << '\n';
		cout << "owner: " << body["owner"].get<string>() << '\n';
		for (const auto& movie : body["movies"])
			cout << '#' << movie["id"].get<int>() << ": " << movie["title"].get<string>() << '\n';
		// Flushing the internal buffer to STDOUT
		cout.flush();
	} else {
		cout << "ERROR: " << body["error"].get<string>() << endl;
	}
	close_connection(sockfd);
}

void Client::add_collection() {
	cout << "title=";
	cout.flush();
	string title;
	getline(cin, title);
	const string& num_movies = parse_cred("num_movies=");
	if (num_movies.empty() || !contains_digits(num_movies)) return;
	int n = stoi(num_movies);
	std::vector<int> ids(n);
	string aux;
	for (int i = 0; i < n; ++i) {
		string movie_id = "movie_id[" + std::to_string(i) + "]=";
		aux = parse_cred(movie_id.data());
		if (aux.empty() || !contains_digits(aux)) return;
		ids[i] = stoi(aux);
	}

	int sockfd = open_connection(ip.data(), port, AF_INET, SOCK_STREAM, 0);
	json payload;
	payload["title"] = title;
	string empty = "";
	char *message = compute_post_request(ip, port, collections, content_type, payload.dump(), empty, jwt_token);
	send_to_server(sockfd, message);
	free(message);

	const string& response = receive_from_server(sockfd);
	int status = get_status(response);
	json body = json::parse(get_json(response));
	if (status < 300) {
		collection_id = body["id"].get<int>();
	} else {
		cout << "ERROR: " << body["error"].get<string>() << endl;
		close_connection(sockfd);
		return;
	}
	close_connection(sockfd);

	adding = true;
	ret_movies = true;
	for (int i = 0; i < n; ++i) {
		ret_movies = true;
		curr_movie_id = ids[i];
		add_mov_coll();
		if (!ret_movies) {
			cout << "ERROR: " << json_helper["error"].get<string>() << endl;
			return;
		}
	}

	/* Call get_collection() to print accordingly */
	get_collection();
	adding = false;
	collection_id = -1;
}

void Client::add_mov_coll() {
	string coll_id, movie_id;
	
	if (!adding) {
		coll_id = parse_cred("collection_id=");
		if (coll_id.empty() || !contains_digits(coll_id)) return;
		movie_id = parse_cred("movie_id=");
		if (movie_id.empty() || !contains_digits(movie_id)) return;
	} else {
		coll_id = std::to_string(collection_id);
		movie_id = std::to_string(curr_movie_id);
	}
	
	int sockfd = open_connection(ip.data(), port, AF_INET, SOCK_STREAM, 0);
	const string& full_url = string(collections.data()) + "/" + coll_id + "/movies";

	json payload;
	payload["id"] = stoi(movie_id);
	string empty = "";
	char *message = compute_post_request(ip, port, full_url, content_type, payload.dump(), empty, jwt_token);
	send_to_server(sockfd, message);
	free(message);

	const string& response = receive_from_server(sockfd);
	int status = get_status(response);
	if (status < 300) {
		if (!adding) 
			cout << "SUCCESS: Film adăugat în colecție" << endl;
	} else {
		json body = json::parse(get_json(response));
		if (!adding) {
			cout << "ERROR: " << body["error"].get<string>() << endl;
		} else {
			/* if errors occurred, save the error*/
			json_helper = body;
			ret_movies = false;
		}
	}

	close_connection(sockfd);
}

void Client::del_collection() {
	const string& coll_id = parse_cred("id=");
	if (coll_id.empty() || !contains_digits(coll_id)) return;
	
	const string& full_url = string(collections.data()) + "/" + coll_id;
	
	int sockfd = open_connection(ip.data(), port, AF_INET, SOCK_STREAM, 0);
	string empty = "";
	char *message = compute_delete_request(ip, port, full_url, empty, jwt_token);
	send_to_server(sockfd, message);
	free(message);

	const string& response = receive_from_server(sockfd);
	int status = get_status(response);
	if (status < 300) {
		cout << "SUCCESS: Colecție ștearsă" << endl;
	} else {
		json body = json::parse(get_json(response));
		cout << "ERROR: " << body["error"].get<string>() << endl;
	}

	close_connection(sockfd);
}

void Client::del_mov_coll() {
	const string& coll_id = parse_cred("collection_id=");
	if (coll_id.empty() || !contains_digits(coll_id)) return;
	const string& movie_id = parse_cred("movie_id=");
	if (movie_id.empty() || !contains_digits(movie_id)) return;
	
	int sockfd = open_connection(ip.data(), port, AF_INET, SOCK_STREAM, 0);
	
	const string& full_url = string(collections.data()) + "/" + coll_id + "/movies/" + movie_id;
	string empty = "";
	char *message = compute_delete_request(ip, port, full_url, empty, jwt_token);
	send_to_server(sockfd, message);
	free(message);
	
	const string& response = receive_from_server(sockfd);
	int status = get_status(response);
	if (status < 300) {
		cout << "SUCCESS: Film șters din colecție" << endl;
	} else {
		json body = json::parse(get_json(response));
		cout << "ERROR: " << body["error"].get<string>() << endl;
	}

	close_connection(sockfd);
}

int main()
{
	std::ios::sync_with_stdio(false);
	Client c;
	string command;
	std::unordered_map<string, std::function<void()>> execute = {
		{ LOGIN_ADMIN,  [&c]() { c.login();           } },
		{ ADD_USER,     [&c]() { c.add_user();        } },
		{ GET_USERS,    [&c]() { c.get_users();       } },
		{ LOGOUT_ADMIN, [&c]() { c.logout_admin();    } },
		{ DELETE_USER,  [&c]() { c.delete_user();     } },
		{ LOGIN,        [&c]() { c.login_user();      } },
		{ LOGOUT,       [&c]() { c.logout();          } },
		{ GET_ACCESS,   [&c]() { c.get_access();      } },
		{ GET_MOVIES,   [&c]() { c.get_movies();      } },
		{ GET_MOVIE,    [&c]() { c.get_movie();       } },
		{ ADD_MOVIE,    [&c]() { c.add_movie();       } },
		{ UPDATE_MOVIE, [&c]() { c.update_movie();	  } },
		{ DELETE_MOVIE, [&c]() { c.delete_movie();    } },
		{ GET_COLLECTS, [&c]() { c.get_collections(); } },
		{ GET_COLLECT,  [&c]() { c.get_collection();  } },
		{ ADD_COLLECT,  [&c]() { c.add_collection();  } },
		{ ADD_MOV_COLL, [&c]() { c.add_mov_coll();    } },
		{ DEL_MOV_COLL, [&c]() { c.del_mov_coll();    } },
		{ DEL_COLLECT,  [&c]() { c.del_collection();  } },
		{ EXIT,         [&c]() {					  } }
	};

	while (true) {
		getline(cin, command);
		// gets the pair (key, value)
		auto comm = execute.find(command);
		if (comm != execute.end()) {
			if (command == EXIT)
				break;
			/* Execute the command */
			comm->second();
		} else {
			cout << "Unrecognized command" << endl;
		}
	}

	return 0;
}

