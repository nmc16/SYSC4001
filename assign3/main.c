/* 
 * File:   main.c
 * Author: Nicolas McCallum 100936816
 * 
 * Main file that creates NUM_CONSUMERS consumers defined in the header file
 * proc.h and one producer to simulate a multiprocessor scheduler.
 * 
 * Spawns one producer thread that will produce NUM_PROC processes defined in 
 * proc.h and places them into one of 3 resource queues on one of the consumers.
 * 
 * Each process can be FIFO which executes without blocking, RR which will
 * execute until the time slice ends, or NORMAL which can be blocked or run until
 * the end of the time slice.
 * 
 * The consumer emulates the run time of the process and prints to stdout the
 * results of each execution.
 *
 * Created on December 5, 2015
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
void *emulate_run(proc_struct proc, int thread_num, int rq);


// Circular buffers for consumers
consumer_struct consumers[NUM_CONSUMERS];

// Mutex locks for each consumer
pthread_mutex_t mutexes[NUM_CONSUMERS];

// List of threads spawned (NUM_CONSUMERS + producer + balancer)
pthread_t threads[NUM_CONSUMERS + 2];

// Counter for process ID
int pid = 0;

/*
 * Main thread that will spawn the threads for the producer and consumer as well
 * as the load balancer.
 */
