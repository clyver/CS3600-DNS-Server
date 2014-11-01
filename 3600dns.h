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
	int ID;
	int FLAGS;
	int QR;
	int OPCODE;
	int AA;
	int TC;
	int RD;
	int RA;
	int Z;
	unsigned int QDCOUNT;
	unsigned int ANCOUNT;
	unsigned int NSCOUNT;
	unsigned int ARCOUNT;
}header;
	
// The question
typedef struct question_s{
	int QNAME;
	int QTYPES;
	int QCLASS;
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
