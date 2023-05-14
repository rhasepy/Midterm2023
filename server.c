#include "utils.h"

// ####################### -- ####################### -- ####################### -- ####################### //
// -- Server Fields -- //
int serverFlag = TRUE;
int clientCapacity = 0;
char fifoName[HALF_BUFFER];
char workingDirectory[SMALL_BUFFER];

int isSigint = FALSE;

// -- Worker Processes -- //
pid_t* workerPool = NULL;
pid_t parentPid;

// -- Shared Memory Part -- //
int shmFd = -1;
// TODO: ADDED int* for the client number pair number and pid
pid_t* clientQueue = NULL;
int* clientIdQueue = NULL;
int* availableWorker = NULL;

// -- Semaphore Part -- //
sem_t* fullSem;
sem_t* emptySem;
sem_t* mutexSem;
sem_t* mutexAvailableWorker;

// -- Worker Part -- //
int WorkerMain(int WorkerID);
// ####################### -- ####################### -- ####################### -- ####################### //

void waitAllWorkers()
{
    for (int i = 0; i < clientCapacity; ++i) {
        int status;
        waitpid(workerPool[i], &status, 0);
    }
}

void cleanupServer()
{
    for (int i = 0; i < clientCapacity; ++i)
        kill(workerPool[i], SIGINT);

    waitAllWorkers();

    sem_close(emptySem);
    sem_close(fullSem);
    sem_close(mutexSem);
    sem_close(mutexAvailableWorker);

    sem_unlink(EMPTY_SEM);
    sem_unlink(FULL_SEM);
    sem_unlink(MUTEX_SEM);
    sem_unlink(AV_WRK_SEM);

    shm_unlink(CLIENT_QUEUE_MEM);
    shm_unlink(CLIENT_ID_QUEUE);
    shm_unlink(AV_WKR_MEM);

    if (TRUE == serverFlag) {
        unlink(fifoName);
    }

    if (workerPool != NULL) {
        free(workerPool);
    }

    for (int i = 0; i < clientCapacity; ++i) {
        char workerFifoName[HALF_BUFFER];
        sprintf(workerFifoName, "WORKER_%d", i);
        unlink(workerFifoName);
    }
}

void sigintHandler()
{
    if (isSigint == FALSE) {
        isSigint = TRUE;
        exit(1);
    }  
}

void initServerArgs(int argc, char const *argv[])
{
    if (argc != 3) {
       fprintf(stderr, "[ERROR] Usage must be -> ./biboServer <dirname> <max. #ofClients>\n");
    	_exit(EXIT_FAILURE);
    }

    sprintf(workingDirectory, "%s/", argv[1]);

    struct stat st = {0};
    if (FALSE == stat(workingDirectory, &st)) {
        int result = mkdir(workingDirectory, 0777);
        if (FALSE == result) {
            perror("error creating directory");
            exit(EXIT_FAILURE);
        }
    }

    clientCapacity = atoi(argv[2]);
    if (clientCapacity == 0) {
        fprintf(stderr, "[ERROR] Please give client capacity positive integer.\n");
        _exit(EXIT_FAILURE);
    }
}

void killServer()
{
    kill(parentPid, SIGINT);
}

// Worker side function to create worker fifo
int createWorkerFifos(int WorkerID)
{
    char workerFifoName[HALF_BUFFER];
    memset(workerFifoName, '\0', HALF_BUFFER);
    sprintf(workerFifoName, "WORKER_%d", WorkerID);

    umask(0);
    if (FALSE == mkfifo(workerFifoName, USR_READ_WRITE) && errno != EEXIST) {
        perror("Fifo can not create...");
        exit(EXIT_FAILURE);
    }

    int workerFd = open(workerFifoName, O_RDONLY);
    if (FALSE == workerFd) {
        perror("Fifo can not open...");
        exit(EXIT_FAILURE);
    }

    // Dummy writer side opened from server
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("Sigpipe ignoring error...\n");
        exit(EXIT_FAILURE);
    }

    return workerFd;
}

void initWorkers()
{
    workerPool = (pid_t*) calloc(clientCapacity, sizeof(pid_t));

    for (int i = 0; i < clientCapacity; ++i) {
        
        workerPool[i] = fork();

        // Fork failing
        if (workerPool[i] == -1) {
            perror("Fork failed");

        // Start children process
        } else if (workerPool[i] == 0) {
            int status = WorkerMain(i);
            exit(status);
        }
    }
}

void initSemaphores()
{
    emptySem = sem_open(EMPTY_SEM, O_CREAT, USR_READ_WRITE, SHARED_MEM_SIZE / sizeof(pid_t));
    fullSem = sem_open(FULL_SEM, O_CREAT, USR_READ_WRITE, 0);
    mutexSem = sem_open(MUTEX_SEM, O_CREAT, USR_READ_WRITE, 1);

    mutexAvailableWorker = sem_open(AV_WRK_SEM, O_CREAT, USR_READ_WRITE, 1);
}

