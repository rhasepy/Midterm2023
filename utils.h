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

#define FULL_SEM            "FULL_SEM"
#define EMPTY_SEM           "EMPTY_SEM"
#define MUTEX_SEM           "MUTEX_SEM"
#define AV_WRK_SEM          "AV_WRK_SEM"

#define CLIENT_QUEUE_MEM    "CLIENT_QUEUE_MEM"
#define CLIENT_ID_QUEUE     "CLIENT_ID_QUEUE_MEM"
#define AV_WKR_MEM          "AV_WRK_MEM"

#define SHARED_MEM_SIZE     40000 * sizeof(pid_t)

#define PROT_READ_WRITE     PROT_READ | PROT_WRITE
#define USR_READ_WRITE      S_IRUSR | S_IWUSR

#define REQUEST_BUFFER_SIZE 25600
#define LNG_BUFFER_SIZE     2048
#define BUFFER_SIZE         1024
#define HALF_BUFFER         512
#define SMALL_BUFFER        64

#define FALSE               -1
#define TRUE                 0

enum messageType
{
    HELP,
    LIST,
    KILL,
    QUIT,
    READF,
    WRITEF,
    UPLOAD,
    UNKNOWN,
    DOWNLOAD,
    STRING_MSG,
    CONNECT_REQ,
    COMMAND_END,
    WORKER_ENDPOINT,
    TRY_CONNECT_REQ,
    CONNECTION_ACCEPTED,
    CONNECTION_DECLINED
};

#define MSG_BUFFER_SIZE     1024 - sizeof(enum messageType)
struct message_t
{
    enum messageType type;
    char content[MSG_BUFFER_SIZE];
};

void respondList(int respfd, const char* root);
void respondReadF(int respfd, struct message_t req);
void respondWriteF(int respfd, struct message_t req);

// TODO: will change
void respondUpload(int respfd, struct message_t req);
// TODO: will change
void respondDowload(int respfd, struct message_t req);

void respondHelp(int respfd, struct message_t req);
void respondEnd(int respfd);
void respondUnknown(int respfd);

int char_count(const char* string, char c, size_t size);
char* time_as_string();
char** read_file(int fd_, int* lines, const int lock);
pid_t getAndShiftPID(pid_t* queue, int len);
int getAndShiftInt(int* queue, int len);
int findIndex(pid_t* queue, int target);

#endif