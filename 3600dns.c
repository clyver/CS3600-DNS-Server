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
char * get_header() {
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
	
	// Tack it into a string
	int header_size = 1024;
	char *my_header[header_size];
	memcpy(my_header, &this_header, header_size);
	return my_header;
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

// Given a name (label/data), return an english name
char * crunch_name(char *buff, int i) {
	char *name = NULL;
	int name_len = 0;
	short x = buff[i];
	
	while(buff[i]) {
		x = buff[i];
		if ( (x & 192) == 192) {
			x = x & 0x00111111;
			x = x << 8;
			x = x | buff[i+1];
			i = x;
			continue;
		}
		i++; // get not the label but the data
		name = realloc(name, name_len + x);
		while(x) {
			name[name_len] = buff[i];
			name_len++;
			i++;
			x = x - 1;
		}
		// after putting on each word, we want to put an the '.'
		name[name_len] = '.';
		name_len++;
	}
	// after we are done, we want to null terminate and get tid of last '.'
	name[name_len-1] = '\0';
	return name;
}	

// Qname and qtype are identical, one call can return either
char * get_qname_type() {
	char tmp[2];
	tmp[0] = 1;
	tmp[1] = '\0';
	return tmp;
}

short char_to_short(char *str_char) {
	unsigned short short_port = (unsigned short) strtoul(str_char, NULL, 0);
	return short_port;
}


// Given the 'server:port' argument, see if there
// is a specified port (optional), else 53
short get_port(char *input) {
	// Get the pointer to the colon in the string, else null
	char *port = strstr(input, ":");
	short x;
	if (port) {
		// We want to exclude ":" so ++ to get the number
		port++;
		x = char_to_short(port);
		x = htons(x);
		return x;
	}
	else {
		short x = 53;
		x = htons(x);
		return x;
	}
}

char * get_server(char *input) {
	if (input[0] != '@') {
		return "";		
	} else {
                input++;
                int server_len = 0;
                int i;
                for (i = 0; i < strlen(input); i++) {
                        if (input[i] == ':') {
                                 break;
                        }
                }
                server_len = i;
                char *fluffy = (char *) malloc(server_len);
                strncpy(fluffy, input, server_len);
                
                fluffy[server_len] = '\0';
                return fluffy;
	}
}

// Get the value of a field in the packet that takes up two indexes.
// Many of our field fall into this category, so be able to extract them
short fetch_field(char *buff, int i) {
	short f1 = buff[i];
	short f2 = buff[i+1];
	
	short field = f1 << 8;
	field = field | f2;

	return field;
}

// Retrieve the rcode from a packet
short get_rcode(char *buff) {
	short rcode = buff[3] & 0x0f;
	return rcode;	
}

// Get the number of answers from this packet's header
short get_num_answers(char *buff) {
	return fetch_field(buff, 6);
}

// Walk over a name piece of data and return the beginning of the next piece
int walk_over_name(char * buff, int i) {
        int x = buff[i];
        if ( (x & 192) == 192) {
                // If x is a pointer, we're done as well
                i += 2;
                return i;
        }
        if (!x) {
                // If x is zero, we're done
                i++;
                return i;
        }
        // Else this is a normal label, go the next
        i = i + x + 1;
        return walk_over_name(buff, i);
}

// Get the auth bit in the header
char * get_auth(char *buff) {
	
	short x = buff[2];
	short auth = x & 0x04;
	if(auth == 4) {
		return "auth";
	} else {
		return "nonauth";
	}
	
}

char * get_answer_type(int i) {
	if( i == 5) {
		return "CNAME";
	}
	if( i == 1) {
		return "IP";
	}
	if ( i == 0x002) {
		return "NS";
	}
	if ( i == 0x00f) {
		return "MX";
	}
	else {
		return "ERROR\tBAD ATYPE RETURN!";
	}
}

int main(int argc, char *argv[]) {
	/**
   	* I've included some basic code for opening a socket in C, sending
   	* a UDP packet, and then receiving a response (or timeout).  You'll 
   	* need to fill in many of the details, but this should be enough to
   	* get you started.
   	*/

	// Initialize the command line arguments
	char *extra = NULL;
	char *name = NULL;
	char *server = NULL;

	// Populate the arguments, if there is the optional argument, we handle it
	if ( (strcmp(argv[1], "-mx") == 0) || (strcmp(argv[1],  "-ns") ==0) ) {
		// Extra credit scenario 
		extra = argv[1];
		server = argv[2];
		name = argv[3];
	} else {
		// Normal scenario
		server = argv[1];
		name = argv[2];
	}

	// CAll get_header to return the string of a header struct
	char *my_header = get_header();

	// Begin to craft our question
	// Get our qname
	char *my_qname = qname(name);
 	
	// Set up our size
	int header_size = 12;
	int qname_size = strlen(my_qname) + 1;
	int qtype_size = 2;
	int qclass_size = 2;

	// Construct our packet
	int packet_index = 0;
	int packet_size = header_size  + qname_size + qtype_size + qclass_size;
	char *packet = (char *) malloc(packet_size);

	// Append the header onto to the packet
	memcpy(packet, my_header, header_size);
	packet_index = packet_index + header_size;

	// Append qname onto the packet
        memcpy(packet + packet_index, my_qname, qname_size);
	packet_index = packet_index + qname_size;

	// If this is an ma or ns request we change the type
	if (extra) {
		if (strcmp(extra, "-mx") == 0) {
			packet[packet_index++] = 0;
			packet[packet_index++] = 0x0f;
		}
		if (strcmp(extra, "-ns") == 0) {
			packet[packet_index++] = 0;
			packet[packet_index++] = 0x02;
		}
	} else {
		// Append qtype onto the packet
		packet[packet_index++] = 0;	
		packet[packet_index++] = 1;
	}

	// Append qclass onto the size
        packet[packet_index++] = 0;
        packet[packet_index++] = 1;	
	
	dump_packet(packet, packet_size);
	
 		
	// send the DNS request (and call dump_packet with your request)
 	 
 	// first, open a UDP socket  
  	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  	// next, construct the destination address
  	struct sockaddr_in out;
  	out.sin_family = AF_INET;

	// Get the ip address of the server we're pinging
 	char *real_server = get_server(server);	
	if ( strcmp(real_server, "") == 0) {
		printf("The input server is not in a valid form\n");
		return -1;
	}
	
	out.sin_port = get_port(server);
  	out.sin_addr.s_addr = inet_addr(real_server);

	if (sendto(sock, packet, packet_size, 0, &out, sizeof(out)) < 0) {
    		// an error occurred
		printf("Much bad. So error. @250");
		return -1;
 	}

  	// wait for the DNS reply (timeout: 5 seconds)
  	struct sockaddr_in in;
  	socklen_t in_len = sizeof(in);

  	// construct the socket set
  	fd_set socks;
  	FD_ZERO(&socks);
  	FD_SET(sock, &socks);

  	// construct the timeout
  	struct timeval t;
	// We want to hardcode in 5 seconds i think.
  	t.tv_sec = 5;
  	t.tv_usec = 0;

	int buff_len = 65536;
	char buff[buff_len];
  	// wait to receive, or for a timeout
  	if (select(sock + 1, &socks, NULL, NULL, &t)) {
    		if ( recvfrom(sock, buff, buff_len, 0, &in, &in_len) < 0) {
      			// an error occured
			perror("Recvfrom");
			return -1;
    		}	
  	} else {
    		// a timeout occurred
		printf("NORESPONSE\n");		
		return -1;
  	}
	short header_id = fetch_field(buff, 0);
	// Error if the header we receive does not match the header we sent
	if (header_id != 1337) {
		printf ("Headers do not match.\n");
		return -1;
	}

	short rcode = get_rcode(buff);
	// Rcode is zero if there is no error. If there is error, which one?
	if (rcode) {
		if (rcode == 1) {
			printf("Format error \n");
			return -1;
		}
		if (rcode == 2) {
			printf("Server failure \n");
			return -1;
		}
		if (rcode == 3) {
			printf("NOTFOUND\n");
			return -1;
		}
		if (rcode == 4) {
			printf("Not Implemented \n");
			return -1;
		}

		if (rcode == 5) {
			printf("Not Implemented \n");
			return -1;
		}
		else {
			printf("Unknown error in rcode");
			return -1;
		}



	}	
	//short num_answers = get_num_answers(buff);
	int question_name_end = walk_over_name(buff, 12);
	short qtype = fetch_field(buff, question_name_end);
	short qclass = fetch_field(buff, question_name_end + 2);
	
	// We increment +4 to get to the beginning of answer
	int answer = question_name_end + 4;

 	
	int num_answers = get_num_answers(buff);

	// We have found the first answer
	// For all answers, return either the cname or ip adress with auth
	for(int a = 0; a < num_answers; a++) { 
		// Get this answer's data
		int answer_name_end = walk_over_name(buff, answer);
		short atype = fetch_field(buff, answer_name_end);
		short aclass = fetch_field(buff, answer_name_end + 2);
		short ardata_len = fetch_field(buff, answer_name_end + 8);
		
		char *type = get_answer_type(atype);
		

		int rdata = answer_name_end + 10;
		char *auth = get_auth(buff);
		
		if (type == "IP") {
		   // If this in an IP address, shoot it out
		   unsigned char tmp[4];
                   tmp[0] = buff[rdata];
                   tmp[1] = buff[rdata+1];
                   tmp[2] = buff[rdata+2];
                   tmp[3] = buff[rdata+3];
		   printf("%s\t%d.%d.%d.%d\t%s\n",type, tmp[0], tmp[1], tmp[2], tmp[3], auth);
		} 
		if (type == "CNAME") {
		   // If it's a cname, we have to pull out the name from rdata and shoot it out
		   char *name = crunch_name(buff, rdata);
		   printf("%s\t%s\t%s\n", type, name, auth);
		}
		if (type == "MX") {
		   // We handle the mail requests
		   short pref = fetch_field(buff, rdata);
		   char *name = crunch_name(buff, rdata+2);
		   printf("%s\t%s\t%d\t%s\n", type, name, pref, auth);
		   // If it's a mail server request we handle it
		   
		}
		if (type == "NS") {
	    	  // Finally, we handle ns
		  char *name = crunch_name(buff, rdata);
		  printf("%s\t%s\t%s\n", type, name, auth);
		}
		answer = answer_name_end + 10 + ardata_len;
	}

	return 0;
}
