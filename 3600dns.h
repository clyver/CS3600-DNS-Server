/*
 * CS3600, Spring 2014
 * Project 2 Starter Code
 * (c) 2013 Alan Mislove
 *
 */

#ifndef __3600DNS_H__
#define __3600DNS_H__


#endif

// We define all the components of a DNS Packet

// The header
typedef struct header_s{
	short ID:16;

	int RD:1;
 	int TC:1;	
	int AA:1;
	int OPCODE:4;
	int qr:1;

	int RCODE:4;
	int Z:3;
	int RA:1;

	short QDCOUNT:16;
	short ANCOUNT:16;
	short NSCOUNT:16;
	short ARCOUNT:16;
}header;
	
// The question
typedef struct question_s{
	short QNAME:16;
	short QTYPES;
	short QCLASS;
}question;

// The answer
typedef struct answer_s{
	int NAME;
	int TYPE;
	int CLASS;
	int TTL;
	int RDLENGTH;
	int RDATA;
}answer;
