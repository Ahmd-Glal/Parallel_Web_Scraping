#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct queueElement {
    char url[300];
    int depth;
    struct queueElement* next;
} queueElement;

typedef struct queue {
    queueElement* front;
    queueElement* rear;
    int size;
} queue;

void createQueue(queue* q) {
    q->front = q->rear = NULL;
    q->size = 0;
}

void append(queue* q, const char* url,int depth) {
    queueElement* elm = (queueElement*)malloc(sizeof(queueElement));
    if (elm == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    strncpy_s(elm->url, url, sizeof(elm->url) - 1);
    elm->url[sizeof(elm->url) - 1] = '\0';
    elm->next = NULL;
    elm->depth = depth;

    if (q->rear == NULL) {
        q->front = q->rear = elm;
    }
    else {
        q->rear->next = elm;
        q->rear = elm;
    }
    q->size++;
}


int queueEmpty(queue* q) {
    return q->size == 0;
}

int queueSize(queue* q) {
    return q->size;
}

void serve(queue* q, char *url,int *depth) {
    queueElement* temp = q->front;
    if (queueEmpty(q))printf("lock\n");
    *depth = temp->depth;
    strcpy(url, temp->url);
    q->front = temp->next;
    free(temp);
    if (q->front == NULL) {
        q->rear = NULL;
    }
    q->size--;
}

void clearQueue(queue* q) {
    queueElement* temp = q->front;
    while (q->front != NULL) {
        temp = q->front;
        q->front = temp->next;
        free(temp);
    }
    q->rear = NULL;
    q->size = 0;
}

void traverse(queue* q) {
    queueElement* temp = q->front;
    while (temp != NULL) {
        printf("%s\n", temp->url);
        temp = temp->next;
    }
}