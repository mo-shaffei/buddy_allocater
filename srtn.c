#include "Headers/headers.h"
#include "Headers/ProcessStruct.h"
#include "Headers/ProcessHeap.h"
#include "Headers/MessageBuffer.h"
#include "Headers/EventsQueue.h"
#include "Headers/DoubleLinkedList.h"
#include <math.h>

#define SIZE_2 0
#define SIZE_4 1
#define SIZE_8 2
#define SIZE_16 3
#define SIZE_32 4
#define SIZE_64 5
#define SIZE_128 6
#define SIZE_256 7

void ProcessArrivalHandler(int);

void InitIPC();

void InitMemList();

int ReceiveProcess();

void CleanResources();

void ExecuteProcess();

void ChildHandler(int signum);

void LogEvents(unsigned int, unsigned int);

void AddEvent(enum EventType);

int gMsgQueueId = 0;
Process *gpCurrentProcess = NULL;
heap_t *gProcessHeap = NULL;
short gSwitchContext = 0;
event_queue gEventQueue = NULL;
LIST gFreeMemList[8];
int gFreeMem = 1024;

int main(int argc, char *argv[]) {
    printf("SRTN: *** Scheduler here\n");
    initClk();
    InitIPC();
    //initialize processes heap
    gProcessHeap = (heap_t *) calloc(1, sizeof(heap_t));
    gEventQueue = NewEventQueue();
    InitMemList();

    signal(SIGUSR1, ProcessArrivalHandler);
    signal(SIGINT, CleanResources);
    signal(SIGCHLD, ChildHandler);

    pause(); //wait for the first process to arrive
    unsigned int start_time = getClk(); //store simulation start time
    while ((gpCurrentProcess = HeapPop(gProcessHeap)) != NULL) {
        ExecuteProcess(); //starts the process with the least remaining time and handles context switching
        gSwitchContext = 0; //toggle switch context off until a signal handler turns it on
        while (!gSwitchContext)
            pause(); //pause to avoid busy waiting and only wakeup to handle signals
    }
    unsigned int end_time = getClk(); //store simulation end time
    LogEvents(start_time, end_time);
}

void ProcessArrivalHandler(int signum) {
    //keep looping as long as a process was received in the current iteration
    while (!ReceiveProcess());

    if (!gpCurrentProcess) //if there's no process currently running no extra checking is needed
        return;

    //current runtime of a process = current time - (arrival time of process + total waiting time of the process)
    //then subtract this quantity from total runtime to get remaining runtime
    gpCurrentProcess->mRemainTime =
            gpCurrentProcess->mRuntime - (getClk() - (gpCurrentProcess->mArrivalTime + gpCurrentProcess->mWaitTime));
    
    if (HeapPeek(gProcessHeap)->mRuntime < gpCurrentProcess->mRemainTime) { //if a new process has a shorter runtime
        if (kill(gpCurrentProcess->mPid, SIGTSTP) == -1) //stop current process
            perror("RR: *** Error stopping process");

        gpCurrentProcess->mLastStop = getClk(); //store the stop time of the current process
        gSwitchContext = 1; //toggle switch context on so main loop can execute a new process
        HeapPush(gProcessHeap, gpCurrentProcess->mRemainTime, gpCurrentProcess); //push current process back into heap
        AddEvent(STOP);
    }
}

void InitIPC() {
    key_t key = ftok(gFtokFile, gFtokCode); //same parameters used in process_generator so we attach to same queue
    gMsgQueueId = msgget(key, 0);
    if (gMsgQueueId == -1) {
        perror("SRTN: *** Scheduler IPC init failed");
        raise(SIGINT);
    }
    printf("SRTN: *** Scheduler IPC ready!\n");

}

int ReceiveProcess() {
    Message msg;
    //receive a message but do not wait, if not found return immediately
    while (msgrcv(gMsgQueueId, (void *) &msg, sizeof(msg.mProcess), 0, IPC_NOWAIT) == -1) {
        perror("SRTN: *** Error in receive");
        return -1;

    }

    //below is executed if a message was retrieved from the message queue
    printf("SRTN: *** Received by scheduler\n");
    Process *pProcess = malloc(sizeof(Process)); //allocate memory for the received process

    while (!pProcess) {
        perror("SRTN: *** Malloc failed");
        printf("SRTN: *** Trying again");
        pProcess = malloc(sizeof(Process));
    }
    *pProcess = msg.mProcess; //store the process received in the allocated space
    pProcess->mMemAlloc = ceil(log2(pProcess->mMemsize));
    //push the process pointer into the process heap, and use the process runtime as the value to sort the heap with
    HeapPush(gProcessHeap, pProcess->mRemainTime, pProcess);

    return 0;
}

void CleanResources() {
    printf("SRTN: *** Cleaning scheduler resources\n");
    Process *pProcess = NULL;
    while ((pProcess = HeapPop(gProcessHeap)) != NULL) //while processes heap is not empty
        free(pProcess); //free memory allocated by this process

    Event *pEvent = NULL;
    while (EventQueueDequeue(gEventQueue, &pEvent)) //while event queue is not empty
        free(pEvent); //free memory allocated by the event
    printf("SRTN: *** Scheduler clean!\n");
    exit(EXIT_SUCCESS);
}

