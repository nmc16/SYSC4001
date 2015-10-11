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
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>

#include "message.h"
#include "error_types.h"

#define MAX_DEVICES 6

int msgid;

static int message_sent = 0;
static int started = 0;
static int running = 1;
proc_info device_list[MAX_DEVICES];
int devices = 0;

void add_device(struct proc_msg msg) {

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
        // Update the info
        device_list[devices].device = msg.pinfo.device;
        device_list[devices].pid = msg.pinfo.pid;
        device_list[devices].threshold = msg.pinfo.threshold;
        strcpy(device_list[devices].name, msg.pinfo.name);

        // Alert user that device was registered
        printf("[Device Registered] PID: %d, Type: %c, Threshold: %ld, Name: %s\n", device_list[devices].pid,
        		device_list[devices].device, device_list[devices].threshold, device_list[devices].name);

    	// Increase the devices counter
        devices++;
    }
}

void remove_device(pid_t pid) {
	int i;
	char flag = 0;

	// Check for the device in the array
	for (i = 0; i < devices; i++) {
		if (device_list[i].pid == pid) {
			flag = 1;
		    break;
		}
	}

	// If the devices exists remove it and shift N-1 to the index of the deleted item
	if (flag) {
		// Alert user that device was deleted
		printf("[Device Stopped] PID: %d, Type: %c, Threshold: %ld, Name: %s\n", device_list[i].pid,
		        device_list[i].device, device_list[i].threshold, device_list[i].name);
		device_list[i] = device_list[devices - 1];

		// Decrease counter
		devices--;
	}
	printf("asda\n");
}

void send_stop(pid_t pid) {
	struct proc_msg msg;

	msg.msg_type = pid;
	msg.pinfo.data = STOPCODE;

	if (msgsnd(msgid, (void *)&msg, sizeof(msg.pinfo), 0) == -1) {
		fprintf(stderr, "Stop signal failed to be sent to PID %d\n", pid);
		exit(4);
	}

	// Delete PID from list
	remove_device(pid);
}

void send_ack(struct proc_msg msg) {
    msg.msg_type = msg.pinfo.pid;
    msg.pinfo.data = ACKCODE;
    if (msgsnd(msgid, (void *)&msg, sizeof(msg.pinfo), 0) == -1) {
		fprintf(stderr, "Acknowledge signal failed to be sent\n");
	    exit(4);
	}
    printf("Sent acknowledge signal for pid %d (Name: %s)!\n", msg.pinfo.pid, msg.pinfo.name);
}

void alarm_handler(int signum) {
	// Check the interrupt
	switch(signum) {

	// Alarm sent from child
	case SIGALRM:
		message_sent = 1;
		break;

	// Control+C was pressed
	case SIGINT:
		// If the program hasn't started, start it, otherwise close the child and parent
		if (started == 0) {
			started = 1;
		} else {
			running = 0;
		}
	}
}

char get_actuator_code(char device) {
	if (device == TEMP_SENSOR_TYPE) {
		return AC_ACTUATOR_TYPE;
	} else if (device == SMOKE_SENSOR_TYPE) {
		return BELL_ACTUATOR_TYPE;
	} else {
		// Some error occurred, non-existent device type was passed
		return -1;
	}
}

void activate_actuator(struct proc_msg msg) {
	// Set data to start
	msg.pinfo.data = 0;
	msg.msg_type = -1;

	// Find PID of actuator if it exists otherwise print error
	char actuator = get_actuator_code(msg.pinfo.device);
	if (actuator == -1) {
		printf("Unknown device type, cannot find actuator!\n");
		exit(5);
	}

	int i;
	for (i = 0; i < MAX_DEVICES; i++) {
		// If we found the one we want, send the message to its PID
		if (device_list[i].device == actuator) {
			msg.msg_type = device_list[i].pid;
		}
	}

	// Check that a match was found
	if (msg.msg_type == -1) {
		printf("No actuator could be found for device %s\n", msg.pinfo.name);
		return;
	}

	// Send the appropriate action to the actuator
	if (actuator == AC_ACTUATOR_TYPE) {
		strcpy(msg.pinfo.action, "start ac");
	} else {
		strcpy(msg.pinfo.action, "ring smoke alarm");
	}

	// Send the message over the message queue
	if (msgsnd(msgid, (void *)&msg, sizeof(msg.pinfo), 0) == -1) {
		fprintf(stderr, "msgsnd failed\n");
	    exit(4);
	}

	// Wait for the acknowledge signal back
	if (msgrcv(msgid, (void *)&msg, sizeof(msg.pinfo), 6, 0) == -1) {
	    fprintf(stderr, "Failed during checking the ack from actuator: %d\n", errno);
	    exit(3);
	} else {
		printf("[ACTUATOR ACKNOWLEDGE] Actuator %s [%ld] sent acknowledge after performing action \"%s\"\n",
				msg.pinfo.name, msg.pinfo.pid, msg.pinfo.action);
	}
}

void send_to_parent(struct proc_msg msg) {
	// Add message to message queue with the action taken
	msg.msg_type = PRNTCODE;
	if (msgsnd(msgid, (void *)&msg, sizeof(msg.pinfo), 0) == -1) {
		fprintf(stderr, "msgsnd failed\n");
	    exit(4);
	}

    kill(getppid(), SIGALRM);
	printf("Sent alarm to parent\n");
	return;
}

