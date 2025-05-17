CPPFLAGS=-Wall -Werror -Wno-error=unused-variable -std=c++17

SRCS := client.cpp helpers.cpp requests.cpp
OBJS := $(SRCS:.cpp=.o)

client: $(OBJS)
	g++ $(CPPFLAGS) -o $@ $^

%.o: %.cpp
	g++ $(CPPFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -f $(OBJS) client
