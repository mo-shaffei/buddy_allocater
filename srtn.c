#include "Headers/headers.h"
#include "Headers/ProcessStruct.h"
#include "Headers/ProcessHeap.h"
#include "Headers/MessageBuffer.h"
#include "Headers/EventsQueue.h"
#include "Headers/DoubleLinkedList.h"
#include "Headers/ProcessQueue.h"
#include <math.h>

void ProcessArrivalHandler(int);

void InitIPC();

void InitMemList();

int ReceiveProcess();

void CleanResources();

int ExecuteProcess();

void ChildHandler(int);

void LogEvents(unsigned int, unsigned int);

void AddEvent(enum EventType);

int AllocateMem(int);

void FreeMem(int, int);

int IsEven(int);

int gMsgQueueId = 0;
Process *gpCurrentProcess = NULL;
heap_t *gProcessHeap = NULL;
short gSwitchContext = 0;
event_queue gEventQueue = NULL;
LIST gFreeMemList[8];
int gFreeMem = 1024;
queue gTempQueue;

int main(int argc, char *argv[]) {
    printf("SRTN: *** Scheduler here\n");
    initClk();
    InitIPC();
    //initialize processes heap
    gProcessHeap = (heap_t *) calloc(1, sizeof(heap_t));
    gTempQueue = NewProcQueue();
    gEventQueue = NewEventQueue();
    InitMemList();

    signal(SIGUSR1, ProcessArrivalHandler);
    signal(SIGINT, CleanResources);
    signal(SIGCHLD, ChildHandler);

    pause(); //wait for the first process to arrive
    unsigned int start_time = getClk(); //store simulation start time
    while ((gpCurrentProcess = HeapPop(gProcessHeap)) != NULL) {
        if (ExecuteProcess() == -1) {//starts the process with the least remaining time and handles context switching
            ProcEnqueue(gTempQueue, gpCurrentProcess); //if execution failed place this process in the temp queue
            continue;
        }
        gSwitchContext = 0; //toggle switch context off until a signal handler turns it on
        while (!ProcQueueEmpty(gTempQueue)) {//re-push all processes that failed to run in the main heap
            Process *pProcess;
            ProcDequeue(gTempQueue, &pProcess);
            HeapPush(gProcessHeap, pProcess->mRemainTime, pProcess);
        }
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

    Process *pNewProcess = HeapPeek(gProcessHeap);
    if (pNewProcess->mRuntime < gpCurrentProcess->mRemainTime) { //if a new process has a shorter runtime
        if (pNewProcess->mMemAlloc > gFreeMem) //no memory available for this process so no context switching
            return;

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
    pProcess->mMemAlloc = pow(2, ceil(log2(pProcess->mMemSize))); //approximate to the first power of 2
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

int ExecuteProcess() {
    if (gpCurrentProcess->mRuntime == gpCurrentProcess->mRemainTime) { //if this process never ran before
        gpCurrentProcess->mMemAddr = AllocateMem(gpCurrentProcess->mMemAlloc); //allocate memory for this process
        if (gpCurrentProcess->mMemAddr == -1) //if allocation failed
            return -1;

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
            return -1;
        }
        gpCurrentProcess->mWaitTime += getClk() - gpCurrentProcess->mLastStop;  //update the waiting time of the process
        AddEvent(CONT);
    }
    return 0;
};

void ChildHandler(int signum) {
    if (!waitpid(gpCurrentProcess->mPid, NULL, WNOHANG)) //if current process did not terminate
        return;
    FreeMem(gpCurrentProcess->mMemAddr, gpCurrentProcess->mMemAlloc);  //free memory allocated for this process
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
        InsertSort(gFreeMemList[7], i);
}

int AllocateMem(int mem_size) {
    int desired_index = (int) log2(mem_size) - 1; //calculate the index of the list containing this allocation size
    int found_index = desired_index; //start searching in the desired list
    while (IsListEmpty(gFreeMemList[found_index])) { //as long as no allocation of this size is available in this list
        found_index++; //increment index to search in the next list with larger allocation unit

        if (found_index > 7) //no memory is available so reject this allocation request
            return -1;
    }

    //if the free memory is in a list of a bigger allocation unit we need to split this large block to our desired size
    while (found_index != desired_index) { //keep splitting bigger blocks until we find a suitable block
        NODE node = RemHead(gFreeMemList[found_index]); //get the first available memory block in the current list
        int addr = node->data; //store the address of the memory block
        free(node); //remove this block from this unit as it will be divided in half
        int current_alloc = pow(2, found_index + 1); //calculate the block size of the current unit
        int split_addr = (2 * addr + current_alloc) / 2; //calculate the address which divides the block in half
        //int split_addr = (addr + current_alloc) / 2; //calculate the address which divides the block in half
        found_index--; //go down one allocation unit
        //store the two new blocks in the smaller allocation unit
        InsertSort(gFreeMemList[found_index], split_addr);
        InsertSort(gFreeMemList[found_index], addr);
    }
    //after the above loop we are sure that a block of the desired size is available
    NODE node = RemHead(gFreeMemList[desired_index]); //get the first available memory block
    int addr = node->data; //store the address of the memory block
    free(node); //remove this block from this unit as it will be allocated to the requesting process
    gFreeMem -= mem_size; //subtract this block size from the free memory
    return addr; //return the address of this block to the requesting process
}

void FreeMem(int mem_addr, int mem_size) {
    int index = (int) log2(mem_size) - 1; //calculate the index of the list containing this allocation size
    InsertSort(gFreeMemList[index], mem_addr); //add this memory address to the corresponding list

    while (index < 7) //if this block is less than 256 in size then we might need to join small blocks
    {
        NODE node = GetHead(gFreeMemList[index]); //get the first memory block in this list
        int current_mem_size = (int) pow(2, index + 1); //calculate the block size of the current unit
        while (node->succ != NULL) { //traverse this list until the tail's predecessor has been visited
            //if this is a valid merge(index of this address is even and next address is exactly after one block)
            if (IsEven(node->data / mem_size) && (node->succ->data - node->data) == current_mem_size) {
                InsertSort(gFreeMemList[index + 1], node->data); //add the address of first block to the larger list
                NODE temp = node->succ->succ; //get the first block after the two blocks that will be merged together
                //remove both blocks from the current list as they have been merged into one in the next list
                free(RemoveNode(gFreeMemList[index], node->succ));
                free(RemoveNode(gFreeMemList[index], node));
                //next node to traverse from is the one after the two merged blocks
                node = temp;
                if (!temp) //if this node is NULL then there are no more blocks to inspect in this list
                    break;
                continue; //otherwise start loop from beg and inspect the next node
            }
            node = node->succ; //iterate to the next node to insepct it
        }
        index++; //after finishing inspecting the current list increment index to go the next list
    }
    gFreeMem += mem_size; //add the freed memory to the free memory variable
}

int IsEven(int num) {
    if (num % 2 == 0)
        return 1;
    return 0;
}