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
** NODE n = GetHead(LIST l)
** get the head node of the list, without removing it
*/
NODE GetHead(LIST);

/*
** NODE n = GetTail(LIST l)
** get the tail node of the list, without removing it
*/
NODE GetTail(LIST);

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
** NODE rn = RemoveNode(LIST l, NODE n)
** remove the node n (that must be in the list l) from the list and return it (rn==n)
*/
NODE RemoveNode(LIST, NODE);

LIST NewList(void) {
    LIST tl = malloc(sizeof(struct List));
    if (tl != NULL) {
        tl->head = tl->tail = NULL;
        return tl;
    }
    return NULL;
}

int IsListEmpty(LIST l) {
    return (l->head == NULL);
}

NODE GetHead(LIST l) {
    return l->head;
}

NODE GetTail(LIST l) {
    return l->tail;
}

NODE AddHead(LIST l, int data) {
    NODE n = malloc(sizeof(NODE));
    n->data = data;
    n->pred = NULL;
    n->succ = l->head;
    if (l->head == NULL) {
        l->head = l->tail = n;
        return n;
    }
    l->head->pred = n;
    l->head = n;
    return n;
}

NODE RemHead(LIST l) {
    NODE h = l->head;

    if (h == NULL)
        return NULL;

    if (h->succ == NULL) {
        l->head = l->tail = NULL;
        return h;
    }

    h->succ->pred = NULL;
    l->head = h->succ;
    return h;
}

NODE RemTail(LIST l) {
    NODE t = l->tail;

    if (t == NULL)
        return NULL;

    if (t->pred == NULL) {
        l->head = l->tail = NULL;
        return t;
    }

    t->pred->succ = NULL;
    l->head = t->pred;
    return t;
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

void InsertSort(LIST l, int data) {
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

void PrintList(LIST l) {
    NODE node = l->head;
    while (node) {
        printf("%d->", node->data);
        node = node->succ;
    }
    printf("NULL\n");
}
#endif //SRTN_BUDDY_DOUBLELINKEDLIST_H
