#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "webserver.h"
#include <semaphore.h>

#define MAX_REQUEST 100

int port, numThread;

sem_t mutex, empty, full;
int out = 0, in = 0;

int buffer[MAX_REQUEST];


void *listener()
{
    int r;
    struct sockaddr_in sin;
    struct sockaddr_in peer;
    int peer_len = sizeof(peer);
    int sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);
    r = bind(sock, (struct sockaddr *) &sin, sizeof(sin));
    if(r < 0) {
        perror("Error binding socket:");
        return;
    }

    r = listen(sock, 5);
    if(r < 0) {
        perror("Error listening socket:");
        return;
    }

    printf("HTTP server listening on port %d\n", port);
    while (1)
        {
            int s;
            s = accept(sock, NULL, NULL);
            if (s < 0) break;
            sem_wait(&empty);
            sem_wait(&mutex);
            buffer[in] = s;
            in = (in + 1) % numThread;
            sem_post(&mutex);
            sem_post(&full);

            //process(s);
        }

    close(sock);
}

void *worker() {
    printf("worker thread created\n");
    while(1){
        sem_wait(&full);
        sem_wait(&mutex);

        int fd = buffer[out];
        out = (out + 1) % numThread;

        sem_post(&mutex);
        sem_post(&empty);
        process(fd);


    }

}

void thread_control(int numThreads) {
    pthread_t workerThreads[numThreads];
    pthread_t listenerThread;

    sem_init(&mutex, 0, 1);
    sem_init(&full, 0, 0);
    sem_init(&empty, 0, numThreads);



    pthread_create(&listenerThread, NULL, listener, NULL);

    for(int i = 0; i < numThreads; i++) {
        pthread_create(&workerThreads[i], NULL, worker, NULL);
    }






    pthread_join(listenerThread, NULL);

    for(int i = 0; i < numThreads; i++) {
        pthread_join(workerThreads[i], NULL);
    }

}

int main(int argc, char *argv[])
{
    if(argc != 3 || atoi(argv[1]) < 2000 || atoi(argv[1]) > 50000)
        {
            fprintf(stderr, "./webserver_multi PORT(2001 ~ 49999) #_of_threads\n");
            return 0;
        }

    int i;
    port = atoi(argv[1]);
    numThread = atoi(argv[2]);
    thread_control(numThread);
    return 0;
}