int main(int argc, char** argv) {
    void *t_result;
    int i = 0;
    
    // Use process ID to seed random values
    srand(getpid());
    
    // Create the mutex lock for threads
    printf("[MAIN] Creating mutex locks...\n");
    for (i = 0; i < NUM_CONSUMERS; i++) {
        if (pthread_mutex_init(&mutexes[i], NULL) != 0) {
            fprintf(stderr, "[MAIN] Could not initialize mutex! Errno: %d\n", 
                    errno);
            exit(EXIT_FAILURE);
        }
    }
    
    // Set the counters to 0 for the resource queues
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
    
    // Spawn the consumer threads and pass the consumer number to it
    for (i = 0; i < NUM_CONSUMERS; i++) {
        int *arg = malloc(sizeof(*arg));
        *arg = i;
        if (pthread_create(&(threads[i + 1]), NULL, consumer, arg) != 0) {
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
    
    // Delete mutexes 
    for (i = 0; i < NUM_CONSUMERS; i++) {
        if (pthread_mutex_destroy(&mutexes[i]) != 0) {
            fprintf(stderr, "[MAIN] Could not destroy mutex! Errno: %d\n", errno);
            exit(EXIT_FAILURE);
        }
    }
    
    printf("[MAIN] Program completed.\n");
    return (EXIT_SUCCESS);
}

/**
 * Producer function to be run by the producer thread, 
 * 
 * Creates random process information stored in proc_struct structure
 * defined in proc.h. Creates process with execution time 100ms to 500ms
 * and calculates the priority and time slice.
 * 
 * Adds the process to a consumer_struct. Attempts to add even number to each
 * consumer. Adds RR and FIFO type processes to run queue 0, and NORMAL types to
 * run queue 1.
 */
void *producer() {
    printf("[PRODUCER] Started...\n");
    
    // Create the new process
    proc_struct proc;

    // Create the max processes
    int c = 0;
    int i;
    for (i = 0; i < NUM_PROC; i++) {
        // Calculate new random priority
        int type = rand() % RATIO + 1;
        int priority;
        
        // 1/5 chance to be FIFO 
        if (type == 0) {
            priority = rand() % PRIO_REAL_MAX;
            proc.sched_type = SCHED_FIFO;
            
        } else if (type == 1) {
            // 1/5 chance to be RR    
            priority = rand() % PRIO_REAL_MAX;
            proc.sched_type = SCHED_RR;
        
        } else {
            // 3/5 chance to be normal
            priority = PRIO_NORM_MIN + rand() % (PRIO_NORM_MAX - PRIO_NORM_MIN);
            proc.sched_type = SCHED_NORMAL;
        }
        proc.static_prio = priority;
        proc.dynamic_prio = priority;
        
        // Reset the process average sleep time to 0
        proc.sleep_avg = 0;
        
        // Calculate the time slice for the process
        int quantum;
        if (priority < 120) {
            quantum = (140 - priority) * 20;
        } else {
            quantum = (140 - priority) *5;
        }
        
        // Make quantum larger so there is not as much output
        proc.quantum = quantum * 1000;
        
        // Set the execute time to 100ms to 500ms in microseconds
        int exec = 100000 + rand() % 400000;
        proc.time_execute = exec;
        proc.time_remain = exec;
        
        // Set the arrival time of the process and start sleep timer
        gettimeofday(&proc.arrival_time, NULL);
        gettimeofday(&proc.sleep_start, NULL);
        
        // Request mutex lock
        pthread_mutex_lock(&mutexes[c]);
        
        // Set the new process ID
        proc.id = pid;
        pid++;
        
        // If the priority is FIFO or RR put into RQ0 else add to RQ1
        if (priority < 100) {
            consumers[c].rq_zero[consumers[c].zero_size] = proc;
            consumers[c].zero_size = (consumers[c].zero_size + 1) % QUEUE_SIZE;
        
        } else {
            // Otherwise put the task in RQ1
            consumers[c].rq_one[consumers[c].one_size] = proc;
            consumers[c].one_size = (consumers[c].one_size + 1) % QUEUE_SIZE;
        }
        
        // Release lock
        pthread_mutex_unlock(&mutexes[c]);
        
        // Increase consumer counter
        c = (c + 1) % NUM_CONSUMERS;
        
        printf("[PRODUCER] Created process: pid %d, priority %d, quantum %d and"
                " expected execution time %.4f ms\n", proc.id, proc.static_prio,
                proc.quantum, (exec / 1000));
        
        // Remove sleep if not worrying about output speed
        usleep(100000);
    }
    
    pid++;
    printf("[PRODUCER] Done.\n");
    pthread_exit(NULL);
}

/**
 * Consumer thread function that selects the highest priority process from
 * one of the run queues in order RQ0 > RQ1 > RQ2.
 * 
 * Removes the process selected and emulates the run.
 * 
 * Once there are no more processes left in the run queues and the producer
 * is not producing anymore, the thread exits.
 * 
 * @param arg int value that represents the consumer number
 */
void *consumer(void *arg) {
    int thread_num = *((int *) arg);
    printf("[CONSUMER %d] Started...\n", thread_num);
    
    // Create placeholder for running process
    proc_struct proc;
    
    while (1) {
        // Request mutex lock
        pthread_mutex_lock(&mutexes[thread_num]);
    
        // Check process queues and select the process with the highest priority
        if (consumers[thread_num].zero_size > 0) {
            proc = *find_and_remove_min_prio(consumers[thread_num].rq_zero, 
                                             consumers[thread_num].zero_size);
            
            consumers[thread_num].zero_size = (consumers[thread_num].zero_size - 1) % QUEUE_SIZE;
            pthread_mutex_unlock(&mutexes[thread_num]);
            emulate_run(proc, thread_num, 0);

        } else if (consumers[thread_num].one_size > 0) {
            proc = *find_and_remove_min_prio(consumers[thread_num].rq_one, 
                                             consumers[thread_num].one_size);
            
            consumers[thread_num].one_size = (consumers[thread_num].one_size - 1) % QUEUE_SIZE;
            pthread_mutex_unlock(&mutexes[thread_num]);
            emulate_run(proc, thread_num, 1);
    
        } else if (consumers[thread_num].two_size > 0) {
            proc = *find_and_remove_min_prio(consumers[thread_num].rq_two, 
                                             consumers[thread_num].two_size);
            
            consumers[thread_num].two_size = (consumers[thread_num].two_size - 1) % QUEUE_SIZE;
            pthread_mutex_unlock(&mutexes[thread_num]);
            emulate_run(proc, thread_num, 2);
    
        } else {
            // Check if there are no more processes being created and exit if not
            if (pid >= NUM_PROC) {
                pthread_mutex_unlock(&mutexes[thread_num]);
                printf("[CONSUMER %d] No programs left to execute! Exiting...\n", thread_num);
                pthread_exit(NULL);
            }
            pthread_mutex_unlock(&mutexes[thread_num]);
        }
    }
    
}

/**
 * Function that finds the lowest priority value (ie highest priority process)
 * and removes it from the circular buffer by shifting all other processes
 * by one index towards index 0.
 * 
 * @param rq run queue to select process from
 * @param count size of the run queue
 * @return highest priority process to run
 */
proc_struct *find_and_remove_min_prio(proc_struct rq[QUEUE_SIZE], int count) {
    int min_index = 0, min = PRIO_MAX, i;
    
    // Find the lowest priority value in the list and save the index
    for (i = 0; i < count; i++) {
        if (rq[i].dynamic_prio < min) {
            min = rq[i].dynamic_prio;
            min_index = i;
        }
    }
    
    // Store the process and create a return pointer to it
    proc_struct proc = rq[min_index];
    proc_struct *p = malloc(sizeof(proc));
    *p = proc;

    // Shift all the values after the removed value by 1
    for (i = min_index; i < count - 1; i++) {
        rq[i] = rq[i + 1];
    }
    
    return p;
}

/**
 * Thread function that balances the run queues in each consumer so they
 * have even numbers of processes in each queue.
 */
void *balance() {
    printf("Balancer!\n");
    pthread_exit(NULL);
}

/**
 * Function that emulates the process execution time given the schedule type.
 * 
 * If the type is FIFO, it runs for the entire "estimated" execution time
 * and completes. If the type is RR it runs until time slice or time remaining
 * ends. If the type is NORMAL runs until blocked or until time slice ends.
 * 
 * If a process is blocked (ie waiting for event) it waits for 5 times the 
 * time slice to emulate the block.
 * 
 * Keeps track of remaining execution time and adds the process back 
 * to the appropriate run queue if it has not finished.
 * 
 * Calculates and displays turnaround time for each finished process.
 * 
 * @param proc process to emulate
 * @param thread_num thread number to add process back to
 * @param rq run queue to add process back to
 */
void *emulate_run(proc_struct proc, int thread_num, int rq) {
    int add_back = 0;
    
    // Check the process schedule type 
    if (proc.sched_type == SCHED_FIFO) {
        // If FIFO then it runs to completion without blocking
        usleep(proc.time_execute);
        
        // Convert time to milliseconds to display
        float exec = proc.time_execute / 1000;
        
        // Calculate the turnaround time
        struct timeval finish;
        gettimeofday(&finish, NULL);
        float turnaround =  (double) (finish.tv_usec - proc.arrival_time.tv_usec) / 1000 +
                            (double) (finish.tv_sec - proc.arrival_time.tv_sec) * 1000;
        
        // Print the process information
        printf("[CONSUMER %d] Finished FIFO process: pid %d, priority %d, "
               "from RQ%d for %#.2f ms. Turnaround time was %#.3f ms.\n", 
               thread_num, proc.id, proc.dynamic_prio, rq, exec, turnaround);
        
        add_back = 0;
        
    } else if (proc.sched_type == SCHED_RR) {
        // If RR then it executes for time slice/quantum
        // If there is less time remaining than quantum, finish process
        if (proc.time_remain < proc.quantum) {
            usleep(proc.time_remain);
        
            // Calculate the turnaround time
            struct timeval finish;
            gettimeofday(&finish, NULL);
            float turnaround =  (double) (finish.tv_usec - proc.arrival_time.tv_usec) / 1000 +
                                (double) (finish.tv_sec - proc.arrival_time.tv_sec) * 1000;
        
            // Print the process information
            printf("[CONSUMER %d] Finished RR process: pid %d, priority %d, "
                   "from RQ%d for %d us. Turnaround time was %#.3f ms.\n", 
                    thread_num, proc.id, proc.dynamic_prio, rq, 
                    proc.time_remain, turnaround);
            
            add_back = 0;
            
        } else {
            // Run process for time slice/quantum and put back into RQ0
            usleep(proc.quantum);
            
            // Set the new remaining time
            proc.time_remain = proc.time_remain - proc.quantum;
            
            printf("[CONSUMER %d] Executed RR process: pid %d, priority %d, "
                    "quantum %d from RQ%d for %d us. Time remaining %d us\n", 
                    thread_num, proc.id, proc.dynamic_prio, proc.quantum, rq,
                    proc.quantum, proc.time_remain);
            
            add_back = 1;
        }
        
    } else {
        // Process is type NORMAL
        if (proc.time_remain < proc.quantum) {
            usleep(proc.time_remain);
        
            // Calculate the turnaround time
            struct timeval finish;
            gettimeofday(&finish, NULL);
            float turnaround =  (double) (finish.tv_usec - proc.arrival_time.tv_usec) / 1000 +
                                (double) (finish.tv_sec - proc.arrival_time.tv_sec) * 1000;
        
            // Print the process information
            printf("[CONSUMER %d] Finished NORMAL process: pid %d, priority %d, "
                   "from RQ%d for %d us. Turnaround time was %#.3f ms.\n", 
                    thread_num, proc.id, proc.dynamic_prio, rq, 
                    proc.time_remain, turnaround);
            
            add_back = 0;
            
        } else {
            int old_prio = proc.dynamic_prio;
            
            // Get the sleep time in clock ticks and add to sleep average
            struct timeval sleep_end, execution_end;
            gettimeofday(&sleep_end, NULL);
            double sleep_time = (double) (sleep_end.tv_usec - proc.sleep_start.tv_usec) +
                                (double) (sleep_end.tv_sec - proc.sleep_start.tv_sec);
        
            float ticks = sleep_time / 100;
        
            // Set the sleep average to the max value if it exceeds it
            if (proc.sleep_avg + ticks > MAX_SLEEP_AVG) {
                proc.sleep_avg = MAX_SLEEP_AVG;
            } else {
                proc.sleep_avg = proc.sleep_avg + ticks;
            }
            
            // Generate random number to see if process needs to wait for event
            int wait = rand() % 2;
            int s;
            if (!wait) {
                // Do not use the entire time slice and wait for event
                usleep(10000 + (proc.quantum * 5));
                s = 10000;
                
                // Set the new remaining time
                proc.time_remain = proc.time_remain - 10000;
                
            } else {
                // Otherwise use entire timeslice
                usleep(proc.quantum);
                s = proc.quantum;
                
                // Set the new remaining time
                proc.time_remain = proc.time_remain - proc.quantum;
            }
            
            // Set the process back to sleep
            gettimeofday(&proc.sleep_start, NULL);
            
            // Calculate the ticks that passed during execution
            gettimeofday(&execution_end, NULL);
            sleep_time = (double) (execution_end.tv_usec - sleep_end.tv_usec) +
                        (double) (execution_end.tv_sec - sleep_end.tv_sec);
        
            ticks = sleep_time / 100;
        
            // Subtract the ticks from the sleep average
            proc.sleep_avg = proc.sleep_avg - ticks;
        
            // Calculate bonus factor
            int bonus = 10 * (proc.sleep_avg / MAX_SLEEP_AVG);
            
            // Set the dynamic priority according depending on bonus
            int dynamic_prio;
            if (proc.dynamic_prio - bonus + 5 < 139) {
                dynamic_prio = proc.dynamic_prio - bonus + 5;
            } else {
                dynamic_prio = 139;
            }
        
            // Make sure new priority not set to real time process
            if (dynamic_prio < 100) {
                dynamic_prio = 100;
            }
            
            proc.dynamic_prio = dynamic_prio;
            
            printf("[CONSUMER %d] Executed NORMAL process: pid %d, old priority %d,"
                    " new priority %d, quantum %d from RQ%d for %d us. Time remaining %d us\n", 
                    thread_num, proc.id, old_prio, proc.dynamic_prio, proc.quantum, rq,
                    s, proc.time_remain);
            
            // Set the resource queue to return to
            if (dynamic_prio > 130) {
                rq = 2;
            } else {
                rq = 1;
            }
            
            add_back = 1;
        }
    }

    // Add back to RQ if process did not finish
    if (add_back) {
        pthread_mutex_lock(&mutexes[thread_num]);
        if (rq == 0) {
            consumers[thread_num].rq_zero[consumers[thread_num].zero_size] = proc;
            consumers[thread_num].zero_size = (consumers[thread_num].zero_size + 1) % QUEUE_SIZE;
        } else if (rq == 1) {
            consumers[thread_num].rq_one[consumers[thread_num].one_size] = proc;
            consumers[thread_num].one_size = (consumers[thread_num].one_size + 1) % QUEUE_SIZE;
         } else {
            consumers[thread_num].rq_two[consumers[thread_num].two_size] = proc;
            consumers[thread_num].two_size = (consumers[thread_num].two_size + 1) % QUEUE_SIZE;
         }
        pthread_mutex_unlock(&mutexes[thread_num]);
    }
}