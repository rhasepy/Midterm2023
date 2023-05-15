#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

// log file path
#define LOG_PATH            "LOGS"

// named semaphore names
#define FULL_SEM            "FULL_SEM"
#define EMPTY_SEM           "EMPTY_SEM"
#define MUTEX_SEM           "MUTEX_SEM"
#define AV_WRK_SEM          "AV_WRK_SEM"

// shared memory names
#define CLIENT_QUEUE_MEM    "CLIENT_QUEUE_MEM"
#define CLIENT_ID_QUEUE     "CLIENT_ID_QUEUE_MEM"
#define AV_WKR_MEM          "AV_WRK_MEM"

// shared memory size
#define SHARED_MEM_SIZE     40000 * sizeof(pid_t)

// some permission
#define PROT_READ_WRITE     PROT_READ | PROT_WRITE
#define USR_READ_WRITE      S_IRUSR | S_IWUSR

// some utiltiy constants
#define LOG_BUFFER_SIZE     4 * 1024
#define REQUEST_BUFFER_SIZE 25600
#define LNG_BUFFER_SIZE     2048
#define BUFFER_SIZE         1024
#define HALF_BUFFER         512
#define SMALL_BUFFER        64

// system call is failed return -1 
// this is syntax increase readability
// for example: if (FALSE == systyem_call(...someParameter...)) then handle it
#define FALSE               -1
#define TRUE                 0

// the enum is opcode for message send and receive from client and server
enum messageType
{
    // help command
    HELP,
    // list command
    LIST,
    // kill command
    KILL,
    // quit command
    QUIT,
    // readF command
    READF,
    // writeF command
    WRITEF,
    // upload command
    UPLOAD,
    // error message or information message unknown operation
    UNKNOWN,
    // filename for client download file
    FILENAME,
    // download command
    DOWNLOAD,
    // upload ok is accepting upload request from server
    UPLOAD_OK,
    // just string message allowed
    STRING_MSG,
    // connection command
    CONNECT_REQ,
    // command end information about ending command from server side
    COMMAND_END,
    // downlaod ok is accepting download request
    DOWNLOAD_OK,
    // file content is byte which is file content
    FILE_CONTENT,
    // worker endpoint is worker fifo name
    WORKER_ENDPOINT,
    // try connect command
    TRY_CONNECT_REQ,
    // connection accepted information
    CONNECTION_ACCEPTED,
    // connection declined information
    CONNECTION_DECLINED
};

// message_t contains opcode and content
// type is operation content, content is parameter, message etc.
#define MSG_BUFFER_SIZE     1024 - sizeof(enum messageType)
struct message_t
{
    enum messageType type;
    char content[MSG_BUFFER_SIZE];
};

// some utility functions for server side
void respondEnd(int respfd);
void sendOneMsg(int respfd, const char* content);
void sendLine(int respfd, const char* str);
void sendStr(int respfd, const char* str);
void respondList(int respfd, const char* root, int clientLogFd);
void respondHelp(int respfd, struct message_t req, int clientLogFd);
void respondReadF(int respfd, struct message_t req, const char* root, int clientLogFd);
void respondWriteF(int respfd, struct message_t req, const char* root, int clientLogFd);
void respondUpload(int respfd, int workerFd, struct message_t req, const char* root, int clientLogFd);
void respondDowload(int respfd, int workerFd, struct message_t req, const char* root, int clientLogFd);
void listFilesAndDirectories(int respfd, const char* path, int level, int clientLogFd);
void clearFileContent(char** data, int len);
void char2DToFile(int fd, char** content, int size);

// some string generating functions
char* timeAsString();
char* initLogName(const char* name);
char** readFile(int fd_, int* lines);
char* readFileAs1D(int fd, size_t* _size);

// some utility functions for data structure
int getAndShiftInt(int* queue, int len);
int findIndex(pid_t* queue, int target);
int charCount(const char* string, char c, size_t size);
pid_t getAndShiftPID(pid_t* queue, int len);

// some utility function for client side
struct message_t prepareConnectionRequest(const char* connectionCommand);
struct message_t prepareCommand(const char* input);

#endif