void initSharedMemories()
{
    // Init client queue
    shmFd = shm_open(CLIENT_QUEUE_MEM, O_CREAT | O_RDWR, USR_READ_WRITE);
    if (shmFd == FALSE) {
        perror("[ERROR] Shared Mem Create");
        exit(1);
    }
    ftruncate(shmFd, SHARED_MEM_SIZE);
    
    clientQueue = mmap(NULL, SHARED_MEM_SIZE, PROT_READ_WRITE, MAP_SHARED, shmFd, 0);
    if (shmFd == FALSE) {
        perror("[ERROR] Shared Mem mmap");
        exit(1);
    }

    for (int i = 0; i < SHARED_MEM_SIZE / sizeof(pid_t); ++i) {
        clientQueue[i] = -1;
    }
    // Init client queue

    // Init client queue for id
    shmFd = shm_open(CLIENT_ID_QUEUE, O_CREAT | O_RDWR, USR_READ_WRITE);
    if (shmFd == FALSE) {
        perror("[ERROR] Shared Mem Create");
        exit(1);
    }
    ftruncate(shmFd, SHARED_MEM_SIZE);
    
    clientIdQueue = mmap(NULL, SHARED_MEM_SIZE, PROT_READ_WRITE, MAP_SHARED, shmFd, 0);
    if (shmFd == FALSE) {
        perror("[ERROR] Shared Mem mmap");
        exit(1);
    }

    for (int i = 0; i < SHARED_MEM_SIZE / sizeof(int); ++i) {
        clientIdQueue[i] = -1;
    }
    // Init client queue for id

    // Init worker counter
    shmFd = shm_open(AV_WKR_MEM, O_CREAT | O_RDWR, USR_READ_WRITE);
    if (shmFd == FALSE) {
        perror("[ERROR] Shared Mem Create");
        exit(1);
    }
    ftruncate(shmFd, sizeof(int));
    
    availableWorker = mmap(NULL, sizeof(int), PROT_READ_WRITE, MAP_SHARED, shmFd, 0);
    if (shmFd == FALSE) {
        perror("[ERROR] Shared Mem mmap");
        exit(1);
    }
    *availableWorker  = clientCapacity;
    // Init worker counter
}

void initSignals()
{
    struct sigaction sigintAction;
    memset(&sigintAction, 0, sizeof(sigintAction));
    sigintAction.sa_handler = &sigintHandler;
    sigaction(SIGINT, &sigintAction, NULL);
}

int createServerFifo()
{
    umask(0);
    memset(fifoName, '\0', HALF_BUFFER);
    sprintf(fifoName, "SERVER_%d", getpid());

    if (FALSE == mkfifo(fifoName, USR_READ_WRITE) && errno != EEXIST) {
        perror("Fifo can not create...");
        exit(EXIT_FAILURE);
    }
    
    fprintf(stdout, "Server Started PID %d...\n", getpid());
    fprintf(stdout, "waiting for clients...\n");

    int returnFd = open(fifoName, O_RDONLY);
    if (FALSE == returnFd) {
        perror("Fifo can not open...");
        exit(EXIT_FAILURE);
    }

    int dummyFd = open(fifoName, O_WRONLY);
    if (FALSE == dummyFd) {
        perror("Fifo can not open for writer dummy...");
        exit(EXIT_FAILURE);
    }

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("Sigpipe ignoring error...\n");
        exit(EXIT_FAILURE);
    }

    return returnFd;
}

int openClientResponseFifo(pid_t clientPid)
{
    char clientEndPoint[HALF_BUFFER];
    sprintf(clientEndPoint, "CLIENT_%d", clientPid);
    
    int clientFd = open(clientEndPoint, O_WRONLY);
    return clientFd;
}

void openWorkerFifosDummy()
{
    for (int i = 0; i < clientCapacity; ++i) {
        char tempFifoName[HALF_BUFFER];
        sprintf(tempFifoName, "WORKER_%d", i);

        int dummyFd = open(tempFifoName, O_WRONLY);
        if (FALSE == dummyFd) {
            perror("server open fifo");
            exit(EXIT_FAILURE);
        }
    } 
}

