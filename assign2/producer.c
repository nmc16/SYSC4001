/*
 * producer.c
 *
 *  Created on: Nov 4, 2015
 *      Author: nic
 */

#include <errno.h>
#include <string.h>
#include "sem_helper.h"
#include "shm_helper.h"
#include "buffer.h"

int running = 1;
int bytes_read = 0;
int in = 0;
struct text_buf *shared_stuff;

struct text_buf produce() {
	struct text_buf tb;
	int nread;

	nread = read(bytes_read, tb.buffer, TXTBUFSIZ);
	if (nread == -1) {
		fprintf(stderr, "Error occurred while reading the file! Error Code: %d", errno);
		exit(EXIT_FAILURE);
	}

	bytes_read = bytes_read + nread;
	tb.count = nread;
	printf("Read %d bytes: \n%s\n", nread, tb.buffer);
	return tb;
}

void append(struct text_buf tb) {
	printf("in: %d, count: %d, string: %s\n", in, tb.count, tb.buffer);
	shared_stuff[in] = tb;
	in = (in + 1) % NBUFFERS;
}

void main(void) {

	struct text_buf tb;
	void *shared_memory = (void *)0;

	// Declare integers for the semaphores S, N, and E and the shared memory of the buffers
	int semsid, semnid, semeid, shmid;


	shmid = shmget((key_t)SHMKEY, sizeof(struct text_buf) * NBUFFERS, 0666 | IPC_CREAT);
	semsid = semget((key_t)SEMSKEY, 1, 0666 | IPC_CREAT);
	semnid = semget((key_t)SEMNKEY, 1, 0666 | IPC_CREAT);
	semeid = semget((key_t)SEMEKEY, 1, 0666 | IPC_CREAT);

	// Initialize the semaphores
	if (!init_sem(semsid, 1) || !init_sem(semnid, 0) || !init_sem(semeid, NBUFFERS)) {
		exit(EXIT_FAILURE);
	}

	shared_memory = shmat(shmid, (void *)0, 0);
	if (shared_memory == (void *)-1) {
	    fprintf(stderr, "Could not map shared memory! Error Code: %d\n", errno);
	    exit(EXIT_FAILURE);
	}

	printf("Memory attached at %X\n", (int)shared_memory);
	shared_stuff = (struct text_buf *)shared_memory;

	while (running) {
		// Produce a new message to put on the buffer
		tb = produce();

		// Wait until there is empty space on the buffers
		sem_wait(semeid);

		// Wait until the consumer has left the critical section
		sem_wait(semsid);

		// Add the items
		append(tb);
		printf("got here\n");

		// Release the semaphore for CS
		sem_signal(semsid);

		// Signal that an item has been added to the buffer
		sem_signal(semnid);
	}

}
