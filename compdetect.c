#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <net/ethernet.h>
#include <netinet/ether.h>
#include <netdb.h>  
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/ioctl.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <bits/ioctls.h>
#include <net/if.h>  
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>  
#include <time.h> 

 /* IP Header struct  */
#define PCKT_LEN 8192
#define DATA "testchecksum"

struct ipheader 
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned int ihl:4;
    unsigned int version:4;
#elif __BYTE_ORDER == __BIG_ENDIAN
    unsigned int version:4;
    unsigned int ihl:4;
#else
# error "Please fix <bits/endian.h>"
#endif
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
};

/* TCP Header struct */
struct tcpheader 
{
  unsigned short int  source;
  unsigned short int  dest;
  unsigned int seq;
  unsigned int ack_seq;
  unsigned int res1:4,
    doff:4,
    fin:1,
    syn:1,
    rst:1,
    psh:1,
    ack:1,
    urg:1,
    ece:1,
    cwr:1;
  unsigned short int  window;
  unsigned short int check;
  unsigned short int  urg_ptr;
};

/* TCP packet struct packet needed for calculating the TCP header checksum */
struct checksumTCPPacket 
{
  uint32_t srcAddr;
  uint32_t dstAddr;
  uint8_t zero;
  uint8_t protocol;
  uint16_t TCP_len;
};

/* Config file */
struct config {
	char key[100];
	char value[100];
};

/* Checksum function */
unsigned short csum(unsigned short *buf, int len)
{
        unsigned short oddbyte;
        unsigned long sum;
        for(sum=0; len>0; len-=2){
        	sum += *buf++;
        }

        if(len==1) {
          oddbyte=0;
          *((u_char*)&oddbyte) = *(u_char*)buf;
          sum+=oddbyte;
        }

        sum = (sum >> 16) + (sum &0xffff);
        sum += (sum >> 16);
        return (unsigned short)(~sum);
}




/* UDP CONFIG */

#define IP4_HDRLEN 20         // IPv4 header length
#define UDP_HDRLEN  8         // UDP header length, excludes data
#define NUM_UDP_PACKETS 4000
#define PAYLOAD_LEN 100

uint16_t checksum (uint16_t *addr, int len);
uint16_t udp4_checksum (struct ip, struct udphdr, uint8_t *, int);
char *allocate_strmem (int);
uint8_t *allocate_ustrmem (int);
int *allocate_intmem (int);
 
/* TIMER CONFIG */
#define THRESHOLD 0.1 //100ms

