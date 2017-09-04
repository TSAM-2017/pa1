#ifndef MY_CONST
#define  MY_CONST

#define BLOCK_SIZE 512 // bytes
#define PACKET_SIZE 516 // size of tftp packet in bytes

#define MODE_OCTET 	"octet"
#define MODE_MAIL 	"mail"
#define MODE_NET	"netascii"

typedef enum {
	RRQ 	= 1, // read request
	WRQ 	= 2, // write reques
	DATA 	= 3, // data
	ACK 	= 4, // acknowledgement
	ERROR 	= 5  // Error
} opcode_t;

typedef enum {
	NDEF 		= 0, // not defined (may contain error msg)
	NFOUND 		= 1, // file not found
	ACCESS		= 2, // access violation
	ILLEGAL		= 4, // illegal TFTP operation
	ID_UNKNOWN	= 5  // unknown transfer ID
} error_codes_t;	// not all codes are acounted for since we will not support upploads

typedef enum {
	NET 	= 1,
	OCTET 	= 2,
	MAIL 	= 3,
} tftp_mode;

typedef struct packet {
	short opcode;
	char filename[PACKET_SIZE];
	char mode[PACKET_SIZE];
	char data[BLOCK_SIZE];
	int data_len;
	int block_nr;
	short err_code;
} packet_t;

#endif
