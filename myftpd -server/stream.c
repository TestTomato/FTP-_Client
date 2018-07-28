/*
 * Author: Au Yong Tuck Loen , Bryan Leong Yi Jun
 * Date: 29/07/18
 * Filename: stream.h
 * Description: Functions for reading and writing data concerning sockets.
 */

 #include <sys/types.h>
 #include <netinet/in.h>
 #include <unistd.h>
 #include "stream.h"

// Loops write function  to socket 'sd' until entire 'n_bytes' is written from 'buf'
int write_n_bytes(int sd , char *buf, int n_bytes)
{
	int n = 0;
	int w = 0;
	
	for (n = 0 ; n < n_bytes; n += w)
	{
		if( ( w = write(sd, buf+n, n_bytes-n) ) <= 0 )
		return (w); /* Write error */

	}

	return n;
}
// Loops read function from socket 'sd' until entire 'n_bytes' is read into 'buf'
int read_n_bytes(int sd, char *buf, int n_bytes)
{
	int n = 0;
	int r = 1;

	for (n = 0 ;(n < n_bytes) && (r > 0) ; n += r)
	{
		if( (r = read(sd, buf+n , n_bytes-n) ) < 0)
		return (r); /* Read Error */
	}
	
	return n;
}

// Writing one byte char on the socket 
int write_opcode(int sd, char opcode)
{
	if (write(sd, (char*)&opcode, 1) != 1) 
	{return (-1);}

	return 1;
}

// Reading one byte char off the socket 
int read_opcode(int sd, char *opcode)
{
	char data;

	if(read(sd, (char*)&data, 1) != 1) 
	{return -1;}

	*opcode = data;

	return 1;
}

// Writing two byte integer value on the socket 
int write_two_byte_length(int sd, int twoByteLength)
{
	short data = twoByteLength;

	data = htons(data);  /* conversion to network byte order */

	if (write(sd, &data, 2) != 2) 
	{return (-1)};

	return 1;
}

// Reading two byte integer value off the socket 
int read_two_byte_length(int sd, int *twoByteLength)
{
	short data = 0;

	if (read(sd, &data, 2) != 2) 
	{return (-1)};

	short convert = ntohs(data); /* conversion to host byte order */
  	int t = (int)convert;

  	*twoByteLength = t;
	return 1;
}


// Writing four byte integer value on the socket 
int write_four_byte_length(int sd, int fourByteLength)
{
	int data = htonl(fourByteLength); /* conversion to network byte order */

	if (write(sd,&data, 4) != 4) 
	{return (-1);}

	return 1;
}

// Reading four byte integer value on the socket 
int read_four_byte_length(int sd, int *fourByteLength)
{
	int data = 0;

 	if (read(sd, &data, 4) != 4)
	{return (-1);}

	int convert = ntohl(data); /* conversion to host byte order */
  	*fourByteLength = convert;

	return 1;
}
