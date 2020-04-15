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
#include <fcntl.h>

#define MAXLINE  6000	
#define MAX 	 80
  
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
void sentFile(int sockfd, char* filename) { 
	// Buffer used to receive file 
	char buff[MAX];
	
	// Create file 
	FILE *fp;

	// Open file and ensure it opens
	if((fp=fopen(filename, "r")) == NULL) {
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
void pre_probe_cli(int tcp_port, char* dst_ip, char* filename) {
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
	servaddr.sin_addr.s_addr = inet_addr(dst_ip); 
	servaddr.sin_port = htons(tcp_port); 

	// Connect client socket to server socket 
	if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
		perror("connection with the server failed..."); 
        exit(EXIT_FAILURE); 
	} 

	// Send config file
	sentFile(sockfd, filename); 

	// Close the socket 
	close(sockfd); 
}

void probe_cli(int src_port, int dst_port, char* dst_ip, int packet_size, int packet_train_length, int imt) {
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
    servaddr.sin_port = htons(dst_port); 
    servaddr.sin_addr.s_addr = inet_addr(dst_ip); 

	// Assign IP, PORT for client information (needed for making the right port)
	memset(&cliaddr, 0, sizeof(cliaddr));
    cliaddr.sin_family = AF_INET; 
    cliaddr.sin_port = htons(src_port); 
    cliaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	// Needed to make sure it shows right source port
	if (bind(sockfd, (SA*) &cliaddr, sizeof(cliaddr)) < 0) {
		perror("bind error");
		exit(EXIT_FAILURE);
	}

	// Needed for random seed
	srand(time(0));	


	int packet_length = packet_size / sizeof(short);
	/*
	char **high_entrophy_packet_train = malloc(packet_train_length * sizeof(char*));
	char **low_entrophy_packet_train = malloc(packet_train_length * sizeof(char*));
	unsigned short id_num = 0;
	for (int i = 0; i < packet_train_length; i++) {
		// Create ID
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
		++id_num;
	}
	*/

	printf("Hit\n");
	printf("Hit\n");

	short *high_entrophy_packet_train = (short *)malloc(packet_train_length * packet_length * sizeof(short));
	short *low_entrophy_packet_train = (short *)malloc(packet_train_length * packet_length * sizeof(short));

	unsigned short id_num = 0;
	int myFile = open("/dev/urandom", O_RDONLY);            
	for (int i = 0; i < packet_train_length; i++) {
		for (int j = 0; j < packet_length; j++) {
			if (j == 0) {
				*(low_entrophy_packet_train + (i * packet_length) + j) = id_num;
				*(high_entrophy_packet_train + (i * packet_length) + j) = id_num;
			} else {
				unsigned short rand;            
				read(myFile, &rand, sizeof(rand)) ;
				*(high_entrophy_packet_train + (i * packet_length) + j) = rand;
			}
		}
		++id_num;
	}
	printf("MadeIt\n");

	// Send message to server to indicate next packet will be start of Low Entrophy
	// packet train. Let's server know to start timer
	// Ensures the server gets the message. 
	/*
	char* start_msg = malloc(256);
	strcpy(start_msg, "Start LOW UDP Train");
	int len = sizeof(servaddr);
	while (strcmp(start_msg, buffer) != 0) {
		sendto(sockfd, start_msg, strlen(start_msg), 
				MSG_CONFIRM, (const SA*) &servaddr,  
				sizeof(servaddr)); 
		rcvd_msg = recvfrom(sockfd, (char *)buffer, MAXLINE,
							MSG_DONTWAIT, ( struct sockaddr *)&servaddr,
							&len);
		buffer[rcvd_msg] = '\0';
	}
	*/

	int array[100];
	// Sending Low Entrophy Packet Train
	for (int i = 0; i < packet_train_length; i++) {
		sendto(sockfd, (low_entrophy_packet_train + (i * packet_length)), sizeof((low_entrophy_packet_train + (i * packet_length))), 
		// sendto(sockfd, array, sizeof(array), 
				MSG_CONFIRM, (const SA*) &servaddr,  
				sizeof(servaddr)); 
	}

	/*
	// Message to send to server to stop recording time
	char* end_msg = malloc(256); 
	strcpy(end_msg, "End LOW UDP Train");
	// Ensures the server gets the message. 
	sendto(sockfd, end_msg, strlen(end_msg), 
			MSG_CONFIRM, (const SA*) &servaddr,  
			sizeof(servaddr)); 

	*/
	// Sleep for 15 seconds inbetween trains
	sleep(imt);
	
	/*
	// Prepare to send High UDP Train
	strcpy(start_msg, "Start HIGH UDP Train");
	len = sizeof(servaddr);
	while (strcmp(start_msg, buffer) != 0) {
		sendto(sockfd, start_msg, strlen(start_msg), 
				MSG_CONFIRM, (const SA*) &servaddr,  
				sizeof(servaddr)); 
		rcvd_msg = recvfrom(sockfd, (char *)buffer, MAXLINE,
							MSG_DONTWAIT, (SA*)&servaddr,
							&len);
		buffer[rcvd_msg] = '\0';
	}
	*/

	// Sending High Entrophy Packet Train
	for (int i = 0; i < packet_train_length; i++) {
		sendto(sockfd, (high_entrophy_packet_train + (i * packet_length)), sizeof((high_entrophy_packet_train + (i * packet_length))), 
				MSG_CONFIRM, (const SA*) &servaddr,  
				sizeof(servaddr)); 
	}

	/*
	// Message to send to server to stop recording time
	strcpy(end_msg, "End HIGH UDP Train");
	// Ensures the server gets the message. 
	sendto(sockfd, end_msg, strlen(end_msg), 
			MSG_CONFIRM, (const SA*) &servaddr,  
			sizeof(servaddr)); 
	*/

	// Close socket when done
    close(sockfd); 
}

void post_probe_cli(int tcp_port, char* dst_ip) {
	int sockfd; 
	struct sockaddr_in servaddr; 

	// Create TCP socket and make sure it was created successfully
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
	servaddr.sin_addr.s_addr = inet_addr(dst_ip); 
	servaddr.sin_port = htons(tcp_port); 

	// Keep attempting a connection to the server until one is achieved.
    while (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
	}

	// Buffer to receive message from server on its findings
    char buff[MAX]; 
	bzero(buff, sizeof(buff)); 
	read(sockfd, buff, sizeof(buff));
	printf("Server Findings: %s\n", buff); 
	
    // Close the socket when done
    close(sockfd); 
}

int main(int argc, char** argv) { 
	/* Config settings */
	int settings_count = 0;
	struct config* config_settings = create_config(&settings_count, argv[1]);

	if (config_settings == NULL) {
		perror("Error creating config settings");
		return EXIT_FAILURE;
	}

	populate_config(config_settings, argv[1]);

	pre_probe_cli(	atoi(get_value(config_settings, "tcp_prepost_port", settings_count)),
					get_value(config_settings, "p1_server_ip", settings_count),
					argv[1]);
	probe_cli(	atoi(get_value(config_settings, "udp_source_port", settings_count)), 
				atoi(get_value(config_settings, "udp_dest_port", settings_count)),
				get_value(config_settings, "p1_server_ip", settings_count),
				atoi(get_value(config_settings, "udp_payload_size", settings_count)),
				atoi(get_value(config_settings, "packet_train_length", settings_count)),
				atoi(get_value(config_settings, "imt", settings_count)));
	post_probe_cli(	atoi(get_value(config_settings, "tcp_prepost_port", settings_count)),
					get_value(config_settings, "p1_server_ip", settings_count));
	return 0;
}
