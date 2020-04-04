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
#define MAX 	 800
  
#define SLEEP_DURATION 5
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
		perror("Error IN Opening File .. ");
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

	// Empty the server struct before using
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

	// Send config file
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


	int packet_train_length = 6000;
	int packet_length = 1001;
	char **high_entrophy_packet_train = malloc(packet_train_length * sizeof(char*));
	char **low_entrophy_packet_train = malloc(packet_train_length * sizeof(char*));
	for (int i = 0; i < packet_train_length; i++) {
		// Create ID
		int id_num = i + 1;
		char hold_num[5];
		sprintf(hold_num, "%04d", id_num);
		
		high_entrophy_packet_train[i] = malloc(packet_length * sizeof(char));
		low_entrophy_packet_train[i] = malloc(packet_length * sizeof(char));
		for (int j = 0; j < packet_length - 1; j++) {
			if (j < 4) {
				high_entrophy_packet_train[i][j] = hold_num[j];
				low_entrophy_packet_train[i][j] = hold_num[j];
			} else {
				int rand_num = rand() % packet_length;
				char hold_rand_num[2];
				sprintf(hold_rand_num, "%d", rand_num);
				high_entrophy_packet_train[i][j] = hold_rand_num[0]; // All rand nums
				low_entrophy_packet_train[i][j] = '0';
			}
		}

		high_entrophy_packet_train[i][packet_length - 1] = '\0';
		low_entrophy_packet_train[i][packet_length - 1] = '\0';
	}

	// Send message to server to indicate next packet will be start of Low Entrophy
	// packet train. Let's server know to start timer
	// Ensures the server gets the message. 
	char* start_msg = malloc(256);
	strcpy(start_msg, "Start LOW UDP Train");
	int len = sizeof(servaddr);
	while (strcmp(start_msg, buffer) != 0) {
		sendto(sockfd, start_msg, strlen(start_msg), 
				MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
				sizeof(servaddr)); 
		rcvd_msg = recvfrom(sockfd, (char *)buffer, MAXLINE,
							MSG_DONTWAIT, ( struct sockaddr *)&servaddr,
							&len);
	}

	// Sending Low Entrophy Packet Train
	for (int i = 0; i < packet_train_length; i++) {
		sendto(sockfd, (const char *)low_entrophy_packet_train[i], strlen(low_entrophy_packet_train[i]), 
				MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
				sizeof(servaddr)); 
	}

	// Message to send to server to stop recording time
	char* end_msg = malloc(256); 
	strcpy(end_msg, "End LOW UDP Train");
	// Ensures the server gets the message. 
	sendto(sockfd, end_msg, strlen(end_msg), 
			MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
			sizeof(servaddr)); 

	// Sleep for 15 seconds inbetween trains
	sleep(SLEEP_DURATION);
	
	// Prepare to send High UDP Train
	strcpy(start_msg, "Start HIGH UDP Train");
	len = sizeof(servaddr);
	while (strcmp(start_msg, buffer) != 0) {
		sendto(sockfd, start_msg, strlen(start_msg), 
				MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
				sizeof(servaddr)); 
		rcvd_msg = recvfrom(sockfd, (char *)buffer, MAXLINE,
							MSG_DONTWAIT, ( struct sockaddr *)&servaddr,
							&len);
	}

	// Sending High Entrophy Packet Train
	for (int i = 0; i < packet_train_length; i++) {
		sendto(sockfd, (const char *)high_entrophy_packet_train[i], strlen(high_entrophy_packet_train[i]), 
				MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
				sizeof(servaddr)); 
	}

	// Message to send to server to stop recording time
	strcpy(end_msg, "End HIGH UDP Train");
	// Ensures the server gets the message. 
	sendto(sockfd, end_msg, strlen(end_msg), 
			MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
			sizeof(servaddr)); 

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
        // perror("connection with the server failed..."); 
	}

	// Buffer to receive message from server on its findings
    char buff[MAX]; 
	bzero(buff, MAX); 
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
