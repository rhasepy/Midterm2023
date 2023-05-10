#include "utils.h"

char clientFifo[HALF_BUFFER];

void cleanupClient()
{
    unlink(clientFifo);
}

void createFifo()
{
    umask(0);
    
    if (FALSE == mkfifo(clientFifo, USR_READ_WRITE) && errno != EEXIST) {
        perror("[ERROR] client can not create fifo...");
        exit(EXIT_FAILURE);
    }

    if (FALSE == atexit(cleanupClient)) {
        perror("[ERROR] atexit on client...");
        exit(EXIT_FAILURE);
    }
}

struct message_t prepareConnectionRequest(const char* connectionCommand)
{
    struct message_t connReq;
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

int main(int argc, char const *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "[ERROR] Usage: biboClient <Connect/tryConnect> ServerPID\n");
        exit(EXIT_FAILURE);
    }

    sprintf(clientFifo, "CLIENT_%d", getpid());
    createFifo();

    struct message_t connReq = prepareConnectionRequest(argv[1]);

    char serverFifo[HALF_BUFFER];
    sprintf(serverFifo, "SERVER_%d", atoi(argv[2]));

    int serverFd = open(serverFifo, O_WRONLY);
    if (FALSE == serverFd) {
        perror("open server fifo");
        exit(EXIT_FAILURE);
    }
    write(serverFd, &connReq, sizeof(struct message_t));

    int clientFd = open(clientFifo, O_RDONLY);
    if (FALSE == clientFd) {
        perror("open client fifo");
        exit(EXIT_FAILURE);
    }

    struct message_t resp;
    read(clientFd, &resp, sizeof(struct message_t));

    fprintf(stdout, "Message Type: %d, Message Content: %s\n", resp.type, resp.content);

    return 0;
}