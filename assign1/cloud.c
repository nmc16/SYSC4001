/*
 * cloud.c
 *
 * Server for controller to send information to. Sends information via
 * FIFO and is read based on client/server model. Displays
 * the alarm information send from the parent.
 *
 *  Created on: Oct 10, 2015
 *      Author: Nicolas McCallum 100936816
 */

#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include "message.h"

char running = 1;

/**
 * Signal handler that checks for the interrupt signal and stops the
 * program gracefully if entered.
 */
void signal_handler(int signum) {
	// Check if Control+C was pressed
	if (signum == SIGINT) {
		// Stop the program gracefully
		running = 0;
	}
}

int main(int argc, char *argv[]) {
	int server_fifo_id;
	struct proc_info pinfo;

	// Set up the signal handler
	struct sigaction new_signal;
	new_signal.sa_handler = signal_handler;
	sigemptyset(&new_signal.sa_mask);
	new_signal.sa_flags = 0;

	// Link the interrupt signal to the signal handler
	if (sigaction(SIGINT, &new_signal, NULL) != 0) {
		fprintf(stderr, "[ERROR] Could not link interrupt handler: %d\n", errno);
		exit(INITERR);
	}

	// Create the server side FIFO
	if (mkfifo(SERVER_FIFO_NAME, 0777) != 0) {
		fprintf(stderr, "[ERROR] Could not create server FIFO: %d\n", errno);
		exit(FICRERR);
	}
	printf("[INIT] Server FIFO created successfully...\n");

	// Open the server FIFO in read only mode
	server_fifo_id = open(SERVER_FIFO_NAME, O_RDONLY);
	if (server_fifo_id == -1) {
		// If there was an interrupt let the handler deal with it
		if (errno != EINTR) {
			fprintf(stderr, "[ERROR] Could not open server FIFO: %d\n", errno);
	    	exit(FIOPERR);
		}
	}
	printf("[INIT] Starting read on server FIFO...\n");

	while(running) {
		if (read(server_fifo_id, &pinfo, sizeof(pinfo)) > 0) {
			printf("[DATA] Controller sent data from PID %d (%s), device type %c, "
					"with data %d and threshold %ld\n", pinfo.pid, pinfo.name, pinfo.device, pinfo.data,
					pinfo.threshold);
		}
	}

	// Unlink the FIFO and release resources
	printf("[STOPPING] Closing server FIFO...\n");
	close(server_fifo_id);
	unlink(SERVER_FIFO_NAME);
	exit(0);
}

