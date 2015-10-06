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

static int message_sent = 0;
long int stop_code = 5;
proc_info device_list[MAX_DEVICES];
int devices = 0;

void send_stop(pid_t pid) {
	struct proc_msg msg;

	msg.msg_type = pid;
	msg.pinfo.data = stop_code;

	if (msgsnd(msgid, (void *)&msg, sizeof(msg.pinfo), 0) == -1) {
		fprintf(stderr, "Stop signal failed to be sent to PID %d\n", pid);
		exit(4);
	}

	// TODO Delete from list
}

void add_to_device_list(struct proc_msg msg) {

    int i;
    char flag = 0;

    // Check if the device is registered in the device list yet
    for (i = 0; i < devices; i++) {
        if (device_list[i].pid == msg.pinfo.pid) {
            flag = 1;
            break;
        }
    }

    // Add to the list if it doesn't already exist
    if (!flag) {
    	// Increase the devices counter
        devices++;

        // Update the info
        device_list[devices].device = msg.pinfo.device;
        device_list[devices].pid = msg.pinfo.pid;
        device_list[devices].threshold = msg.pinfo.threshold;
        strcpy(device_list[devices].name, msg.pinfo.name);

        // Alert user that device was registered
        printf("[Device Registered] PID: %d, Type: %c, Threshold: %ld, Name: %s\n", device_list[devices].pid,
        		device_list[devices].device, device_list[devices].threshold, device_list[devices].name);
    }
}

void send_ack(struct proc_msg msg) {
    msg.msg_type = 4;
    if (msgsnd(msgid, (void *)&msg, sizeof(msg.pinfo), 0) == -1) {
		fprintf(stderr, "Acknowledge signal failed to be sent\n");
	    exit(4);
	}
    printf("Sent acknowledge signal for pid %d (Name: %s)!\n", msg.pinfo.pid, msg.pinfo.name);
}

void alarm_handler(int signum) {
    printf("Alarm handler 4real\n");
	message_sent = 1;
}

void activate_actuator(struct proc_msg msg) {
	// Set data to start
	msg.pinfo.data = 1;

	// Set the type to 2 so it is only read actuator
	msg.msg_type = 0;

	// Send the message over the message queue
	if (msgsnd(msgid, (void *)&msg, sizeof(msg.pinfo), 0) == -1) {
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
	if (msgsnd(msgid, (void *)&msg, sizeof(msg.pinfo), 0) == -1) {
		fprintf(stderr, "msgsnd failed\n");
	    exit(4);
	}

    kill(getppid(), SIGALRM);
	printf("Sent alarm to parent\n");
	return;
}

int run_child() {
    long int device_msg_code = 1;
    long int device_init_code = 3;

    int test = 0;

    struct proc_msg msg;

    // Run for ever
    while(1) {

		// Check for the init message on the message queue
        if (msgrcv(msgid, (void *)&msg, sizeof(msg.pinfo), device_init_code, IPC_NOWAIT) == -1) {
            // Check that it is not an expected error from blank message
            if (errno != ENOMSG && errno != EAGAIN) {
    		    fprintf(stderr, "Failed during checking the init messages: %d\n", errno);
    		    exit(3);
            }
    	} else {
            send_ack(msg);
            add_to_device_list(msg);
        }

    	// Look for message on the message queue from device
    	if (msgrcv(msgid, (void *)&msg, sizeof(msg.pinfo), device_msg_code, IPC_NOWAIT) == -1) {
            // Check that it is not an expected error from blank message
            if (errno != ENOMSG && errno != EAGAIN) {
    		    fprintf(stderr, "Failed during checking the device messages: %d\n", errno);
    		    exit(3);
            }
    	} else {
            // Print the info received from the device
    	    printf("Message received from device [%ld] %s (type %c) with data %d (threshold %ld)\n", msg.pinfo.pid,
    	    	    msg.pinfo.name, msg.pinfo.device, msg.pinfo.data, msg.pinfo.threshold);
    	    test++;
    	    if (msg.pinfo.data > msg.pinfo.threshold) {
    	    	//activate_actuator(msg);
    	     	send_to_parent(msg);
    	    }
        }

    	if (test > 10) {
    		test = 0;
    		send_stop(msg.pinfo.pid);
    	}

    }

	return 0;
}

int run_parent() {
	long int msg_to_receive = 2;
	struct proc_msg msg;

    // Set up the signal handler
	struct sigaction new_signal;
	new_signal.sa_handler = alarm_handler;
	sigemptyset(&new_signal.sa_mask);
	new_signal.sa_flags = 0;

	if (sigaction(SIGALRM, &new_signal, NULL) != 0) {
		 perror("Error: Parent could not handle SIGALRM");
	}

	while(1) {
		if (message_sent == 1) {
			if (msgrcv(msgid, (void *)&msg, sizeof(msg.pinfo), msg_to_receive, 0) == -1) {
				fprintf(stderr, "msgrcv failed with error: %d\n", errno);
			    exit(3);
			}
			printf("[PARENT] Alarm received from device [%d] %s (type %c) with data %d (threshold %ld) dealt with my action %s\n",
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
