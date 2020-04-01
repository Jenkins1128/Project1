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
#define MAXFILE 100
  
#define PAYLOAD_SIZE 1002

void receive_file(int sockfd) {
	char buff[MAXLINE];

	FILE *fp;
	if ( (fp = fopen("received_config_file.c", "w")) == NULL) {
		perror("Error in opening file");
		exit(EXIT_FAILURE);
	}

	printf("Start\n");
	while ( read(sockfd, buff, MAXLINE) > 0) {
		printf("Start\n");
		printf("%s\n", buff);
		fprintf(fp, "%s", buff);
	}

	fclose(fp);
	printf("File received.\n");
}

void send_file(int sockfd) {
	char buff[100];

	FILE *fp;
	if ( (fp = fopen("client.c", "r")) == NULL) {
		perror("Error in opening file");
		exit(EXIT_FAILURE);
	}

	printf("We started\n");
	while ( fgets(buff, MAXFILE, fp) != NULL) {
		printf("%s\n", buff);
		write(sockfd, buff, sizeof(buff));
	}
	printf("We finished\n");
	
	fclose(fp);
	printf("File sent.\n");
}

int main() { 
    int sockfd; 
	int tcp_sockfd, tcp_connfd;
    char buffer[MAXLINE]; 
    char *hello = "Hello from client"; 
    struct sockaddr_in servaddr, cliaddr; 


    memset(&servaddr, 0, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(DST_PORT); 
    servaddr.sin_addr.s_addr = inet_addr(DST_IP); 

	memset(&cliaddr, 0, sizeof(cliaddr));
    cliaddr.sin_family = AF_INET; 
    cliaddr.sin_port = htons(SRC_PORT); 
    cliaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	/* Pre-Probing Phase TCP Phase */
    if ( (tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 

	if (connect(tcp_sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
		perror("connection to server failed.");
		exit(0);
	}
	
	receive_file(tcp_sockfd);
	close(tcp_sockfd);
	printf("Finished\n");
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

