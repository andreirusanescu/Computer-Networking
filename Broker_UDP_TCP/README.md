# Copyright Andrei-Marian Rusanescu 323CC 2024-2025

## Data Structures used
I used the STL (standard template library) of C++ for for
the following structures: `vector`, `unordered_map`, `unordered_set`,
`string` and I defined myself a structure for the subscribers: `subscriber_t`.

1. `vector` - used it due to its internal resizable array,
basically whenever push_back() is called, whether it be for
a new `pollfd` structure or anything else, internally it
checks if it has enough capacity to emplace it directly
or reallocates the internal vector by some criteria. Moreover,
elements stored by this vector are deallocated automatically when
the object is going out of scope, reducing the risk of memory leaks.

2. `unordered_map` - basically a hashtable that maps key to value.
Used it for mapping the ID (string) of the subscriber to its
corresponding structure `subscriber_t` that contains more information,
that is later on used, for example when checking if a new connection
with an ID is connected or not. Furthermore, I used another one for
mapping the FD of the TCP connection to its corresponding ID used
to retrieve the whole structure of the subscriber. Why didn't I map the
FD to the structure directly ? Because that would be inneficient
holding the structure 2 times in memory and this would also increase
synchronization issues between data reads/writes.
This way I get `O(1)` access - very efficient.

3. `unordered_set` - used in the `subscriber_t` structure to store
the set of topics the client subscribed to in O(1). This is efficient
in the case of inserting or erasing a topic for the (un)subscribe
commands. I could of used a vector but the overhead would have increased
as I should have iterated through all the topics and checked with
strcmp if they are equal.

4. `subscriber_t` - decided to call it this way to better make the
distinction between a variable called subscriber or the type itself.
Good practice in C/C++.

5. `string` - I used a string and not `char *` because a map of char *
compares addresses, not the actual content - the string is also guaranteed
to be null terminated - you can also access the size of it.

## Overall Implementation
The server has to be run before the subscribers with the command
`./server <PORT>` - the server will then listen for connections
on any IP available. `Nagle`'s algorithm is disabled for performance
and reusable addresses are enabled in case the server is run multiple times
in a shorter period of time. The server has 3 main file descriptors, one
for `TCP` connections used by the subscribers, another for `UDP` connections
used by clients that update the topics of the server and another for
`STDIN` input from the user - this is used in case the user wants to
close the server using the command `exit`.

In order to listen on multiple interfaces for activity, I used the `POLL` API
from `poll.h`. The following scenarios are possible:

1. `A TCP client wants to connect to the server`:
I implemented the following protocol for this: when a client connects to
the server it has to send its `ID`. Afterwards, it has to wait for a 
confirmation from the server that its ID is not already used. If it is used
the client's connection is closed, otherwise there are 2 scenarios possible:
either the client has connected priorly on the server -> its status is
updated to connected in the map of ID -> subscriber_t structure, or it's the
client's first time being on the server, a subscriber_t structure is created
and the corresponding mappings. Its FD is also pushed back in the vector of
pollfds.

2. `An UDP client wants to make an update on a certain topic`:
The update is processed using a lot of memcpy's to retrieve data from the
payload of the sender, with respect to the format used. Then for each
connected subscriber, and for each topic of a subscriber it is checked if
they match, i.e. if the subscriber subscribed to this topic. If they do match,
the subscriber is sent the update. The match function is using dynammic
programming and it is inspired from my solution to this leetcode problem:
`https://leetcode.com/problems/wildcard-matching/`.
The subscriber has to be sent the sender's ip addres, its port, the topic, the
data type of the content and the content. I could just send a buffer or a 
structure holding all this data but that would be inneficient because the 
content has variable format and sending 1500 bytes over the network just for 
an integer is not good. So, the `protocol` I implemented is basically
send the sender's ip and port along with the topic_size (the topic size
is the size of the topic + size of data + the content length + 1 for the
null terminator). This way the receiver (subscriber) knows for how many
bytes to expect when receiving the content. Afterwards, I send the content
with exactly how many bytes it uses - content_len.

3. `STDIN input - exit`: Only the `exit` command is processed.

4. `Subscribe/Unsubscribe notification` from the TCP clients.
Here I used the following protocol: I encapsulated the choice of
subscribe/unsubscribe in the first byte of the received notification
and the topic in the following bytes. The subscriber's set of topics is updated
efficiently according to the request.

## Subscriber
In the subscriber I made sure of respecting the protocols aforementioned
above - basically string processing and memcpy's to retrieve or print data.
I used strtok for spliting the input.
Instead of using a switch case like I did in the server, I used an array of
functions to print the data in accordance with its type.

## Conclusions
This was an interesting assignment where I could practice my C++ skills
and better understand the POLL API along with UDP and TCP protocols, while
also brushing up on low level string management and memory accesses. Now
I am confident I can implement a TCP/UDP server or use the `socket.h` API.
