#include "utils.h"

char* time_as_string()
{
	time_t t = time(NULL);
  	struct tm tm = *localtime(&t);

  	char* as_string = (char*)calloc(sizeof(char),200);

  	sprintf(as_string,"2022:%02d:%02d:%02d:%02d:%02d", tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  	
	return as_string;
}

int char_count(const char* string, char c, size_t size)
{	
	int ctr = 0;

	for (int i = 0; i < size; ++i)
		if(string[i] == c)
			++ctr;
	return ctr;
}

char** read_file(int fd_, int* lines, const int lock)
{	
	struct flock file_lock;
	int fd = fd_;

	if(0 == lock) {

		memset(&file_lock,0,sizeof(file_lock));
		file_lock.l_type = F_WRLCK;
		fcntl(fd,F_SETLKW,&file_lock);		
	}
	lseek(fd,0,SEEK_SET);

	size_t size = lseek(fd,0,SEEK_END);
	if(size <= 0) {
		close(fd);
        perror("File Read Error (Empty or Null File)");
        exit(-1);
	}

	lseek(fd,0,SEEK_SET);
	char* readed_file = (char*)calloc((size+1), sizeof(char));
	read(fd,readed_file,size);
	readed_file[size] = '\0';

	int line_size = char_count(readed_file,'\n',size) + 1;
	char** parsed_file = (char**)calloc(sizeof(char*),line_size);
	
	int index = 0;
	char * token = strtok(readed_file,"\n");
	while(token != NULL)
	{	
		int size = strlen(token) + 1;
		// Added
		size = 5096;
		
		parsed_file[index] = (char*)calloc(size, sizeof(char));
		strcpy(parsed_file[index],token);
		++index;		

		token = strtok(NULL,"\n");
	}

	free(readed_file);
	*lines = index;

	if(0 == lock) {
		file_lock.l_type = F_UNLCK;
		fcntl(fd,F_SETLKW,&file_lock);
	}

	return parsed_file;	
}

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

int findIndex(pid_t* queue, int target)
{
    for (int i = 0; i < SHARED_MEM_SIZE / sizeof(pid_t); ++i) {
        if (queue[i] == target) {
            return i;
        }
    }
    return 0;
}

void sendStr(int respfd, const char* str)
{
	struct message_t msg;
	memset(msg.content, '\0', MSG_BUFFER_SIZE);
	msg.type = STRING_MSG;

	sprintf(msg.content, "%s", str);
	write(respfd, &msg, sizeof(struct message_t));
}

void listFilesAndDirectories(int respfd, const char* path, int level) 
{
    DIR *dir;
    struct dirent *entry;
    struct stat fileStat;

    // Open the directory
    dir = opendir(path);
    if (dir == NULL) {
        printf("Unable to open directory: %s\n", path);
		respondUnknown(respfd);
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
            listFilesAndDirectories(respfd, filePath, level + 1);
    }

    // Close the directory
    closedir(dir);
}

void respondHelp(int respfd, struct message_t req)
{
	char param[SMALL_BUFFER];
	memset(param, '\0', SMALL_BUFFER);
	sscanf(req.content, "%s", param);

	struct message_t response;
    memset(response.content, '\0', MSG_BUFFER_SIZE);

	// if the client request help supported commands
	if (strcmp(param, "XX") == 0) {
		sprintf(response.content, "\tAvailable comments are : \nhelp, list, readF, writeT, upload, download, quit, killServer\n\n");
		response.type = COMMAND_END;
		write(respfd, &response, sizeof(struct message_t));
		return;
	}

	// if the client request help about command usage
	if (strcmp(param, "help") == 0) {
		sprintf(response.content, "help <command> (optional)\n\tshows that available command\n\thelp <command> shows that usage of command\n\n");
	} else if (strcmp(param, "list") == 0) {
		sprintf(response.content, "list\n\tshows that available all files from server side\n\n");
	} else if (strcmp(param, "readF") == 0) {
		sprintf(response.content, "readF <file> <line#>\n\tdisplay the #th line of the <file>, returns with an error if <file> does not exists\n\n");
	} else if (strcmp(param, "writeF") == 0) {
		sprintf(response.content, "writeF <file> <line#> <string>\n\tput <string> the #th line of the <file>, If the file does not exists in Servers directory creates and edits the file at the same time\n\n");
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

void respondUnknown(int respfd)
{
	struct message_t msg;
	memset(msg.content, '\0',  MSG_BUFFER_SIZE);
	msg.type = COMMAND_END;
	
	sprintf(msg.content, "Unknown..\n");
	write(respfd, &msg, sizeof(struct message_t));
}

void respondEnd(int respfd)
{
	struct message_t msg;
	memset(msg.content, '\0',  MSG_BUFFER_SIZE);
	msg.type = COMMAND_END;
	write(respfd, &msg, sizeof(struct message_t));
}

void respondList(int respfd, const char* root)
{
	listFilesAndDirectories(respfd, root, 0);
	respondEnd(respfd);
}

// TODO: will implement
void respondReadF(int respfd, struct message_t req)
{
    struct message_t response;
    memset(response.content, '\0', MSG_BUFFER_SIZE);
    response.type = COMMAND_END;
    sprintf(response.content, "DUMMY RESPONSE\n");
    write(respfd, &response, sizeof(struct message_t));
}

// TODO: will implement
void respondWriteF(int respfd, struct message_t req)
{
    struct message_t response;
    memset(response.content, '\0', MSG_BUFFER_SIZE);
    response.type = COMMAND_END;
    sprintf(response.content, "DUMMY RESPONSE\n");
    write(respfd, &response, sizeof(struct message_t));
}

// TODO: will change
void respondUpload(int respfd, struct message_t req)
{
    struct message_t response;
    memset(response.content, '\0', MSG_BUFFER_SIZE);
    response.type = COMMAND_END;
    sprintf(response.content, "DUMMY RESPONSE\n");
    write(respfd, &response, sizeof(struct message_t));
}

// TODO: will change
void respondDowload(int respfd, struct message_t req)
{
    struct message_t response;
    memset(response.content, '\0', MSG_BUFFER_SIZE);
    response.type = COMMAND_END;
    sprintf(response.content, "DUMMY RESPONSE\n");
    write(respfd, &response, sizeof(struct message_t));
}