/*
 * Author: Au Yong Tuck Loen , Bryan Leong Yi Jun
 * Date: 29/07/18
 * Filename: stream.h
 * Description: Prototype functions for reading and writing data concerning sockets.
 */


/* Writing a stream of bytes to socket 'sd' from buffer 'buf'
 * returns number of bytes written, else prints write error
 */
int write_n_bytes(int sd , char *buf, int n_bytes);


/* Reading a stream of bytes from socket 'sd' to buffer 'buf'
 * returns number of bytes written, else prints read error
 */
int read_n_bytes(int sd, char *buf, int n_bytes);


/* To write one byte char from socket 'sd' to opcode
 * return value of -1 : write fail
 * return value of 1 : write success
 */
int write_opcode(int sd, char opcode);


/* To read one byte char from socket 'sd' to opcode
 * return value of -1 : read fail
 * return value of 1 : read success
 */
int read_opcode(int sd, char *opcode);


/* For writing 2 byte integer value from 'twoByteLength' to socket 'sd'
 * return value of -1 : write fail
 * return value of 1 : write success
 */
int write_two_byte_length(int sd, int twoByteLength);


/* For reading 2 byte integer value from socket 'sd' to 'twoByteLength'
 * return value of -1 : read fail
 * return value of 1 : read success 
 */
int read_two_byte_length(int sd, int *twoByteLength);


/* For writing 4 byte integer value from 'fourByteLength' to socket 'sd'
 * return value of -1 : write fail
 * return value of 1 : write success
 */
int write_four_byte_length(int sd, int fourByteLength);


/* For reading 4 byte integer value from socket 'sd' to 'fourByteLength'
 * return value of -1 : read fail
 * return value of 1 : read success
 */
int read_four_byte_length(int sd, int *fourByteLength);
