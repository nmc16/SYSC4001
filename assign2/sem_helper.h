/*
 * sem_helper.h
 *
 *  Created on: Nov 4, 2015
 *      Author: nic
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
