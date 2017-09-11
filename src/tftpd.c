
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
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "types_and_constants.h"
#include "file_util.c"

/*
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
*/
/**
 * get file name from packet and places it in buffer
 * @param  buff   [description]
 * @param  packet [description]
 * @return
 */
/*int get_filename(char* buffer, char* packet){
	int n = snprintf(buffer, 100, "%s", packet+2);
	if (n < 0) {
        fprintf(stderr, "hit an error reading packet\n");
        return -1;
    }
	return n;
}*/

/**
 * craft data packet with information recieved and place it in buffer
 * @param buffer  a buffer of at least len PACKET_SIZE
 * @param blocknr sequential number of the packet (note: must be unique as per protocol per transmission)
 * @param data    data packed in 8-bit bytes with a max of BLOCK_SIZE bytes
 */
/*int make_data_packet(unsigned char* buffer, short blocknr, char* data, int size){
	memset(buffer, 0, PACKET_SIZE+1);
	sprintf((char*)buffer, "%c%c%c%c", 0x00, 0x03, 0x00, blocknr);
	memcpy((char*)buffer +4, data, size);
	return 0;
}*/

/**
 * send file to client
 * @param file   pointer to file that is to be sent
 * @param mode   read mode for the file
 * @param client [description]
 */
/*void send_file(FILE* file, tftp_mode mode, struct sockaddr_in client, int sockfd){
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

		// recieve response
		memset(recbuffer, '\0', sizeof(recbuffer));
		recvfrom(sockfd, recbuffer, PACKET_SIZE, 0, (struct sockaddr *) &client, &rec_size);
		if (recbuffer[1] == 4) {
			// ack
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
}*/

// global variables
struct sockaddr_in server, client; // structures for handling internet addresses
unsigned int block_number, ack_block_number; // block numbers of send_packet and rec_packet
char send_packet[PACKET_SIZE]; // package to send (sendto)
char rec_packet[PACKET_SIZE]; // received packet (recvfrom)
int data_bytes; // number of bytes in data field of send_packet
FILE *file; // requested file

// function declarations
void begin_send_file(char *path_name, int sockfd);
void end_send_file();
void send_data_packet(int sockfd);
void send_error_packet(int sockfd, error_codes_t error_code, char *error_message);
void ack_packet_true(int sockfd);
void handle_rrq(int sockfd, char* dir_name);
void mode_netascii(char *path);

/*
*	open file and send first data packet
*	@param path_name	the path to the file
*	@param sockfd		socket desciptor
*/
void begin_send_file(char *path_name, int sockfd) {
    if (!file) {
        // open file stream if not already open
        file = fopen(path_name, "r"); // "r" for read only
        //printf("file was opened\n");
    }
    if(!file) {
        send_error_packet(sockfd, 1, "File was not opened");
        exit(-1);
    }

    // set block number to 1, for the first packet
    block_number = 1;
    send_data_packet(sockfd);
}

/*
*	Closes file if it's open
*/
void end_send_file() {
    if (file != NULL){
        fclose(file);
        file = NULL;
    }
}

/*
*	builds a data packet, reads into the data field from file up to a max BLOCK_SIZE bytes
*	sends the packet and closes the file if last byte of file was read
*	@param sockfd	socket descriptor
*	DATA packet:
*	               2 bytes     2 bytes     n bytes
*                  ----------------------------------
*                 | Opcode |   Block #  |   Data     |
*                  ----------------------------------
*/
void send_data_packet(int sockfd) {
    memset(send_packet, 0, sizeof(send_packet)); // fill block of memory with 0
    data_bytes = 0; // number of bytes in data field
    int byte; // a byte from the file
    // build a data packet
    send_packet[1] = 3; // index 0-1 are for the opcode, data has opcode 3
    send_packet[2] = (block_number >> 8) & 255; // index 2-3 are for the blocknumber
    send_packet[3] = block_number & 255; // index 2-3 are for the blocknumber
    // fill data field in packet with bytes from file
    for (data_bytes += 4; data_bytes < BLOCK_SIZE+4; data_bytes++) {
        // +4 because of opcode and blocknumber
        byte = fgetc(file);
        if (byte == EOF) {
            //printf("--- END OF FILE ---\n");
            end_send_file();
            break;
        }
        send_packet[data_bytes] = (char)byte;
    }
    data_bytes -= 4;
    //printf("packet size: %d, data field size: %d\n", data_bytes+4, data_bytes);

    // send data packet
    sendto(sockfd, send_packet, data_bytes+4, 0, (struct sockaddr *) &client, (socklen_t) sizeof(client));
}

