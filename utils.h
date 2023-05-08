#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#define PROT_READ_WRITE PROT_READ | PROT_WRITE
#define USR_READ_WRITE S_IRUSR | S_IWUSR

#define REQUEST_BUFFER_SIZE 25600
#define LNG_BUFFER_SIZE 2048
#define BUFFER_SIZE 1024
#define HALF_BUFFER 512

#define FALSE -1
#define TRUE 0

// File Operations
int char_count(const char* string, char c, size_t size);
char* time_as_string();
char** read_file(int fd_, int* lines, const int lock);

#endif