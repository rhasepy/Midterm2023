#include "utils.h"

// parse writeT options
void parseWriteT(char* str, char* array1, char* array2, char* array3, char* array4)
{
	char* token = strtok(str, " ");
    if (token != NULL) {
        strcpy(array1, token);
        token = strtok(NULL, " ");
        if (token != NULL) {
            strcpy(array2, token);
            token = strtok(NULL, " ");
            if (token != NULL) {
                strcpy(array3, token);
                token = strtok(NULL, "\n"); 
                if (token != NULL) {
                    strcpy(array4, token);
                } else {
                    strcpy(array4, "XX");
                }
            } else {
                strcpy(array3, "XX");
                strcpy(array4, "XX");
            }
        } else {
            strcpy(array2, "XX");
            strcpy(array3, "XX");
            strcpy(array4, "XX");
        }
    } else {
        strcpy(array1, "XX");
        strcpy(array2, "XX");
        strcpy(array3, "XX");
        strcpy(array4, "XX");
    }
}

// char 2d to write file
void char2DToFile(int fd, char** content, int size)
{
	for (int i = 0; i < size; ++i) {
		write(fd, content[i], strlen(content[i]));
		if (i < (size - 1))
			write(fd, "\n", 1);
	}	
}

// return time information
char* timeAsString()
{
	time_t t = time(NULL);
  	struct tm tm = *localtime(&t);

  	char* as_string = (char*)calloc(sizeof(char),200);

  	sprintf(as_string,"2022:%02d:%02d:%02d:%02d:%02d", tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  	
	return as_string;
}

// initiliaze logger name
char* initLogName(const char* name)
{
	time_t t = time(NULL);
  	struct tm tm = *localtime(&t);

  	char* as_string = (char*)calloc(sizeof(char),200);

  	sprintf(as_string,"%s_2022_%02d_%02d_%02d_%02d_%02d", name, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  	
	return as_string;	
}

// how much occurences c in string
int charCount(const char* string, char c, size_t size)
{	
	int ctr = 0;

	for (int i = 0; i < size; ++i)
		if(string[i] == c)
			++ctr;
	return ctr;
}

// read file and return its content
char* readFileAs1D(int fd, size_t* _size)
{
	size_t size = lseek(fd,0,SEEK_END);

	*_size = size;

	lseek(fd,0,SEEK_SET);
	char* readed_file = (char*)calloc(size, sizeof(char));
	read(fd, readed_file, size);

	return readed_file;
}

// read file and return its content line by line
char** readFile(int fd_, int* lines)	
{	
	int fd = fd_;

	lseek(fd,0,SEEK_SET);
	size_t size = lseek(fd,0,SEEK_END);
	if(size <= 0) {
		*lines = 0;
		close(fd);
		return NULL;
	}

	lseek(fd,0,SEEK_SET);
	char* readed_file = (char*)calloc((size+1), sizeof(char));
	read(fd,readed_file,size);
	readed_file[size] = '\0';

	int line_size = charCount(readed_file,'\n',size) + 1;
	char** parsed_file = (char**)calloc(sizeof(char*),line_size);
	
	int index = 0;
	char * token = strtok(readed_file,"\n");
	while(token != NULL) {	
		int size = strlen(token) + 1;		
		parsed_file[index] = (char*)calloc(size, sizeof(char));
		strcpy(parsed_file[index],token);
		++index;		

		token = strtok(NULL,"\n");
	}

	free(readed_file);
	*lines = index;

	return parsed_file;	
}

// release all file resources
void clearFileContent(char** data, int len)
{
	for (int i = 0; i < len; ++i)
		free(data[i]);
	free(data);
}

// get element and shift queue (dequeue)
pid_t getAndShiftPID(pid_t* queue, int len)
{
    pid_t returnVal = queue[0];

    // Consume client id from client queue (shifting)
    for (int i = 0; i < len - 1; ++i) {
        queue[i] = queue[i + 1];
    }
    queue[len - 1] = -1;

    return returnVal;
}

// get element and shift queue (dequeue)
int getAndShiftInt(int* queue, int len)
{
    int returnVal = queue[0];

    // Consume client id from client queue (shifting)
    for (int i = 0; i < len - 1; ++i) {
        queue[i] = queue[i + 1];
    }
    queue[len - 1] = -1;

    return returnVal;	
}

// search target first occurence index
int findIndex(pid_t* queue, int target)
{
    for (int i = 0; i < SHARED_MEM_SIZE / sizeof(pid_t); ++i) {
        if (queue[i] == target) {
            return i;
        }
    }
    return 0;
}

// prepare connection request
struct message_t prepareConnectionRequest(const char* connectionCommand)
{
    struct message_t connReq;
    memset(connReq.content, '\0', MSG_BUFFER_SIZE);
    sprintf(connReq.content, "%d", getpid());

    if (strcmp(connectionCommand, "Connect") == 0) {
        connReq.type = CONNECT_REQ;
    } else if (strcmp(connectionCommand, "tryConnect") == 0) {
        connReq.type = TRY_CONNECT_REQ;
    } else {
        fprintf(stderr, "[ERROR] Usage: biboClient <Connect/tryConnect> ServerPID\n");
        exit(EXIT_FAILURE);
    }
    return connReq;
}

// prepare command from user
struct message_t prepareCommand(char* input)
{
    struct message_t commandRequest;
    commandRequest.type = UNKNOWN;
    memset(commandRequest.content, '\0', MSG_BUFFER_SIZE);

    if (input == NULL || strlen(input) == 0) {
        return commandRequest;
    }

    // command max has 4 parameter and i pushed string 4 x 'XX'. (e.g. writeT <file> <line #> <string>)
    // because worst case string is empty if the string is empty
    // parsed 4 token is 'XX' then command is unknown and empty command
    // worker does not respond  
    char tokenizerStr[BUFFER_SIZE];
    memset(tokenizerStr, '\0', BUFFER_SIZE);
    sprintf(tokenizerStr, "%s XX XX XX XX", input);

    char command[SMALL_BUFFER];
    memset(command, '\0', SMALL_BUFFER);

    char param1[SMALL_BUFFER];
    memset(param1, '\0', SMALL_BUFFER);

    char param2[SMALL_BUFFER];
    memset(param2, '\0', SMALL_BUFFER);

    char param3[SMALL_BUFFER];
    memset(param3, '\0', SMALL_BUFFER);

    sscanf(tokenizerStr, "%s %s %s %s", command, param1, param2, param3);

    if (strcmp(command, "help") == 0) {
        commandRequest.type = HELP;
        sprintf(commandRequest.content, "%s", param1);
    } else if (strcmp(command, "list") == 0) {
        commandRequest.type = LIST;
    } else if (strcmp(command, "readF") == 0) {
        commandRequest.type = READF;
        sprintf(commandRequest.content, "%s %s", param1, param2);
    } else if (strcmp(command, "writeT") == 0) {
        commandRequest.type = WRITET;
		parseWriteT(input, command, param1, param2, param3);
        sprintf(commandRequest.content, "%s %s %s", param1, param2, param3);
    } else if (strcmp(command, "upload") == 0) {
        commandRequest.type = UPLOAD;
        sprintf(commandRequest.content, "%s", param1);
    } else if (strcmp(command, "download") == 0) {
        commandRequest.type = DOWNLOAD;
        sprintf(commandRequest.content, "%s", param1);
    } else if (strcmp(command, "quit") == 0) {
        commandRequest.type = QUIT;
    } else if (strcmp(command, "killServer") == 0) {
        commandRequest.type = KILL;
    }

    return commandRequest;
}

// send string to client no multiple string
void sendStr(int respfd, const char* str)
{
	struct message_t msg;
	memset(msg.content, '\0', MSG_BUFFER_SIZE);
	msg.type = STRING_MSG;

	sprintf(msg.content, "%s", str);
	write(respfd, &msg, sizeof(struct message_t));
}

// send one line to client no multiple line
void sendLine(int respfd, const char* str)
{
	struct message_t msg;
	memset(msg.content, '\0', MSG_BUFFER_SIZE);
	msg.type = STRING_MSG;

	sprintf(msg.content, "%s\n", str);
	write(respfd, &msg, sizeof(struct message_t));
}

// search all directory and sub directory recursively
void listFilesAndDirectories(int respfd, const char* path, int level, int clientLogFd) 
{
    DIR *dir;
    struct dirent *entry;
    struct stat fileStat;

    // Open the directory
    dir = opendir(path);
    if (dir == NULL) {
		char temp[SMALL_BUFFER];
		memset(temp, '\0', SMALL_BUFFER);
		sprintf(temp, "Unable to open directory: %s\n", path);
		sendOneMsg(respfd, temp);
        return;
    }

    // Read directory entries
    while ((entry = readdir(dir)) != NULL) {
        char filePath[BUFFER_SIZE];
		memset(filePath, '\0', BUFFER_SIZE);
        snprintf(filePath, sizeof(filePath), "%s/%s", path, entry->d_name);

        // Get file/directory information
        if (stat(filePath, &fileStat) < 0)
            continue;

		if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
			
			int indentCtr = level + 1;
			sendStr(respfd, "|");

			if (S_ISDIR(fileStat.st_mode))
				--indentCtr;

			for (int i = 0; i < indentCtr; i++)
				sendStr(respfd, "-");

			char temp[HALF_BUFFER];
			memset(temp, '\0', HALF_BUFFER);
			sprintf(temp, "%s\n", entry->d_name);
			sendStr(respfd, temp);
		}

        // Recursively call for subdirectories (excluding . and ..)
        if (S_ISDIR(fileStat.st_mode) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            listFilesAndDirectories(respfd, filePath, level + 1, clientLogFd);
    }

    // Close the directory
    closedir(dir);
}

// help response to client
void respondHelp(int respfd, struct message_t req, int clientLogFd)
{
	char param[SMALL_BUFFER];
	memset(param, '\0', SMALL_BUFFER);
	sscanf(req.content, "%s", param);

	struct message_t response;
    memset(response.content, '\0', MSG_BUFFER_SIZE);

	char logLine[LNG_BUFFER_SIZE];
	memset(logLine, '\0', LNG_BUFFER_SIZE);
	
	// if the client request help supported commands
	if (strcmp(param, "XX") == 0) {
		char* time = timeAsString();
		sprintf(logLine, "[%s]help command, no parameters\n", time);
		write(clientLogFd, logLine, strlen(logLine));
		free(time);

		sprintf(response.content, "\tAvailable comments are : \nhelp, list, readF, writeT, upload, download, quit, killServer\n\n");
		response.type = COMMAND_END;
		write(respfd, &response, sizeof(struct message_t));
		return;
	}

	char* time = timeAsString();
	sprintf(logLine, "[%s]help command, parameters <%s>\n", time, param);
	write(clientLogFd, logLine, strlen(logLine));
	free(time);

	// if the client request help about command usage
	if (strcmp(param, "help") == 0) {
		sprintf(response.content, "help <command> (optional)\n\tshows that available command\n\thelp <command> shows that usage of command\n\n");
	} else if (strcmp(param, "list") == 0) {
		sprintf(response.content, "list\n\tshows that available all files from server side\n\n");
	} else if (strcmp(param, "readF") == 0) {
		sprintf(response.content, "readF <file> <line#>\n\tdisplay the #th line of the <file>, returns with an error if <file> does not exists\n\n");
	} else if (strcmp(param, "writeT") == 0) {
		sprintf(response.content, "writeT <file> <line#> <string>\n\tput <string> the #th line of the <file>, If the file does not exists in Servers directory creates and edits the file at the same time\n\n");
	} else if (strcmp(param, "upload") == 0) {
		sprintf(response.content, "upload <file>\n\tupload <file> to server side from client side\n\n");
	} else if (strcmp(param, "download") == 0) {	
		sprintf(response.content, "download <file>\n\tdownload <file> from server side to client side\n\n");
	} else if (strcmp(param, "quit") == 0) {
		sprintf(response.content, "quit\n\tSend write request to Server side log file and quits\n\n");
	} else if (strcmp(param, "killServer") == 0) {
		sprintf(response.content, "killServer\n\tSends a kill request to the Server\n\n");
	} else {
		sprintf(response.content, "Unknown parameter for help\n\n");
	}

	response.type = COMMAND_END;
	write(respfd, &response, sizeof(struct message_t));
}

// send one message to client no multiple message
void sendOneMsg(int respfd, const char* content)
{
	struct message_t msg;
	memset(msg.content, '\0',  MSG_BUFFER_SIZE);
	msg.type = COMMAND_END;
	
	sprintf(msg.content, "%s\n", content);
	write(respfd, &msg, sizeof(struct message_t));
}

// send end command to client to client finished waiting mesage
void respondEnd(int respfd)
{
	struct message_t msg;
	memset(msg.content, '\0',  MSG_BUFFER_SIZE);
	msg.type = COMMAND_END;
	write(respfd, &msg, sizeof(struct message_t));
}

// send list content to client
void respondList(int respfd, const char* root, int clientLogFd)
{
	char logLine[LNG_BUFFER_SIZE];
	memset(logLine, '\0', LNG_BUFFER_SIZE);

	// print log
	char* time = timeAsString();
	sprintf(logLine, "[%s]list command\n", time);
	write(clientLogFd, logLine, strlen(logLine));
	free(time);

	// all directory and files search and send client
	listFilesAndDirectories(respfd, root, 0, clientLogFd);

	// end of command
	respondEnd(respfd);
}

// read file content
void respondReadF(int respfd, struct message_t req, const char* root, int clientLogFd)
{
	char param[HALF_BUFFER];
	memset(param, '\0', HALF_BUFFER);
	char param2[HALF_BUFFER];
	memset(param2, '\0', HALF_BUFFER);
	sscanf(req.content, "%s %s", param, param2);

	char logLine[LNG_BUFFER_SIZE];
	memset(logLine, '\0', LNG_BUFFER_SIZE);

	char fileFullPath[BUFFER_SIZE];
	memset(fileFullPath, '\0', BUFFER_SIZE);
	sprintf(fileFullPath, "%s/%s", root, param);

	int fd = open(fileFullPath, O_RDONLY);
	if (FALSE == fd) {
		sendOneMsg(respfd, "File does not exist!\n");
		return;
	}

	// lock file
	struct flock file_lock;
	memset(&file_lock,0,sizeof(file_lock));
	file_lock.l_type = F_WRLCK;
	fcntl(fd,F_SETLKW,&file_lock);		

	int fileLine = -1;
	char** fileContent = readFile(fd, &fileLine);

	// if parameter does not exist then send whole file
	if (strcmp(param2, "XX") == 0) {

		char* time = timeAsString();
		sprintf(logLine, "[%s]readF command, parameters <%s>\n", time, param);
		write(clientLogFd, logLine, strlen(logLine));
		free(time);

		// whole file iterating
		for (int i = 0; i < fileLine; ++i) {

			// whole line sending with 1024 byte chunks.
			for (int j = 0; j < strlen(fileContent[i]); j += MSG_BUFFER_SIZE) {
				char buffer[MSG_BUFFER_SIZE];
				memset(buffer, '\0', MSG_BUFFER_SIZE);
				strncpy(buffer, fileContent[i] + j, MSG_BUFFER_SIZE - 1);
				buffer[MSG_BUFFER_SIZE - 1] = '\0';
				sendLine(respfd, buffer);
			}
		}
		// send end of response to client
		sendOneMsg(respfd, "");
		clearFileContent(fileContent, fileLine);

		// unlock file
		file_lock.l_type = F_UNLCK;
		fcntl(fd,F_SETLKW,&file_lock);
		close(fd);
		return;
	}

	// print log
	char* time = timeAsString();
	sprintf(logLine, "[%s]readF command, parameters <%s> <%s>\n", time, param, param2);
	write(clientLogFd, logLine, strlen(logLine));
	free(time);

	// if the line parameter exist then check line valid or not
	int line = atoi(param2) - 1;
	if (line >= fileLine || line < 0) {
		sendOneMsg(respfd, "File line number too big or too low...\n");
		clearFileContent(fileContent, fileLine);
		
		// unlock file
		file_lock.l_type = F_UNLCK;
		fcntl(fd,F_SETLKW,&file_lock);
		close(fd);
		return;
	}

	// custom line sending with 1024 byte chunks. (line may be greater than 1024 byte)
	for (int j = 0; j < strlen(fileContent[line]); j += MSG_BUFFER_SIZE) {
		char buffer[MSG_BUFFER_SIZE];
		memset(buffer, '\0', MSG_BUFFER_SIZE);
		strncpy(buffer, fileContent[line] + j, MSG_BUFFER_SIZE - 1);
		buffer[MSG_BUFFER_SIZE - 1] = '\0';
		sendLine(respfd, buffer);
	}
	// send end of response to client
	sendOneMsg(respfd, "");
	clearFileContent(fileContent, fileLine);

	// unlock file
	file_lock.l_type = F_UNLCK;
	fcntl(fd,F_SETLKW,&file_lock);
	close(fd);
}

// write file content
void respondWriteT(int respfd, struct message_t req, const char* root, int clientLogFd)
{
	char param[HALF_BUFFER];
	memset(param, '\0', HALF_BUFFER);
	char param2[HALF_BUFFER];
	memset(param2, '\0', HALF_BUFFER);
	char param3[HALF_BUFFER];
	memset(param3, '\0', HALF_BUFFER);

	sscanf(req.content, "%s %s %[^\n]", param, param2, param3);
	if (strcmp(param2, "XX") == 0) {
		sendOneMsg(respfd, "There is not line number\n");
		return;
	}

	// log file line
	char logLine[LNG_BUFFER_SIZE];
	memset(logLine, '\0', LNG_BUFFER_SIZE);

	// full path of file
	char fullPath[BUFFER_SIZE];
	memset(fullPath, '\0', BUFFER_SIZE);

	sprintf(fullPath, "%s/%s", root, param);
	int fd = open(fullPath, O_RDWR);
	if (errno == ENOENT) {
		fd = open(fullPath, O_CREAT | O_RDWR);
	}

	if (FALSE == fd) {
		sendOneMsg(respfd, "File does not exist and can not create from server side\n");
		return;
	}

	// lock file
	struct flock file_lock;
	memset(&file_lock,0,sizeof(file_lock));
	file_lock.l_type = F_WRLCK;
	fcntl(fd,F_SETLKW,&file_lock);	

	// read file as a 2D char array
	int lineSize = -1;
	char** fileContent = readFile(fd, &lineSize);
	
	// reset the file
	close(fd);
	fd = open(fullPath, O_WRONLY | O_TRUNC);
	ftruncate(fd, 0);

	// there is not line in command
	if (strcmp(param3, "XX") == 0) {
		char* time = timeAsString();
		sprintf(logLine, "[%s]writeT command, parameters <%s> <%s>\n", time, param, param2);
		write(clientLogFd, logLine, strlen(logLine));
		free(time);

		// fill file again
		char2DToFile(fd, fileContent, lineSize);
		// put new line parameter the end of file
		if (lineSize > 0)
			write(fd, "\n", 1);
		write(fd, param2, strlen(param2));

		// unlock file
		file_lock.l_type = F_UNLCK;
		fcntl(fd,F_SETLKW,&file_lock);

		// respond about writeT finished
		sendOneMsg(respfd, "writeT OK!...\n");
		clearFileContent(fileContent, lineSize);
		close(fd);
		return;
	} 

	// print log
	char* time = timeAsString();
	sprintf(logLine, "[%s]writeT command, parameters <%s> <%s> <%s>\n", time, param, param2, param3);
	write(clientLogFd, logLine, strlen(logLine));
	free(time);

	// if the line valid in command
	int editLine = atoi(param2) - 1;
	if (editLine >= 0 && editLine < lineSize) {
		// fill file again
		for (int i = 0; i < lineSize; ++i) {
			if (i == editLine)
				write(fd, param3, strlen(param3));
			else
				write(fd, fileContent[i], strlen(fileContent[i]));

			// last line does not need new line
			if (i < (lineSize - 1))
				write(fd, "\n", 1);
		}

		// unlock file
		file_lock.l_type = F_UNLCK;
		fcntl(fd,F_SETLKW,&file_lock);
		close(fd);

		// respond about writeT finished
		sendOneMsg(respfd, "writeT OK!...\n");
		clearFileContent(fileContent, lineSize);
		return;
	} else if (editLine == -1 && strcmp(param2, "XX") != 0) {
		char2DToFile(fd, fileContent, lineSize);
		write(fd, "\n", 1);
		write(fd, param2, strlen(param2));

		if (strcmp(param3, "XX") != 0) {
			write(fd, " ", 1);
			write(fd, param3, strlen(param3));
		}
		
		// unlock file
		file_lock.l_type = F_UNLCK;
		fcntl(fd,F_SETLKW,&file_lock);
		close(fd);

		// respond about writeT finished
		sendOneMsg(respfd, "writeT OK!...\n");
		clearFileContent(fileContent, lineSize);
		return;
	}

	// back up file
	char2DToFile(fd, fileContent, lineSize);
	
	// unlock file
	file_lock.l_type = F_UNLCK;
	fcntl(fd,F_SETLKW,&file_lock);
	close(fd);
	
	// respond about writeT finished
	sendOneMsg(respfd, "Invalid line...\n");
	clearFileContent(fileContent, lineSize);
}

// send file content to client
void respondDowload(int respfd, int workerFd, struct message_t req, const char* root, int clientLogFd)
{
	// full path of file
	char fullPath[BUFFER_SIZE];
	memset(fullPath, '\0', BUFFER_SIZE);
	sprintf(fullPath, "%s/%s", root, req.content);

	// print log
	char logLine[LNG_BUFFER_SIZE];
	memset(logLine, '\0', LNG_BUFFER_SIZE);
	char* time = timeAsString();
	sprintf(logLine, "[%s]download command, parameters <%s>\n", time, req.content);
	write(clientLogFd, logLine, strlen(logLine));
	free(time);
	
	// open file to be send client
	int fd = open(fullPath, O_RDONLY, 0777);
	if (FALSE == fd) {
		sendOneMsg(respfd, "Invalid file...\n");
		return;
	}

	// lock file
	struct flock file_lock;
	memset(&file_lock,0,sizeof(file_lock));
	file_lock.l_type = F_WRLCK;
	fcntl(fd,F_SETLKW,&file_lock);
	
	// send file name to client
	struct message_t downloadResponse;
    memset(downloadResponse.content, '\0', MSG_BUFFER_SIZE);
    downloadResponse.type = FILENAME;
    sprintf(downloadResponse.content, "%s", req.content);
    write(respfd, &downloadResponse, sizeof(struct message_t));

	// if the client create file that will download
	read(workerFd, &downloadResponse, sizeof(struct message_t));
	if (downloadResponse.type != DOWNLOAD_OK) {
		sendOneMsg(respfd, "Download FAIL!\n");
		// unlock file
		file_lock.l_type = F_UNLCK;
		fcntl(fd,F_SETLKW,&file_lock);
		close(fd);
	}

	// get file size
	lseek(fd, 0, SEEK_SET);
	size_t size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	// send each byte via message structure
	for (int i = 0; i < size; ++i) {
		unsigned char c;
		read(fd, &c, sizeof(unsigned char));
		downloadResponse.type = FILE_CONTENT;
		memset(downloadResponse.content, '\0',  MSG_BUFFER_SIZE);
		downloadResponse.content[0] = c;
		downloadResponse.content[1] = '\0';
		write(respfd, &downloadResponse, sizeof(struct message_t));
	}

	// information about command OK
	sendOneMsg(respfd, "Download OK!\n");

	// unlock file
	file_lock.l_type = F_UNLCK;
	fcntl(fd,F_SETLKW,&file_lock);
	close(fd);
}

//  receive file content from client
void respondUpload(int respfd, int workerFd, struct message_t req, const char* root, int clientLogFd)
{
	// Get file name from request
	char fullPath[BUFFER_SIZE];
	memset(fullPath, '\0', BUFFER_SIZE);
	sprintf(fullPath, "%s/%s", root, req.content);

	// print log
	char logLine[LNG_BUFFER_SIZE];
	memset(logLine, '\0', LNG_BUFFER_SIZE);
	char* time = timeAsString();
	sprintf(logLine, "[%s]download command, parameters <%s>\n", time, req.content);
	write(clientLogFd, logLine, strlen(logLine));
	free(time);

	// If the file exist in server then send error message to client
	if (access(fullPath, F_OK) == TRUE) {
		sendOneMsg(respfd, "File already exist...\n");
		return;		
	}

	// open if the file OK
	int fd = open(fullPath, O_CREAT | O_WRONLY, USR_READ_WRITE);
	if (FALSE == fd) {
		sendOneMsg(respfd, "Undefined error while file creating...\n");
		return;
	}

	// lock file
	struct flock file_lock;
	memset(&file_lock,0,sizeof(file_lock));
	file_lock.l_type = F_WRLCK;
	fcntl(fd,F_SETLKW,&file_lock);	

	// Send confirmation message to start uploading file
	struct message_t response;
    memset(response.content, '\0', MSG_BUFFER_SIZE);
    response.type = UPLOAD_OK;
    sprintf(response.content, "Send me file\n");
    write(respfd, &response, sizeof(struct message_t));	

	// And read client fifo untill command end
	struct message_t clientMsg;
	memset(clientMsg.content, '\0', MSG_BUFFER_SIZE);
	do {
		// if the command type is command end then finished function
		// but command type is content then write file content
		read(workerFd, &clientMsg, sizeof(struct message_t));
		if (clientMsg.type == FILE_CONTENT) {
			unsigned char c = (unsigned char) clientMsg.content[0];
			write(fd, &c, 1);
		}
	} while (clientMsg.type != COMMAND_END);

	// unlock file
	file_lock.l_type = F_UNLCK;
	fcntl(fd,F_SETLKW,&file_lock);

	// information about command OK
	sendOneMsg(respfd, "Upload OK!\n");
}