void ExecuteProcess() {
    if (gpCurrentProcess->mRuntime == gpCurrentProcess->mRemainTime) { //if this process never ran before
        gpCurrentProcess->mPid = fork(); //fork a new child and store its pid in the process struct
        while (gpCurrentProcess->mPid == -1) { //if forking fails
            perror("SRTN: *** Error forking process");
            printf("SRTN: *** Trying again...\n");
            gpCurrentProcess->mPid = fork();
        }
        if (!gpCurrentProcess->mPid) { //if child then execute the process
            char buffer[10]; //buffer to convert runtime from int to string
            sprintf(buffer, "%d", gpCurrentProcess->mRuntime);
            char *argv[] = {"process.out", buffer, NULL};
            execv("process.out", argv);
            perror("SRTN: *** Process execution failed");
            exit(EXIT_FAILURE);
        }
        AddEvent(START);
        gpCurrentProcess->mWaitTime = getClk() - gpCurrentProcess->mArrivalTime;
    } else { //this process was stopped and now we need to resume it
        if (kill(gpCurrentProcess->mPid, SIGCONT) == -1) { //continue process
            printf("SRTN: *** Error resuming process %d", gpCurrentProcess->mId);
            perror(NULL);
            return;
        }
        gpCurrentProcess->mWaitTime += getClk() - gpCurrentProcess->mLastStop;  //update the waiting time of the process
        AddEvent(CONT);
    }
};

void ChildHandler(int signum) {
    if (!waitpid(gpCurrentProcess->mPid, NULL, WNOHANG)) //if current process did not terminate
        return;

    gSwitchContext = 1; //set flag to 1 so main loop knows it's time to switch context
    gpCurrentProcess->mRemainTime = 0; //process finished so remaining time should be zero
    AddEvent(FINISH);
}

void LogEvents(unsigned int start_time, unsigned int end_time) {  //prints all events in the terminal
    unsigned int runtime_sum = 0, waiting_sum = 0, count = 0;
    double wta_sum = 0, wta_squared_sum = 0;

    FILE *pFile = fopen("Events.txt", "w");
    Event *pEvent = NULL;
    while (EventQueueDequeue(gEventQueue, &pEvent)) { //while event queue is not empty
        PrintEvent(pEvent);
        OutputEvent(pEvent, pFile);
        if (pEvent->mType == FINISH) {
            runtime_sum += pEvent->mpProcess->mRuntime;
            waiting_sum += pEvent->mCurrentWaitTime;
            count++;
            wta_sum += pEvent->mWTaTime;
            wta_squared_sum += pEvent->mWTaTime * pEvent->mWTaTime;
            free(pEvent->mpProcess);
        }
        free(pEvent); //free memory allocated by the event
    }
    fclose(pFile);
    //cpu utilization = useful time / total time
    double cpu_utilization = runtime_sum * 100.0 / (end_time - start_time);
    double avg_wta = wta_sum / count;
    double avg_waiting = (double) waiting_sum / count;
    double std_wta = sqrt((wta_squared_sum - (2 * wta_sum * avg_wta) + (avg_wta * avg_wta * count)) / count);

    pFile = fopen("Stats.txt", "w");
    printf("\nCPU utilization = %.2f\n", cpu_utilization);
    printf("Avg WTA = %.2f\n", avg_wta);
    printf("Avg Waiting = %.2f\n", avg_waiting);
    printf("STD WTA = %.2f\n\n", std_wta);

    fprintf(pFile, "Avg Waiting = %.2f\n", avg_waiting);
    fprintf(pFile, "\nCPU utilization = %.2f\n", cpu_utilization);
    fprintf(pFile, "Avg WTA = %.2f\n", avg_wta);
    fprintf(pFile, "STD WTA = %.2f\n\n", std_wta);
}

void AddEvent(enum EventType type) {
    Event *pEvent = malloc(sizeof(Event));
    while (!pEvent) {
        perror("RR: *** Malloc failed");
        printf("RR: *** Trying again");
        pEvent = malloc(sizeof(Event));
    }
    pEvent->mTimeStep = getClk();
    if (type == FINISH) {
        pEvent->mTaTime = getClk() - gpCurrentProcess->mArrivalTime;
        pEvent->mWTaTime = (double) pEvent->mTaTime / gpCurrentProcess->mRuntime;
    }
    pEvent->mpProcess = gpCurrentProcess;
    pEvent->mCurrentWaitTime = gpCurrentProcess->mWaitTime;
    pEvent->mType = type;
    pEvent->mCurrentRemTime = gpCurrentProcess->mRemainTime;
    EventQueueEnqueue(gEventQueue, pEvent);
}

void InitMemList() {
    //we have 8 different allocation sizes starting from 2 bytes up to 256 bytes
    //because a process only requests memory <= 256 bytes, so no need for bigger chunks of memory
    for (int i = 0; i < 8; ++i)
        gFreeMemList[i] = NewList();

    //we have four 256 partitions at addresses 0, 256, 512, and 768
    for (int i = 0; i < 1024; i += 256)
        insertSort(gFreeMemList[SIZE_256], i);
}
