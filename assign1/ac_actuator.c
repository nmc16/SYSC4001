/*
 * ac_actuator.c
 *
 *  Created on: Oct 6, 2015
 *      Author: N McCallum 100936816
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include "message.h"

int msgid;
char *name;
struct proc_msg msg;

void send_init() {
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

int main(int argc, char *argv[]) {
	// Check that correct command line args were passed
	if (argc != 3) {
		perror("Temperature sensor takes exactly 2 argument (Path for Message Queue, Name)!\n");
		exit(1);
	}

	name = malloc(sizeof(name));
	strcpy(name, argv[2]);

	msgid = msgget(ftok(argv[1], 1), 0666 | IPC_CREAT);
	printf("Connecting to message queue: %d, key %d\n", msgid, ftok(argv[1], 1));

	// Set the message type to init
	msg.msg_type = INITCODE;

	strcpy(msg.pinfo.name, name);
	strcpy(msg.pinfo.action, "Start AC");
	msg.pinfo.device = AC_ACTUATOR_TYPE;
	msg.pinfo.data = 0;
	msg.pinfo.pid = getpid();
	msg.pinfo.threshold = 0;

	send_init();

	while(1) {
		// Look for quit message from controller
		if (msgrcv(msgid, (void *)&msg, sizeof(msg.pinfo), getpid(), 0) == -1) {
			fprintf(stderr, "Failed during checking the stop message: %d\n", errno);
			exit(3);
		}

		if (msg.pinfo.data == STOPCODE) {
			printf("Stop signal received, shutting down...\n");
			break;
		} else {
			printf("Started AC, from action %s caused by PID %d\n", msg.pinfo.action, msg.pinfo.pid);
			send_ack();
		}
	}

	exit(0);

}