struct config* create_config(int* count, char* config_file) {
	FILE *fPtr = fopen(config_file, "r");
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

void populate_config(struct config* settings, char* config_file) {
	int i = 0;

	FILE *fPtr = fopen(config_file, "r");
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

int main(int argc, char *argv[])
{
	/* Check parameters */
	if(argc != 2)
	{
		printf("- Invalid parameters!!!\n");
		printf("- Usage: %s myconfig.json\n", argv[0]);
		exit(-1);
	}

	/* Config settings */
	int settings_count = 0;
	char* config_file = argv[1];
	struct config* config_settings = create_config(&settings_count, config_file);

	if (config_settings == NULL) {
		perror("Error creating config settings");
		return EXIT_FAILURE;
	}

	populate_config(config_settings, config_file);

	char *source_ip = get_value(config_settings, "source_ip", settings_count);
	char *server_ip = get_value(config_settings, "server_ip", settings_count);
	int tcp_source_port = atoi(get_value(config_settings, "tcp_source_port", settings_count));
	int udp_source_port = atoi(get_value(config_settings, "udp_source_port", settings_count));
	int udp_dest_port = atoi(get_value(config_settings, "udp_dest_port", settings_count));
	int tcp_dest_head_port = atoi(get_value(config_settings, "tcp_dest_head_port", settings_count));
	int tcp_dest_tail_port = atoi(get_value(config_settings, "tcp_dest_tail_port", settings_count));
	int udp_payload_size = atoi(get_value(config_settings, "udp_payload_size", settings_count));
	int packet_train_length = atoi(get_value(config_settings, "packet_train_length", settings_count));
	int udp_ttl = atoi(get_value(config_settings, "udp_ttl", settings_count));

	/*  ---- HEAD SYN PACKET ---- */

	/* socket */
	int sd;
	/* Data to be appended at the end of the tcp header */
	char *data;
	/* Packet buffer */
	char buffer[PCKT_LEN];
	/* checksum TCP header to calculate the TCP header's checksum */
	struct checksumTCPPacket checksumPacket;
	/* checksum TCP Header */
	char *checksum_packet;
	/* Size of the headers */
	struct ipheader *ip = (struct ipheader *) buffer;
	struct tcpheader *tcp = (struct tcpheader *) (buffer + sizeof(struct ipheader));
	data = (char *) (buffer + sizeof(struct ipheader) + sizeof(struct tcpheader));
	/* Copy data to data */
	strcpy(data, DATA);
	/* Declare sender and destination sockets and setting buffer size 8 */
	struct sockaddr_in sin, din;
	int one = 1;
	const int *val = &one;
	memset(buffer, 0, sizeof(buffer));
	/* Creating socket */
	sd = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
	if(sd < 0)
	{
	   perror("socket() error");
	   exit(-1);
	}
	else
	{
	  printf("socket()-SOCK_RAW and tcp protocol is OK.\n");
	}
	/* Setting socket options, telling kernal we have custom headers*/
	if(setsockopt(sd, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) < 0)
	{
	   perror("setsockopt() error");
	   exit(-1);
	}else{
	  printf("setsockopt() is OK\n");
	}
	/* Address family */
	din.sin_family = AF_INET;
	din.sin_port = htons(tcp_dest_head_port);
	din.sin_addr.s_addr = inet_addr(server_ip);
	/* Setting structure */
	ip->ihl = 5;
	ip->version = 4;
	ip->tos = 0;
	ip->tot_len = sizeof(struct ipheader) + sizeof(struct tcpheader) + strlen(data);
	ip->id = htons(54321);
	ip->frag_off = 0;
	ip->ttl = 64;
	ip->protocol = IPPROTO_TCP; 
	ip->check = 0; 
	ip->saddr = inet_addr(source_ip);
	ip->daddr = inet_addr(server_ip);
	ip->check = csum((unsigned short *) buffer, ip->tot_len);
	/* Setting TCP structure */
	tcp->source = htons(tcp_source_port);
	tcp->dest = htons(tcp_dest_head_port);
	tcp->seq = 0;
	tcp->ack_seq = 0;
	tcp->doff = 5;
	tcp->cwr = 0; 
	tcp->ece = 0; 
	tcp->psh = 0;
	tcp->rst = 0;
	tcp->syn = 1;
	tcp->ack = 0;
	tcp->fin = 0;
	tcp->window = htons(32767);
	tcp->check = 0; 
	tcp->urg_ptr = 0;
	/* Calculating the checksum for the TCP header */
	checksumPacket.srcAddr = inet_addr(source_ip); 
	checksumPacket.dstAddr = inet_addr(server_ip); 
	checksumPacket.zero = 0;
	checksumPacket.protocol = IPPROTO_TCP; 
	checksumPacket.TCP_len = htons(sizeof(struct tcpheader) + strlen(data));
	/* Populate the checksum packet */
	checksum_packet = (char *) malloc((int) (sizeof(struct checksumTCPPacket) + sizeof(struct tcpheader) ));
	memset(checksum_packet, 0, sizeof(struct checksumTCPPacket) + sizeof(struct tcpheader) + strlen(data));
	/* Copy checksum header */
	memcpy(checksum_packet, (char *) &checksumPacket, sizeof(struct checksumTCPPacket));
	/* update checksum */
	tcp->check = 0; 
	/* Copy tcp header + data to  TCP header for checksum */
	memcpy(checksum_packet + sizeof(struct checksumTCPPacket), tcp, sizeof(struct tcpheader));
	/* Set the TCP header's check field */
	tcp->check = (csum((unsigned short *) checksum_packet, (int) (sizeof(struct checksumTCPPacket) + 
	      sizeof(struct tcpheader) + strlen(data))));

	/* Calculate the time */
	clock_t t; 
	t = clock(); 

	if(sendto(sd, buffer, ip->tot_len, 0, (struct sockaddr *)&din, sizeof(din)) < 0)
	{
	   perror("sendto() error");
	   exit(-1);
	}else{
		sleep(.5);
	}

	/* ----  UDP PACKET TRAIN  ---- */

	int i, status, datalen, frame_length, udp_sd, bytes, *ip_flags;
	char *interface, *target, *src_ip, *dst_ip;
	struct ip iphdr;
	struct udphdr udphdr;
	uint8_t *udp_data, *src_mac, *dst_mac, *ether_frame;
	struct addrinfo hints, *res;
	struct sockaddr_in *ipv4;
	struct sockaddr_ll device;
	struct ifreq ifr;
	void *tmp;

	/* Allocate memory for various arrays. */
	src_mac = allocate_ustrmem (6);
	dst_mac = allocate_ustrmem (6);
	udp_data = allocate_ustrmem (IP_MAXPACKET);
	ether_frame = allocate_ustrmem (IP_MAXPACKET);
	interface = allocate_strmem (40);
	target = allocate_strmem (40);
	src_ip = allocate_strmem (INET_ADDRSTRLEN);
	dst_ip = allocate_strmem (INET_ADDRSTRLEN);
	ip_flags = allocate_intmem (7);
	/* Interface to send packet through. */
	strcpy (interface, "ens33");
	/* Setting udp socket. */
	if ((udp_sd = socket (AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		perror ("socket() failed to get socket descriptor for using ioctl() ");
		exit (EXIT_FAILURE);
	}
	/* Using ioctl() to look up interface name and get its MAC address. */
	memset (&ifr, 0, sizeof (ifr));
	snprintf (ifr.ifr_name, sizeof (ifr.ifr_name), "%s", interface);
	if (ioctl (udp_sd, SIOCGIFHWADDR, &ifr) < 0) {
		perror ("ioctl() failed to get source MAC address ");
		return (EXIT_FAILURE);
	}
	/* Copy source MAC address. */
	memcpy (src_mac, ifr.ifr_hwaddr.sa_data, 6);
	/* Find interface index from interface name and store index in */
	memset (&device, 0, sizeof (device));
	if ((device.sll_ifindex = if_nametoindex (interface)) == 0) {
		perror ("if_nametoindex() failed to obtain interface index ");
		exit (EXIT_FAILURE);
	}
	/* Source IPv4 address */
	strcpy (src_ip, source_ip);
	/* Destination IPv4 address */
	strcpy (target, server_ip);
	/* Hints for getaddrinfo() */
	memset (&hints, 0, sizeof (struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = hints.ai_flags | AI_CANONNAME;
	/* Resolve target using getaddrinfo(). */
	if ((status = getaddrinfo (target, NULL, &hints, &res)) != 0) {
		fprintf (stderr, "getaddrinfo() failed: %s\n", gai_strerror (status));
		exit (EXIT_FAILURE);
	}
	ipv4 = (struct sockaddr_in *) res->ai_addr;
	tmp = &(ipv4->sin_addr);
	if (inet_ntop (AF_INET, tmp, dst_ip, INET_ADDRSTRLEN) == NULL) {
		status = errno;
		fprintf (stderr, "inet_ntop() failed.\nError message: %s", strerror (status));
		exit (EXIT_FAILURE);
	}
	freeaddrinfo (res);
	/* Fill out sockaddr_ll. */
	device.sll_family = AF_PACKET;
	device.sll_protocol = htons (ETH_P_IP);
	memcpy (device.sll_addr, ether_ntoa((struct ether_addr*)dst_mac), 6);
	device.sll_halen = 6;
	/* UDP data */
	datalen = udp_payload_size;
	udp_data[0] = 'J';
	udp_data[1] = 'e';
	udp_data[2] = 'n';
	udp_data[3] = 'k';
	udp_data[4] = 'i';
	udp_data[5] = 'n';
	udp_data[6] = 's';

	// IPv4 header
	iphdr.ip_hl = IP4_HDRLEN / sizeof (uint32_t);
	iphdr.ip_v = 4;
	iphdr.ip_tos = 16;
	iphdr.ip_len = htons (IP4_HDRLEN + UDP_HDRLEN + datalen);
	iphdr.ip_id = htons (0);
	ip_flags[0] = 0;
	ip_flags[1] = 0;
	ip_flags[2] = 0;
	ip_flags[3] = 0;
	iphdr.ip_off = htons ((ip_flags[0] << 15)
	                  + (ip_flags[1] << 14)
	                  + (ip_flags[2] << 13)
	                  +  ip_flags[3]);
	iphdr.ip_ttl = udp_ttl;
	iphdr.ip_p = IPPROTO_UDP;

	/* Source IPv4 address */
	if ((status = inet_pton (AF_INET, src_ip, &(iphdr.ip_src))) != 1) {
		fprintf (stderr, "inet_pton() failed.\nError message: %s", strerror (status));
		exit (EXIT_FAILURE);
	}

	/* Destination IPv4 address (32 bits) */
	if ((status = inet_pton (AF_INET, dst_ip, &(iphdr.ip_dst))) != 1) {
		fprintf (stderr, "inet_pton() failed.\nError message: %s", strerror (status));
		exit (EXIT_FAILURE);
	}

	/* IPv4 header checksum, set to 0 when calculating checksum */
	iphdr.ip_sum = 0;
	iphdr.ip_sum = checksum ((uint16_t *) &iphdr, IP4_HDRLEN);

	/* UDP header */
	udphdr.source = htons (udp_source_port);
	udphdr.dest = htons (udp_dest_port);
	udphdr.len = htons (UDP_HDRLEN + datalen);
	udphdr.check = udp4_checksum (iphdr, udphdr, udp_data, datalen);
	/* Ethernet frame header */
	frame_length = IP4_HDRLEN + UDP_HDRLEN + datalen;
	/* IPv4 header */
	memcpy (ether_frame, &iphdr, IP4_HDRLEN);
	/* UDP header */
	memcpy (ether_frame + IP4_HDRLEN, &udphdr, UDP_HDRLEN);
	/* UDP data */
	memcpy (ether_frame + IP4_HDRLEN + UDP_HDRLEN, udp_data, datalen);

	/* Set socket for UDP */
	if ((udp_sd = socket (PF_PACKET, SOCK_DGRAM, htons (ETH_P_ALL))) < 0) {
		perror ("socket() failed ");
		exit (EXIT_FAILURE);
	}

	/* Send Packet train */
	for (int i = 0; i < NUM_UDP_PACKETS; i++) {
		// Send ethernet frame to socket.
		if ((bytes = sendto (udp_sd, ether_frame, frame_length, 0, (struct sockaddr *) &device, sizeof (device))) <= 0) {
		  perror ("sendto() failed");
		  exit (EXIT_FAILURE);
		}
	}

	close (udp_sd);

	/* Free allocated memory. */
	free (src_mac);
	free (dst_mac);
	free (udp_data);
	free (ether_frame);
	free (interface);
	free (target);
	free (src_ip);
	free (dst_ip);
	free (ip_flags);

	/*  ---- TAIL SYN PACKET ---- */

	/* Changing destitation port for tail syn packet */
	din.sin_family = AF_INET;
	din.sin_port = htons(tcp_dest_tail_port);
	din.sin_addr.s_addr = inet_addr(server_ip);
	tcp->dest = htons(tcp_dest_tail_port);
	/* Calculating the checksum for the TCP header */
	checksumPacket.srcAddr = inet_addr(source_ip); 
	checksumPacket.dstAddr = inet_addr(server_ip);
	checksumPacket.zero = 0; 
	checksumPacket.protocol = IPPROTO_TCP;
	checksumPacket.TCP_len = htons(sizeof(struct tcpheader) + strlen(data)); 
	/* Populate the checksum packet */
	checksum_packet = (char *) malloc((int) (sizeof(struct checksumTCPPacket) + sizeof(struct tcpheader) ));
	memset(checksum_packet, 0, sizeof(struct checksumTCPPacket) + sizeof(struct tcpheader) + strlen(data));
	/* Copy checksum header */
	memcpy(checksum_packet, (char *) &checksumPacket, sizeof(struct checksumTCPPacket));
	/* Update checksum */
	tcp->check = 0; 
	/* Copy tcp header + data to fake TCP header for checksum */
	memcpy(checksum_packet + sizeof(struct checksumTCPPacket), tcp, sizeof(struct tcpheader));
	/* Set the TCP header's check field */
	tcp->check = (csum((unsigned short *) checksum_packet, (int) (sizeof(struct checksumTCPPacket) + 
	      sizeof(struct tcpheader) + strlen(data))));

	/* Sending Tail SYN Packet */
	if(sendto(sd, buffer, ip->tot_len, 0, (struct sockaddr *)&din, sizeof(din)) < 0)
	{
	   exit(-1);
	}
	/* Calculating time */
	t = clock() - t; 
	double time_taken = ((double)t)/CLOCKS_PER_SEC; //seconds 
	/* Checking whether compression was detected */
	if(time_taken > THRESHOLD){
	   printf("\nTime: %f Compression Detected! \n", time_taken); 
	}else{
	   printf("\nTime: %f No compression was detected. \n", time_taken);  
	}
	 
	close(sd);
	return (EXIT_SUCCESS);
}

/* Computing the internet checksum (RFC 1071).
 * Note that the internet checksum does not preclude collisions. */
uint16_t
checksum (uint16_t *addr, int len)
{
  int count = len;
  register uint32_t sum = 0;
  uint16_t answer = 0;

  // Sum up 2-byte values until none or only one byte left.
  while (count > 1) {
    sum += *(addr++);
    count -= 2;
  }

  // Add left-over byte, if any.
  if (count > 0) {
    sum += *(uint8_t *) addr;
  }

  // Fold 32-bit sum into 16 bits; we lose information by doing this,
  // increasing the chances of a collision.
  // sum = (lower 16 bits) + (upper 16 bits shifted right 16 bits)
  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }

  // Checksum is one's compliment of sum.
  answer = ~sum;
  return (answer);
}

/* Build IPv4 UDP pseudo-header and call checksum function. */
uint16_t
udp4_checksum (struct ip iphdr, struct udphdr udphdr, uint8_t *payload, int payloadlen)
{
  char buf[IP_MAXPACKET];
  char *ptr;
  int chksumlen = 0;
  int i;

  ptr = &buf[0];  // ptr points to beginning of buffer buf

  // Copy source IP address into buf (32 bits)
  memcpy (ptr, &iphdr.ip_src.s_addr, sizeof (iphdr.ip_src.s_addr));
  ptr += sizeof (iphdr.ip_src.s_addr);
  chksumlen += sizeof (iphdr.ip_src.s_addr);

  // Copy destination IP address into buf (32 bits)
  memcpy (ptr, &iphdr.ip_dst.s_addr, sizeof (iphdr.ip_dst.s_addr));
  ptr += sizeof (iphdr.ip_dst.s_addr);
  chksumlen += sizeof (iphdr.ip_dst.s_addr);

  // Copy zero field to buf (8 bits)
  *ptr = 0; ptr++;
  chksumlen += 1;

  // Copy transport layer protocol to buf (8 bits)
  memcpy (ptr, &iphdr.ip_p, sizeof (iphdr.ip_p));
  ptr += sizeof (iphdr.ip_p);
  chksumlen += sizeof (iphdr.ip_p);

  // Copy UDP length to buf (16 bits)
  memcpy (ptr, &udphdr.len, sizeof (udphdr.len));
  ptr += sizeof (udphdr.len);
  chksumlen += sizeof (udphdr.len);

  // Copy UDP source port to buf (16 bits)
  memcpy (ptr, &udphdr.source, sizeof (udphdr.source));
  ptr += sizeof (udphdr.source);
  chksumlen += sizeof (udphdr.source);

  // Copy UDP destination port to buf (16 bits)
  memcpy (ptr, &udphdr.dest, sizeof (udphdr.dest));
  ptr += sizeof (udphdr.dest);
  chksumlen += sizeof (udphdr.dest);

  // Copy UDP length again to buf (16 bits)
  memcpy (ptr, &udphdr.len, sizeof (udphdr.len));
  ptr += sizeof (udphdr.len);
  chksumlen += sizeof (udphdr.len);

  // Copy UDP checksum to buf (16 bits)
  // Zero, since we don't know it yet
  *ptr = 0; ptr++;
  *ptr = 0; ptr++;
  chksumlen += 2;

  // Copy payload to buf
  memcpy (ptr, payload, payloadlen);
  ptr += payloadlen;
  chksumlen += payloadlen;

  // Pad to the next 16-bit boundary
  for (i=0; i<payloadlen%2; i++, ptr++) {
    *ptr = 0;
    ptr++;
    chksumlen++;
  }

  return checksum ((uint16_t *) buf, chksumlen);
}

/* Allocate memory for an array of chars. */
char *
allocate_strmem (int len)
{
  void *tmp;

  if (len <= 0) {
    fprintf (stderr, "ERROR: Cannot allocate memory because len = %i in allocate_strmem().\n", len);
    exit (EXIT_FAILURE);
  }

  tmp = (char *) malloc (len * sizeof (char));
  if (tmp != NULL) {
    memset (tmp, 0, len * sizeof (char));
    return (tmp);
  } else {
    fprintf (stderr, "ERROR: Cannot allocate memory for array allocate_strmem().\n");
    exit (EXIT_FAILURE);
  }
}

/* Allocate memory for an array of unsigned chars. */
uint8_t *
allocate_ustrmem (int len)
{
  void *tmp;

  if (len <= 0) {
    fprintf (stderr, "ERROR: Cannot allocate memory because len = %i in allocate_ustrmem().\n", len);
    exit (EXIT_FAILURE);
  }

  tmp = (uint8_t *) malloc (len * sizeof (uint8_t));
  if (tmp != NULL) {
    memset (tmp, 0, len * sizeof (uint8_t));
    return (tmp);
  } else {
    fprintf (stderr, "ERROR: Cannot allocate memory for array allocate_ustrmem().\n");
    exit (EXIT_FAILURE);
  }
}

/* Allocate memory for an array of ints. */
int *
allocate_intmem (int len)
{
  void *tmp;

  if (len <= 0) {
    fprintf (stderr, "ERROR: Cannot allocate memory because len = %i in allocate_intmem().\n", len);
    exit (EXIT_FAILURE);
  }

  tmp = (int *) malloc (len * sizeof (int));
  if (tmp != NULL) {
    memset (tmp, 0, len * sizeof (int));
    return (tmp);
  } else {
    fprintf (stderr, "ERROR: Cannot allocate memory for array allocate_intmem().\n");
    exit (EXIT_FAILURE);
  }
}


