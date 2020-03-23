
// Client side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
  
#define SRC_PORT 9876 

#define DST_PORT 8765 
#define DST_IP "192.168.56.101"

#define MAXLINE 4000	
  
#define PAYLOAD_SIZE 1002
// Driver code 
int main() { 
    int sockfd; 
    char buffer[MAXLINE]; 
    char *hello = "Hello from client"; 
    struct sockaddr_in servaddr, cliaddr; 
  
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

	for (int i = 0; i < 6000; i++) {
		int n, len; 

		int curr_id = 5999;
		char hold_num[5];
		sprintf(hold_num, "%04d", i);

		char payload[PAYLOAD_SIZE];

		for (int i = 0; i < PAYLOAD_SIZE - 1; i++) {
			if (i < 4) {
				payload[i] = hold_num[i];
			} else {
				payload[i] = '0';
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
    return 0; 
} 

