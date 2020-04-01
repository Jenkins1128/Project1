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

void receive_file(int sockfd) {
	char buff[MAXLINE];

	FILE *fp;
	if ( (fp = fopen("recieved_config_file.c", "w")) == NULL) {
		perror("Error in opening file");
		exit(EXIT_FAILURE);
	}

	while ( read(sockfd, buff, MAXLINE) > 0) {
		fprintf(fp, "%s", buff);
	}

	fclose(fp);
	printf("File received.\n");
}

void send_file(int sockfd) {
	char buff[MAXLINE];

	FILE *fp;
	if ( (fp = fopen("client.c", "r")) == NULL) {
		perror("Error in opening file");
		exit(EXIT_FAILURE);
	}

	while ( fgets(buff, MAXLINE, fp) != NULL) {
		write(sockfd, buff, sizeof(buff));
	}
	
	fclose(fp);
	printf("File sent.\n");
}
  
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
	if( (tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("tcp socket creation failed"); 
        exit(EXIT_FAILURE); 
	}
	

    if ( bind(tcp_sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ) { 
        perror("tcp bind failed"); 
        exit(EXIT_FAILURE); 
    } 

	if ((listen(tcp_sockfd, 5)) != 0) {
		perror("tcp listen failed");
		exit(EXIT_FAILURE);
	}

	tcp_len = sizeof(cliaddr);

	if ( (tcp_connfd = accept(tcp_sockfd, (struct sockaddr *)&cliaddr, &tcp_len)) < 0) {
		perror("tcp server accept failed");
		exit(EXIT_FAILURE);
	}
	
	// receive_file(tcp_sockfd);
	send_file(tcp_sockfd);
	close(tcp_sockfd);
	

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

