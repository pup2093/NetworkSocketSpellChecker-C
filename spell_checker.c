#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#define BACKLOG 10
#define DEFAULT_PORT_STR "14966"
#define EXIT_USAGE_ERROR 1
#define EXIT_GETADDRINFO_ERROR 2
#define EXIT_BIND_FAILURE 3
#define EXIT_LISTEN_FAILURE 4
#define MAX_LINE 64
#define DEFAULT_DICTIONARY "socketdictionary.txt"
#define NUM_WORKERS 4 //this is how many worker threads the main thread creates
#define BUFF_SIZE 20 //total # of spaces 





//function declarations
int getlistenfd(char*);
ssize_t readLine(int fd, void *buffer, size_t n);
void *consumer(void *args);
int isEmpty(int param[20]);



//struct that contains data to be operated on concurrently
typedef struct
{
	int sockets[BUFF_SIZE];
	int nextempty;
	int nextfull;
	sem_t full;
	sem_t empty;
	sem_t mutex;


} Buff;

Buff buff;


char dictionary[99171][40];//99172
int numwords;





int main(int argc, char **argv) {
	int listenfd;         /* listen socket descriptor */
	int connectedfd;       /* connected socket descriptor */
	struct sockaddr_storage client_addr;
	socklen_t client_addr_size;
	//char line[MAX_LINE];
	ssize_t bytes_read;
	char client_name[MAX_LINE];
	char client_port[MAX_LINE];
	char *port;
	char *dict_file;
	FILE *fp;
	//char dictionary[99172][40]; //dont need to protect dictionary, as we are only reading
	//int numwords;


	sem_init(&buff.full, 0, 0);
	sem_init(&buff.empty, 0, BUFF_SIZE);
	sem_init(&buff.mutex, 0, 1);



	pthread_t tids[NUM_WORKERS];
	


	if (argc < 3) {
		dict_file = DEFAULT_DICTIONARY;
	} else {
		dict_file = argv[2];
	}
	if (argc < 2) {
		port = DEFAULT_PORT_STR;
	} else {
		port = argv[1];
	}

	//read dictionary file and store into an array of strings
	numwords = 0;
	fp = fopen(dict_file, "r");




	if (fp == NULL)
	{
		printf("File could not be opened\n");
		return 0;
	}
	else
	{

		while (fgets(dictionary[numwords], sizeof(dictionary[numwords]), fp)) {

			int len = strlen(dictionary[numwords]);
			int ch;
			for (ch = 0; ch < strlen(dictionary[numwords]); ch++) {

				if (isalpha(dictionary[numwords][ch])) { //if its a letter do nothing
					;
				} else { //if its not a letter, replace with \0 (null character)
					dictionary[numwords][ch] = '\0';
				}

			}


			numwords++;
		}






		fclose(fp);
	}


	int j;
	for (j = 0; j < numwords; j++) {

		if (strcasecmp(dictionary[j], "abbas") == 0) { //testing. Will have to modify to make strcmp() case-insensitive
			printf("Welcome\n");

			break;
		}

		//printf("%s\n", dictionary[j]);
	}

	//create threads. As far as I know we put them into an array so we can call join on them
	int t;
	int *ref;
	int f;
	//void *ret;
	//pthread_create(&producerthread, NULL, producer, NULL);


	for (t = 0; t < NUM_WORKERS; t++) {
		//ref = (int *) malloc(sizeof(int));
		//*ref = t;
		pthread_create(tids + t,  NULL, consumer, NULL);
	}
	//pthread_exit(NULL);
	//join threads
	//pthread_join(producerthread, NULL);

	/*for (f = 0; f < NUM_WORKERS; f++) {
		pthread_join(*(tids + f), NULL);

	}*/


	//exit(0);




	//main thread or producer now listens for incoming sockets
	listenfd = getlistenfd(port);
	for (;;) {
		client_addr_size = sizeof(client_addr);
		if ((connectedfd = accept(listenfd, (struct sockaddr*)&client_addr,
		                          &client_addr_size)) == -1) {
			fprintf(stderr, "accept error\n");
			continue;
		}
		if (getnameinfo((struct sockaddr*)&client_addr, client_addr_size,
		                client_name, MAX_LINE, client_port, MAX_LINE,
		                0) != 0) {
			fprintf(stderr, "error getting name information about client\n");
		} else {
			printf("accepted connection from %s:%s\n", client_name,
			       client_port);

		}

		sem_wait(&buff.empty);
		//add connnectedfd to socket array (the socket array is the work queue)
		int s;
		for (s = 0; s < sizeof(buff.sockets); s++) {
			if (buff.sockets[s] == 0) {
				buff.sockets[s] = connectedfd;
				break;
			}
		}

		//signal sleeping worker threads that there's a new socket in the work queue
		sem_post(&buff.full);

		





	}

	for (f = 0; f < NUM_WORKERS; f++) {
		pthread_join(*(tids + f), NULL);

	}


	printf("threads returned\n");

	exit(0);






	return 0;
	//pthread_exit(NULL);
}






/* given a port number or service as string, returns a
descriptor that we can pass to accept() */
/* given a port number or service as string, returns a
   descriptor that we can pass to accept() */
