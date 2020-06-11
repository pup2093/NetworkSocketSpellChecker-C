#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>



int main(int argc, char const *argv[])
{
	//For the data having directly to do with the spell checking(except the dictionary), make sure to wrap it into a struct


	int socket_desc;
	//in_addr_t = inet_addr
	struct sockaddr_in server;
	



	socket_desc = socket(AF_INET, SOCK_STREAM, 0); //socket() creates a socket and returns a socket descriptor
	/*AP_INET = IP version 4
	SOCK_STREAM = TCP protocol
	0 means IP protocol
	*/

	if (socket_desc == -1) {
		printf("Could not create socket\n");
	}

	server.sin_addr.s_addr = inet_addr("74.125.235.20"); //this is the loopback address, which will be default
	//note: later on, you will have to let the user pass in an IP address on the command line
	server.sin_family = AF_INET;
	server.sin_port = htons(80);//

	//Connect tot remote server
	if(connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0){
		puts("connect error");
		return 1; 
	}

	puts("Connected");





	return 0;
}