#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <time.h>
#include <netdb.h> 
  
#define SRC_PORT 9876 

#define DST_PORT 8765 
#define TCP_PORT 8080
#define DST_IP   "192.168.56.101"

#define MAXLINE  6000	
#define MAX 	 80
  
#define PAYLOAD_SIZE 1002
#define SA struct sockaddr 

/* Function used to receive file */
void recvFile(int sockfd) { 
	// Buffer used to receive file 
	char buff[MAX]; 
	
	// Opening file that that will hold received file
	FILE *fp;
	
	// Ensure the file 
	if((fp = fopen("received.c", "w")) == NULL) {
		perror("Error IN Opening File");
		return ;
	}
	
	// Read the file and write it to file
	while(read(sockfd, buff, MAX) > 0) {
		fprintf(fp, "%s", buff);
	}
	
	// Close file pointer after writing to the file
	fclose(fp);
} 

/* Function used to send file */
void sentFile(int sockfd) { 
	// Buffer used to receive file 
	char buff[MAX];
	
	// Create file 
	FILE *fp;

	// Open file and ensure it opens
	if((fp=fopen("client.c", "r")) == NULL) {
		printf("Error IN Opening File .. \n");
		return ;
	}
	
	// Read the contents of the file into the buffer and send it over socket
	while (fgets(buff, MAX, fp) != NULL) {
		write(sockfd, buff, sizeof(buff));
	}
	
	// Close file pointer after writing to the file
	fclose(fp);					
} 

/* Function used for handling pre-phobing phase */
void pre_probe_cli() {
	int sockfd; 
	struct sockaddr_in servaddr; 

	// Create TCP socket and make sure it was created successfully
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
		perror("socket creation failed..."); 
        exit(EXIT_FAILURE); 
	}
	
	// Allow the socket to reuse a socket in use (needed for consecutive runs)
	int on = IP_PMTUDISC_DO;
	int sso_return = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	bzero(&servaddr, sizeof(servaddr)); 

	// Assign IP, PORT for server information
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = inet_addr(DST_IP); 
	servaddr.sin_port = htons(TCP_PORT); 

	// Connect client socket to server socket 
	if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
		perror("connection with the server failed..."); 
        exit(EXIT_FAILURE); 
	} 

	// Fnction for sending File 
	sentFile(sockfd); 

	// Close the socket 
	close(sockfd); 
}

void probe_cli() {
    int sockfd, rcvd_msg; 
    char buffer[MAXLINE]; 
    struct sockaddr_in servaddr, cliaddr; 

    // Create UDP socket and make sure it was created succesfully
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
	
	// Set socket to have DON'T FRAGMENT bit set
	int on = IP_PMTUDISC_DO;
	setsockopt(sockfd, IPPROTO_IP, IP_MTU_DISCOVER, &on, sizeof(on));
  
	// Assign IP, PORT for server information
    memset(&servaddr, 0, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(DST_PORT); 
    servaddr.sin_addr.s_addr = inet_addr(DST_IP); 

	// Assign IP, PORT for client information (needed for making the right port)
	memset(&cliaddr, 0, sizeof(cliaddr));
    cliaddr.sin_family = AF_INET; 
    cliaddr.sin_port = htons(SRC_PORT); 
    cliaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	// Needed to make sure it shows right source port
	if (bind(sockfd, (struct sockaddr *) &cliaddr, sizeof(cliaddr)) < 0) {
		perror("bind error");
		exit(EXIT_FAILURE);
	}

	// Needed for random seed
	srand(time(0));	

	// Creating Packet Train
	char packet_train[6000][PAYLOAD_SIZE];
	for (int i = 0; i < sizeof(packet_train)/sizeof(packet_train[0]); i++) {
		// Create ID
		int id_num = i + 1;
		char hold_num[5];
		sprintf(hold_num, "%04d", id_num);

		for (int j = 0; j < PAYLOAD_SIZE - 1; j++) {
			if (j < 4) {
				packet_train[i][j] = hold_num[j];
			} else {
				int rand_num = rand() % 10;
				char hold_rand_num[2];
				sprintf(hold_rand_num, "%d", rand_num);
				// packet_train[i][j] = hold_rand_num[0];
				packet_train[i][j] = '0'; // All 0's in
			}
			packet_train[i][PAYLOAD_SIZE - 2] = '\0'; // All 0's in
		}
	}

	// Send message to server to indicate next packet will be start of Low Entrophy
	// packet train. Let's server know to start timer
	// Ensures the server gets the message. 
	char* start_msg = "Start LOW UDP Train";
	int len = sizeof(servaddr);
	while (strcmp(start_msg, buffer) == 0) {
		sendto(sockfd, start_msg, strlen(start_msg), 
			MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
				sizeof(servaddr)); 
		rcvd_msg = recvfrom(sockfd, (char *)buffer, MAXLINE,
							MSG_DONTWAIT, ( struct sockaddr *)&servaddr,
							&len);
	}

	// Sending Packet Train
	for (int i = 0; i < sizeof(packet_train)/sizeof(packet_train[0]); i++) {
		sendto(sockfd, (const char *)packet_train[i], strlen(packet_train[i]), 
			MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
				sizeof(servaddr)); 
	}

	// Message to send to server to stop recording time
	char end_msg[100] = "End LOW UDP Train";
	// Ensures the server gets the message. 
	while (strcmp(start_msg, buffer) == 0) {
		printf("Running END\n");
		sendto(sockfd, (const char *)end_msg, strlen(end_msg), 
			MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
				sizeof(servaddr)); 
		rcvd_msg = recvfrom(sockfd, (char *)buffer, MAXLINE,
							MSG_DONTWAIT, ( struct sockaddr *)&servaddr,
							&len);
	}

	// Close socket when done
    close(sockfd); 
}

void post_probe_cli() {
	int sockfd; 
	struct sockaddr_in servaddr; 

	// Create TCP socket and make sure it was created successfully
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
		perror("socket creation failed..."); 
        exit(EXIT_FAILURE); 
	}
	
	// Allow the socket to reuse a socket in use (needed for consecutive runs)
	int on = IP_PMTUDISC_DO;
	int sso_return = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	bzero(&servaddr, sizeof(servaddr)); 

	// Assign IP, PORT for server information
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = inet_addr(DST_IP); 
	servaddr.sin_port = htons(TCP_PORT); 

	// Keep attempting a connection to the server until one is achieved.
    while (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
        perror("connection with the server failed..."); 
	}

	// Buffer to receive message from server on its findings
    char buff[MAX]; 
	bzero(buff, sizeof(buff)); 
	read(sockfd, buff, sizeof(buff)); 
	printf("Server Findings: %s\n", buff); 
  
    // Close the socket when done
    close(sockfd); 
}

int main() { 
	pre_probe_cli();
	probe_cli();
	post_probe_cli();
	return 0;
}
