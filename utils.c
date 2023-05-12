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

void respondList(int respfd, struct message_t req)
{
    struct message_t response;
    memset(response.content, '\0', MSG_BUFFER_SIZE);
    response.type = COMMAND_END;
    sprintf(response.content, "DUMMY RESPONSE\n");
    write(respfd, &response, sizeof(struct message_t));
}

void respondHelp(int respfd, struct message_t req)
{
    struct message_t response;
    memset(response.content, '\0', MSG_BUFFER_SIZE);
    response.type = COMMAND_END;
    sprintf(response.content, "DUMMY RESPONSE\n");
    write(respfd, &response, sizeof(struct message_t));
}

void respondReadF(int respfd, struct message_t req)
{
    struct message_t response;
    memset(response.content, '\0', MSG_BUFFER_SIZE);
    response.type = COMMAND_END;
    sprintf(response.content, "DUMMY RESPONSE\n");
    write(respfd, &response, sizeof(struct message_t));
}

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

void respondUnknown(int respfd)
{
	struct message_t msg;
	memset(msg.content, '\0',  MSG_BUFFER_SIZE);
	msg.type = COMMAND_END;
	
	sprintf(msg.content, "Unknown request..\n");
	write(respfd, &msg, sizeof(struct message_t));
}

void respondEnd(int respfd)
{
	struct message_t msg;
	memset(msg.content, '\0',  MSG_BUFFER_SIZE);
	msg.type = COMMAND_END;
	write(respfd, &msg, sizeof(struct message_t));
}