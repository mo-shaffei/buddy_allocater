//
// Created by shaffei on 3/24/20.
//

#ifndef OS_STARTER_CODE_EVENTSTRUCT_H
#define OS_STARTER_CODE_EVENTSTRUCT_H

#include "ProcessStruct.h"

enum EventType {
    START, STOP, CONT, FINISH
};

typedef struct EventStruct {
    enum EventType mType;
    Process *mpProcess;
    unsigned int mTimeStep;
    unsigned int mCurrentRemTime;
    unsigned int mCurrentWaitTime;
    unsigned int mTaTime;
    double mWTaTime;
} Event;

void PrintEvent(const Event *pEvent) { //print an event using the same output file format
    printf("At time %d ", pEvent->mTimeStep);
    switch (pEvent->mType) {
        case START:
            printf("allocated %d bytes for process %d ", pEvent->mpProcess->mMemSize, pEvent->mpProcess->mId);
            printf("from %d to %d", pEvent->mpProcess->mMemAddr, pEvent->mpProcess->mMemAlloc + pEvent->mpProcess->mMemAddr - 1);
            break;
        case STOP:
            printf("process %d stopped ", pEvent->mpProcess->mId);
            printf("arr %d total %d ", pEvent->mpProcess->mArrivalTime, pEvent->mpProcess->mRuntime);
            printf("remain %d wait %d", pEvent->mCurrentRemTime, pEvent->mCurrentWaitTime);
            break;
        case CONT:
            printf("process %d resumed ", pEvent->mpProcess->mId);
            printf("arr %d total %d ", pEvent->mpProcess->mArrivalTime, pEvent->mpProcess->mRuntime);
            printf("remain %d wait %d", pEvent->mCurrentRemTime, pEvent->mCurrentWaitTime);
            break;
        case FINISH:
            printf("freed %d bytes from process %d ", pEvent->mpProcess->mMemSize, pEvent->mpProcess->mId);
            printf("from %d to %d", pEvent->mpProcess->mMemAddr, pEvent->mpProcess->mMemAlloc + pEvent->mpProcess->mMemAddr - 1);
            break;
        default:
            printf("error ");
            break;
    }

    printf("\n");
}

void OutputEvent(const Event *pEvent, FILE *pFile) { //print an event using the same output file format
    fprintf(pFile,"At time %d ", pEvent->mTimeStep);
    switch (pEvent->mType) {
        case START:
            fprintf(pFile,"allocated %d bytes for process %d ", pEvent->mpProcess->mMemSize, pEvent->mpProcess->mId);
            fprintf(pFile,"from %d to %d", pEvent->mpProcess->mMemAddr, pEvent->mpProcess->mMemAlloc + pEvent->mpProcess->mMemAddr - 1);
            break;
        case STOP:
            fprintf(pFile,"process %d stopped ", pEvent->mpProcess->mId);
            fprintf(pFile,"arr %d total %d ", pEvent->mpProcess->mArrivalTime, pEvent->mpProcess->mRuntime);
            fprintf(pFile,"remain %d wait %d", pEvent->mCurrentRemTime, pEvent->mCurrentWaitTime);
            break;
        case CONT:
            fprintf(pFile,"process %d resumed ", pEvent->mpProcess->mId);
            fprintf(pFile,"arr %d total %d ", pEvent->mpProcess->mArrivalTime, pEvent->mpProcess->mRuntime);
            fprintf(pFile,"remain %d wait %d", pEvent->mCurrentRemTime, pEvent->mCurrentWaitTime);
            break;
        case FINISH:
            fprintf(pFile,"freed %d bytes from process %d ", pEvent->mpProcess->mMemSize, pEvent->mpProcess->mId);
            fprintf(pFile,"from %d to %d", pEvent->mpProcess->mMemAddr, pEvent->mpProcess->mMemAlloc + pEvent->mpProcess->mMemAddr - 1);
            break;
        default:
            fprintf(pFile,"error ");
            break;
    }
    fprintf(pFile, "\n");
}

#endif //OS_STARTER_CODE_EVENTSTRUCT_H
