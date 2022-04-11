/* server.c - code for server program. Do not rename this file */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#define QLEN 6 /* size of request queue */

int main(int argc, char** argv) {

	struct protoent* ptrp; 	/* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold server's address */
	struct sockaddr_in cad; /* structure to hold client's address */
	int sd, sd2; 		/* socket descriptors */
	int port; 		/* protocol port number */
	int alen; 		/* length of address */
	int optval = 1; 	/* boolean value when we set socket option */
	char buf[1000]; 	/* buffer for string the server sends */

	char* word;		// Secret word
	uint8_t guesses;	// Number of guesses
	uint8_t wordSize;	// Size of the word for malloc
	char* display;		// Board shown to players
	char guess;		//user input
	int check;		//boolean
	int check2;		//boolean
	int pid;		//for parent/child process IDs


	if (argc != 4) {
		fprintf(stderr, "Error: Wrong number of arguments\n");
		fprintf(stderr, "usage:\n");
		fprintf(stderr, "./server server_port num_guesses word_to_guess \n");
		exit(EXIT_FAILURE);
	}

	memset((char*)&sad, 0, sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */
	sad.sin_addr.s_addr = INADDR_ANY; /* set the local IP address */

	//Get port #
	port = atoi(argv[1]);

	//Verify guesses
	guesses = atoi(argv[2]);
	if (guesses < 1 || guesses > 25){
		printf("Error: Number of guesses must be between 1 and 25\n");
		exit(0);
	}

	//Verify word size
	wordSize = strlen(argv[3]);
	if (wordSize > 254) {
		printf("Error: Word cannot exceed 254 characters \n");
		exit(0);
	}

	//Get word
	char lowerCase;
	word = malloc(wordSize + 1);
	for (int i = 0; argv[3][i] != '\0'; i++) {
		lowerCase = tolower(argv[3][i]);
		if (lowerCase < 97 || lowerCase > 122) {
			printf("Error: No special characters allowed \n");
			exit(0);
		}
		word[i] = (char) lowerCase;
	}


	//create what is to be displayed and updated
	display = malloc(wordSize);
	for (int i = 0; i < wordSize; i++) {
		display[i] = '-';
	}

	// Displays information about the server
	printf("Port: %d \nNumber of Guesses: %d \nWord: %s\n", port, guesses, word);

	/* tests for illegal port value */
	if (port > 0) {
		sad.sin_port = htons((u_short)port);
	}
	else { /* print error message and exit */
		fprintf(stderr, "Error: Bad port number %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	/* Map TCP transport protocol name to protocol number */
	if (((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create a socket */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Allow reuse of port - avoid "Bind failed" issues */
	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
		fprintf(stderr, "Error Setting socket option failed\n");
		exit(EXIT_FAILURE);
	}

	/* Bind a local address to the socket */
	if (bind(sd, (struct sockaddr*) & sad, sizeof(sad)) < 0) {
		fprintf(stderr, "Error: Bind failed\n");
		exit(EXIT_FAILURE);
	}

	/* Specify size of request queue */
	if (listen(sd, QLEN) < 0) {
		fprintf(stderr, "Error: Listen failed\n");
		exit(EXIT_FAILURE);
	}

	/* Main server loop - accept and handle requests */
	while (1) {
		alen = sizeof(cad);
		if ((sd2 = accept(sd, (struct sockaddr*) & cad, (socklen_t*) &alen)) < 0) {
			fprintf(stderr, "Error: Accept failed\n");
			exit(EXIT_FAILURE);
		}

		//send word size, wait for ACK
		send(sd2, &wordSize, sizeof(uint8_t), 0);
		recv(sd2, buf, sizeof(char), 0);

		// Fork, parent loops to receive additonal clients, child runs the game
		pid = fork();

		//if parent, close socket and loop
		if (pid > 0) {
			close(sd2);
		}
		//if fork errors
		else if (pid < 0){
			fprintf(stderr, "Error: fork failed\n");
			close(sd2);
			exit(EXIT_FAILURE);
		}
		//else we are a child, run the game
		else {
			while (guesses > 0){
				//send numGuesses
				send(sd2, &guesses, sizeof(uint8_t), 0);
				recv(sd2, buf, sizeof(char), 0);

				//send board, wait for ACK
				send(sd2, display, strlen(display), 0);
				recv(sd2, buf, sizeof(char), 0);

				// Wait to receive a guess and process the guess
				recv(sd2, buf, sizeof(char), 0);
				guess = (char) buf[0];

				// Verify if the guess is correct or incorrect, if correct: update the board
				check = 0;
				for (int i = 0; i < wordSize; i++) {
					if (word[i] == guess && display[i] == '-') {
						check = 1;
						display[i]= guess;
					}
				}

				// If guess is incorrect, decrement number of guesses
				if (check == 0){
					guesses--;
				}

				// Loop through the board to see if the word has been completed
				check2 = 1;
				for (int i = 0; i < wordSize; i++){
					if (display[i] == '-'){
						check2 = 0;
					}
				}

				// If the word has been completed, send win condition to client
				if (check2 == 1){
					guesses = 255;
					send(sd2, &guesses, sizeof(uint8_t), 0);
					recv(sd2, buf, sizeof(char), 0);
					send(sd2, word, strlen(word), 0);
					close(sd2);
					exit(0);
				}
			}

			// If number of guesses == 0 and the word is not solved, we send lose condition to client, close socket, exit
			send(sd2, &guesses, sizeof(uint8_t), 0);
			close(sd2);
			exit(0);
		}
	}
}
