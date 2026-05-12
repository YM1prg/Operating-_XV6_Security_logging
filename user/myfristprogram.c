#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
Void *worker(void *arg) {
Char *message = (char *)arg;
printf("Thread says: %s\n", message);
return NULL;
}
int main() {
pthread_t thread1, thread2;
Char *msg1 = "Hello from Thread 1!";
Char *msg2 = "Hello from Thread 2!";
// Create threads
pthread_create(&thread1, NULL, worker, msg1);
pthread_create(&thread2, NULL, worker, msg2);
// Wait for threads to finish
pthread_join(thread1, NULL);
pthread_join(thread2, NULL);
printf("Main thread finished.\n");
return 0;
}
