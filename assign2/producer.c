/*
 * producer.c
 *
 *  Producer that takes in the file using redirection and reads the
 *  data into a buffer of size BUFSIZ. Splits the buffer into
 *  shared memory structs with a buffer size of 128 that form
 *  the circular buffer.
 *
 *  Continues to read from the file until the EOF is reached which
 *  is indicated by a 0 from the return of read().
 *
 *  Created on: November 4, 2015
 *      Author: Nicolas McCallum 100936816
 */

#include "sem_helper.h"
#include "shm_helper.h"
#include "buffer.h"

int running = 1;
int bytes_read = 0;
int in = 0;
struct text_buf *shared_stuff;

/**
 * Alarm handler that will gracefully shutdown the producer when Control+C
 * is pressed so that the shared memory and semaphores are deleted.
 *
 * param signum: Signal identifier to check for
 */
void alarm_handler(int signum) {
	// Check the interrupt
	switch(signum) {
		// Control+C was pressed
		case SIGINT:
			running = 0;
	}
}

/**
 * Reads from the file passed in via redirection and splits it into
 * buffers that can be put on shared memory.
 *
 * param tb: Array of size 100 text_buf structs to place the copied information into
 * return: int of the amount of messages copied
 */
int produce(struct text_buf tb[100]) {
	char inbuf[BUFSIZ];
	int i = 0, nread, startindex = 0;

	// Read the file into the buffer and check if the read failed
	nread = read(0, inbuf, BUFSIZ);
	if (nread == -1) {
		fprintf(stderr, "Error occurred while reading the file! Error Code: %d\n", errno);
		exit(EXIT_FAILURE);
	}

	// Check if EOF has been reached and return if it has
	if (nread == 0) {
		return 0;
	}

	// Add to the amount of bytes read to keep track of multiple reads required on 1 file
	bytes_read = bytes_read + nread;
	int endindex = nread;

	do {
		// Set the indexes to read between
		startindex = TXTBUFSIZ * i;
		endindex = TXTBUFSIZ * (i + 1);
		if (endindex > nread) {
			endindex = nread;
		}

		// Copy the 128 byte or less substring into the buffer
		strncpy(tb[i].buffer, inbuf + startindex, endindex - startindex);

		// Set the count to the bytes read
		tb[i].count = endindex - startindex;

		// Display the information produced
		printf("Start: %d, End %d, i: %d, Read %d/%d bytes: %s\n\n",
				startindex, endindex, i, tb[i].count, nread, tb[i].buffer);

		// Loop until the index has reached the end of the bytes read
		i++;
	} while(endindex < nread);

	return i;
}

/**
 * Appends the text_buf given into shared memory at the location
 * pointed to by in. Continually increases in so there is a new
 * memory location to write to next.
 *
 * param tb: text_buf struct to add to shared memory
 */
void append(struct text_buf tb) {
	shared_stuff[in].count = tb.count;
	strncpy(shared_stuff[in].buffer, tb.buffer, tb.count);
	in = (in + 1) % NBUFFERS;
}

int main(void) {
	struct text_buf tb[100];
	void *shared_memory = (void *)0;

	// Declare integers for the semaphores S, N, and E and the shared memory of the buffers
	int semsid, semnid, semeid, shmid;
	int produced;

	// Set up the signal handler
	struct sigaction new_signal;
	new_signal.sa_handler = alarm_handler;
	sigemptyset(&new_signal.sa_mask);
	new_signal.sa_flags = SA_RESTART;

	// Add the handler to handle SIGALRM and SIGINT
	if (sigaction(SIGINT, &new_signal, NULL) != 0) {
		fprintf(stderr, "Error could not handle SIGINT");
		exit(EXIT_FAILURE);
	}

	// Get the shared memory ID and create it to be of size 100 buffers if not already
	shmid = shmget((key_t)SHMKEY, sizeof(struct text_buf) * NBUFFERS, 0666 | IPC_CREAT);
	if (shmid == -1) {
	    fprintf(stderr, "Could not get shared memory id! Error Code: %d\n", errno);
	    exit(EXIT_FAILURE);
	}

	// Get the IDs for the semaphores needed for mutex
	semsid = semget((key_t)SEMSKEY, 1, 0666 | IPC_CREAT);
	semnid = semget((key_t)SEMNKEY, 1, 0666 | IPC_CREAT);
	semeid = semget((key_t)SEMEKEY, 1, 0666 | IPC_CREAT);

	// Check that all the semaphores were successfully received
	if (semsid == -1 || semnid == -1 || semeid == -1) {
		fprintf(stderr, "Could not get one or more of the semaphores! Error Code: %d\n", errno);
		exit(EXIT_FAILURE);
	}

	// Initialize the semaphores to their initial values
	if (!init_sem(semsid, 1) || !init_sem(semnid, 0) || !init_sem(semeid, NBUFFERS)) {
		exit(EXIT_FAILURE);
	}

	// Map the pointer to the shared memory location
	shared_memory = shmat(shmid, (void *)0, 0);
	if (shared_memory == (void *)-1) {
	    fprintf(stderr, "Could not map shared memory! Error Code: %d\n", errno);
	    exit(EXIT_FAILURE);
	}

	printf("Memory attached at %X\n", (int)shared_memory);
	shared_stuff = (struct text_buf *)shared_memory;

	while(running) {
		// Produce a new message to put on the buffer
		produced = produce(tb);

		// If nothing is produced sleep so the producer stays alive
		if (produced == 0) {
			sleep(5);
		}

		int i;
		for(i = 0; i < produced; i++) {
			// Wait until there is empty space on the buffers
			if(!sem_wait(semeid)) {
				exit(EXIT_FAILURE);
			}

			// Wait until the consumer has left the critical section
			if(!sem_wait(semsid)) {
				exit(EXIT_FAILURE);
			}

			// Add the items
			append(tb[i]);

			// Release the semaphore for CS
			if (!sem_signal(semsid)) {
				exit(EXIT_FAILURE);
			}

			// Signal that an item has been added to the buffer
			if(!sem_signal(semnid)) {
				exit(EXIT_FAILURE);
			}
		}
	}

	// Detach the shared memory and delete it
	if (shmdt(shared_memory) == -1) {
	    fprintf(stderr, "Error could not detach from shared memory! Error Code: %d\n", errno);
	    exit(EXIT_FAILURE);
	}
	if (shmctl(shmid, IPC_RMID, 0) == -1) {
	    fprintf(stderr, "Error deleting the shared memory! Error Code: %d\n", errno);
	    exit(EXIT_FAILURE);
	}

	// Delete the semaphores
	if (!del_sem(semsid) || !del_sem(semnid) || !del_sem(semeid)) {
		fprintf(stderr, "Error deleting one or more semaphore(s)! Error Code: %d\n", errno);
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);

}
