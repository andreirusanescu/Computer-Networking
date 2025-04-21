
CFLAGS = -Wall -g -Werror -Wno-error=unused-variable

# Portul pe care asculta serverul
PORT = 12345

# Adresa IP a serverului
IP_SERVER = 192.168.0.2

all: server subscriber

utils.o: utils.cpp

server: server.cpp utils.o

subscriber: subscriber.cpp utils.o

.PHONY: clean 	

clean:
	rm -rf server subscriber *.o
