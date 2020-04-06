#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <netdb.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <sys/time.h>
  
#define MAXLINE   6000 
#define MAX 80
#define SA struct sockaddr 

struct config {
	char key[100];
	char value[100];
};

struct config* create_config(int* count) {
	FILE *fPtr = fopen("myconfig.json", "r");
	int bufferSize = 1000;
	char buffer[bufferSize];
	int totalRead = 0;

	if (fPtr == NULL) {
		perror("Unable to open config file:");
		exit(EXIT_FAILURE);
	}

	while (fgets(buffer, bufferSize, fPtr) != NULL) {
		totalRead = strlen(buffer);

		if (buffer[totalRead - 1] == '\n') {
			buffer[totalRead - 1] = '\0';
		} else {
			buffer[totalRead - 1] = buffer[totalRead - 1];
		}

		(*count)++;
	}
	fclose(fPtr);
	struct config* config_settings = malloc((*count) * sizeof(struct config));
	return config_settings;
}

void populate_config(struct config* settings) {
	int i = 0;

	FILE *fPtr = fopen("myconfig.json", "r");
	int bufferSize = 1000;
	char buffer[bufferSize];
	char key[100];
	char value[100];
	int totalRead = 0;

	if (fPtr == NULL) {
		perror("Unable to open config file:");
		exit(EXIT_FAILURE);
	}

	while (fgets(buffer, bufferSize, fPtr) != NULL) {
		totalRead = strlen(buffer);

		if (buffer[totalRead - 1] == '\n') {
			buffer[totalRead - 1] = '\0';
		} else {
			buffer[totalRead - 1] = buffer[totalRead - 1];
		}

		char delim[3] = ":";
		strcpy(((struct config*)settings + i)->key, strtok(buffer, delim));
		strcpy(((struct config*)settings + i)->value, strtok(NULL, delim));
		++i;
	}
	fclose(fPtr);
	return;
}

char* get_value(struct config* settings, char* key_name, int count) {
	for (int i = 0; i < count; i++) {
		if (strcmp((settings + i)->key, key_name) == 0) {
			return (settings + i)->value;
		}
	}
	return "NaN";
}

/* Function used to receive file */
void recvFile(int sockfd) { 
	// Buffer used to receive file 
	char buff[MAX]; 
	
	// Opening file that that will hold received file
	FILE *fp;
	
	// Ensure the file 
	if((fp = fopen("received.c", "w")) == NULL) {
		perror("Error IN Opening File");
		exit(EXIT_FAILURE);
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
		exit(EXIT_FAILURE);
	}
	
	// Read the contents of the file into the buffer and send it over socket
	while (fgets(buff, MAX, fp) != NULL) {
		write(sockfd, buff, sizeof(buff));
	}
	
	// Close file pointer after writing to the file
	fclose(fp);					
} 

/* Function to get current time in ms */
long getMsTime() {
	struct timeval curr_time;
	gettimeofday(&curr_time, NULL);
	long ms = curr_time.tv_sec * 1000 + curr_time.tv_usec / 1000;
	return ms;
}

/* Function for pre probing phase */
void pre_probe_server(int tcp_port) {
	int sockfd, connfd, len;
	struct sockaddr_in servaddr, cli;

	// Create TCP socket and make sure it was created successfully
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
		perror("socket creation failed..."); 
        exit(EXIT_FAILURE); 
	}

	// Set socket option to resuse port in case it's already been used
	// Needed for when running program back to back
	int on = IP_PMTUDISC_DO;
	int sso_return = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	// Empty the server struct before using
	bzero(&servaddr, sizeof(servaddr)); 

	// Assign IP, PORT for server information
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(tcp_port); 

	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
		perror("socket bind failed...\n"); 
		exit(EXIT_FAILURE); 
	} 

	// Now server is ready to listen and verification 
	if ((listen(sockfd, 5)) != 0) { 
		perror("Listen failed...\n"); 
		exit(EXIT_FAILURE); 
	} 
	
	// Get size of cli struct
	len = sizeof(cli); 

	// Attempt connection
	if ((connfd = accept(sockfd, (SA*)&cli, &len)) < 0) { 
		perror("server acccept failed...\n"); 
		exit(EXIT_FAILURE); 
	}

	// Receive the file client is attempting to send
	recvFile(connfd); 

	// After receiving file, close the socket 
	close(sockfd); 
}

