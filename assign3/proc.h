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
#define PRIO_REAL_MAX 100
#define RATIO 5

// Structure for process information
typedef struct proc_struct {
    int id;
    int priority;
    int sched_type;
    int quantum;
    int time_execute;
    int time_remain;
    struct timeval starttime;
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

