// Server side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
  
#define PORT    8765
#define MAXLINE 5000 
  
// Driver code 
int main() { 
	// TCP Stuff
	int tcp_sockfd, tcp_connfd, tcp_len;
	// UDP Stuff
    int udp_sockfd, udp_len, udp_rcvd; 
    char buffer[MAXLINE]; 
    char *hello = "Hello from server"; 
    struct sockaddr_in servaddr, cliaddr; 

    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 

    // Filling server information 
    servaddr.sin_family = AF_INET; // IPv4 
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 

      
	/* Pre-Probing Phase TCP Connection */
	/*
    if ( (tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) { 
        perror("tcp socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 

    // Bind the tcp socket with the server address 
    if ( bind(tcp_sockfd, (const struct sockaddr *)&servaddr,  
            sizeof(servaddr)) < 0 ) 
    { 
        perror("tcp bind failed"); 
        exit(EXIT_FAILURE); 
    } 

	// Server rdy to listen
	if ((listen(tcp_sockfd, 5)) != 0) {
		perror("Listen failed...");
		exit(EXIT_FAILURE);
	}
	tcp_len = sizeof(cliaddr);
	printf("Listening\n");

	// Accept packets from client
	if( (tcp_connfd = accept(tcp_sockfd, (struct sockaddr *)&cliaddr, &tcp_len)) < 0) {
		perror("TCP server accept failed");
		printf("error");
	}
	printf("waiting for packets");

	int file_buffer_size = 1000;
	char file_buffer[file_buffer_size];
	FILE *fp;
	fp = fopen("rcvd_file.txt", "w");
	if (fp == NULL) {
		perror("TCP rcvd file error when opening");
		exit(EXIT_FAILURE);
	}

	while( read(tcp_connfd, file_buffer, file_buffer_size) > 0) {
		fprintf(fp, "%s", file_buffer);
	}
	
	printf("Config file recieved.\n");
	close(tcp_sockfd);
	*/

	/* End Pre-Probing Phase TCP Connection */




	/* Probing Phase */

    // Creating udp socket file descriptor 
    if ( (udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("udp socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
      
      
      
    // Bind the socket with the server address 
    if ( bind(udp_sockfd, (const struct sockaddr *)&servaddr,  
            sizeof(servaddr)) < 0 ) 
    { 
        perror("udp bind failed"); 
        exit(EXIT_FAILURE); 
    } 
   
    udp_len = sizeof(cliaddr);  //udp_len is value/resuslt 
  
	while (1) {
		udp_rcvd = recvfrom(udp_sockfd, (char *)buffer, MAXLINE,  
					MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
					&udp_len); 
		buffer[udp_rcvd] = '\0'; 
		printf("Client : %s\n", buffer); 
		sendto(udp_sockfd, (const char *)hello, strlen(hello),  
			MSG_CONFIRM, (const struct sockaddr *) &cliaddr, 
				udp_len); 
		printf("Hello message sent.\n");  
	}

	/* End Probing Phase */



	/* Post-Probing Phase TCP Connection */


	/* End Post-Probing Phase TCP Connection */
      
    return 0; 
} 

