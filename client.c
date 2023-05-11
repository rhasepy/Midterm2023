#include "utils.h"

struct message_t response;
struct message_t command;
char clientFifo[HALF_BUFFER];
int workerFd = -1;

void cleanupClient()
{
    if (FALSE != workerFd) {
        command.type = QUIT;
        write(workerFd, &command, sizeof(struct message_t));
    }

    unlink(clientFifo);
}

void sigintHandler()
{
    exit(1);
}

void initSignals()
{
    struct sigaction sigintAction;
    memset(&sigintAction, 0, sizeof(sigintAction));
    sigintAction.sa_handler = &sigintHandler;
    sigaction(SIGINT, &sigintAction, NULL);
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

    initSignals();

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

    // Get connection accept or not
    fprintf(stdout, "Waiting for Que..\n");
    read(clientFd, &response, sizeof(struct message_t));
    if (response.type == CONNECTION_DECLINED) {
        fprintf(stdout, "Connection declined...\n");
        exit(EXIT_SUCCESS);
    }

    // Get worker address to connect worker's fifo
    read(clientFd, &response, sizeof(struct message_t));
    if (response.type == WORKER_ENDPOINT) {
        workerFd = open(response.content, O_WRONLY);
        if (FALSE == workerFd) {
            perror("invalid worker endpoint");
            exit(EXIT_FAILURE);
        }
        fprintf(stdout, "Connection established:\n");
    } else {
        perror("client do not know worker end point");
        exit(EXIT_FAILURE);
    }

    for (;;) {
        fprintf(stdout, "Enter comment: ");
        scanf("%s", command.content);
        if (strcmp(command.content, "quit") == 0) {
            exit(EXIT_SUCCESS);
        } else {
            command.type = COMMAND_START;
        }
        write(workerFd, &command, sizeof(struct message_t));

        do {
            read(clientFd, &response, sizeof(struct message_t));
            fprintf(stdout, "%s", response.content);
        } while (response.type != COMMAND_END);
    }

    return 0;
}