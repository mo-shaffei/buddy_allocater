//
// Created by shaffei on 3/24/20.
//

#ifndef OS_STARTER_CODE_MESSAGEBUFFER_H
#define OS_STARTER_CODE_MESSAGEBUFFER_H

#include "ProcessStruct.h"

typedef struct MessageBuffer {
    long mType;
    Process mProcess;
} Message;
#endif //OS_STARTER_CODE_MESSAGEBUFFER_H