/*
*	builds an error packet and sends it
*	ERROR packet:
*              2 bytes     2 bytes      string    1 byte
*              -----------------------------------------
*             | Opcode |  ErrorCode |   ErrMsg   |   0  |
*              -----------------------------------------
*
*/
void send_error_packet(int sockfd, error_codes_t error_code, char *error_message) {
    memset(send_packet, 0, sizeof(send_packet)); // fill block of memory with 0
    // build an error packet
    send_packet[1] = 5; // index 0-1 are for the opcode, error has opcode 5
    send_packet[3] = error_code; // index 2-3 are for the errorcode
    strcpy(&send_packet[4], error_message); // index 4 has the error message

    // send error packet
    sendto(sockfd, send_packet, 4+strlen(error_message)+1, 0, (struct sockaddr *) &client, (socklen_t) sizeof(client));

    if (file) {
        // there was an error, file will not be used anymore
        end_send_file();
        //printf("Error packet sent. Closing file.\n");
    }
}

/*
*	sends the next data packet if can
*	@param sockfd	socket descriptor
*	ACK packet:
*                 2 bytes     2 bytes
*                 ---------------------
*                | Opcode |   Block #  |
*                 ---------------------
*/
void ack_packet_true(int sockfd) {
    if (file) {
        // then file is open and next packet can be sent
        block_number++; // inc blocknumber for next packet
        //printf("---current blocknumber: %d\n", block_number);
        send_data_packet(sockfd);
     }else {
        // then file is closed because the last packet has been delivered
        //printf("file delivered\n -end-\n");
        exit(0);
    }
}

/*
*	handles a read request by getting the filename and mode of the RRQ packet,
*	constructs the path to the file, converts the file if needed, then begins
*	sending the first data packet
*	@param sockfd	socket descriptor
*	@param dir_name	name of the directory the file is in
*	RRQ packet:
*           2 bytes     string    1 byte     string   1 byte
*           ------------------------------------------------
*          | Opcode |  Filename  |   0  |    Mode    |   0  |
*           ------------------------------------------------
*/
void handle_rrq(int sockfd, char* dir_name) {
    memset(send_packet, 0, sizeof(send_packet));
    char *file_name, *rrq_mode;

    // get filename and mode
    file_name = (char*)&rec_packet[2];
    rrq_mode = (char*)&rec_packet[2+strlen(file_name)+1];
    for (int i = 0; rrq_mode[i]; i++) {
        // mode string could be a mix of upper and lower case chars
        rrq_mode[i] = tolower(rrq_mode[i]);
    }
    //printf("file_name: %s\n", file_name);
    //printf("rrq_mode: %s\n", rrq_mode);

    // have dir_name and file_name, get full path
    char full_path[256];
    memset(full_path, 0, sizeof(full_path));
    join_path(full_path, 256, dir_name, file_name);
    //printf("full_path: %s\n", full_path);

    // check mode
    if (!strcmp(rrq_mode, MODE_NET)) {
        // then the mode is netascii (so it is assumed that a text file is being sent)
        // A host which receives netascii mode data must translate the data to its own format
        mode_netascii(full_path); // convert file
    }
    else if (!strcmp(rrq_mode, MODE_OCTET)) {
        // then mode is octet (data is transferred and stored exactly as it is)
    }
    else if (!strcmp(rrq_mode, MODE_MAIL)) {
        // then mode is mail
        // mail mode is obsolete and should not be implemented or used
    }

    // list which file is requested form which IP and port
    printf("file \"%s\" requested from %s:%d\n", file_name, inet_ntoa(client.sin_addr),
            (int) ntohs(client.sin_port));

    // begin sending file
    begin_send_file(full_path, sockfd);
}

