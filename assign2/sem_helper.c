/*
 * sem_helper.c
 *
 * 	Holds shared methods for semaphore operations that both the consumer and
 * 	producer use.
 *
 * 	Can initialize a semaphore to a value, delete a semaphore, and perform
 * 	the wait() and signal() methods.
 *
 *  Created on: November 4, 2015
 *      Author: Nicolas McCallum 100936816
 */
#include <errno.h>
#include "sem_helper.h"

int init_sem(int semid, int value) {
    union semun sem_union;

    sem_union.val = value;
    if (semctl(semid, 0, SETVAL, sem_union) == -1) {
    	fprintf(stderr, "Semaphore (%d) could not be initialized! Error Code: %d\n", semid, errno);
    	return 0;
    }
    return 1;
}

int del_sem(int semid) {
    union semun sem_union;

    if (semctl(semid, 0, IPC_RMID, sem_union) == -1) {
		fprintf(stderr, "Semaphore (%d) could not be deleted! Error Code: %d\n", semid, errno);
		return 0;
    }
    return 1;
}

int sem_wait(int semid) {
    struct sembuf sem_b;

    sem_b.sem_num = 0;
    sem_b.sem_op = -1;
    sem_b.sem_flg = SEM_UNDO;

    if (semop(semid, &sem_b, 1) == -1) {
        fprintf(stderr, "Wait on semaphore id %d failed! Error Code: %d\n", semid, errno);
        return 0;
    }
    return 1;
}

int sem_signal(int semid) {
	struct sembuf sem_b;

	sem_b.sem_num = 0;
	sem_b.sem_op = 1;
	sem_b.sem_flg = SEM_UNDO;

	if (semop(semid, &sem_b, 1) == -1) {
		fprintf(stderr, "Signal on semaphore id %d failed! Error Code %d", semid, errno);
		return 0;
	}
	return 1;
}
