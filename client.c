#include "utils.h"

int workerFd = -1;
char clientFifo[HALF_BUFFER];

void cleanupClient()
{
    if (FALSE !=  workerFd) {
        struct message_t quitMsg;
        memset(quitMsg.content, '\0', MSG_BUFFER_SIZE);
        quitMsg.type = QUIT;

        write(workerFd, &quitMsg,  sizeof(struct message_t));
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

struct message_t prepareCommand(const char* input)
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
    } else if (strcmp(command, "writeF") == 0) {
        commandRequest.type = WRITEF;
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

int main(int argc, char const *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "[ERROR] Usage: biboClient <Connect/tryConnect> ServerPID\n");
        exit(EXIT_FAILURE);
    }

    initSignals();

    sprintf(clientFifo, "CLIENT_%d", getpid());

    // create fifo file
    createFifo();

    // preparing connect request
    struct message_t connReq = prepareConnectionRequest(argv[1]);

    // initiliaze server fifo
    char serverFifo[HALF_BUFFER];
    sprintf(serverFifo, "SERVER_%d", atoi(argv[2]));

    // send connect request to server
    int serverFd = open(serverFifo, O_WRONLY);
    if (FALSE == serverFd) {
        perror("open server fifo");
        exit(EXIT_FAILURE);
    }
    write(serverFd, &connReq, sizeof(struct message_t));
    close(serverFd);

    // initiliaze client fifo
    int clientFd = open(clientFifo, O_RDONLY);
    if (FALSE == clientFd) {
        perror("open client fifo");
        exit(EXIT_FAILURE);
    }

    // dummy writer fifo for protecting read EOF from fifo
    int dummyFd = open(clientFifo, O_WRONLY);
    if (FALSE == dummyFd) {
        perror("open dummy client fifo");
        exit(EXIT_FAILURE);
    }

    // receive response from server about connection status (accepted or declined)
    struct message_t response;
    fprintf(stdout, "Waiting for Que..\n");
    read(clientFd, &response, sizeof(struct message_t));
    if (response.type == CONNECTION_DECLINED) {
        fprintf(stdout, "Connection declined...\n");
        exit(EXIT_SUCCESS);
    }

    // get worker address to connect worker's fifo
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

    // send request to worker and receive response
    for (;;) {

        // take command from terminal
        char userInput[HALF_BUFFER];
        memset(userInput, '\0', HALF_BUFFER);
        fprintf(stdout, "Enter comment: ");
        fgets(userInput, HALF_BUFFER, stdin); 

        // prepare and send request to worker
        struct message_t command = prepareCommand(userInput);
        if (command.type == DOWNLOAD) {
            write(workerFd, &command, sizeof(struct message_t));
            // TODO: download byte with chunk
        } else if (command.type == UPLOAD) {
            write(workerFd, &command, sizeof(struct message_t));
            // TODO: server ready
            // TODO: send byte with chunk
        } else {
            write(workerFd, &command, sizeof(struct message_t));
        }
        
        // retrieve responses from worker
        do {    
            read(clientFd, &response, sizeof(struct message_t));
            fprintf(stdout, "%s", response.content);
        } while (response.type != COMMAND_END);

        // if the  server send OK message and my request quit or kill
        // then client exiting
        if (command.type == QUIT) {
            // TODO: Retrieve log file
            workerFd = FALSE;
            exit(EXIT_SUCCESS);
        } else if (command.type == KILL) {
            workerFd = FALSE;
            exit(EXIT_SUCCESS);
        }
    }
    return 0;
}