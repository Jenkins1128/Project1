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

/* Nodes that will be used as settings for linked list */
struct config {
	char key[100];
	char value[100];
};

/* Function used to create a linked list of config settings */
struct config* create_config(int* count, char* filename) {
	FILE *fPtr = fopen(filename, "r");
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

/* Handles parsing the config file and inserting key-value pairs into linked list */
void populate_config(struct config* settings, char* filename) {
	int i = 0;

	FILE *fPtr = fopen(filename, "r");
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

/* Gets value of any setting from linked list */
char* get_value(struct config* settings, char* key_name, int count) {
	for (int i = 0; i < count; i++) {
		if (strcmp((settings + i)->key, key_name) == 0) {
			return (settings + i)->value;
		}
	}
	return "NaN";
}

/* Function used to receive file */
char* recvFile(int sockfd) { 
	// Buffer used to receive file 
	char buff[MAX]; 
	
	// Opening file that that will hold received file
	FILE *fp;

	char* filename = "myconfig.json";
	
	// Ensure the file 
	if((fp = fopen(filename, "w")) == NULL) {
		perror("Error IN Opening File");
		exit(EXIT_FAILURE);
	}
	
	// Read the file and write it to file
	while(read(sockfd, buff, MAX) > 0) {
		fprintf(fp, "%s", buff);
	}

	
	// Close file pointer after writing to the file
	fclose(fp);
	return filename;
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
struct config* pre_probe_server(int tcp_port, int *count) {
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
	char* filename = recvFile(connfd); 
	struct config* config_settings = create_config(count, filename);

	if (config_settings == NULL) {
		perror("Error creating config settings");
		exit(EXIT_FAILURE);
	}

	populate_config(config_settings, filename);

	// After receiving file, close the socket 
	close(connfd);
	close(sockfd); 

	return config_settings;
}

/* Function for probing phase of server 
 * Returns compression results found
*/
int probe_serv(int port, int packet_size, int packet_train_length, int imt) {
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
    if (bind(sockfd, (SA*)&servaddr, sizeof(servaddr)) < 0) { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
   
	// Size of cli struct
    len = sizeof(cliaddr);  
  
	// Variables to hold times of start and end of packet trains
	long low_start;
	long low_end;
	long high_start;
	long high_end;
	int packet_length = packet_size / sizeof(uint16_t);

	// Payload data structures
	uint16_t *high_entrophy_packet_train = (uint16_t *)malloc(packet_train_length * packet_length * sizeof(uint16_t));
	uint16_t *low_entrophy_packet_train = (uint16_t *)malloc(packet_train_length * packet_length * sizeof(uint16_t));

	int packet_id = 0;

	// Receive the first packet and wait for it.
	rcvd = recvfrom(sockfd, (low_entrophy_packet_train + (packet_id * packet_length)), sizeof((low_entrophy_packet_train + (packet_id * packet_length))),
				MSG_WAITALL, (SA*) &cliaddr, 
				&len); 
	packet_id++;
	// After getting the first packet, get the time
	low_start = getMsTime();
	// Update the end time whenever a new packet is received
	low_end = getMsTime();

	// While the most recent packet received wasn't more than 15s ago, keep attempting to receive packets
	// keep attempting to receive
	while ((getMsTime() - low_end) < ((imt * 1000) - 250)) {
		rcvd = recvfrom(sockfd, (low_entrophy_packet_train + (packet_id * packet_length)), sizeof((low_entrophy_packet_train + (packet_id * packet_length))),
					MSG_DONTWAIT, (SA*) &cliaddr, 
					&len); 
		if (rcvd > 0) {
			// Only update the packet_id and update last packet received time
			low_end = getMsTime();
			packet_id++;
		}
	}

	/* High Packet Train */
	packet_id = 0;
	// Receive the first packet and wait for it.
	rcvd = recvfrom(sockfd, (low_entrophy_packet_train + (packet_id * packet_length)), sizeof((low_entrophy_packet_train + (packet_id * packet_length))),
				MSG_DONTWAIT, (SA*) &cliaddr, 
				&len); 
	packet_id++;
	// After getting the first packet, get the time
	high_start = getMsTime();
	// Update the end time whenever a new packet is received
	high_end = getMsTime();
	// While the most recent packet received wasn't more than 15s ago, keep attempting to receive packets
	// keep attempting to receive
	while ((getMsTime() - high_end) < (imt * 1000) - 250) {
		rcvd = recvfrom(sockfd, (low_entrophy_packet_train + (packet_id * packet_length)), sizeof((low_entrophy_packet_train + (packet_id * packet_length))),
					MSG_DONTWAIT, (SA*) &cliaddr, 
					&len); 
		if (rcvd > 0) {
			// Only update the packet_id and update last packet received time
			high_end  = getMsTime();
			packet_id++;
		}
	}

	// Close socket once done
    close(sockfd); 

	// Calculate elapsed time for high and low entrophy data
	int low_entrophy_time = low_end - low_start;
	int high_entrophy_time = high_end - high_start;

	// Free after use
	free(high_entrophy_packet_train);
	free(low_entrophy_packet_train);

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
int main(int argc, char** argv) { 
	/* Config settings */
	int settings_count = 0;
	struct config* config_settings = create_config(&settings_count, argv[1]);

	if (config_settings == NULL) {
		perror("Error creating config settings");
		return EXIT_FAILURE;
	}

	populate_config(config_settings, argv[1]);

	int client_settings_count = 0;
	struct config* client_config_settings = pre_probe_server(	atoi(get_value(config_settings, "tcp_prepost_port", settings_count)),
																&client_settings_count);
	sleep(1.5);
	int compression_result = probe_serv(atoi(get_value(client_config_settings, "udp_dest_port", client_settings_count)),
										atoi(get_value(client_config_settings, "udp_payload_size", client_settings_count)),
										atoi(get_value(client_config_settings, "packet_train_length", client_settings_count)),
										atoi(get_value(client_config_settings, "imt", client_settings_count)));
	sleep(1.5);
	post_probe_serv(compression_result, atoi(get_value(client_config_settings, "tcp_prepost_port", client_settings_count)));
	free(client_config_settings);
	return 0;
}
