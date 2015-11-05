/*
 * sem_helper.h
 *
 *	Header file for semaphore operations. Gives producer and
 *	consumer access to sem_helper.c methods. Also holds constants
 *	for the number of buffers allowed and the keys for each semaphore.
 *
 *  Created on: November 4, 2015
 *      Author: Nicolas McCallum 100936816
 */

#ifndef SEM_HELPER_H_
#define SEM_HELPER_H_

#define NBUFFERS 100
#define SEMSKEY 1234
#define SEMNKEY 1235
#define SEMEKEY 1236

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/sem.h>

union semun {
    int val;                    /* value for SETVAL */
    struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
    unsigned short int *array;  /* array for GETALL, SETALL */
    struct seminfo *__buf;      /* buffer for IPC_INFO */
};

extern int init_sem(int semid, int value);
extern int del_sem(int semid);
extern int sem_wait(int semid);
extern int sem_signal(int semid);

#endif /* SEM_HELPER_H_ */
