#ifndef PTHREADPOOL_H
#define PTHREADPOOL_H

struct node {
    struct node* next;
    int *client_socket;
};
typedef struct node node_t;

void enqueue(int client_socket);
int dequeue();

#endif