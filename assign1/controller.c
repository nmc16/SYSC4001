/*
 * controller.c
 *
 *  Created on: Oct 3, 2015
 *      Author: N McCallum 100936816
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/msg.h>
#include "message.h"
#include <errno.h>
#include <signal.h>
#include <string.h>

#define MAX_DEVICES 6

int msgid;

struct d_list {
	proc_info *pinfo;
	struct d_list *next;
} device_list;

static int message_sent = 0;

void alarm_handler(int signum) {
	message_sent = 1;
}

void activate_actuator(struct proc_msg msg) {
	// Set data to start
	msg.pinfo.data = 1;

	// Set the type to 2 so it is only read actuator
	msg.msg_type = 0;

	// Send the message over the message queue
	if (msgsnd(msgid, (void *)&msg, sizeof(msg), 0) == -1) {
		fprintf(stderr, "msgsnd failed\n");
	    exit(4);
	}
}

void send_to_parent(struct proc_msg msg) {
	// Set up the signal to send
	struct sigaction new_signal;
	new_signal.sa_handler = alarm_handler;
	sigemptyset(&new_signal.sa_mask);
	new_signal.sa_flags = 0;

	// Add message to message queue with the action taken
	msg.msg_type = 2;
	strcpy(msg.pinfo.action, "Test");
	if (msgsnd(msgid, (void *)&msg, sizeof(msg), 0) == -1) {
		fprintf(stderr, "msgsnd failed\n");
	    exit(4);
	}

	// Create alarm and send to parent proc
	sigaction(SIGALRM, &new_signal, NULL);
	return;
}

int run_child() {
	long int msg_to_receive = 1;
	proc_info *device_list = malloc(sizeof(proc_info) * MAX_DEVICES);
	int devices = 0;

    struct proc_msg msg;

    // Run for ever
    while(1) {
    	// Look for message on the message queue from device
    	if (msgrcv(msgid, (void *)&msg, sizeof(msg.pinfo), msg_to_receive, 0) == -1) {
    		fprintf(stderr, "msgrcv failed with error: %d\n", errno);
    		exit(3);
    	}

    	// Print the info received from the device
    	printf("Message received from device [%ld] %s (type %c) with data %d (threshold %ld)\n", msg.pinfo.pid,
    	    	msg.pinfo.name, msg.pinfo.device, msg.pinfo.data, msg.pinfo.threshold);

    	// Check if the device is registered in the device list yet
    	int i;
    	char flag = 0;
    	for (i = 0; i < devices; i++) {
    		if (device_list[i].pid == msg.pinfo.pid) {
    			flag = 1;
    			break;
    		}
    	}

    	// Add to the list if it doesn't already exist
    	if (!flag) {
    		devices++;
    		device_list[devices].device = msg.pinfo.device;
    		device_list[devices].pid = msg.pinfo.pid;
    		device_list[devices].threshold = msg.pinfo.threshold;
    	}

    	if (msg.pinfo.data > msg.pinfo.threshold) {
    		activate_actuator(msg);
    		send_to_parent(msg);
    	}

    }

	return 0;
}

int run_parent() {
	long int msg_to_receive = 2;
	struct proc_msg msg;

	while(1) {
		pause();
		if (message_sent) {
			if (msgrcv(msgid, (void *)&msg, sizeof(struct proc_msg), msg_to_receive, 0) == -1) {
				fprintf(stderr, "msgrcv failed with error: %d\n", errno);
			    exit(3);
			}
			printf("Message received from device [%s] %s (type %s) with data %s (threshold %s) dealt with my action %s",
					msg.pinfo.pid, msg.pinfo.name, msg.pinfo.device, msg.pinfo.data, msg.pinfo.threshold,
					msg.pinfo.action);
			message_sent = 0;
		}
	}
}

int main(int argc, char *argv[]) {
	pid_t pid;

	// Check to make sure the correct amount of arguments were passed
	if (argc != 2) {
		perror("Controller takes exactly 1 argument!");
		exit(1);
	}

	// Create the message queue if it doesn't already exist
	msgid = msgget(ftok(argv[1], 1), 0666 | IPC_CREAT);
	printf("Connecting to message queue: %d, key %d\n", msgid, ftok(argv[1], 1));

	pid = fork();

	switch(pid) {
		case -1:
			// Process creation failed
			perror("Controller process creation failed!");
			exit(2);
		case 0:
			// Child process
			run_child();
			break;
		default:
			// Parent process
			run_parent();
			break;
	}

	if (msgctl(msgid, IPC_RMID, 0) == -1) {
	    fprintf(stderr, "msgctl(IPC_RMID) failed\n");
	    exit(EXIT_FAILURE);
	}
	exit(0);
}
