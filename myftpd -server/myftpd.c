/*
 * Author: Au Yong Tuck Loen , Bryan Leong Yi Jun
 * Date: 29/07/18
 * Filename: myftpd.c
 * Description: 
 */



//Command codes convert to char
#define cmd_Put  'P'
#define cmd_Get  'G'
#define cmd_Pwd  'A'
#define cmd_dir  'B'
#define cmd_cd	 'C'
#define cmd_data 'D'

// ack codes for cmd_Put
// #define ACK_PUT_SUCCESS '0'
// #define ACK_PUT_FILENAME '1'
// #define ACK_PUT_CREATEFILE '2'
// #define ACK_PUT_OTHER '3'

// error messages for cmd_Put ack codes
// #define ACK_PUT_FILENAME_MSG "the server cannot accept the file as there is a filename clash"
// #define ACK_PUT_CREATEFILE_MSG "the server cannot accept the file because it cannot create the named file"
// #define ACK_PUT_OTHER_MSG "the server cannot accept the file due to other reasons"

// ack codes for cmd_Get
// #define ACK_GET_FIND '0'
// #define ACK_GET_OTHER '1'

// error messages for cmd_Get ack codes
// #define ACK_GET_FIND_MSG "the server cannot find requested file"
// #define ACK_GET_OTHER_MSG "the server cannot send the file due to other reasons"

// ack codes for cmd_cd
// #define ACK_CD_SUCCESS '0'
// #define ACK_CD_FIND '1'

// default error message
#define UNEXPECTED_ERROR_MSG "unexpected behaviour"

#define FILE_BLOCK_SIZE 512

// default server listening port
#define SERV_TCP_PORT 40007

// log file
#define LOGPATH "/myftpd.log"	

// struct for managing socket descriptor, client id and logfile path
typedef struct
{
	int sd; // socket
	int clientID; // client id
	char logfile[256];  // absolute log path
} descriptors;

/*
 * Accepts a client id, formatted output string, and va_list of args.
 * Outputs current time, client id, and passed format string to log file.
 *	Remove timestamp maybe
 */
void logger(descriptors *d, char* argformat, ... )
{
	int fileDescriptor;
	if( (fileDescriptor = open(d->logfile, O_WRONLY | O_APPEND | O_CREAT,0766)) == -1 )
	{
		perror("ERROR: Unable to write to log file");
		exit(0);
	}

	va_list args;
	time_t timedata;
	struct tm * timevalue;
	char timeformat[64];
	char* loggerformat;
	char* clientIDFormat = "client %d-";
	char clientIDString[64] = "";

	time(&timedata);
	timevalue = localtime (&timedata);
	asctime_r(timevalue, timeformat); // string representation of time
	timeformat[strlen(timeformat) - 1] = '-';//replace \n

	if(d->clientIDString != 0)
	{
		sprintf(clientIDString, clientIDFormat, d->cid);
	}

	loggerformat = (char*) malloc((strlen(timeformat) + strlen(clientIDString) + strlen(argformat) + 2) * sizeof(char));

	strcpy(loggerformat, timeformat);
	strcat(loggerformat, clientIDString);
	strcat(loggerformat, argformat);
	strcat(loggerformat, "\n");

	va_start(args, argformat); // start the va_list after argformat
	vdprintf(fileDescriptor,  loggerformat,args);
	va_end(args); // end the va_list

	free(loggerformat);

	close(fileDescriptor);
}


