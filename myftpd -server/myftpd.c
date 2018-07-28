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
void logger(descriptors *d, char* thingsToWriteIntoLog, ... )
{
	int fileDescriptor;
	if( (fileDescriptor = open(d->logfile, O_WRONLY | O_APPEND | O_CREAT, 0766)) == -1 )
	{
		perror("ERROR: Unable to write to log file");
		exit(0);
	}

	va_list args;
	char* loggerformat;
	char* clientIDFormat = "client %d-";
	char clientIDString[64] = "";


	if(d->clientIDString != 0)
	{
		sprintf(clientIDString, clientIDFormat, d->clientID);
	}

	loggerformat = (char*) malloc((strlen(clientIDString) + strlen(thingsToWriteIntoLog) + 2) * sizeof(char));

	strcat(loggerformat, clientIDString);
	strcat(loggerformat, thingsToWriteIntoLog);
	strcat(loggerformat, "\n");

	va_start(args, thingsToWriteIntoLog); // start the va_list after thingsToWriteIntoLog
	vdprintf(fileDescriptor,  loggerformat, args);
	va_end(args); // end the va_list

	free(loggerformat);
	close(fileDescriptor);
}

// Handles protocol process to send a requested file from the client to the server.
void handle_put(descriptors *desc)
{
	logger(desc, "COMMENCING: command 'put'");

	int filenamelength;
	char ackcode;
	char opcode;
	int fd;

	/* read filename and length */
	if( read_two_byte_length(desc->sd, &filenamelength) == -1)
	{
		logger(desc,"ERROR: failed to read 2 byte length");
		return;
	}

	char filename[filenamelength + 1];

	if(read_n_bytes(desc->sd, filename, filenamelength) == -1)
	{
		logger(desc, "ERROR: failed to read filename");
		return;
	}
	
	filename[filenamelength] = '\0';
	logger(desc, "PUT %s", filename);

	
	/* attempt to create file */
	if ((fd = open(filename,O_RDONLY)) != -1)
	{
		logger(desc, "ALERT: file '%s' exists", filename);
	}
	else if ((fd = open(filename,O_WRONLY | O_CREAT, 0766 )) == -1 )
	{
		logger(desc,"ERROR: cannot create file '%s'", filename);
	}

	int filesize;

	/* read filesize */
	if(read_four_byte_length(desc->sd, &filesize) == -1)
	{
		logger(desc, "ERROR: failed to read filesize");
		return;
	}

	int block_size = FILE_BLOCK_SIZE;
	if (FILE_BLOCK_SIZE > filesize)
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
		if ((nr = read_n_bytes(desc->sd, filebuffer, block_size)) == -1)
		{
			logger(desc,"ERROR: failed to read bytes");
			close(fd);
			return;
		}
		if ((nw = write(fd,filebuffer,nr)) < nr)
		{
			logger(desc, "ERROR: failed to write %d bytes, wrote %d bytes instead", nr, nw);
			close(fd);
			return;
		}
		filesize -= nw;
	}
	close(fd);
	logger(desc,"SUCCESS: PUT completed");
}

// Handles protocol process to send a requested file from the server to the client.
void handle_get(descriptors *desc)
{
	logger(desc, "COMMENCING: command 'get'");

	int fd;
	struct stat inf;
	int filesize;
	int filenamelength;

	/* read filename and length */
	if (read_two_byte_length(desc->sd, &filenamelength) == -1)
	{
		printf("ERROR: failed to read 2 byte length");
		return;
	}

	char filename[filenamelength + 1];

	if (read_n_bytes(desc->sd, filename, filenamelength) == -1)
	{
		printf("ERROR: failed to read filename");
		return;
	}
	filename[filenamelength] = '\0';

	logger(desc, "GET %s", filename);


	/* process the file */
	if ((fd = open(filename, O_RDONLY)) == -1)
	{
		logger(desc,"ERROR: Server is unable to locate requested file");
		return;
	}

	if (fstat(fd, &inf) < 0)
	{
		logger(desc, "ERROR: fstat error");
		return;
	}

	filesize = (int)inf.st_size;

	/* reset file pointer */
	lseek(fd, 0, SEEK_SET);

	if (write_four_byte_length(desc->sd, filesize) == -1)
	{
		logger(desc, "ERROR: failed to send filesize");
		return;
	}

	int nr = 0;
	char buf[FILE_BLOCK_SIZE];

	while ((nr = read(fd, buf, FILE_BLOCK_SIZE)) > 0)
	{
		if (write_n_bytes(desc->sd, buf, nr) == -1)
		{
			logger(desc, "ERROR: failed to send file content");
			return;
		}
	}
	logger(desc,"SUCCESS: GET complete");
}

// Handles protocol process to display current directory path of server.
void handle_pwd(descriptors *desc)
{
	logger(desc, "COMMENCING: command 'pwd'");

	char cwd[1024];

	getcwd(cwd, sizeof(cwd));

	if(write_two_byte_length(desc->sd, strlen(cwd)) == -1)
	{
		logger(desc, "ERROR: failed to write length");
		return;
	}

	if(write_n_bytes(desc->sd, cwd, strlen(cwd)) == -1)
	{
		logger(desc, "ERROR: failed to write directory");
		return;
	}

	logger(desc, "SUCCESS: PWD complete");
}

// Handles protocol process to display directory listing of the server.
void handle_dir(descriptors *desc)
{
	logger(desc, "COMMENCE: command 'dir'");

	char files[1024] = "";

	DIR *dir;
	struct dirent *ent;
	dir = opendir(".");
	if (dir)
	{
		while ((ent = readdir(dir)) != NULL)
		{
			strcat(files, ent->d_name);
			strcat(files, "\n");
		}
		files[strlen(files) - 1] = '\0';
		closedir(dir);
		logger(desc, "SUCCESS: DIR success");
	}

	if(write_four_byte_length(desc->sd, strlen(files)) == -1)
	{
		logger(desc, "ERROR: failed to write length");
		return;
	}

	if(write_n_bytes(desc->sd, files, strlen(files)) == -1)
	{
		logger(desc, "ERROR: failed to write file list");
		return;
	}
	logger(desc,"SUCCESS: DIR complete");
}

// Handles protocol process to change directory of the server.
void handle_cd(descriptors *desc)
{
	logger(desc,"COMMENCE: command 'cd'");

	int size;

	if(read_two_byte_length(desc->sd, &size) == -1)
	{
		logger(desc,"ERROR: failed to read size");
		return;
	}

	char token[size + 1];

	if(read_n_bytes(desc->sd,token,size) == -1)
	{
		logger(desc,"ERROR: failed to read token");
		return;
	}
	token[size] = '\0';

	logger(desc,"CD %s",token);


	if(chdir(token) == 0)
	{
		logger(desc,"SUCCESS: CD success");
	}
	else
	{
		logger(desc,"ERROR: CD cannot find directory");
	}

	logger(desc,"SUCCESS: CD complete");
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
