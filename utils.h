#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
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

#define CLIENT_QUEUE_MEM    "CLIENT_QUEUE_MEM"

#define SHARED_MEM_SIZE     10000*sizeof(int)

#define PROT_READ_WRITE PROT_READ | PROT_WRITE
#define USR_READ_WRITE S_IRUSR | S_IWUSR

#define REQUEST_BUFFER_SIZE 25600
#define LNG_BUFFER_SIZE 2048
#define BUFFER_SIZE 1024
#define HALF_BUFFER 512

#define FALSE -1
#define TRUE 0

int char_count(const char* string, char c, size_t size);
char* time_as_string();
char** read_file(int fd_, int* lines, const int lock);

#endif