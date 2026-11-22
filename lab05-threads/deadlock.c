#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

/*
 * Звичайна збірка:   gcc -Wall -Wextra -pthread deadlock.c -o deadlock
 *                    (threads беруть mutex у різному порядку -> deadlock)
 * Виправлена версія: gcc -Wall -Wextra -pthread -DFIXED deadlock.c -o deadlock_fixed
 *                    (обидва threads беруть mutex1, потім mutex2)
 */

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

void *worker1(void *arg)
{
    (void)arg;

    printf("Thread 1: locking mutex1\n");
    pthread_mutex_lock(&mutex1);
    sleep(1);

    printf("Thread 1: locking mutex2\n");
    pthread_mutex_lock(&mutex2);

    printf("Thread 1: critical section\n");

    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    return NULL;
}

void *worker2(void *arg)
{
    (void)arg;

#ifdef FIXED
    /* однаковий порядок блокувань: mutex1 -> mutex2 */
    printf("Thread 2: locking mutex1\n");
    pthread_mutex_lock(&mutex1);
    sleep(1);

    printf("Thread 2: locking mutex2\n");
    pthread_mutex_lock(&mutex2);

    printf("Thread 2: critical section\n");

    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
#else
    /* зворотний порядок блокувань: mutex2 -> mutex1 (небезпечно) */
    printf("Thread 2: locking mutex2\n");
    pthread_mutex_lock(&mutex2);
    sleep(1);

    printf("Thread 2: locking mutex1\n");
    pthread_mutex_lock(&mutex1);

    printf("Thread 2: critical section\n");

    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
#endif
    return NULL;
}

int main(void)
{
    pthread_t thread1;
    pthread_t thread2;

    pthread_create(&thread1, NULL, worker1, NULL);
    pthread_create(&thread2, NULL, worker2, NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("Both threads finished\n");
    return 0;
}
