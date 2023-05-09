#include "utils.h"

// ####################### -- ####################### -- ####################### -- ####################### //
// Server configuration fileds
char workingDirectory[HALF_BUFFER];
int clientCapacity = 0;

// Worker process pids
pid_t* workerPool = NULL;

// Client id list
int shmFd = -1;
int* clientQueue = NULL;

// Client queue mutual exclusion tools between worker and server
sem_t* emptySem;
sem_t* fullSem;
sem_t* mutexSem;

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

    sem_unlink(EMPTY_SEM);
    sem_unlink(FULL_SEM);
    sem_unlink(MUTEX_SEM);

    shm_unlink(CLIENT_QUEUE_MEM);
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
    emptySem = sem_open(EMPTY_SEM, O_CREAT, USR_READ_WRITE, clientCapacity);
    fullSem = sem_open(FULL_SEM, O_CREAT, USR_READ_WRITE, 0);
    mutexSem = sem_open(MUTEX_SEM, O_CREAT, USR_READ_WRITE, 1);
}

void initSharedMemories()
{
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

    for (int i = 0; i < SHARED_MEM_SIZE / sizeof(int); ++i) {
        clientQueue[i] = -1;
    }
}

void initSignals()
{
    struct sigaction sigintAction;
    memset(&sigintAction, 0, sizeof(sigintAction));
    sigintAction.sa_handler = &sigintHandler;
    sigaction(SIGINT, &sigintAction, NULL);
}

int getQueueDelimetor()
{
    for (int i = 0; i < SHARED_MEM_SIZE / sizeof(int); ++i) {
        if (clientQueue[i] == -1) {
            return i;
        }
    }
    return 0;
}

void serverActivity()
{
    int clientIdGenerator = 0;

    while (1)
    {
        // TODO: How much worker available
        // TODO: Command connect or not check available workers (mutual excluision)
        int a;
        scanf("%d", &a);

        sem_wait(emptySem);
        sem_wait(mutexSem);
        
        int queueDelimetor = getQueueDelimetor();
        clientQueue[queueDelimetor++] = ++clientIdGenerator;
        
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
        sem_wait(fullSem);
        sem_wait(mutexSem);

        int clientID = clientQueue[0];
        fprintf(stdout, "Client Id: %d\n", clientID);

        // Consume client id from client queue (shifting)
        for (int i = 0; i < clientCapacity - 1; ++i) {
            clientQueue[i] = clientQueue[i + 1];
        }
        clientQueue[clientCapacity - 1] = -1;

        sem_post(mutexSem);
        sem_post(emptySem);

        // TODO: open fifo with client ID
        sleep(100000);
    }
    return EXIT_SUCCESS;
}