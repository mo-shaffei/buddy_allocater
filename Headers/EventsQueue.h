//
// Created by shaffei on 3/24/20.
//Implements a queue of events
////original source: http://rosettacode.org/wiki/Queue/Definition#C

#ifndef OS_STARTER_CODE_EVENTS_queue_H
#define OS_STARTER_CODE_EVENTS_queue_H

#include "EventStruct.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct e_node_t e_node_t, *e_node, *event_queue;
struct e_node_t {
    Event *val;
    e_node prev, next;
};
typedef Event *EVENT_DATA;

#define HEAD_E(q) q->prev
#define TAIL_E(q) q->next

event_queue NewEventQueue() {
    e_node q = (e_node) malloc(sizeof(node_t));
    q->next = q->prev = 0;
    return q;
}

int EventQueueEmpty(event_queue q) {
    return !HEAD_E(q);
}

void EventQueueEnqueue(event_queue q, EVENT_DATA val) {
    e_node nd = (e_node) malloc(sizeof(node_t));
    nd->val = val;
    if (!HEAD_E(q))
        HEAD_E(q) = nd;
    nd->prev = TAIL_E(q);
    if (nd->prev)
        nd->prev->next = nd;
    TAIL_E(q) = nd;
    nd->next = 0;
}

int EventQueueDequeue(event_queue q, EVENT_DATA *val) {
    e_node tmp = HEAD_E(q);
    if (!tmp)
        return 0;
    *val = tmp->val;

    HEAD_E(q) = tmp->next;
    if (TAIL_E(q) == tmp)
        TAIL_E(q) = 0;
    free(tmp);

    return 1;
}

int EventQueuePeek(event_queue q, EVENT_DATA *val) {
    e_node tmp = HEAD_E(q);
    if (!tmp)
        return 0;
    *val = tmp->val;
    return 1;
}

#endif
