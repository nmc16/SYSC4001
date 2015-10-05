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
#include "message.h"

char *name;
long int threshold;
int msgid;
struct proc_msg msg;

void init_alarm(int data) {
	printf("[ALARM] Temperature %ld greater than threshold!\n", data);
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
		perror("Temperature sensor takes exactly 3 arguments (Path for Message Queue, Name, Threshold)!");
		exit(1);
	}

	msgid = msgget(ftok(argv[1], 1), 0666 | IPC_CREAT);
	printf("Connecting to message queue: %d, key %d\n", msgid, ftok(argv[1], 1));

	// Remove this later
	name = malloc(sizeof(name));
	strcpy(name, argv[2]);
	threshold = strtol(argv[3], NULL, 10);


	msg.msg_type = 1;
	strcpy(msg.pinfo.name, name);
	strcpy(msg.pinfo.action, "Start AC");
	msg.pinfo.device = 0;
	msg.pinfo.pid = getpid();
	msg.pinfo.threshold = threshold;

	while(1) {
		// Look for quit message from controller

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
