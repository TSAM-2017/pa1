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
int make_data_packet(char* buffer, short blocknr, char* data, int size){
	memset(buffer, 0, PACKET_SIZE+1);
	short code = (short)DATA;
	buffer[1] = htons(code);
	buffer[3] = htons(blocknr);
	strncpy(&buffer[4], data, size);
	return 0;
}

int main(int argc, char** argv){
	// validate arguments
	if(argc != 3){
		fprintf(stderr, "expected usage: %s <port> <dir>\n", argv[0] );
		return -1;
	}
	int port_number = atoi(argv[1]);
	char* dir_name = argv[2];
	if(dir_name[strlen(dir_name)-1] == '/'){ dir_name[strlen(dir_name)-1] = '\0';}

    int sockfd;
    struct sockaddr_in server, client;
    char message[512];

    /* Create and bind a UDP socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;

    /* Network functions need arguments in network byte order instead
     * of host byte order. The macros htonl, htons convert the
     * values.
     */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port_number);
    bind(sockfd, (struct sockaddr *) &server, (socklen_t) sizeof(server));

	printf("dir name %s\n", dir_name);
	printf("starting on port: %d\n", port_number);

    for (;;) {
        /* Receive up to one byte less than declared, because it will
         * be NUL-terminated later.
         */
        socklen_t len = (socklen_t) sizeof(client);
        ssize_t n = recvfrom(sockfd, message, sizeof(message) - 1,
                             0, (struct sockaddr *) &client, &len);

        message[n] = '\0';

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
		tftp_mode current_mode = get_mode(message);
		char msg[512];
		int i = 1;
		int j;
		while((j = read_block_from_file(file, i, msg)) > 511){
			i++;
		}
		close_file(file);

        // sendto(sockfd, message, (size_t) n, 0,
        //         (struct sockaddr *) &client, len);
    }
}
