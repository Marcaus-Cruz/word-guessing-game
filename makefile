all: client.c server.c
	gcc -Wall -o client client.c
	gcc -Wall -o server server.c

clean:
	rm server
	rm client