void serverActivity()
{
    int fifoFd = createServerFifo();

    openWorkerFifosDummy();

    int clientId = 1;
    while (1)
    {
        struct message_t msg;
        if (read(fifoFd, &msg, sizeof(struct message_t)) != sizeof(struct message_t)) {
            fprintf(stderr, "[ERROR] Request invalid");
            continue;
        }

        pid_t clientPid = atoi(msg.content);
        char clientFifo[HALF_BUFFER];
        sprintf(clientFifo, "CLIENT_%d", clientPid);

        int clientFd = open(clientFifo, O_WRONLY);
        if (FALSE == clientFd) {
            continue;
        }

        sem_wait(mutexAvailableWorker);
        if (*availableWorker == 0) {

            fprintf(stdout, "Connection request PID %d... Que FULL\n", clientPid);

            if (msg.type == TRY_CONNECT_REQ) {

                struct message_t resp;
                resp.type = CONNECTION_DECLINED;
                memset(resp.content, '\0', MSG_BUFFER_SIZE);
                write(clientFd, &resp, sizeof(struct message_t));
                close(clientFd);

                sem_post(mutexAvailableWorker);
                continue;

            } else if (msg.type == CONNECT_REQ) {
                
                struct message_t resp;
                resp.type = CONNECTION_ACCEPTED;
                memset(resp.content, '\0', MSG_BUFFER_SIZE);
                write(clientFd, &resp, sizeof(struct message_t));
                close(clientFd);
                
            } else {
                
                struct message_t resp;
                resp.type = UNKNOWN;
                sprintf(resp.content, "Please send connect or try connect command!\n");
                write(clientFd, &resp, sizeof(struct message_t));
                close(clientFd);

                sem_post(mutexAvailableWorker);
                continue;
            }

        } else {
            struct message_t resp;
            memset(resp.content, '\0', MSG_BUFFER_SIZE);
            if (msg.type == TRY_CONNECT_REQ || msg.type == CONNECT_REQ) {
                resp.type = CONNECTION_ACCEPTED;
            } else {
                resp.type = UNKNOWN; 
                sprintf(resp.content, "Please send connect or try connect command!\n"); 
            }
            write(clientFd, &resp, sizeof(struct message_t));
            close(clientFd);
        }
        sem_post(mutexAvailableWorker);

        // Enqueued client synchronized block
        sem_wait(emptySem);
        sem_wait(mutexSem);
        int queueDelimetor = findIndex(clientQueue, -1);
        clientQueue[queueDelimetor] = clientPid;
        clientIdQueue[queueDelimetor++] = clientId++;
        sem_post(mutexSem);
        sem_post(fullSem);
    }
}

int sendResponse(int fd, int workerFd, struct message_t request)
{
    if (request.type == HELP) {
        respondHelp(fd, request);
    } else if (request.type == LIST) {
        respondList(fd, workingDirectory);
    } else if (request.type == READF) {
        respondReadF(fd, request, workingDirectory);
    }  else if (request.type == WRITEF)  {
        respondWriteF(fd, request, workingDirectory);
    } else if (request.type == UPLOAD) {
        respondUpload(fd, workerFd, request, workingDirectory);
    } else if (request.type ==  DOWNLOAD) {
        respondDowload(fd, workerFd, request, workingDirectory);
    } else if (request.type == QUIT) {
        respondEnd(fd);
        return FALSE;
    } else if (request.type == KILL) {
        killServer();
        return FALSE;
    } else {
        sendOneMsg(fd, "Unknown request...\n");
    }

    return TRUE;
}

int main(int argc, char const *argv[])
{   
    atexit(cleanupServer);

    parentPid = getpid();

    initServerArgs(argc, argv);
    initSignals();
    initSharedMemories();
    initSemaphores();
    initWorkers();

    serverActivity();
    waitAllWorkers();

    return 0;
}

int WorkerMain(int WorkerID)
{
    serverFlag = FALSE;

    int workerFd = createWorkerFifos(WorkerID);
    while (1)
    {
        // Begining of Synchronization Block
        sem_wait(fullSem);
        sem_wait(mutexSem);

        // Consume client pid for open fifo
        pid_t clientPid = getAndShiftPID(clientQueue, clientCapacity);
        int clientId = getAndShiftInt(clientIdQueue, clientCapacity);

        // Server knows information about "how much available worker?" thanks to this shared memory
        sem_wait(mutexAvailableWorker);
        *availableWorker = (*availableWorker) - 1;
        sem_post(mutexAvailableWorker);

        sem_post(mutexSem);
        sem_post(emptySem);
        // End of Synchronization Block

        // Open client fifo to send response
        int clientFd = openClientResponseFifo(clientPid);
        if (FALSE == clientFd) {
            fprintf(stderr, "[WORKER ERROR] Client does not exist!\n");
            continue;
        } else {
            fprintf(stdout, "Client PID %d connected as \"client%d\"\n", clientPid, clientId);
        }

        // Sending worker fifo domain for client connect to worker about sending request
        struct message_t response, request;
        memset(response.content, '\0', MSG_BUFFER_SIZE);
        response.type = WORKER_ENDPOINT;
        sprintf(response.content, "WORKER_%d", WorkerID);
        write(clientFd, &response, sizeof(struct message_t));

        // Receive client request and process request
        do {
            // Receive client request
            read(workerFd, &request, sizeof(struct message_t));
            fprintf(stdout, "%d - %s\n", request.type, request.content);

            // Do client request and send response
        } while (FALSE != sendResponse(clientFd, workerFd, request));
        fprintf(stdout, "client%d diconnected..\n", clientId);

        // End of worker task
        sem_wait(mutexAvailableWorker);
        *availableWorker = (*availableWorker) + 1;
        sem_post(mutexAvailableWorker);
    }
    return EXIT_SUCCESS;
}