/* 
 * File:   proc.h
 * Author: nic
 *
 * Created on December 5, 2015, 12:42 AM
 */

#ifndef PROC_H
#define PROC_H

#include <sys/time.h>

#define NUM_CONSUMERS 4
#define NUM_PROC 20
#define QUEUE_SIZE 20
#define PRIO_MAX 140
#define PRIO_NORM_MIN 100
#define PRIO_NORM_MAX 140
#define SCHED_NORMAL 3
#define PRIO_REAL_MAX 100
#define MAX_SLEEP_AVG 10000
#define RATIO 5


// Structure for process information
typedef struct proc_struct {
    int id;                     // Process ID
    int static_prio;            // Process static priority 
    int dynamic_prio;           // Dynamic priority for NORMAL processes 
    int sched_type;             // Scheduling type (FIFO, NORMAL, or RR)
    int quantum;                // Time slice
    int time_execute;           // Estimated execution time
    int time_remain;            // Remaining time to completion
    int sleep_avg;              // Average sleep time for time slice calculation
    struct timeval sleep_start; // Time process starts sleeping in RQ
    struct timeval arrival_time;// Time process created
} proc_struct;    

// Structure for circular buffer
typedef struct consumer_struct {
    struct proc_struct rq_zero[QUEUE_SIZE];
    struct proc_struct rq_one[QUEUE_SIZE];
    struct proc_struct rq_two[QUEUE_SIZE];
    int zero_size;
    int one_size;
    int two_size;
} consumer_struct;

#endif /* PROC_H */