void run_child() {
    struct proc_msg msg;
    int messages = 0;

    // Run for ever
    while(running) {

		// Check for the init message on the message queue
        if (msgrcv(msgid, (void *)&msg, sizeof(msg.pinfo), INITCODE, IPC_NOWAIT) == -1) {
            // Check that it is not an expected error from blank message
            if (errno != ENOMSG && errno != EAGAIN) {
    		    fprintf(stderr, "Failed during checking the init messages: %d\n", errno);
    		    exit(3);
            }
    	} else {
            send_ack(msg);
            add_device(msg);
        }

        // Check if there has been a message from a device that quit
        if (msgrcv(msgid, (void *)&msg, sizeof(msg.pinfo), QUITCODE, IPC_NOWAIT) == -1) {
        	// Check that it is not an expected error from blank message
        	if (errno != ENOMSG && errno != EAGAIN) {
        	    fprintf(stderr, "Failed during checking the init messages: %d\n", errno);
        	    exit(3);
        	}
        } else {
        	// Remove the device from the registered devices list
        	remove_device(msg.pinfo.pid);
        }

    	// Look for message on the message queue from device
    	if (msgrcv(msgid, (void *)&msg, sizeof(msg.pinfo), DATACODE, IPC_NOWAIT) == -1) {
            // Check that it is not an expected error from blank message
            if (errno != ENOMSG && errno != EAGAIN) {
    		    fprintf(stderr, "Failed during checking the device messages: %d\n", errno);
    		    exit(3);
            }
    	} else {
            // Print the info received from the device
    	    printf("Message received from device [%ld] %s (type %c) with data %d (threshold %ld)\n", msg.pinfo.pid,
    	    	    msg.pinfo.name, msg.pinfo.device, msg.pinfo.data, msg.pinfo.threshold);
    	    messages++;
    	    if (msg.pinfo.data > msg.pinfo.threshold) {
    	    	activate_actuator(msg);
    	     	send_to_parent(msg);
    	    }
        }

    	// If 10 messages are received, send stop to last device to send a message
    	// Used to show stop code working
    	if (messages > 10) {
    		messages = 0;
    		send_stop(msg.pinfo.pid);
    	}
    }

    printf("[CHILD] Child closing...\n");
}

void run_parent() {
	int server_fifo_id;
	struct proc_msg msg;

	// Wait until Control+C is pressed to start monitoring
	while(!started) {
		// Sleep to not keep CPU time
		sleep(1);
	}

	// Open the server FIFO in read only mode
	server_fifo_id = open(SERVER_FIFO_NAME, O_WRONLY);
	if (server_fifo_id == -1) {
		// If there was an interrupt let the handler deal with it
		if (errno != EINTR) {
			fprintf(stderr, "[ERROR] Parent could not open server FIFO: %d\n", errno);
	    	exit(FIOPERR);
		}
	}


	printf("[PARENT] Parent is now monitoring...\n");
	while(running) {
		// If the alarm has been received read the message from the queue
		if (message_sent == 1) {
			if (msgrcv(msgid, (void *)&msg, sizeof(msg.pinfo), PRNTCODE, 0) == -1) {
				fprintf(stderr, "msgrcv failed with error: %d\n", errno);
			    exit(3);
			}

			// Print the data
			printf("[PARENT] Alarm received from device [%d] %s (type %c) with data %d (threshold %ld) dealt with by action \"%s\"\n",
					msg.pinfo.pid, msg.pinfo.name, msg.pinfo.device, msg.pinfo.data, msg.pinfo.threshold,
					msg.pinfo.action);

			// Send the data to the cloud
			if(write(server_fifo_id, &msg.pinfo, sizeof(msg.pinfo)) != 0) {
				if (errno != EINTR) {
					fprintf(stderr, "[ERROR] Parent could not write to server FIFO: %d\n", errno);
					running = 0;
				}
			}
			message_sent = 0;
		}
	}

	printf("[PARENT] Parent closing...\n");
	close(server_fifo_id);
}

int main(int argc, char *argv[]) {
	pid_t pid;

	// Check to make sure the correct amount of arguments were passed
	if (argc != 2) {
		perror("Controller takes exactly 1 argument!");
		exit(1);
	}

	// Set up the signal handler
	struct sigaction new_signal;
	new_signal.sa_handler = alarm_handler;
	sigemptyset(&new_signal.sa_mask);
	new_signal.sa_flags = SA_RESTART;

	// Add the handler to handle SIGALRM and SIGINT
	if (sigaction(SIGALRM, &new_signal, NULL) != 0) {
		 perror("Error: Could not handle SIGALRM");
	}
	if (sigaction(SIGINT, &new_signal, NULL) != 0) {
		perror("Error: Could not handle SIGINT");
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
		// If the message queue has already been deleted it will throw EINVAL
		if (errno != EINVAL) {
			fprintf(stderr, "Could not delete message queue!: %d\n", errno);
	    	exit(EXIT_FAILURE);
		}
	} else {
		printf("Closed message queue...\n");
	}

	exit(0);
}