// Handles put command to upload file to server-------------------------
void handle_put(descriptors *desc)
{
	logger(desc, "SUCCESS: received cmd 'put'");

	int filenamelength;
	char errCode;
	char cmdCode;
	int fd;

	// read filename and length
	if (read_twobytelength(desc->sd, &filenamelength) == -1)
	{
		logger(desc, "ERROR: failed to read 2 byte length");
		return;
	}

	char filename[filenamelength + 1];

	if (read_nbytes(desc->sd, filename, filenamelength) == -1)
	{
		logger(desc, "ERROR: failed to read filename");
		return;
	}
	
	filename[filenamelength] = '\0';
	
	logger(desc,"SUCCESS: 'put' %s",filename);

	// create new file
	if ( (fd = open(filename,O_RDONLY)) != -1 )
	{
		logger(desc, "ALERT: file already exists: %s", filename);
		//errCode = ACK_PUT_FILENAME;
	}
	else if ( (fd = open(filename,O_WRONLY | O_CREAT, 0766 )) == -1 )
	{
		logger(desc,"ERROR: unable to create file: %s",filename);
		//errCode = ACK_PUT_CREATEFILE;
	}

	/* write acknowledgement */
	if ( write_code(desc->sd, cmd_Put) == -1 )
	{
		logger(desc,"ERROR: failed to write command 'put'");
		return;
	}

	if (write_code(desc->sd, errCode) == -1)
	{
		logger(desc,"ERROR: failed to write errCode:%c", errCode);
		return;
	}

	if (errCode != ACK_PUT_SUCCESS)
	{
		logger(desc,"SUCCESS: command 'put' completed");
		return;
	}

	/* read response from client */
	if (read_code(desc->sd, &opcode) == -1)
	{
		logger(desc,"ERROR: failed to read code");
	}
	/* expect to read OP_DATA */
	if (opcode != OP_DATA)
	{
		logger(desc,"ERROR: unexpected opcode:%c, expected: %c", opcode, OP_DATA);
		return;
	}

	int filesize;

	/* read filesize */
	if(read_fourbytelength(desc->sd, &filesize) == -1)
	{
		logger(desc,"ERROR: failed to read filesize");
		return;
	}


	int block_size = FILE_BLOCK_SIZE;
	if(FILE_BLOCK_SIZE > filesize)
	{
		block_size = filesize;
	}
	
	char filebuffer[block_size];
	int nr = 0;
	int nw = 0;

	while(filesize > 0)
	{
		if(block_size > filesize)
		{
			block_size = filesize;
		}
		if( (nr = read_nbytes(desc->sd,filebuffer,block_size)) == -1)
		{
			logger(desc,"ERROR: failed to read bytes");
			close(fd);
			return;
		}
		if( (nw = write(fd,filebuffer,nr)) < nr )
		{
			logger(desc,"ERROR: failed to write %d bytes, wrote %d bytes instead", nr, nw);
			close(fd);
			return;
		}
		filesize -= nw;
	}
	close(fd);
	logger(desc,"SUCCESS: command 'put' completed");
}


// Handles get command to download file from server---------------------
void handle_get(descriptors *desc)
{
	logger(desc,"GET");

	int fd;
	struct stat inf;
	int filesize;
	int filenamelength;
	char errCode;


	/* read filename and length */
	if( read_twobytelength(desc->sd,&filenamelength) == -1){
		printf("ERROR: failed to read 2 byte length");
		return;
	}

	char filename[filenamelength + 1];

	if(read_nbytes(desc->sd,filename,filenamelength) == -1){
		printf("ERROR: failed to read filename");
		return;
	}
	filename[filenamelength] = '\0';

	logger(desc,"GET %s",filename);


	/* process the file */
	if( (fd = open(filename, O_RDONLY)) == -1){
		errCode = ACK_GET_FIND;
		logger(desc,"%s",ACK_GET_FIND_MSG);
		if(write_code(desc->sd,OP_GET) == -1){
			logger(desc,"ERROR: failed to write opcode:%c",OP_GET);
			return;
		}
		if(write_code(desc->sd,errCode) == -1){
			logger(desc,"ERROR: failed to write errCode:%c",errCode);
		}
		return;
	}

	if(fstat(fd, &inf) < 0) {
		logger(desc,"fstat error");
		errCode = ACK_GET_OTHER;
		logger(desc,"%s",ACK_GET_OTHER_MSG);
		if(write_code(desc->sd,OP_GET) == -1){
			logger(desc,"ERROR: failed to write opcode:%c",OP_GET);
			return;
		}
		if(write_code(desc->sd,errCode) == -1){
			logger(desc,"ERROR: failed to write errCode:%c",errCode);
		}
		return;
	}

	filesize = (int)inf.st_size;

	/* reset file pointer */
	lseek(fd,0,SEEK_SET);


	/* send the data */
	if( write_code(desc->sd,OP_DATA) == -1){
		logger(desc,"ERROR: failed to send OP_DATA");
		return;
	}

	if(write_fourbytelength(desc->sd,filesize) == -1){
		logger(desc,"ERROR: failed to send filesize");
		return;
	}

	int nr = 0;
	char buf[FILE_BLOCK_SIZE];

	while((nr = read(fd,buf,FILE_BLOCK_SIZE)) > 0){
		if ( write_nbytes(desc->sd,buf,nr) == -1){
			logger(desc,"ERROR: failed to send file content");
			return;
		}
	}
	logger(desc,"GET success");
	logger(desc,"GET complete");
}


