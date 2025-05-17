# COPYRIGHT Rusanescu Andrei-Marian 2025

To begin with, I wrote the REST API client in C++. Since C++ typically uses
`std::cin` and `std::cout`, I disabled synchronization with the C stdio streams 
using `std::ios::sync_with_stdio(false)` to improve performance. After 
benchmarking, I found that using `std::cout` with synchronization disabled was 
almost twice as fast as using the C-style printf function.

Instead of writing a long and inefficient main function that checks every user 
command using an extended if–else if chain (a switch statement wouldn’t help 
here, since strings cannot be used in a jump table like integral types and must 
still be compared byte by byte), I used a hash map to associate command strings 
with their corresponding handler methods.

The main logic resides in the Client class. I chose to encapsulate the 
implementation in a class to limit the scope of what would otherwise be global 
variables, making them private members instead.

I used constants wherever possible to reduce the likelihood of errors and 
employed macros to improve readability. Additionally, I used `std::string` in
combination with `std::string_view` to benefit from their interoperability.
This approach maintains the safety and rich feature set of C++ strings, while
still allowing for efficient, non-owning views similar to what C offers, but in 
a more robust and type-safe manner. I used string_view for the url's.
I took some of the implementation from the lab and rewrote it with strings
due to the fact that I used the outputs of those function later on as strings.
 
## Parsing methods
1. `parse_input` - decides whether a string contains only one word (one token)
if split by space (' ').

2. `get_cookie` - sets the cookie for the current session by extracting it
from the given input string.

3. `get_status` - extracts the status of the request out of the header (string).

4. `contains_digits` - returns true if the input string contains only digits,
false otherwise.

5. `is_double` - checks if an input string is the numerical representation of
a double number. If it contains more than 1 dot or non numerical characters
it returns false.

6. `get_json` - extracts the `JSON` body out of the header.

## Client commands
1. `login` - method used by an admin. Asks for username and password - both
should not contain any whitespaces.

2. `add_user` - adds an user to an admin. Asks for username and password.
Method is effective when the user logged in as admin.

3. `get_users` - displays the users of an admin with their id, username and 
password. 
Method is effective when the user logged in as admin.

4. `delete_user` - deletes an user. Asks for username.
Method is effective when the user logged in as admin.

5. `login_user` - login function for a regular user. Asks for admin_username,
username and password of the user.
Method is effective when there is no user currently logged in.

6. `get_access` - gets a `JWT access token` to the movie/collection library.
Sets the `jwt_token` member.
Method is effective when there is a user currently logged in.

7. `logout_admin` - logs out an admin and resets the JWT token and cookie.
Method is effective when the user logged in as admin.

8. `logout` - logs out a regular user and resets the JWT token and cookie.
Method is effective when the user is logged in as a regular user.

9. `get_movies` - displays the list of movies a user has access to (ID and title).
This method is effective only if the JWT token has been acquired beforehand.

10. `get_movie` - displays the details of a movie. Asks for ID.
This method is effective only if the JWT token has been acquired beforehand.

11. `add_movie` - adds a movie to a user's profile. Asks for title
(string - spaces are permitted), year (integral), description (string - spaces
are permitted) and rating (double). If any of the aforementioned fields are
not set correctly the function will not add the movie.
This method is effective only if the JWT token has been acquired beforehand.

12. `update_movie` - updates an existing movie (asks for ID) data.
This method is effective only if the JWT token has been acquired beforehand.

13. `delete_movie` - deletes a movie (asks for ID).
This method is effective only if the JWT token has been acquired beforehand.

14. `get_collections` - displays the collections of a user.
This method is effective only if the JWT token has been acquired beforehand.

15. `get_collection` - displays the data of a collection based on its ID.
ID is either set by the user directly from stdin input or is assigned the
value of `collection_id` that is set in the `add_collection` func. It is called
from `add_collection` to display the data of the newly added collection.
This method is effective only if the JWT token has been acquired beforehand.

16. `add_collection` - adds a new collection to a user's profile. Adds an empty
collection, extracts the ID of the newly added collection from the response
of the server, then calls `add_mov_coll` in a for loop to add `n` movies
to the collection, finishing with the display of the final product.
This method is effective only if the JWT token has been acquired beforehand.

17. `add_mov_col` - adds a movie to a collection. It either asks for the ID
of the collection and the ID of the movie or assigns them from `collection_id`
and `curr_movie_id` members of the class that are set in the `add_collection`
function.
This method is effective only if the JWT token has been acquired beforehand.

18. `del_collection` - deletes a collection based on the ID of it (asks for ID).
This method is effective only if the JWT token has been acquired beforehand
and the user calling it is the owner of it.

19. `del_mov_col` - deletes a movie from a collection. Asks for the ID of the
collection and the ID of the movie.
This method is effective only if the JWT token has been acquired beforehand
and the user calling it is the owner of it and the movie exists.


