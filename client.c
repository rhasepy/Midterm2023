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

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("Sigpipe ignoring error...\n");
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
    close(serverFd);

    int clientFd = open(clientFifo, O_RDONLY);
    if (FALSE == clientFd) {
        perror("open client fifo");
        exit(EXIT_FAILURE);
    }

    int dummyFd = open(clientFifo, O_WRONLY);
    if (FALSE == dummyFd) {
        perror("open dummy client fifo");
        exit(EXIT_FAILURE);
    }

    struct message_t resp;
    read(clientFd, &resp, sizeof(struct message_t));
    fprintf(stdout, "Message Type: %d, Message Content: %s\n", resp.type, resp.content);
    
    read(clientFd, &resp, sizeof(struct message_t));
    fprintf(stdout, "Message Type: %d, Message Content: %s\n", resp.type, resp.content);

    int workerFd;
    if (resp.type == WORKER_ENDPOINT) {
        workerFd = open(resp.content, O_WRONLY);
        if (FALSE == workerFd) {
            perror("invalid worker endpoint");
            exit(EXIT_FAILURE);
        }
    } else {
        perror("client do not know worker end point");
        exit(EXIT_FAILURE);
    }

    struct message_t command;
    scanf("%s", command.content);
    write(workerFd, &command, sizeof(struct message_t));

    sleep(2);

    // Loop:
        // TODO: Open Worker fifo for write
        // TODO: Write Command
        // TODO: Take Response and do it for type

    return 0;
}