// Handles pwd command to show current path on the server---------------
void handle_pwd(descriptors *desc)
{
	logger(desc,"PWD");

	char cwd[1024];

	getcwd(cwd, sizeof(cwd));

	if(write_code(desc->sd,OP_PWD) == -1){
		logger(desc, "ERROR: failed to write opcode");
		return;
	}

	if(write_two_byte_length(desc->sd, strlen(cwd)) == -1){
		logger(desc, "ERROR: failed to write length");
		return;
	}

	if(write_nbytes(desc->sd, cwd, strlen(cwd)) == -1){
		logger(desc, "ERROR: failed to write directory");
		return;
	}

	logger(desc, "PWD complete");
}


// Handles dir command to list out files in the directory of the server-
void handle_dir(descriptors *desc)
{
	logger(desc,"dir: listing filenames in directory");

	char files[1024] = "";

	DIR *dir;
	struct dirent *ent;
	dir = opendir(".");
	if (dir){
		while ((ent = readdir(dir)) != NULL){
			strcat(files,ent->d_name);
			strcat(files, "\n");
		}
		files[strlen(files)-1] = '\0';
		closedir(dir);
		logger(desc, "DIR success");
	}

	if(write_code(desc->sd,OP_DIR) == -1){
		logger(desc, "ERROR: failed to write opcode");
		return;
	}

	if(write_fourbytelength(desc->sd, strlen(files)) == -1){
		logger(desc, "ERROR: failed to write length");
		return;
	}

	if(write_nbytes(desc->sd, files, strlen(files)) == -1){
		logger(desc, "ERROR: failed to write file list");
		return;
	}
	logger(desc,"SUCCESS: dir complete");
}


// Handles cd command to change directory of the server-----------------
void handle_cd(descriptors *desc)
{
	logger(desc,"cd: change directory");

	int size;
	char errCode;

	if(read_twobytelength(desc->sd, &size) == -1)
	{
		logger(desc, "ERROR: failed to read size");
		return;
	}

	char token[size + 1];

	if(read_nbytes(desc->sd,token,size) == -1)
	{
		logger(desc, "ERROR: failed to read token");
		return;
	}
	token[size] = '\0';

	logger(desc,"cd %s",token);

	if(chdir(token) == 0)
	{
		errCode = ACK_CD_SUCCESS;
		logger(desc,"Change Directory operation success");
	}
	else
	{
		errCode = ACK_CD_FIND;
		logger(desc,"ERROR: Change Directory uanble to locate directory");
	}

	if(write_code(desc->sd, OP_CD) == -1)
	{
		logger(desc, "ERROR: failed to send cd");
		return;
	}

	if(write_code(desc->sd, errcode) == -1)
	{
		logger(desc, "ERROR: failed to send error code");
		return;
	}
	logger(desc, "CD complete");
}


// Remove children processes that are completed-------------------------
void claim_children()
{
	pid_t pid = 1;
	while (pid > 0) //while there is a pid, process will run
	{
		pid = waitpid(0, (int *)0, WNOHANG);
	}
}

