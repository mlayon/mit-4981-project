#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


struct Data
{
    bool done;
    bool flag;
    size_t counter;
    pthread_mutex_t lock;
    pthread_cond_t condition;
};


static void *readFunc(void *arg)
{
    struct Data *data;
    
    data = arg;
    
    while(!(data->done))
    {
        pthread_mutex_lock(&data->lock);
        
        while(!(data->flag))
        {
            pthread_cond_wait(&data->condition, &data->lock);
        }
        
        printf("read - %ld\n", data->counter);
        data->flag = false;
        pthread_cond_signal(&data->condition);
        
        pthread_mutex_unlock(&data->lock);
    }
    
    return 0;
}


static void *writeFunc(void *arg)
{
    struct Data *data;
    
    data = arg;
    
    for(int i = 0; i < 10; i++)
    {
        pthread_mutex_lock(&data->lock);
        
        while(data->flag)
        {
            pthread_cond_wait(&data->condition, &data->lock);
        }
        
        data->counter++;
        printf("write - %ld\n", data->counter);
        data->flag = true;
        pthread_cond_signal(&data->condition);

        pthread_mutex_unlock(&data->lock);
    }
    
    data->done = true;
    
    return 0;
}


int main(int argc, const char *argv[])
{
    pthread_t read_thread_id;
    pthread_t write_thread_id;
    struct Data data;

    data.done = false;
    data.counter = 0;
    data.flag = false;

    if(pthread_mutex_init(&data.lock, NULL) != 0)
    {
        perror("pthread_mutex_init");
        
        exit(EXIT_FAILURE);
    }
    
    if(pthread_cond_init(&data.condition, NULL) != 0)
    {
        perror("pthread_cond_init");
        
        exit(EXIT_FAILURE);
    }
    
    pthread_create(&read_thread_id, NULL, readFunc, &data);
    pthread_create(&write_thread_id, NULL, writeFunc, &data);

    pthread_join(read_thread_id, NULL);
    pthread_join(write_thread_id, NULL);
    pthread_mutex_destroy(&data.lock);
    
    return EXIT_SUCCESS;
}
