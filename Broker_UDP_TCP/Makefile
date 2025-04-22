
CFLAGS = -Wall -g -Werror -Wno-error=unused-variable

all: server subscriber

utils.o: utils.cpp

server: server.cpp utils.o

subscriber: subscriber.cpp utils.o

.PHONY: clean 	

clean:
	rm -rf server subscriber *.o
