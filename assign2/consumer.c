/*
 * consumer.c
 *
 *  Consumer that constantly reads from the shared memory and writes it to
 *  stdout unless redirected to a file.
 *
 *  Created on: November 5, 2015
 *      Author: Nicolas McCallum 100936816
 */

#include "sem_helper.h"
#include "shm_helper.h"
#include "buffer.h"

int running = 1;
struct text_buf *shared_stuff;
int out = 0;

/**
 * Alarm handler that will gracefully shutdown the consumer when Control+C
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
 * Takes text_buf struct off the shared memory at the current out location
 * and returns it. Increases the out counter for the buffer.
 *
 * return: text_buf of the object taken off the buffer
 */
struct text_buf take() {
	struct text_buf tb;

	// Set the count and buffer from the buffer on shared memory
	tb.count = shared_stuff[out].count;
	strncpy(tb.buffer, shared_stuff[out].buffer, tb.count);

	// Increase pointer location and return the copied buffer
	out = (out + 1) % NBUFFERS;
	return tb;
}

/**
 * Writes the text_buf given by the parameter into stdout unless redirected
 * to a file. Compares the write byte count with the byte count held in the
 * buffer count variable and exists if the values are different.
 *
 * param tb: text_buf read from shared memory to write into the file
 */
void write_to_file(struct text_buf tb) {
	if (write(1, tb.buffer, tb.count) != tb.count) {
		fprintf(stderr, "Error occurred during write operation! Write and buffer byte size do not match!\n");
		exit(EXIT_FAILURE);
	}
}

int main(void) {

	struct text_buf tb;
	void *shared_memory = (void *)0;

	// Declare integers for the semaphores S, N, and E and the shared memory of the buffers
	int semsid, semnid, semeid, shmid;

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

	// Map the pointer to the shared memory location
	shared_memory = shmat(shmid, (void *)0, 0);
	if (shared_memory == (void *)-1) {
	    fprintf(stderr, "Could not map shared memory! Error Code: %d\n", errno);
	    exit(EXIT_FAILURE);
	}

	printf("Memory attached at %X\n", (int)shared_memory);
	shared_stuff = (struct text_buf *)shared_memory;

	while(running) {
		// Wait until there is something to read on shared memory buffer
		sem_wait(semnid);

		// Wait to enter CS
		sem_wait(semsid);

		// Take the message from the buffer
		tb = take();

		// Signal we left CS
		sem_signal(semsid);

		// Signal new space available on buffer
		sem_signal(semeid);

		// Write the text to the file
		write_to_file(tb);
	}

	// Detach the shared memory
	if (shmdt(shared_memory) == -1) {
	    fprintf(stderr, "Error could not detach from shared memory! Error Code: %d\n", errno);
	    exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
