/* 
 * File:   main.c
 * Author: nic
 *
 * Created on December 5, 2015, 12:04 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "proc.h"

void *producer();
void *consumer(void *arg);
proc_struct *find_and_remove_min_prio(proc_struct rq[QUEUE_SIZE], int count);
void *balance();

// Circular buffers for consumers
consumer_struct consumers[NUM_CONSUMERS];

// Mutex lock for load balancing
pthread_mutex_t mutex;

// List of threads spawned (NUM_CONSUMERS + producer + balancer)
pthread_t threads[NUM_CONSUMERS + 2];

// Counter for process ID
int pid = 0;

/*
 * 
 */
int main(int argc, char** argv) {
    void *t_result;
    
    // Use process ID to seed random values
    srand(getpid());
    
    printf("[MAIN] Creating mutex lock...\n");
    // Create the mutex lock for thread balancing
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        fprintf(stderr, "[MAIN] Could not initialize mutex! Errno: %d\n", 
                errno);
        exit(EXIT_FAILURE);
    }
    
    // Set the counters to 0 for the resource queues
    int i;
    for (i = 0; i < NUM_CONSUMERS; i++) {
        consumers[i].zero_size = 0;
        consumers[i].one_size = 0;
        consumers[i].two_size = 0;
    }
    
    printf("[MAIN] Done. Spawning threads...\n");
    // Spawn the producer thread
    if (pthread_create(&(threads[0]), NULL, producer, NULL) != 0) {
        fprintf(stderr, "[MAIN] Could not spawn producer thread! Errno: %d\n", 
                errno);
        exit(EXIT_FAILURE);
    }
    
    // Spawn the consumer threads
    for (i = 0; i < NUM_CONSUMERS; i++) {
        if (pthread_create(&(threads[i + 1]), NULL, consumer, (void *)&i) != 0) {
            fprintf(stderr, "[MAIN] Could not spawn consumer thread (index %d)!"
                    " Errno: %d\n", i, errno);
        exit(EXIT_FAILURE);
        }
    }
    
    // Spawn the balancer thread
    if (pthread_create(&(threads[NUM_CONSUMERS + 1]), NULL, balance, NULL) != 0) {
        fprintf(stderr, "[MAIN] Could not spawn balancer thread! Errno: %d\n", 
                errno);
        exit(EXIT_FAILURE);
    }
    
    
    printf("[MAIN] Done. Waiting for threads to complete...\n");
    // Wait for all threads to complete
    for (i = 0; i < NUM_CONSUMERS + 2; i++) {
        if (pthread_join(threads[i], &t_result) != 0) {
            fprintf(stderr, "[MAIN] Could not join thread (index %d)! Errno: "
                    "%d\n", i, errno);
            exit(EXIT_FAILURE);
        }
    }
    
    // Delete mutex 
    if (pthread_mutex_destroy(&mutex) != 0) {
        fprintf(stderr, "[MAIN] Could not destroy mutex! Errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }
    
    printf("[MAIN] Program completed.\n");
    
    return (EXIT_SUCCESS);
}

void *producer() {
    printf("[PRODUCER] Started...\n");
    
    // Create the new process
    proc_struct proc;

    // Create the max processes
    int c = 0;
    int i;
    for (i = 0; i < NUM_PROC; i++) {
        // Calculate new random priority
        int type = rand() % RATIO;
        int priority;
        
        // 1/5 chance to be FIFO 
        if (type == 0) {
            priority = rand() % PRIO_REAL_MAX;
            proc.sched_type = SCHED_FIFO;
            
        // 1/5 chance to be RR    
        } else if (type == 1) {
            priority = rand() % PRIO_REAL_MAX;
            proc.sched_type = SCHED_RR;
        
        // 3/5 chance to be normal
        } else {
            priority = PRIO_NORM_MIN + rand() % (PRIO_NORM_MAX - PRIO_NORM_MIN);
            proc.sched_type = SCHED_RR;
        }
        proc.priority = priority;
        
        // Set the new process ID
        proc.id = pid;
        pid++;
        
        // Calculate the time slice for the process
        int quantum;
        if (priority < 120) {
            quantum = (140 - priority) * 20;
        } else {
            quantum = (140 - priority) *5;
        }
        proc.quantum = quantum;
        
        // TODO change the hard coded values?
        // Set the execute time to hard coded values for now
        proc.time_execute = 1000;
        proc.time_remain = 1000;
        gettimeofday(&proc.starttime, NULL);
        
        // Request mutex lock
        pthread_mutex_lock(&mutex);
        consumer_struct consumer = consumers[c];
        
        // If the priority is FIFO or RR put into RQ0 else add to RQ1
        if (priority < 100) {
            consumer.rq_zero[consumer.zero_size] = proc;
            printf("(size %d)\n", consumer.zero_size);
            consumer.zero_size++;
            printf("Stored in consumer %d in RQ0 (size %d)\n", c, consumer.zero_size);
        } else {
            consumer.rq_one[consumer.one_size] = proc;
            printf("(size %d)\n", consumer.one_size);
            consumer.one_size++;
            printf("Stored in consumer %d in RQ1 (size %d)\n", c, consumer.one_size);
        }
        
        // Release lock
        pthread_mutex_unlock(&mutex);
        
        // Increase consumer counter
        c = (c + 1) % NUM_CONSUMERS;
        
        printf("[PRODUCER] Created process: pid %d, priority %d, quantum %d\n", proc.id, proc.priority, proc.quantum);
    }
    
    
    printf("[PRODUCER] Done.\n");
    pthread_exit(NULL);
}

void *consumer(void *arg) {
    int thread_num = *(int *) arg;
    printf("[CONSUMER %d] Started...\n", thread_num);
    
    // Create placeholder for running process
    proc_struct proc;
    
    while (1) {
        // Request mutex lock
        pthread_mutex_lock(&mutex);
        consumer_struct consumer = consumers[thread_num];
    
        // Check process queues and select the process with the highest priority
        if (consumer.zero_size > 0) {
            proc = *find_and_remove_min_prio(consumer.rq_zero, consumer.zero_size);
            consumer.zero_size--;
            printf("[CONSUMER %d] Executed process: pid %d, priority %d, quantum %d\n", thread_num, proc.id, proc.priority, proc.quantum);
    
        } else if (consumer.one_size > 0) {
            proc = *find_and_remove_min_prio(consumer.rq_one, consumer.one_size);
            consumer.one_size--;
            printf("[CONSUMER %d] Executed process: pid %d, priority %d, quantum %d\n", thread_num, proc.id, proc.priority, proc.quantum);
    
        } else if (consumer.two_size > 0) {
            proc = *find_and_remove_min_prio(consumer.rq_two, consumer.two_size);
            consumer.two_size--;
            printf("[CONSUMER %d] Executed process: pid %d, priority %d, quantum %d\n", thread_num, proc.id, proc.priority, proc.quantum);
    
        } else {
            // Check if there are no more processes being created and exit if not
            if (pid >= NUM_PROC - 1) {
                pthread_mutex_unlock(&mutex);
                printf("[CONSUMER %d] No programs left to execute!\n", thread_num);
                pthread_exit(NULL);
            }
        }
        
        pthread_mutex_unlock(&mutex);
    }
    
}

proc_struct *find_and_remove_min_prio(proc_struct rq[QUEUE_SIZE], int count) {
    int min_index = 0, min = PRIO_MAX, i;
    for (i = 0; i < count; i++) {
        if (rq[i].priority < min) {
            min = rq[i].priority;
            min_index = i;
        }
    }
    
    proc_struct proc = rq[min_index];
    proc_struct *p = malloc(sizeof(proc));
    *p = proc;
    
    for (i = min_index; i < count - 1; i++) {
        rq[i] = rq[i + 1];
    }
    
    return p;
}

void *balance() {
    printf("Balancer!\n");
    pthread_exit(NULL);
}