// Sets process as daemon-----------------------------------------------
void daemon_init(void)
{
	pid_t pid;
	struct sigaction act;

	if ((pid = fork()) < 0)
	{
		perror("fork"); exit(1);
	}
	else if (pid > 0)
	{
		/* parent */
		printf("myftpd PID: %d\n", pid);
		exit(0);
	}
	else
	{
		/* child */
		setsid();		/* become session leader */
		umask(0);		/* clear file mode creation mask */

		/* catch SIGCHLD to remove zombies from system */
		act.sa_handler = claim_children; /* use reliable signal */
		sigemptyset(&act.sa_mask);       /* not to block other signals */
		act.sa_flags   = SA_NOCLDSTOP;   /* not catch stopped children */
		sigaction(SIGCHLD, (struct sigaction *)&act, (struct sigaction *)0);

	}
}

// Recieve a one byte char command off socket connection from client----
void serve_a_client(int sd)
{	
	char cmdCode;

	logger(desc,"connected");

	while (read_code(desc->sd, &cmdCode) > 0)
	{

		switch(cmdCode)
		{
			case cmd_Put:
				handle_put(desc);
			break;
			case cmd_Get:
				handle_get(desc);
			break;
			case cmd_Pwd:
				handle_pwd(desc);
			break;
			case cmd_Dir:
				handle_dir(desc);
			break;
			case cmd_cd:
				handle_cd(desc);
			break;
			default:
				logger(desc,"invalid opcode recieved");//invalid :. disregard
			break;
		}// end switch

	}// end while

	logger(desc,"disconnected");
	return;
}

int main(int argc, char* argv[])
{
	int nsd;
	pid_t pid;
	unsigned short port = SERV_TCP_PORT; /* server listening port */
	socklen_t cli_addrlen;
	struct sockaddr_in ser_addr, cli_addr;

	descriptors desc;
	desc.clientID = 0;
	desc.sd = 0;

	char initialDir[256] = ".";
	char currentDir[256] = "";

	/* get current directory */
	getcwd(currentDir,sizeof(currentDir));

	if( argc > 2 )
	{
		printf("Command: %s [ initial_current_directory ]\n", argv[0]);
		exit(1);
	}

	/* get the initial directory */
	if (argc == 2)
	{
		strcpy(initialDir,argv[1]);
	}

	if (chdir(initialDir) == -1 )
	{
		printf("ERROR: failed to set initial directory to: %s\n", initialDir);
		exit(1);
	}

	/* setup absolute path to logfile */
	getcwd(currentDir,sizeof(currentDir));
	strcpy(desc.logfile,currentDir);
	strcat(desc.logfile,LOGPATH);

	/* make the server a daemon. */
	daemon_init();
	logger(&desc, "server started");
	logger(&desc, "initial directory set to %s",currentDir);

	/* set up listening socket sd */
	if ((desc.sd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("ERROR: server - socket");
		exit(1);
	}

	/* build server Internet socket address */
	bzero((char *)&ser_addr, sizeof(ser_addr));
	ser_addr.sin_family = AF_INET; // address family
	ser_addr.sin_port = htons(port); // network ordered port number
	ser_addr.sin_addr.s_addr = htonl(INADDR_ANY); // any interface

	/* bind server address to socket sd */
	if (bind(desc.sd, (struct sockaddr *) &ser_addr, sizeof(ser_addr))<0)
	{
		perror("ERROR: server - bind");
		exit(1);
	}

	// become a listening socket
	listen(desc.sd, 5); // 5 maximum connections can be in queue
	logger(&desc,"myftp server now listening on port %hu",port);

	while (1)
	{
		// wait to accept a client request for connection
		cli_addrlen = sizeof(cli_addr);
		nsd = accept(desc.sd, (struct sockaddr *) &cli_addr, &cli_addrlen);
		if (nsd < 0)
		{
			if (errno == EINTR)
			{
				continue;/* if interrupted by SIGCHLD */
				perror("server:accept");
				exit(1);
			}
		}

		/* iterate client id before forking */
		desc.clientID++;

		// fork process
		if ((pid=fork()) <0) //create a child process to handle this client */
		{
			perror("fork");
			exit(1);
		}
		else if (pid > 0)
		{
			close(nsd);
			continue; /* parent to wait for next client */
		}
		else // now in child, serve the current client
		{
			close(desc.sd);
			desc.sd = nsd;
			serve_a_client(&desc);
			exit(0);
		}
	}
}
