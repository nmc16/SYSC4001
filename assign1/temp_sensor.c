/*
 * temp_sensor.c
 *
 *  Created on: Oct 3, 2015
 *      Author: N McCallum 100936816
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include "message.h"

char *name;
long int init_code = 3;
long int ack_code = 4;
long int stop_code = 5;
long int threshold;
int msgid;
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
        if (msgrcv(msgid, (void *)&msg, sizeof(msg.pinfo), ack_code, 0) == -1) {
            fprintf(stderr, "Failed during checking the init messages: %d\n", errno);
            exit(3);
        }

        // Make sure its the right process
        if (msg.pinfo.pid == getpid()) {
            printf("Received acknowledge signal from controller\n");
            break;
        }
    }
}

int check_for_stop() {
	if (msgrcv(msgid, (void *)&msg, sizeof(msg.pinfo), getpid(), IPC_NOWAIT) == -1) {
		if (errno != ENOMSG && errno != EAGAIN) {
			fprintf(stderr, "Failed during checking the stop message: %d\n", errno);
		    exit(3);
		}
		return 0;
	} else {
		if (msg.pinfo.data == stop_code) {
			return 1;
		} else {
			return 0;
		}
	}
}

void init_alarm(int data) {
	printf("!!! [ALARM] Temperature %ld greater than threshold!\n", data);
}

void send_data(int data) {
	msg.pinfo.data = data;
	if (msgsnd(msgid, (void *)&msg, sizeof(msg.pinfo), 0) == -1) {
		fprintf(stderr, "Failed to send data to message queue!\n");
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char *argv[]) {
	// Check that correct command line args were passed
	if (argc != 4) {
		perror("Temperature sensor takes exactly 3 arguments (Path for Message Queue, Name, Threshold)!\n");
		exit(1);
	}

	msgid = msgget(ftok(argv[1], 1), 0666 | IPC_CREAT);
	printf("Connecting to message queue: %d, key %d\n", msgid, ftok(argv[1], 1));

	// Remove this later
	name = malloc(sizeof(name));
	strcpy(name, argv[2]);
	threshold = strtol(argv[3], NULL, 10);

	// Set the message type to init
	msg.msg_type = init_code;

	strcpy(msg.pinfo.name, name);
	strcpy(msg.pinfo.action, "Start AC");
	msg.pinfo.device = TEMP_SENSOR_TYPE;
    msg.pinfo.data = 0;
	msg.pinfo.pid = getpid();
	msg.pinfo.threshold = threshold;

    send_init();    

    // Set the message type to device info
    msg.msg_type = 1;

	while(1) {
		// Look for quit message from controller
		if (check_for_stop()) {
			printf("Stop signal received, shutting down...\n");
			break;
		}

		// Randomize temperature data
		int r = rand() % (threshold + 10);
		printf("Temperature sensor %s reads temperature %ld (Threshold: %ld)\n", msg.pinfo.name, r, msg.pinfo.threshold);

		// Send the data over the message queue
		send_data(r);

		// Check if the value is over the threshold and print alarm if it is
		if (r > threshold) {
			init_alarm(r);
		}
		sleep(2);
	}

	exit(0);
}
