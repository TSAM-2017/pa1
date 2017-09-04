/**
 * utilities for file handling with extra protection
 * and functions suited for tftp handling
 */

#include<stdio.h>
#include <stdlib.h>
#include<string.h>
#include<errno.h>
#include <sys/types.h>
#include <unistd.h>

#include "types_and_constants.h"

/**
 * open file in read mode and assign it to pointer
 *
 * @param  filename     name of file to open (null terminated string)
 * @return {success: file pointer, error: NULL}
 */
FILE* open_file(const char* filename){
    FILE* file_pointer = fopen(filename, "r");
    if (file_pointer == NULL) {
        fprintf(stderr, "%s\n", strerror(errno));
        return NULL;
    }
    return file_pointer;
}

/**
 * close file and return success state
 * @param  file_pointer pointer to file to close
 * @return {success: 0, error: -1}
 */
int close_file(FILE* file_pointer){
    if(fclose(file_pointer)){
        fprintf(stderr, "%s\n", strerror(errno));
        return -1;
    }
    return 0;
}

/**
 * read up to 512 bytes (BLOCK_SIZE) from file from byte nr n (inclusive) and
 * puts results in buffer (which should be of at least BLOCK_SIZE+1 large)
 * note that tftp blocks start index at 1
 *
 * @param  file   file object pointer to work with
 * @param  block  the bloack to read (one indexed)
 * @param  buffer where to place the read data (expected to be at least BLOCK_SIZE+1 bytes large)
 * @return number of bytes read (or negative number in case of error)
 */
int read_block_from_file(FILE* file, int block, char* buffer){
    // set file cursor to correct bosition
    if(fseek(file, (block * BLOCK_SIZE) - BLOCK_SIZE, SEEK_SET)){
        fprintf(stderr, "%s\n", strerror(errno));
        return -1;
    }

    int bytes = fread(buffer, 1, BLOCK_SIZE, file);

    if(bytes == -1){
        fprintf(stderr, "%s\n", strerror(errno));
        return -1;
    }
    // just to be 100% sure of null termination (this may be overkill)
    buffer[bytes] = '\0';

    return bytes;
}

/**
 * concatenate two string to create a valid file path (which is then put in buffer)
 *
 * @param  buffer    buffer that shall take the full path (allocation is done by this function)
 * @param  size      size of buffer (should be sizeof(dir_name) + sizeof(file_name) + 2, to have space for null and '/')
 * @param  dir_name  fyrst part of path (must be null terminated c-string)
 * @param  file_name second part of path (must be null terminated c-string)
 * @return number of bytes in path (or negative number in case of error)
 */
int join_path(char* buffer, size_t size, char* dir_name, char* file_name){
    int n = snprintf(buffer, size, "%s/%s", dir_name, file_name);
    if (n < 0) {
        fprintf(stderr, "hit an error in joining path %s and %s\n", dir_name, file_name);
        return -1;
    }
    return n;
}
