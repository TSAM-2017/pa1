/**
 * a simple tftp server that operates on port given as argument
 * and serves files in directory given as seccond parameter
 *
 * for security reasons it will not support file uppload
 * and only handle files with ASCII compliant names
*/
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "types_and_constants.h"
#include "file_util.c"

// Constants for TFTP Packets
const unsigned short RRQ = 1;
const unsigned short WRQ = 2;
const unsigned short DATA = 3;
const unsigned short ACK = 4;
const unsigned short ERROR = 5;

/**
 * find tftp packet op code
 * @param  packet [description]
 * @return
 */
int get_op_code(char* packet){
	return (int)packet[1];
}

tftp_mode get_mode(char* packet){
	char mode[512];
	memset(mode, '\0', sizeof(mode));
	int n = snprintf(mode, 100, "%s", packet+2);
	n = snprintf(mode, 100, "%s", packet+3+n);

	for (int i = 0; i < n; i++) {
		mode[i] = tolower(mode[i]);
	}

	if(strcmp(mode, MODE_NET)){
		return NET;
	}else if(strcmp(mode, MODE_OCTET)){
		return OCTET;
	}else if(strcmp(mode, MODE_MAIL)){
		return MAIL;
	}

	return n;
}

/**
 * get file name from packet and places it in buffer
 * @param  buff   [description]
 * @param  packet [description]
 * @return
 */
int get_filename(char* buffer, char* packet){
	int n = snprintf(buffer, 100, "%s", packet+2);
	if (n < 0) {
        fprintf(stderr, "hit an error reading packet\n");
        return -1;
    }
	return n;
}

/**
 * craft data packet with information recieved and place it in buffer
 * @param buffer  a buffer of at least len PACKET_SIZE
 * @param blocknr sequential number of the packet (note: must be unique as per protocol per transmission)
 * @param data    data packed in 8-bit bytes with a max of BLOCK_SIZE bytes
 */
int make_data_packet(unsigned char* buffer, short blocknr, char* data, int size){
	memset(buffer, 0, PACKET_SIZE+1);
	sprintf((char*)buffer, "%c%c%c%c", 0x00, 0x03, 0x00, blocknr);
	memcpy((char*)buffer +4, data, size);
	return 0;
}

/**
 * send file to client
 * @param file   pointer to file that is to be sent
 * @param mode   read mode for the file
 * @param client [description]
 */
void send_file(FILE* file, tftp_mode mode, struct sockaddr_in client, int sockfd){
	int ACKs = 0; // num acked data packets
	int count = 0; // num packets sent
	char filebuffer[BLOCK_SIZE];
	char recbuffer[PACKET_SIZE];
	socklen_t rec_size;
	memset(filebuffer, '\0', sizeof(filebuffer));
	memset(recbuffer, '\0', sizeof(recbuffer));
	while (1) {
		// get current block
		count++;
		int size = read_block_from_file(file, count, filebuffer);

		// create packet
		unsigned char packetbuffer[PACKET_SIZE];
		make_data_packet(packetbuffer, count, filebuffer, size);
		int packetLen = size+4;

		// attempt to send
		if(sendto(sockfd, packetbuffer, packetLen, 0, (struct sockaddr *) &client, sizeof(client)) != packetLen){
			fprintf(stderr, "failed to send packet\n");
		}

		// recieve respone
		memset(recbuffer, '\0', sizeof(recbuffer));
		recvfrom(sockfd, recbuffer, PACKET_SIZE, 0, (struct sockaddr *) &client, &rec_size);
		if (recbuffer[1] == 4) {
			/* ack */
			fprintf(stdout, "packet number %d acked\n", count);
			ACKs++;
		}

		// check if sending has finished
		if (size != BLOCK_SIZE) {
			// last message is either short or zero length so break when that happens
			fprintf(stdout, "last packet sent\n");
			break;
		}
	}
}

int main(int argc, char** argv){
	// validate arguments
	// mandatory to have 3: name of program, the port and directory
	if(argc != 3){
		fprintf(stderr, "expected usage: %s <port> <dir>\n", argv[0] );
		exit(0);
	}
	// The arguments
	int port_number = atoi(argv[1]);
	char* dir_name = argv[2];
	if(dir_name[strlen(dir_name)-1] == '/'){ dir_name[strlen(dir_name)-1] = '\0';}

	// Socket file descriptor
	int sockfd;
    struct sockaddr_in server, client;
    char message[512];

    // Create and bind a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	// Initialize the server
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;

    /* Network functions need arguments in network byte order instead
     * of host byte order. The macros htonl, htons convert the
     * values.
     */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port_number);
    bind(sockfd, (struct sockaddr *) &server, (socklen_t), sizeof(server));

	printf("dir name %s\n", dir_name);
	printf("starting on port: %d\n", port_number);
	exit(0);

    for (;;) {
        /* Receive up to one byte less than declared, because it will
         * be NUL-terminated later.
         */
		socklen_t len = (socklen_t)sizeof(client);
        ssize_t n = recvfrom(sockfd, message, sizeof(message) - 1,
                             0, (struct sockaddr *) &client, &len);

        message[n] = '\0';

		// get file path
		char file_name[100];
		memset(file_name, '\0', sizeof(file_name));
		get_filename(file_name, message);
		char full_path[100];
		join_path(full_path, 200, dir_name, file_name);
		fprintf(stdout, "Reading file: %s\n", full_path);
		fflush(stdout);

		FILE* file = open_file(full_path);
		if(!file){

			fprintf(stderr, "unable to open file\n");
			exit(-1);
		}

		send_file(file, get_mode(message), client, sockfd);
		close_file(file);
    }
}
