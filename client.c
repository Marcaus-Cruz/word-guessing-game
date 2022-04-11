/* client.c - code for client program. Do not rename this file */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int main(int argc, char** argv) {

	struct hostent* ptrh; 	/* pointer to a host table entry */
	struct protoent* ptrp; 	/* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold an IP address */
	int sd; 		/* socket descriptor */
	int port; 		/* protocol port number */
	char* host; 		/* pointer to host name */
	char buf[1000]; 	/* buffer for data from the server */
	char* Board;		/* Board displayed to user*/
	uint8_t guess = 1;	/* Number of guesses */
	uint8_t wordLen;	/* Length of the secret word */
	char input; 		/* The user's first char input */
	char input2;		/* The user's second char input, if they input too many chars */
	char prompt[] = "Enter guess: ";
	char inval[] = "Invalid guess (must be a single lowercase character from a-z)"; 

	memset((char*)&sad, 0, sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */

	if (argc != 3) {
		fprintf(stderr, "Error: Wrong number of arguments\n");
		fprintf(stderr, "usage:\n");
		fprintf(stderr, "./client server_address server_port\n");
		exit(EXIT_FAILURE);
	}

	port = atoi(argv[2]); /* convert to binary */
	if (port > 0) /* test for legal value */
		sad.sin_port = htons((u_short)port);
	else {
		fprintf(stderr, "Error: bad port number %s\n", argv[2]);
		exit(EXIT_FAILURE);
	}

	host = argv[1]; /* if host argument specified */

	/* Convert host name to equivalent IP address and copy to sad. */
	ptrh = gethostbyname(host);
	if (ptrh == NULL) {
		fprintf(stderr, "Error: Invalid host: %s\n", host);
		exit(EXIT_FAILURE);
	}

	memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);

	/* Map TCP transport protocol name to protocol number. */
	if (((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create a socket. */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Connect the socket to the specified server. */
	if (connect(sd, (struct sockaddr*) & sad, sizeof(sad)) < 0) {
		fprintf(stderr, "connect failed\n");
		exit(EXIT_FAILURE);
	}

	char test = 'a';

	//get wordLen
	recv(sd, buf, sizeof(buf), 0);
	wordLen = (uint8_t)buf[0];
	send(sd, &test, sizeof(char), 0);

	Board = malloc(wordLen + 1);

	// Game loop
	while (guess > 0) {

		//get num guesses from server
		recv(sd, buf, sizeof(buf), 0);
		guess = (uint8_t)buf[0];
		send(sd, &test, sizeof(char), 0);


		// If the lose condition has been sent by server
		if (guess == 0) {
			printf("Board: %s\n", Board);
			printf("You Lose!\n");
			close(sd);
			exit(0);
		}

		// If the win condition has been sent
		else if (guess == 255) {
			recv(sd, buf, sizeof(buf), 0);
			for (int i = 0; i < wordLen; i++) {
				Board[i] = buf[i];
			}
			Board[wordLen] = '\0';
			printf("Board: %s\n", Board);
			printf("You Won!\n");
			close(sd);
			exit(0);
		}

		//get Board from server
		recv(sd, buf, sizeof(buf), 0);
		for (int i = 0; i < wordLen; i++) {
			Board[i] = buf[i];
		}
		Board[wordLen] = '\0';

		// Print Board to user
		printf("Board: %s ", Board);
		printf("# of guesses: %d\n", guess);
		send(sd, &test, sizeof(char), 0);

		// Prompt user for input
		input = '\0';
		while (input == '\0') {
			do {
				printf("%s", prompt);

				// get first char
				input = getchar();

				if (input == EOF){
					break;
				}

				// If user hits the enter key, we loop
				else if (input == '\n'){
					continue;
				}

				// check length of input, if input2 isn't the new line char or EOF, input is too long
				// If the input length isn't valid, we print an "invalid" error statement to user and loop
				input2 = getchar();
				if (input2 == '\n' || input2 == EOF){
					break;
				}
				else {
					printf("%s\n", inval);
				}

				// junk the rest of the input
				do {
					input = getchar();
				} while (input != '\n' && input != EOF);
			} while (input != EOF);

			// Check if the single char input is a valid char
			if (input > 96 && input < 123) {
				send(sd, &input, sizeof(char), 0);
			}
			// if the input is a new line character, we prompt the user for input again
			else if (input == '\n'){
				input = '\0';
			}
			// otherwise the input is invalid, we print an "invalid" error statement to user and loop
			else {
				printf("%s\n", inval);
				input = '\0';
			}
		}

	}
	close(sd);

	exit(EXIT_SUCCESS);


}
