#ifndef HELPER_H
#define CONFIG_H
#define THREAD_POOL_SIZE 10
struct node
{
    struct node *next;
    int *client_socket;
};
typedef struct node node_t;

void enqueue(int *client_socket);
int *dequeue();

#endif
