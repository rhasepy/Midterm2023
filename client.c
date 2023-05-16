#include "utils.h"

int serverFd = -1;
int workerFd = -1;
int clientFd = -1;
char clientFifo[HALF_BUFFER];

// clean client 
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

// sigint handler for client
void sigintHandler()
{
    fprintf(stdout, "Server killed!\n");
    exit(1);
}

// init signal handler
void initSignals()
{
    struct sigaction sigintAction;
    memset(&sigintAction, 0, sizeof(sigintAction));
    sigintAction.sa_handler = &sigintHandler;
    sigaction(SIGINT, &sigintAction, NULL);
}

// create client fifo
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

// initiliaze client arguments from user
void initClientApp(int argc, char const *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "[ERROR] Usage: biboClient <Connect/tryConnect> ServerPID\n");
        exit(EXIT_FAILURE);
    }

    initSignals();

    sprintf(clientFifo, "CLIENT_%d", getpid());

    // create fifo file
    createFifo();
}

// send connection to server
void connectToServer(char const *argv[])
{
    // preparing connect request
    struct message_t connReq = prepareConnectionRequest(argv[1]);

    // initiliaze server fifo
    char serverFifo[HALF_BUFFER];
    sprintf(serverFifo, "SERVER_%d", atoi(argv[2]));

    // send connect request to server
    serverFd = open(serverFifo, O_WRONLY);
    if (FALSE == serverFd) {
        perror("open server fifo");
        exit(EXIT_FAILURE);
    }
    write(serverFd, &connReq, sizeof(struct message_t));
    close(serverFd);
}

// initiliaze client fifo open dummy writer side to prevent sending EOF to fifo from kernel
void initClientFifo()
{
    // initiliaze client fifo
    clientFd = open(clientFifo, O_RDONLY);
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
}

// get server connection
void getConnection()
{
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
}

// client app activity
void runClientApp()
{
    struct message_t response;
    memset(response.content, '\0', MSG_BUFFER_SIZE);

    // send request to worker and receive response
    for (;;) {
        // take command from terminal
        char userInput[HALF_BUFFER];
        memset(userInput, '\0', HALF_BUFFER);
        fprintf(stdout, "Enter comment: ");
        fgets(userInput, HALF_BUFFER, stdin);
        userInput[strcspn(userInput, "\n")] = '\0';

        // retrieve responses from worker
        int downloadedFd = -1;
        int uploadedFd = -1;

        // prepare and send request to worker
        struct message_t command = prepareCommand(userInput);
        if (command.type == UPLOAD) {
            uploadedFd = open(command.content, O_RDONLY);
            if (FALSE == uploadedFd) {
                perror("File open error");
                continue;
            }
        }
        write(workerFd, &command, sizeof(struct message_t));

        // is command filter about exiting client
        if (command.type == KILL) {
            exit(EXIT_SUCCESS);
        } else if (command.type == QUIT) {
            fprintf(stdout, "waiting for logfile ...\n");
            read(clientFd, &response, sizeof(struct message_t));

            fprintf(stdout, "logfile write request granted\n");
            fprintf(stdout, "bye..\n");
            workerFd = FALSE;
            exit(EXIT_SUCCESS);
        }

        do {
            memset(response.content, '\0', MSG_BUFFER_SIZE);
            read(clientFd, &response, sizeof(struct message_t));

            // if the command type is file name then download file is available. create file
            if (response.type == FILENAME) {

                // create file
                downloadedFd = open(response.content, O_WRONLY | O_CREAT | O_TRUNC, 0777);

                // if the can not create file
                if (FALSE == downloadedFd) {
                    command.type = UNKNOWN;
                    sprintf(command.content, "Error create file client %s\n", strerror(errno));
                    write(workerFd, &command, sizeof(struct message_t));
                }

                // if the create file success then this information sending server
                command.type = DOWNLOAD_OK;
                sprintf(command.content, "Send me\n");
                write(workerFd, &command, sizeof(struct message_t));

            // if the command type is file content then write byte to downloading file 
            } else if (response.type == FILE_CONTENT) {
                unsigned char c = (unsigned char) response.content[0];
                write(downloadedFd, &c, 1);

            // if the command type is upload ok then server allow uploading operation do it
            } else if (response.type == UPLOAD_OK) {

                lseek(uploadedFd, 0, SEEK_SET);
                size_t size = lseek(uploadedFd, 0, SEEK_END);
                lseek(uploadedFd, 0, SEEK_SET);
                for (int i = 0; i < size; ++i) {
                    unsigned char c;
                    read(uploadedFd, &c, sizeof(unsigned char));
                    response.type = FILE_CONTENT;
                    memset(response.content, '\0',  MSG_BUFFER_SIZE);
                    response.content[0] = c;
                    response.content[1] = '\0';
                    write(workerFd, &response, sizeof(struct message_t));
                }
                sendOneMsg(workerFd, "Transfer finished\n");

                memset(response.content, '\0', MSG_BUFFER_SIZE);
                read(clientFd, &response, sizeof(struct message_t));
                fprintf(stdout, "%s", response.content);

            // else is error or information message write to stdout
            } else {
                fprintf(stdout, "%s", response.content);
            }
            // if the retrieve command end then command finished, break loop and read input from user
        } while (response.type != COMMAND_END);
    }
}

int main(int argc, char const *argv[])
{
    initClientApp(argc, argv);

    connectToServer(argv);
    initClientFifo();

    getConnection();

    runClientApp();
    return 0;
}