// Server side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <sys/time.h>
  
#define PORT    8765
#define TCP_PORT1 8080
#define TCP_PORT2 8081
#define MAXLINE 6000 
#define MAX 100
#define SA struct sockaddr 

void func(int sockfd) 
{ 
    char buff[MAX]; 
    int n; 
    // infinite loop for chat 
    for (;;) { 
        bzero(buff, MAX); 
  
        // read the message from client and copy it in buffer 
        read(sockfd, buff, sizeof(buff)); 
        // print buffer which contains the client contents 
        printf("From client: %s\t To client : ", buff); 
        bzero(buff, MAX); 
        n = 0; 
        // copy server message in the buffer 
        while ((buff[n++] = getchar()) != '\n') 
            ; 
  
        // and send that buffer to client 
        write(sockfd, buff, sizeof(buff)); 
  
        // if msg contains "Exit" then server exit and chat ended. 
        if (strncmp("exit", buff, 4) == 0) { 
            printf("Server Exit...\n"); 
            break; 
        } 
    } 
} 

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
	
	fclose(fp);
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

long getMsTime() {
	struct timeval curr_time;
	gettimeofday(&curr_time, NULL);
	long ms = curr_time.tv_sec * 1000 + curr_time.tv_usec / 1000;
	return ms;
}

void pre_probe_server() {
	int sockfd, connfd, len; 				// create socket file descriptor 
	struct sockaddr_in servaddr, cli; 		// create structure object of sockaddr_in for client and server

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 			// creating a TCP socket ( SOCK_STREAM )
	
	if (sockfd == -1) { 
		printf("socket creation failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully created..\n"); 
	
	// empty the 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET;					// specifies address family with IPv4 Protocol 
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 	// binds to any address
	servaddr.sin_port = htons(TCP_PORT1); 				// binds to PORT specified

	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
		printf("socket bind failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully binded..\n"); 

	// Now server is ready to listen and verification 
	if ((listen(sockfd, 5)) != 0) { 
		printf("Listen failed...\n"); 
		exit(0); 
	} 
	else
		printf("Server listening..\n"); 
	
	len = sizeof(cli); 

	// Accept the data packet from client and verification 
	connfd = accept(sockfd, (SA*)&cli, &len); 	// accepts connection from socket
	
	if (connfd < 0) { 
		printf("server acccept failed...\n"); 
		exit(0); 
	} 
	else
		printf("server acccept the client...\n"); 

	// Function for chatting between client and server 
	recvFile(connfd); 

	// After transfer close the socket 
	int return_value = close(sockfd); 
	printf("return:value:%d\n", return_value);

	return;
}

char* probe_serv() {
    int udp_sockfd, udp_len, udp_rcvd; 
    char buffer[MAXLINE]; 
    char *hello = "Hello from server"; 
    struct sockaddr_in servaddr, cliaddr; 

    memset(&servaddr, 0, sizeof(servaddr)); 

    // Filling server information 
    servaddr.sin_family = AF_INET; // IPv4 
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
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
  
	long low_start;
	long low_end;
	int done = 0;
	int start_confirmed = 0;

	// Get start message
	char* start_msg = "Start LOW UDP Train";
	char* end_msg = "End LOW UDP Train";
	while (start_confirmed == 0) {
		udp_rcvd = recvfrom(udp_sockfd, (char *)buffer, MAXLINE,  
					MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
					&udp_len); 
		buffer[udp_rcvd] = '\0';
		printf("%s\n", buffer);
		printf("%s\n", start_msg);
		if (strcmp(buffer, start_msg) == 0) {
			start_confirmed = 1;

			sendto(udp_sockfd, start_msg, strlen(start_msg),
					MSG_CONFIRM, (const struct sockaddr *) &cliaddr,
					sizeof(cliaddr));
		}
	}

	low_start = getMsTime();
	while (strcmp((char *)buffer, end_msg) != 0) {
		udp_rcvd = recvfrom(udp_sockfd, (char *)buffer, MAXLINE,  
					MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
					&udp_len); 

		buffer[udp_rcvd] = '\0'; 
	}
	low_end = getMsTime();
	printf("Time Start: %ld and Time End: %ld\n", low_start, low_end);
	printf("Difference: %ld\n", low_end - low_start);
    int close_return = close(udp_sockfd); 
	printf("close_return: %d\n", close_return);
	return "Blicky";
}

void post_probe_serv(char* tested) {
    int sockfd, connfd, len; 
    struct sockaddr_in servaddr, cli; 
  
    // socket create and verification 
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
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        printf("socket bind failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully binded..\n"); 
  
    // Now server is ready to listen and verification 
    if ((listen(sockfd, 5)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    } 
    else
        printf("Server listening..\n"); 
    len = sizeof(cli); 
  
    // Accept the data packet from client and verification 
    connfd = accept(sockfd, (SA*)&cli, &len); 
    if (connfd < 0) { 
        printf("server acccept failed...\n"); 
        exit(0); 
    } 
    else
        printf("server acccept the client...\n"); 
  
    // Function for chatting between client and server 
    func(connfd); 
  
    // After chatting close the socket 
    close(sockfd); 
}
  
// Driver code 
int main() { 
	pre_probe_server();
	char* returneddd = probe_serv();
	post_probe_serv(returneddd);
    return 0; 
}
