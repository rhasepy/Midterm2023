#include "utils.h"

// ####################### -- ####################### -- ####################### -- ####################### //
// Server configuration fileds
char workingDirectory[HALF_BUFFER];
int clientCapacity = 0;

// Worker process pids
pid_t* workerPool = NULL;

// -- Shared Memory Part -- //
// Client id list
int shmFd = -1;
pid_t* clientQueue = NULL;
// How much worker available
int* availableWorker = NULL;

// -- Semaphore Part -- //
// Client queue mutual exclusion tools between worker and server
sem_t* emptySem;
sem_t* fullSem;
sem_t* mutexSem;
sem_t* mutexAvailableWorker;

// Worker main function
int WorkerMain();
// ####################### -- ####################### -- ####################### -- ####################### //

void cleanupServer()
{
    if (workerPool != NULL) {
        free(workerPool);
    }

    sem_close(emptySem);
    sem_close(fullSem);
    sem_close(mutexSem);
    sem_close(mutexAvailableWorker);

    sem_unlink(EMPTY_SEM);
    sem_unlink(FULL_SEM);
    sem_unlink(MUTEX_SEM);
    sem_unlink(AV_WRK_SEM);

    shm_unlink(CLIENT_QUEUE_MEM);
    shm_unlink(AV_WKR_MEM);
}

void waitAllWorkers()
{
    for (int i = 0; i < clientCapacity; ++i) {

        int status;
        waitpid(workerPool[i], &status, 0);
    }
}

// TODO: Signal Handler will be implement
void sigintHandler()
{
    exit(1);
}

// TODO: Check directory exist or not
void initServerArgs(int argc, char const *argv[])
{
    if (argc != 3) {
       fprintf(stderr, "[ERROR] Usage must be -> ./biboServer <dirname> <max. #ofClients>\n");
    	_exit(EXIT_FAILURE);
    }

    sprintf(workingDirectory, "%s", argv[1]);
    // todo implementation here

    clientCapacity = atoi(argv[2]);
    if (clientCapacity == 0) {
        fprintf(stderr, "[ERROR] Please give client capacity positive integer.\n");
        _exit(EXIT_FAILURE);
    }
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
            int status = WorkerMain();
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

void serverActivity()
{
    while (1)
    {
        pid_t asAclientInput;
        scanf("%d", &asAclientInput);

        sem_wait(mutexAvailableWorker);
        if (*availableWorker == 0) {
            fprintf(stdout, "Workers Busy!\n");
            sem_post(mutexAvailableWorker);
            continue;
        }
        sem_post(mutexAvailableWorker);

        sem_wait(emptySem);
        sem_wait(mutexSem);
        
        int queueDelimetor = getQueueDelimetor(clientQueue, -1);
        clientQueue[queueDelimetor++] = asAclientInput;
        
        sem_post(mutexSem);
        sem_post(fullSem);
    }
}

int main(int argc, char const *argv[])
{   
    atexit(cleanupServer);

    initServerArgs(argc, argv);

    initSignals();

    initSharedMemories();

    initSemaphores();

    initWorkers();

    serverActivity();

    waitAllWorkers();

    return 0;
}

int WorkerMain()
{
    while (1)
    {
        // Begining of Synchronization Block
        sem_wait(fullSem);
        sem_wait(mutexSem);

        // Consume client pid for open fifo
        pid_t clientID = getClientQueue(clientQueue, clientCapacity);
        
        sem_wait(mutexAvailableWorker);
        *availableWorker = (*availableWorker) - 1;
        sem_post(mutexAvailableWorker);

        sem_post(mutexSem);
        sem_post(emptySem);
        // End of Synchronization Block

        char fifoName[HALF_BUFFER];
        memset(fifoName, '\0', HALF_BUFFER);
        sprintf(fifoName, "ClientFifo_%d", clientID);
        fprintf(stdout, "%s\n",  fifoName);

        // TODO: Open fifo and handle response
        sleep(10);
        fprintf(stdout, "Finished\n");
        sem_wait(mutexAvailableWorker);
        *availableWorker = (*availableWorker) + 1;
        sem_post(mutexAvailableWorker);
    }
    return EXIT_SUCCESS;
}