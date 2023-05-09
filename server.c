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
    ftruncate(shmFd, clientCapacity * sizeof(int) * 2);
    
    clientQueue = mmap(NULL, clientCapacity * sizeof(int) * 2, PROT_READ_WRITE, MAP_SHARED, shmFd, 0);
    if (shmFd == FALSE) {
        perror("[ERROR] Shared Mem mmap");
        exit(1);
    }

    for (int i = 0; i < clientCapacity; ++i) {
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

int main(int argc, char const *argv[])
{   
    atexit(cleanupServer);

    initServerArgs(argc, argv);

    initSignals();

    initSharedMemories();

    initSemaphores();

    initWorkers();

    waitAllWorkers();

    return 0;
}

int WorkerMain()
{
    for (int i = 0; i < clientCapacity; ++i) {
        fprintf(stdout, "%d", clientQueue[i]);
    }
    fprintf(stdout, "\n");
    return EXIT_SUCCESS;
}