/* Function for probing phase of server 
 * Returns compression results found
*/
int probe_serv(int port) {
    int sockfd, len, rcvd; 
    char buffer[MAXLINE]; 
    struct sockaddr_in servaddr, cliaddr; 

	// Zero out server struct
    bzero(&servaddr, sizeof(servaddr)); 

    // Filling server information 
    servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(port); 

	// Attempt to create socket 
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 

	// Set socket option to reuse a port that may be in use
	int on = IP_PMTUDISC_DO;
	int sso_return = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
      
    // Bind the socket with the server address 
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
   
	// Size of cli struct
    len = sizeof(cliaddr);  
  
	// Variables to hold times of start and end of packet trains
	long low_start;
	long low_end;

	// Set start and end messages for low entrophy trains
	char* start_msg = malloc(256);
	strcpy(start_msg, "Start LOW UDP Train");
	char* end_msg = malloc(256);
	strcpy(end_msg, "End LOW UDP Train");

	// Wait for server to receive start_msg from client, then continue
	while (strcmp(buffer, start_msg) != 0) {
		rcvd = recvfrom(sockfd, (char *)buffer, MAXLINE,  
					MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
					&len); 
		buffer[rcvd] = '\0';
	}
	
	// Send msg to client to let it know to start sending packet train 
	sendto(sockfd, start_msg, strlen(start_msg),
			MSG_CONFIRM, (const struct sockaddr *) &cliaddr,
			sizeof(cliaddr));

	// Get start time
	low_start = getMsTime();

	// Keep receiving packets until end_msg is received
	while (strcmp(buffer, end_msg) != 0) {
		rcvd = recvfrom(sockfd, (char *)buffer, MAXLINE,  
					MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
					&len); 
		buffer[rcvd] = '\0';
	}

	// Get end time
	low_end = getMsTime();


	// Variables for high entrophy start/end time
	long high_start;
	long high_end;
	// Set start and end msg for high entrophy train
	strcpy(start_msg, "Start HIGH UDP Train");
	strcpy(end_msg, "End HIGH UDP Train");

	// Wait for client to send start msg 
	// to let server know packet train coming next
	while (strcmp(buffer, start_msg) != 0) {
		rcvd = recvfrom(sockfd, (char *)buffer, MAXLINE,  
					MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
					&len); 
		buffer[rcvd] = '\0';
	}

	// Send message back to client to let it know it is ready
	sendto(sockfd, start_msg, strlen(start_msg),
			MSG_CONFIRM, (const struct sockaddr *) &cliaddr,
			sizeof(cliaddr));

	// Get start time
	high_start = getMsTime();

	// Keep receiving packets until end_msg is received
	while (strcmp(buffer, end_msg) != 0) {
		rcvd = recvfrom(sockfd, (char *)buffer, MAXLINE,  
					MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
					&len); 
		buffer[rcvd] = '\0';
	}
	
	// Get end time for high entrophy time
	high_end = getMsTime();

	// Close socket once done
    close(sockfd); 

	// Calculate elapsed time for high and low entrophy data
	int low_entrophy_time = low_end - low_start;
	int high_entrophy_time = high_end - high_start;

	return (high_entrophy_time - low_entrophy_time);
}

/* Post probing phase for client */
void post_probe_serv(int compression_result, int tcp_port) {
	int sockfd, connfd, len;
	struct sockaddr_in servaddr, cli;

	// Create TCP socket and make sure it was created successfully
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
		perror("socket creation failed..."); 
        exit(EXIT_FAILURE); 
	}

	// Set socket option to resuse port in case it's already been used
	// Needed for when running program back to back
	int on = IP_PMTUDISC_DO;
	int sso_return = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	// Empty the server struct before using
	bzero(&servaddr, sizeof(servaddr)); 

	// Assign IP, PORT for server information
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(tcp_port); 

	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
		perror("socket bind failed...\n"); 
		exit(EXIT_FAILURE); 
	} 

	// Now server is ready to listen and verification 
	if ((listen(sockfd, 5)) != 0) { 
		perror("Listen failed...\n"); 
		exit(EXIT_FAILURE); 
	} 
	
	// Get size of cli struct
	len = sizeof(cli); 

	// Attempt connection
	if ((connfd = accept(sockfd, (SA*)&cli, &len)) < 0) { 
		perror("server acccept failed...\n"); 
		exit(EXIT_FAILURE); 
	}
  
    // Send message to client
	// Return proper message to send back to client
	if (compression_result > 100) {
		char buff[MAX] = "Compression detected!";
		write(connfd, buff, sizeof(buff)); 
	} else {
		char buff[MAX] = "No compression was detected.";
		write(connfd, buff, sizeof(buff)); 
	}
  
    // Close sockets when done
	close(connfd);
    close(sockfd); 
}
  
// Driver code 
int main() { 
	/* Config settings */
	int settings_count = 0;
	struct config* config_settings = create_config(&settings_count);

	if (config_settings == NULL) {
		perror("Error creating config settings");
		return EXIT_FAILURE;
	}

	populate_config(config_settings);
	pre_probe_server(atoi(get_value(config_settings, "tcp_prepost_port", settings_count)));
	int compression_result = probe_serv(atoi(get_value(config_settings, "udp_dest_port", settings_count)));
	post_probe_serv(compression_result, atoi(get_value(config_settings, "tcp_prepost_port", settings_count)));
	return 0;
}
