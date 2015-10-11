/*
 * actuator.c
 *
 * Holds the actuators for the smoke and temperature sensors.
 * Attributes are given via the command line: Message Queue Path,
 * Actuator Type, Actuator Name.
 *
 * Actuators send register signal to controller with an actuator type and wait
 * for an acknowledge signal back from the controller.
 *
 * Actuators will wait until a message is sent via the message queue with an
 * action, and then will perform that action by printing it to stdout. Sends
 * Acknowledgment back to controller that action has been processed.
 *
 * If control+C is pressed, the program sends a quit message to the
 * controller to delete it from the registered devices.
 *
 *  Created on: Oct 6, 2015
 *      Author: Nicolas McCallum 100936816
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "message.h"
#include "error_types.h"

int msgid;
char running = 1;
char *name;
char type;
struct proc_msg msg;

void send_init() {
	// Set the message type to init
	msg.msg_type = INITCODE;

    // Send init data
    if (msgsnd(msgid, (void *)&msg, sizeof(msg.pinfo), 0) == -1) {
		fprintf(stderr, "Failed to send init to message queue!\n");
		exit(EXIT_FAILURE);
	}
    printf("Sent init message to controller...\n");

    // Wait for ack signal back
    while(1) {
        if (msgrcv(msgid, (void *)&msg, sizeof(msg.pinfo), getpid(), 0) == -1) {
            fprintf(stderr, "Failed during checking the init messages: %d\n", errno);
            exit(3);
        }

        // Make sure its the right process
        if (msg.pinfo.data == ACKCODE) {
            printf("Received acknowledge signal from controller\n");
            break;
        }
    }
}

void send_ack() {
    msg.msg_type = 6;
    strcpy(msg.pinfo.name, name);
    strcpy(msg.pinfo.action, "Started AC");
    if (msgsnd(msgid, (void *)&msg, sizeof(msg.pinfo), 0) == -1) {
		fprintf(stderr, "Acknowledge signal failed to be sent\n");
	    exit(4);
	}
    printf("Sent acknowledge signal to controller!\n");
}

void set_type(char *input) {
	if (strcmp(input, "ac") == 0) {
		type = AC_ACTUATOR_TYPE;
	} else if (strcmp(input, "bell") == 0) {
		type = BELL_ACTUATOR_TYPE;
	} else {
		perror("[ERROR] Invalid actuator type entered. Must be one of: ac, bell!\n");
		exit(INITERR);
	}
}

void signal_handler(int signum) {
	// Check the interrupt
	switch(signum) {

	// Control+C was pressed
	case SIGINT:
		// End the program loop
		running = 0;

		// Send quit message to the controller
		msg.msg_type = QUITCODE;
		msg.pinfo.pid = getpid();
		if (msgsnd(msgid, (void *)&msg, sizeof(msg.pinfo), 0) == -1) {
			fprintf(stderr, "Quit signal failed to be sent\n");
			exit(4);
		}
	}
}

int main(int argc, char *argv[]) {
	// Check that correct command line args were passed
	if (argc != 4) {
		fprintf(stderr, "[ERROR] Temperature sensor takes exactly 3 arguments "
			            "(Path for Message Queue, Actuator Type, Name)!\n");
		exit(INITERR);
	}

	// Save parameters passed via command line
	name = malloc(sizeof(name));
	strcpy(name, argv[3]);
	set_type(argv[2]);

	// Set up the signal handler
	struct sigaction new_signal;
	new_signal.sa_handler = signal_handler;
	sigemptyset(&new_signal.sa_mask);
	new_signal.sa_flags = 0;
	if (sigaction(SIGINT, &new_signal, NULL) != 0) {
		perror("Error: Could not handle SIGINT");
		exit(INITERR);
	}

	// Connect to the message queue
	msgid = msgget(ftok(argv[1], 1), 0666 | IPC_CREAT);
	if (msgid == -1) {
		fprintf(stderr, "[ERROR] Error connecting to message queue: %d\n",
				errno);
		exit(MQGERR);
	}
	printf("Connecting to message queue: %d, key %d\n",
			msgid, ftok(argv[1], 1));

	// Copy the variables to the message struct
	strcpy(msg.pinfo.name, name);
	msg.pinfo.device = type;
	msg.pinfo.data = 0;
	msg.pinfo.pid = getpid();
	msg.pinfo.threshold = 0;

	// Send initialization message to controller and wait for acknowledgment
	send_init();

	// Run forever
	while(running) {
		// Look for quit message from controller
		if (msgrcv(msgid, (void *)&msg, sizeof(msg.pinfo), getpid(), 0) == -1) {
			fprintf(stderr, "Failed during checking the stop message: %d\n", errno);
			exit(3);
		}

		if (msg.pinfo.data == STOPCODE) {
			printf("Stop signal received, shutting down...\n");
			break;
		} else {
			printf("Triggered action \"%s\" caused by PID %d (%s)\n", msg.pinfo.action, msg.pinfo.pid, msg.pinfo.name);
			send_ack();
		}
	}

	exit(0);

}
