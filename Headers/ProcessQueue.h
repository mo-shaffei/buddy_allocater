//
// Created by shaffei on 3/24/20.
//
// Implements a queue of processes
//original source: http://rosettacode.org/wiki/Queue/Definition#C
#ifndef OS_STARTER_CODE_PROCESSQUEUE_H
#define OS_STARTER_CODE_PROCESSQUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include "ProcessStruct.h"

typedef struct node_t_q node_t_q, *node, *queue;
struct node_t_q {
    Process *val;
    node prev, next;
};
typedef Process *DATA;

#define HEAD(q) q->prev
#define TAIL(q) q->next

queue NewProcQueue() {
    node q = (node) malloc(sizeof(node_t_q));
    q->next = q->prev = 0;
    return q;
}

int ProcQueueEmpty(queue q) {
    return !HEAD(q);
}

void ProcEnqueue(queue q, DATA n) {
    node nd = (node) malloc(sizeof(node_t_q));
    nd->val = n;
    if (!HEAD(q))
        HEAD(q) = nd;
    nd->prev = TAIL(q);
    if (nd->prev)
        nd->prev->next = nd;
    TAIL(q) = nd;
    nd->next = 0;
}

int ProcDequeue(queue q, DATA *val) {
    node tmp = HEAD(q);
    if (!tmp)
        return 0;
    *val = tmp->val;

    HEAD(q) = tmp->next;
    if (TAIL(q) == tmp)
        TAIL(q) = 0;
    free(tmp);

    return 1;
}

int ProcPeek(queue q, DATA *val) {
    node tmp = HEAD(q);
    if (!tmp)
        return 0;
    *val = tmp->val;
    return 1;
}

#endif //OS_STARTER_CODE_PROCESSQUEUE_H
