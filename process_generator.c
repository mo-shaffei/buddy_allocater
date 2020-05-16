#include "Headers/headers.h"
#include "Headers/ProcessQueue.h"
#include "Headers/MessageBuffer.h"
#include <string.h>
#include "math.h"

void ClearResources(int);

void ReadFile();

void InitIPC();

void ExecuteClock();

void ExecuteScheduler();

void SendProcess(Process *);

queue gProcessQueue;
int gMsgQueueId = 0;
pid_t gClockPid = 0;
pid_t gSchedulerPid = 0;

int main(int argc, char *argv[]) {
    //initialize the process queue
    gProcessQueue = NewProcQueue();
    //catch SIGINT
    signal(SIGINT, ClearResources);
    // 1. Read the input files.
    ReadFile();
    //initialize the IPC
    InitIPC();
    // 3. Initiate and create the scheduler and clock processes.
    ExecuteScheduler();
    ExecuteClock();
    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    /*while (!ProcQueueEmpty(gProcessQueue)) {
        //get current time
        int current_time = getClk();
        //temporary process pointer
        Process *pTempProcess;
        //peek the processes queue
        ProcPeek(gProcessQueue, &pTempProcess);
        //keep looping as long as the process on top has arrival time equal to the current time
        bool is_time = false; //flag to indicate whether at least one process matches current time or not
        while (pTempProcess->mArrivalTime == current_time) {
            is_time = true;
            SendProcess(pTempProcess); //send this process to the scheduler
            ProcDequeue(gProcessQueue, &pTempProcess); //dequeue this process from the processes queue
            free(pTempProcess); //free memory allocated by this process
            ProcPeek(gProcessQueue, &pTempProcess); //peek the next process in the queue
        }
        if (is_time) //if at least one process was sent to the scheduler
            kill(gSchedulerPid, SIGUSR1); //send SIGUSR1 to the scheduler
        usleep(900000); //sleep 0.9 sec
    }*/
    // invoke ClearResources() but use zero as parameter to indicate normal exit not interrupt
    ClearResources(0);
}

void ClearResources(int signum) {
    //Clear IPC resources
    if (gMsgQueueId != 0) {
        printf("PG: *** Cleaning IPC resources...\n");
        if (msgctl(gMsgQueueId, IPC_RMID, NULL) == -1)
            perror("PG: *** Error");
        else
            printf("PG: *** IPC cleaned!\n");
    }

    //free queue memory
    printf("PG: *** Cleaning processes queue...\n");
    Process *pTemp;
    while (ProcDequeue(gProcessQueue, &pTemp)) {
        free(pTemp);
    }
    printf("PG: *** Process queue cleaned!\n");

    //if this function is invoked due to an interrupt signal then immediately interrupt all processes
    if (signum == SIGINT) {
        //Interrupt forked processes
        printf("PG: *** Sending interrupt to scheduler\n");
        if (gSchedulerPid)
            kill(gSchedulerPid, SIGINT);
        printf("PG: *** Sending interrupt to clock\n");
        if (gClockPid) {
            destroyClk(false);
            kill(gClockPid, SIGINT);
        }
        //do not leave before both forked processes are done
        wait(NULL);
        wait(NULL);
    } else { //we need to wait until Scheduler exits by itself
        printf("PG: *** Waiting for scheduler to do its job...\n");
        waitpid(gSchedulerPid, NULL, 0); //wait until scheduler exits
        printf("PG: *** Scheduler exit signal received\n");
        printf("PG: *** Sending interrupt to clock\n");
        if (gClockPid) {
            destroyClk(false);
            kill(gClockPid, SIGINT);
        }
        //do not leave before clock is done
        wait(NULL);
    }
    printf("PG: *** Clean!\n");
    exit(EXIT_SUCCESS);
}

