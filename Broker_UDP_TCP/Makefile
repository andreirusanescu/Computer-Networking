
CFLAGS = -Wall -g -Werror -Wno-error=unused-variable

# Portul pe care asculta serverul
PORT = 12345

# Adresa IP a serverului
IP_SERVER = 192.168.0.2

all: server subscriber

common.o: common.cpp

server: server.cpp common.o

subscriber: subscriber.cpp common.o

.PHONY: clean 	

clean:
	rm -rf server subscriber *.o