/*
*	converts the file to netascii mode by making a temporary file that
*	has all \n replaced with \r\n and all \r replaced with \r\0
*	then having the data file pointer point to that temp file
*	(temp file is deleted when program terminates)
*	@param path	the path to the data file
*/
void mode_netascii(char *path) {
    // \r carriage return(CR), \n line feed (LF)

    // in RFC 764
    // change all \n to \r\n
    // change all \r to \r\0

    FILE *fp = tmpfile(); // create a temporary file
    file = fopen(path, "r"); // open data file in read only
    // copy everything from file to fp
    int c = fgetc(file);
    while(c != EOF) {
        if (c == '\n') {
            fputc('\r', fp);
        }
        fputc(c, fp);
        if (c == '\r') {
            fputc('\0', fp);
        }
        c = fgetc(file);
    }
    fclose(file); // close data file
    file = fp; // point to temp file
    if (file) {
        //printf("temp file OPEN\n");
    }
    fseek(file, 0, SEEK_SET); // point to beginning of file
}

/*
*	main function
*	includes a loop, checks the opcode for each packet received
*/
int main(int argc, char** argv) {
    // validate arguments
    // mandatory to have 3: name of program, the port and directory
    if (argc != 3) {
        fprintf(stderr, "expected usage: %s <port> <dir>\n", argv[0] );
        exit(0);
    }
    // The arguments
    int port_number = atoi(argv[1]);
    char* dir_name = argv[2];
    if (dir_name[strlen(dir_name)-1] == '/') {
        dir_name[strlen(dir_name)-1] = '\0';
    }

    // Socket file descriptor
    int sockfd;

    // Create and bind a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    // Initialize the server
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;

    /* Network functions need arguments in network byte order instead
     * of host byte order. The macros htonl, htons convert the
     * values. */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port_number);
    if (bind(sockfd, (struct sockaddr *) &server, (socklen_t) sizeof(server)) < 0) {
        //printf("ERROR on binding. Exit.\n");
        exit(-1);
    }

    //printf("dir name: %s\n", dir_name);
    //printf("starting on port: %d\n", port_number);

    for (;;) {
        //printf("enter loop\n");
        /* Receive up to one byte less than declared, because it will
         * be NUL-terminated later.
         */
        socklen_t len = (socklen_t)sizeof(client);
        ssize_t n = recvfrom(sockfd, rec_packet, sizeof(rec_packet) - 1,
                             0, (struct sockaddr *) &client, &len);
        //printf("received packet: %zu bytes\n", n);

        rec_packet[n] = '\0';
        int op_code = rec_packet[1]; // get opcode
        //printf("opcode: %d\n", op_code);

        if (op_code == RRQ) {
            // Read request
            //printf("RRQ\n");
            handle_rrq(sockfd, dir_name);
        }else if (op_code == WRQ) {
            // Write request, not implemented
            //printf("WRQ\n");
            printf("ERROR: Uploading not allowed. ACCESS VIOLATION. \n");
            send_error_packet(sockfd, ACCESS, "Access violation. Write requests not allowed.\0");
        }else if (op_code == DATA) {
            // Data packet, illegal operation
            //printf("DATA\n");
            printf("ERROR: May not receive data. ILLEGAL OPERATION.\n");
            send_error_packet(sockfd, ILLEGAL, "Illegal TFTP operation.\0");
        }else if (op_code == ACK) {
            // Acknowledgement
            //printf("ACK\n");
            // get block number from the ack packet
            ack_block_number = (((unsigned char*)rec_packet)[2] << 8) + ((unsigned char*)rec_packet)[3];
            if (ack_block_number == block_number) {
   	            // then data packet was delivered and next packet can be sent
                ack_packet_true(sockfd);
            }else {
                // then data packet was not delivered and it must be resent
                //printf("ACK_BLOCK_NUMBER: %d\n", ack_block_number);
                //printf("resend last packet with blocknumber: %d\n", block_number);
                sendto(sockfd, send_packet, data_bytes+4, 0, (struct sockaddr *) &client, (socklen_t) sizeof(client));
            }
        }else if (op_code == ERROR) {
            // Error packet
            //printf("ERROR\n");
            if (file) {
                end_send_file();
            }
            exit(0);
        }
    }
    //printf("out of loop\n");
    exit(0);
}
