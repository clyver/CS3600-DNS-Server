/*
 * CS3600, Spring 2014
 * Project 3 Starter Code
 * (c) 2013 Alan Mislove
 *
 */

#include <math.h>
#include <ctype.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "3600dns.h"

/**
 * This function will print a hex dump of the provided packet to the screen
 * to help facilitate debugging.  In your milestone and final submission, you 
 * MUST call dump_packet() with your packet right before calling sendto().  
 * You're welcome to use it at other times to help debug, but please comment those
 * out in your submissions.
 *
 * DO NOT MODIFY THIS FUNCTION
 *
 * data - The pointer to your packet buffer
 * size - The length of your packet
 */

static void dump_packet(unsigned char *data, int size) {
    unsigned char *p = data;
    unsigned char c;
    int n;
    char bytestr[4] = {0};
    char addrstr[10] = {0};
    char hexstr[ 16*3 + 5] = {0};
    char charstr[16*1 + 5] = {0};
    for(n=1;n<=size;n++) {
        if (n%16 == 1) {
            /* store address for this line */
            snprintf(addrstr, sizeof(addrstr), "%.4x",
               ((unsigned int)p-(unsigned int)data) );
        }
            
        c = *p;
        if (isprint(c) == 0) {
            c = '.';
        }

        /* store hex str (for left side) */
        snprintf(bytestr, sizeof(bytestr), "%02X ", *p);
        strncat(hexstr, bytestr, sizeof(hexstr)-strlen(hexstr)-1);

        /* store char str (for right side) */
        snprintf(bytestr, sizeof(bytestr), "%c", c);
        strncat(charstr, bytestr, sizeof(charstr)-strlen(charstr)-1);

        if(n%16 == 0) { 
            /* line completed */
            printf("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
            hexstr[0] = 0;
            charstr[0] = 0;
        } else if(n%8 == 0) {
            /* half line: add whitespaces */
            strncat(hexstr, "  ", sizeof(hexstr)-strlen(hexstr)-1);
            strncat(charstr, " ", sizeof(charstr)-strlen(charstr)-1);
        }
        p++; /* next byte */
    }

    if (strlen(hexstr) > 0) {
        /* print rest of buffer if not empty */
        printf("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
    }
}

// Our headers are uniform, let's have a helper we can rely on to provide them
header get_header() {
	//We start by constructing all the needed fields

        // As per instructions, id is always 1337
        short id = htons(1337);
        // For a question, we say qr is 0.
        int qr = 0;
        // Standard query, so we say opcode is 0
        int opcode = 0;
        // Authoritative Answer only matters in responses
        int aa = 0;
        // Truncation, matters only in responses
        int tc = 0;
        // Recursion Desired. We want the servre to pursue recursively
        int rd = 1;
        // Recursion Available.  Only matters for responses
        int ra = 0;
        // Reserved for future use.  Must be 0.
        int z = 0;
        // Response Code.  Only matters for responses 
        int rcode = 0;
        // QDCOUNT, the number of entries in this in this question section
        // We have 1 question, so 1
        unsigned int qdcount = htons(1);
        // ANCOUNT, the number of resources records in the answer section
        // We provide no answers, so 0
        unsigned int ancount = 0;
        // NSCOUNT, num of resource records in the authority resources section
        unsigned int nscount = 0;
        // ARCOUNT, num of resource records in the additional records section
        unsigned int arcount = 0;

	 // Pack it all up into a header struct
        header this_header = {id, rd, tc, aa, opcode, qr, rcode,
				 z, ra, qdcount, ancount, nscount, arcount};
	return this_header;
}

//Given this name, return the qname we want to ask
char* qname(char *name) {
	// We have to parse the name to get the length and content of each sect

	//Approach: keep count and accumulate a string until '.'
	
	// The index place in the args we generate
	int index = 0;
	char *args = (char *) malloc(sizeof(name) + 64);
	for (char *token = strtok(name, "."); token != NULL; token = strtok(NULL, ".")) {
		int x = strlen(token);
		char tmp[2];
		tmp[0] = x;
		tmp[1] = '\0';
		memcpy(args + index, tmp, 1);
		index++;
		memcpy(args + index, token, x);
		index = index + x;
	}

	char tmp[2];
	tmp[0] = 0;
	tmp[1] = '\0';
	memcpy(args + index, tmp, 1);	
	return args;

}

// Add a byte onto a string
int append_char(char **string, int *size, char c) {
	// Check for bad input
	if (string == NULL || size == NULL) {
		fprintf(stderr, "Can't append char. Got null.");
		return -1;
	}	
	// Update the size of this string
	*size = *size + sizeof(c);
	realloc( *string, *size);
	*string[*size] = c;
}


int main(int argc, char *argv[]) {
	/**
   	* I've included some basic code for opening a socket in C, sending
   	* a UDP packet, and then receiving a response (or timeout).  You'll 
   	* need to fill in many of the details, but this should be enough to
   	* get you started.
   	*/

	// print out the input
	printf("There are %d args \n", argc);
  	for (int i = 0; i < argc; i++) {
		printf("argv %d: %s \n", i, argv[i]);	
	}

  	// process the arguments
	// TODO: We may have a third argument: 'port'
	char *server = argv[1];
	char *name = argv[2];

	// Pack it all up into a header struct
	header my_header = get_header();

	// Begin to craft our question
	// Get our qname
	char *my_qname = qname(name);

	// Get our qclass and qtype
	char tmp[2];
	tmp[0] = 1;
	tmp[1] = '\0';
	char qclass[1];
	memcpy(qclass, tmp, 1); 
	char *qtype = qclass;
 	
	int header_size = sizeof(my_header);
	int qname_size = sizeof(qname) + 2;
	int qtype_size = sizeof(qtype);
	int qclass_size = sizeof(qclass);

	int packet_index = 0;
	// Construct our packet
	int packet_size = header_size  + qname_size + qclass_size + qtype_size;
	char *packet = (char *) malloc(packet_size);

	memcpy(packet, &my_header, header_size);

        char the_qname[sizeof(name)+2];	
	memcpy(the_qname, qname, qname_size + 2);

	//memcpy(packet + packet_index, qtype, qtype_size);
	//packet_index = packet_index + qtype_size;

	//memcpy(packet + packet_index, qclass, qclass_size);
	
 	dump_packet(&the_qname, 8);
	
	/*
	// send the DNS request (and call dump_packet with your request)
 	 
 	 // first, open a UDP socket  
  	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  	// next, construct the destination address
  	struct sockaddr_in out;
  	out.sin_family = AF_INET;
  	out.sin_port = htons(<<DNS server port number, as short>>);
  	out.sin_addr.s_addr = inet_addr(<<DNS server IP as char*>>);

    	// an error occurred
  	}

  	// wait for the DNS reply (timeout: 5 seconds)
  	struct sockaddr_in in;
  	socklen_t in_len;

  	// construct the socket set
  	fd_set socks;
  	FD_ZERO(&socks);
  	FD_SET(sock, &socks);

  	// construct the timeout
  	struct timeval t;
  	t.tv_sec = <<your timeout in seconds>>;
  	t.tv_usec = 0;

  	// wait to receive, or for a timeout
  	if (select(sock + 1, &socks, NULL, NULL, &t)) {
    		if (recvfrom(sock, <<your input buffer>>, <<input len>>, 0, &in, &in_len) < 0) {
      		// an error occured
    		}	
  	} else {
    		// a timeout occurred
  	}
	
  	// print out the result
  	*/
  	return 0;
}
