//
// Created by shaffei on 5/16/20.
//
//implements a double linked list of integers with insertion sort
//original source: http://rosettacode.org/wiki/Doubly-linked_list/Definition#C

#ifndef SRTN_BUDDY_DOUBLELINKEDLIST_H
#define SRTN_BUDDY_DOUBLELINKEDLIST_H

#include <stdio.h>
#include <stdlib.h>

struct List {
    struct MNode *head;
    struct MNode *tail;
    struct MNode *tail_pred;
};

struct MNode {
    struct MNode *succ;
    struct MNode *pred;
    int data;
};

typedef struct MNode *NODE;
typedef struct List *LIST;

/*
** LIST l = NewList()
** create (alloc space for) and initialize a list
*/
LIST NewList(void);

/*
** int IsListEmpty(LIST l)
** test if a list is empty
*/
int IsListEmpty(LIST);

/*
** NODE n = GetTail(LIST l)
** get the tail node of the list, without removing it
*/
NODE GetTail(LIST);

/*
** NODE n = GetHead(LIST l)
** get the head node of the list, without removing it
*/
NODE GetHead(LIST);

/*
** NODE rn = AddTail(LIST l, NODE n)
** add the node n to the tail of the list l, and return it (rn==n)
*/
NODE AddTail(LIST, int);

/*
** NODE rn = AddHead(LIST l, NODE n)
** add the node n to the head of the list l, and return it (rn==n)
*/
NODE AddHead(LIST, int);

/*
** NODE n = RemHead(LIST l)
** remove the head node of the list and return it
*/
NODE RemHead(LIST);

/*
** NODE n = RemTail(LIST l)
** remove the tail node of the list and return it
*/
NODE RemTail(LIST);

/*
** NODE rn = InsertAfter(LIST l, NODE r, NODE n)
** insert the node n after the node r, in the list l; return n (rn==n)
*/
NODE InsertAfter(LIST, NODE, NODE);

/*
** NODE rn = RemoveNode(LIST l, NODE n)
** remove the node n (that must be in the list l) from the list and return it (rn==n)
*/
NODE RemoveNode(LIST, NODE);

LIST NewList(void) {
    LIST tl = malloc(sizeof(struct List));
    if (tl != NULL) {
        tl->tail_pred = (NODE) &tl->head;
        tl->tail = NULL;
        tl->head = (NODE) &tl->tail;
        return tl;
    }
    return NULL;
}

int IsListEmpty(LIST l) {
    return (l->head->succ == 0);
}

NODE GetHead(LIST l) {
    return l->head;
}

NODE GetTail(LIST) {
    return l->tail_pred;
}

NODE AddTail(LIST l, int data) {
    NODE n = malloc(sizeof(NODE));
    n->data = data;
    n->succ = (NODE) &l->tail;
    n->pred = l->tail_pred;
    l->tail_pred->succ = n;
    l->tail_pred = n;
    return n;
}

NODE AddHead(LIST l, int data) {
    NODE n = malloc(sizeof(NODE));
    n->data = data;
    n->succ = l->head;
    n->pred = (NODE) &l->head;
    l->head->pred = n;
    l->head = n;
    return n;
}

NODE RemHead(LIST l) {
    NODE h;
    h = l->head;
    l->head = l->head->succ;
    l->head->pred = (NODE) &l->head;
    return h;
}

NODE RemTail(LIST l) {
    NODE t;
    t = l->tail_pred;
    l->tail_pred = l->tail_pred->pred;
    l->tail_pred->succ = (NODE) &l->tail;
    return t;
}

NODE InsertAfter(LIST l, NODE n, NODE r) {
    n->pred = r;
    n->succ = r->succ;
    n->succ->pred = n;
    r->succ = n;
    return n;
}

NODE RemoveNode(LIST l, NODE n) {
    if (n == l->head)
        return RemHead(l);
    else if (n == l->tail)
        return RemTail(l);

    n->pred->succ = n->succ;
    n->succ->pred = n->pred;
    return n;
}

void insertSort(LIST l, int data) {
    NODE n = AddHead(l, data);
    while (n->succ != NULL) {
        if (n->data > n->succ->data) {
            int temp = n->data;
            n->data = n->succ->data;
            n->succ->data = temp;
        } else
            return;

        n = n->succ;
    }
}

#endif //SRTN_BUDDY_DOUBLELINKEDLIST_H