void ReadFile() {
    printf("PG: *** Attempting to open input file...\n");
    FILE *pFile;
    char *pLine = NULL;
    size_t len = 0;
    ssize_t read;
    pFile = fopen("processes.txt", "r");
    if (pFile == NULL) {
        perror("PG: *** Error reading from input file");
        exit(EXIT_FAILURE);
    }
    printf("PG: *** Reading input file...\n");
    unsigned int runtime_sum = 0, runtime_squared_sum = 0, count = 0;
    while ((read = getline(&pLine, &len, pFile)) != -1) {
        if (pLine[0] == '#') //skip hashes as they are just comments in the input file
            continue;

        Process *pProcess = malloc(sizeof(Process));
        while (!pProcess) {
            perror("PG: *** Malloc failed");
            printf("PG: *** Trying again");
            pProcess = malloc(sizeof(Process));
        }

        pProcess->mId = atoi(strtok(pLine, "\t"));
        pProcess->mArrivalTime = atoi(strtok(NULL, "\t"));
        pProcess->mRuntime = atoi(strtok(NULL, "\t"));
        pProcess->mPriority = atoi(strtok(NULL, "\t"));
        pProcess->mMemsize = atoi(strtok(NULL, "\t"));
        pProcess->mRemainTime = pProcess->mRuntime;
        pProcess->mWaitTime = 0;
        runtime_sum += pProcess->mRuntime;
        runtime_squared_sum += pProcess->mRuntime * pProcess->mRuntime;
        count++;
        PrintProcess(pProcess);
        ProcEnqueue(gProcessQueue, pProcess);
    }

    double runtime_avg = (double) runtime_sum / count;
    double runtime_std = sqrt(
            (runtime_squared_sum - (2 * runtime_sum * runtime_avg) + (runtime_avg * runtime_avg * count)) / count);
    printf("PG: *** Releasing file resources...\n");
    fclose(pFile);
    if (pLine)
        free(pLine);
    printf("PG: *** Input file done successfully!\n");
    printf("\nPG: *** Total runtime %d/s, %.2f/m, %.2f/h\n", runtime_sum, runtime_sum / 60.0, runtime_sum / (60.0 * 60.0));
    printf("PG: *** Average runtime = %.2f, STD = %.2f\n", runtime_avg, runtime_std);
}

void InitIPC() {
    key_t key = ftok(gFtokFile, gFtokCode);
    gMsgQueueId = msgget(key, IPC_CREAT | 0666);
    if (gMsgQueueId == -1) {
        perror("PG: *** IPC init failed");
        raise(SIGINT);
    }
    printf("PG: *** IPC ready!\n");
}

void ExecuteClock() {
    gClockPid = fork();
    while (gClockPid == -1) {
        perror("PG: *** Error forking clock");
        printf("PG: *** Trying again...\n");
        gClockPid = fork();
    }
    if (gClockPid == 0) {
        printf("PG: *** Clock forking done!\n");
        printf("PG: *** Executing clock...\n");
        char *argv[] = {"clk.out", NULL};
        execv("clk.out", argv);
        perror("PG: *** Clock execution failed");
        exit(EXIT_FAILURE);
    }

}

void ExecuteScheduler() {
    gSchedulerPid = fork();
    while (gSchedulerPid == -1) {
        perror("PG: *** Error forking scheduler");
        printf("PG: *** Trying again...\n");
        gSchedulerPid = fork();
    }
    if (gSchedulerPid == 0) {
        printf("PG: *** Scheduler forking done!\n");
        printf("PG: *** Executing scheduler...\n");
        char *argv[2];
        argv[1] = NULL;
        argv[0] = "srtn.out";
        execv("srtn.out", argv);
        perror("PG: *** Scheduler execution failed");
        exit(EXIT_FAILURE);
    }
}

void SendProcess(Process *pProcess) {
    Message msg;
    msg.mType = 10; //arbitrary value for mType
    msg.mProcess = *pProcess;
    printf("PG: *** Sending process with id %d to scheduler...\n", msg.mProcess.mId);
    if (msgsnd(gMsgQueueId, (void *) &msg, sizeof(msg.mProcess), !IPC_NOWAIT) == -1) {
        perror("PG: *** Error while sending process");
    } else {
        printf("PG: *** Process sent!\n");
    }
}