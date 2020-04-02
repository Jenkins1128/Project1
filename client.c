#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <time.h>
  
#define SRC_PORT 9876 

#define DST_PORT 8765 
#define DST_IP "192.168.56.101"

#define MAXLINE 5000	
#define MAX 100
  
#define PAYLOAD_SIZE 1002
#define SA struct sockaddr 


void recvFile(int sockfd) 
{ 
	char buff[MAX]; 	// to store message from client
	
	FILE *fp;
	fp=fopen("received.c","w"); // stores the file content in recieved.txt in the program directory
	
	if( fp == NULL ){
		printf("Error IN Opening File ");
		return ;
	}
	
	while( read(sockfd,buff,MAX) > 0 )
		fprintf(fp,"%s",buff);
	
	printf("File received successfully !! \n");
	printf("New File created is received.txt !! \n");

} 

void sentFile(int sockfd) 
{ 
	char buff[MAX]; 						// for read operation from file and used to sent operation 
	
	// create file 
	FILE *fp;
	fp=fopen("client.c","r");		// open file uses both stdio and stdin header files
											// file should be present at the program directory

	if( fp == NULL ){
		printf("Error IN Opening File .. \n");
		return ;
	}
	
	while ( fgets(buff,MAX,fp) != NULL )	// fgets reads upto MAX character or EOF 
		write(sockfd,buff,sizeof(buff)); 	// sent the file data to stream
	
	fclose (fp);							// close the file 
	
	printf("File Sent successfully !!! \n");
	
} 

void pre_probe_cli() {
	int sockfd, connfd; 
	struct sockaddr_in servaddr, cli; 

	// socket create and varification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		printf("socket creation failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully created..\n"); 
	
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = inet_addr(DST_IP); 
	servaddr.sin_port = htons(DST_PORT); 

	// connect the client socket to server socket 
	if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
		printf("connection with the server failed...\n"); 
		exit(0); 
	} 
	else
		printf("connected to the server..\n"); 

	// function for sending File 
	sentFile(sockfd); 

	// close the socket 
	close(sockfd); 
}

int main() { 
    int sockfd; 
	int tcp_sockfd, tcp_connfd;
    char buffer[MAXLINE]; 
    char *hello = "Hello from client"; 
    struct sockaddr_in servaddr, cliaddr; 



	memset(&cliaddr, 0, sizeof(cliaddr));
    cliaddr.sin_family = AF_INET; 
    cliaddr.sin_port = htons(SRC_PORT); 
    cliaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	/* Pre-Probing Phase TCP Phase */
	pre_probe_cli();
	/* End Pre-Probaing Phase TCP Phase */

  
	/* Probing Phase */

    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
	int on = IP_PMTUDISC_DO;
	setsockopt(sockfd, IPPROTO_IP, IP_MTU_DISCOVER, &on, sizeof(on));
  
    // Server information 
    memset(&servaddr, 0, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(DST_PORT); 
    servaddr.sin_addr.s_addr = inet_addr(DST_IP); 

	// Client information
	memset(&cliaddr, 0, sizeof(cliaddr));
    cliaddr.sin_family = AF_INET; 
    cliaddr.sin_port = htons(SRC_PORT); 
    cliaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sockfd, (struct sockaddr *) &cliaddr, sizeof(cliaddr)) < 0) {
		perror("bind error");
		exit(EXIT_FAILURE);
	}

	srand(time(0));	
	for (int i = 1; i <= 6000; i++) {
		int n, len; 

		int curr_id = 5999;
		char hold_num[5];
		sprintf(hold_num, "%04d", i);

		char payload[PAYLOAD_SIZE];

		for (int i = 0; i < PAYLOAD_SIZE - 1; i++) {
			if (i < 4) {
				payload[i] = hold_num[i];
			} else {
				// payload[i] = '0'; // All 0's in
				int rand_num = rand() % 10;
				char hold_rand_num[2];
				sprintf(hold_rand_num, "%d", rand_num);
				payload[i] = hold_rand_num[0];
			}
		}
		payload[PAYLOAD_SIZE - 2] = '\0';
		  
		//; sendto(sockfd, (const char *)hello, strlen(hello), 
		sendto(sockfd, (const char *)payload, strlen(payload), 
			MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
				sizeof(servaddr)); 
		printf("Hello message sent.\n"); 
			  
		n = recvfrom(sockfd, (char *)buffer, MAXLINE,  
					MSG_WAITALL, (struct sockaddr *) &servaddr, 
					&len); 
		buffer[n] = '\0'; 
		printf("Server : %s\n", buffer); 
	}
  
    close(sockfd); 

	/* End Probaing Phase */


	/* Post-Probing TCP Phase */

	/* End Post-Probaing TCP Phase */


    return 0; 
} 