int getlistenfd(char *port) {
	int listenfd, status;
	struct addrinfo hints, *res, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM; /* TCP */
	hints.ai_family = AF_INET;	   /* IPv4 */

	if ((status = getaddrinfo(NULL, port, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo error %s\n", gai_strerror(status));
		exit(EXIT_GETADDRINFO_ERROR);
	}

	/* try to bind to the first available address/port in the list.
	   if we fail, try the next one. */
	for (p = res; p != NULL; p = p->ai_next) {
		if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
			continue;
		}

		if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) {
			break;
		}
	}
	freeaddrinfo(res);
	if (p == NULL) {
		fprintf(stderr, "bind error. quitting.\n");
		exit(EXIT_BIND_FAILURE);
	}

	if (listen(listenfd, BACKLOG) < 0) {
		fprintf(stderr, "listen error. quitting.\n");
		close(listenfd);
		exit(EXIT_LISTEN_FAILURE);
	}
	return listenfd;
}

/* FROM KERRISK
Read characters from 'fd' until a newline is encountered. If a newline
character is not encountered in the first (n - 1) bytes, then the
excess
characters are discarded. The returned string placed in 'buf' is
null-terminated and includes the newline character if it was read in
the
first (n - 1) bytes.  The function return value is the number of bytes
placed in buffer (which includes the newline character if encountered,
but excludes the terminating null byte). */
ssize_t readLine(int fd, void *buffer, size_t n) { //fd comes from the array of sockets
	ssize_t numRead;                    /* # of bytes fetched by last read()
*/
	size_t totRead;                     /* Total bytes read so far */
	char *buf;
	char ch;
	if (n <= 0 || buffer == NULL) {
		errno = EINVAL;
		return -1;
	}
	buf = buffer;                       /* No pointer arithmetic on "void *"
*/
	totRead = 0;
	for (;;) {
		numRead = read(fd, &ch, 1);
		if (numRead == -1) {
			if (errno == EINTR)         /* Interrupted --> restart read() */
				continue;
			else
				return -1;              /* Some other error */
		} else if (numRead == 0) {      /* EOF */
			if (totRead == 0)           /* No bytes read; return 0 */
				return 0;
			else                        /* Some bytes read; add '\0' */
				break;
		} else {
			/* 'numRead' must be 1 if we get here
			*/
			if (totRead < n - 1) {      /* Discard > (n - 1) bytes */
				totRead++;
				*buf++ = ch;
			}
			if (ch == '\n')
				break;
		}
	}
	*  buf = '\0';
	return totRead;
}
//Since this includes includes the newline characte, make sure to change it to '\0' once its in char buf[]


//function for the worker threads
void *consumer(void *socket_desc) { //don't know what to pass to this yet! probably just pass nothing
	//Get the socket descriptor
	//int sock = *(int*)socket_desc;
	int sock;
	int i;
	int r;
	int len;
	int c;
	int j;
	char *msg;
	ssize_t bytes_read;
	//int connectedfd;
	char line[MAX_LINE];

	//sem_wait(&buff.full); //if no full slots, then wait to consume
	//sem_wait(&buff.mutex);



	while (1) {
		sem_wait(&buff.full); //if no full slots, then wait to consume
		sem_wait(&buff.mutex); //if another thread holds lock, then wait until it is released
		//once it gets to this point, past sem_wait(mutex), we can now do the critical section part of code

		for (r = 0; r < sizeof(buff.sockets); r++) {

			if (buff.sockets[r] != 0) {
				sock = buff.sockets[r]; //store socket in int variable before removing so we can use it
				buff.sockets[r] = 0; //remove socket

				while ((bytes_read = readLine(sock, line, MAX_LINE - 1)) > 0) {

					printf("just read %s", line);
					//write(sock, line, bytes_read); //this just echos back the client's word. Modify to spell check and echo "ok" or "misspelled"

					//remove newline character from line
					for (c = 0; c < strlen(line); c++) {
						if (isalpha(line[c])) {
							;
						} else {
							line[c] = '\0';
						}
					}

					for (j = 0; j < numwords; j++) {

						if (strcasecmp(dictionary[j], line) == 0) { //testing. Will have to modify to make strcmp() case-insensitive
							//msg = "OK\n";
							msg = strcat(line, " OK\n");
							write(sock, msg, strlen(msg));

							break;
						}

						//printf("%s\n", dictionary[j]);
					}

					if (j == numwords) {
						//msg = "MISSPELLED\n";
						msg = strcat(line, " MISSPELLED\n");
						write(sock, msg, strlen(msg));
					}

					//Also write to a log file!!
					sem_post(&buff.mutex);
					sem_post(&buff.empty);



				}
				printf("connection closed\n");
				close(sock);
				pthread_exit(NULL);

				//sem_post(&buff.mutex);
				//sem_post(&buff.empty);
				break;




			}
			//printf("connection closed\n");
			//close(sock);



		}
		break;


	}
	//sem_post(&buff.mutex);
	//sem_post(&buff.empty);
	//return 0;
	pthread_exit(NULL);




}





//function to check if socket/work array is empty
int isEmpty(int param[BUFF_SIZE]) {
	int l;
	for (l = 0; l < BUFF_SIZE; l++) {
		if (param[l] != 0) {
			return 1;
		}
	}
	return 0;
}







