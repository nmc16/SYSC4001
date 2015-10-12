/*
 * sensor.c
 *
 * Holds two types of sensors, temperature and smoke. Attributes are given
 * via the command line: Message Queue Path, Sensor Type, Sensor Name, Sensor
 * Threshold.
 *
 * Sensor type must be given as one of: temperature, temp, smoke, or the program
 * will exit with error.
 *
 * Sends initialization message on message queue to controller before reading
 * data. Waits until the controller sends an acknowledge signal then starts
 * reading random data. Sends all message data via message struct in message
 * header file on the message queue. If the data is greater than the threshold,
 * an alarm is printed.
 *
 *  Created on: Oct 3, 2015
 *      Author: Nicolas McCallum 100936816
 */

#include <sys/msg.h>
#include "message.h"

char *name;
char type;
long int threshold;
int msgid;
struct proc_msg msg;
int running = 1;

/**
 * Sends initialization message to controller via message queue. Waits until
 * the controller sends back and acknowledge signal that the device was
 * registered.
 */
void send_init() {
	// Set the message type to init
	msg.msg_type = INITCODE;

    // Send init data
    if (msgsnd(msgid, (void *)&msg, sizeof(msg.pinfo), 0) == -1) {
		fprintf(stderr, "Failed to send init to message queue!\n");
		exit(MQSERR);
	}
    printf("[INIT] Sent init message to controller...\n");

    // Wait for ack signal back
    while(1) {
        if (msgrcv(msgid, (void *)&msg, sizeof(msg.pinfo), getpid(), 0) == -1) {
            fprintf(stderr, "[ERROR] Failed during checking the init messages: %d\n", errno);
            exit(MQRERR);
        }

        // Make sure its the right code sent back otherwise ignore the message
        if (msg.pinfo.data == ACKCODE) {
            printf("[INIT] Received acknowledge signal from controller\n");
            break;
        }
    }
}

/**
 * Checks for the stop message sent via message queue from the controller.
 *
 * return: 1 if the stop message exists and 0 if it doesn't
 */
int check_for_stop() {
	if (msgrcv(msgid, (void *)&msg, sizeof(msg.pinfo), getpid(), IPC_NOWAIT) == -1) {
		if (errno != ENOMSG && errno != EAGAIN) {
			fprintf(stderr, "[ERROR] Failed during checking the stop message: %d\n", errno);
		    exit(MQRERR);
		}
		return 0;
	} else {
		if (msg.pinfo.data == STOPCODE) {
			return 1;
		} else {
			return 0;
		}
	}
}

/**
 * Method to print the alarm statement to the console.
 */
void init_alarm(int data) {
	printf("[ALARM] Temperature %d greater than threshold!\n", data);
}

/**
 * Sends the data to the controller via the message queue.
 */
void send_data(int data) {
	// Set the message type to device info
	msg.msg_type = DATACODE;
	msg.pinfo.data = data;

	if (msgsnd(msgid, (void *)&msg, sizeof(msg.pinfo), 0) == -1) {
		fprintf(stderr, "[ERROR] Failed to send data to message queue!\n");
		exit(MQSERR);
	}
}

/**
 * Sets the type for the actuator given the input from console.
 *
 * Must be either 'ac' for ac controller or 'bell' for smoke detector.
 *
 * param input: input string from console.
 */
void set_type(char *input) {
	if (strcmp(input, "temperature") == 0 || strcmp(input, "temp") == 0) {
		type = TEMP_SENSOR_TYPE;
		strcpy(msg.pinfo.action, "Start AC");
	} else if (strcmp(input, "smoke") == 0) {
		type = SMOKE_SENSOR_TYPE;
		strcpy(msg.pinfo.action, "Ring smoke alarm");
	} else {
		perror("[ERROR] Invalid sensor type entered. Must be one of: temperature, temp, smoke!\n");
		exit(INITERR);
	}
}

/**
 * Signal handler that checks for interrupt signal. Upon interrupt signal, message
 * is sent to controller to delete the device from the device list and stops the
 * program.
 */
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
			fprintf(stderr, "[ERROR] Quit signal failed to be sent\n");
			exit(MQSERR);
		}
	}
}

int main(int argc, char *argv[]) {
	// Check that correct command line args were passed
	if (argc != 5) {
		perror("[ERROR] Sensor takes exactly 4 arguments (Path for Message Queue, "
				"Sensor type, Name, Threshold)!\n");
		exit(INITERR);
	}

	// Store the info passed by the user
	name = malloc(sizeof(name));
	strcpy(name, argv[3]);
	threshold = strtol(argv[4], NULL, 10);
	set_type(argv[2]);

	// Set up the signal handler
	struct sigaction new_signal;
	new_signal.sa_handler = signal_handler;
	sigemptyset(&new_signal.sa_mask);
	new_signal.sa_flags = 0;
	if (sigaction(SIGINT, &new_signal, NULL) != 0) {
		fprintf(stderr, "[Error] Could not handle SIGINT");
		exit(INITERR);
	}

	// Connect to message queue given the path
	msgid = msgget(ftok(argv[1], 1), 0666 | IPC_CREAT);
	if (msgid == -1) {
		fprintf(stderr, "[ERROR] Error connecting to message queue: %d\n", errno);
		exit(INITERR);
	}
	printf("[INIT] Connecting to message queue: %d, key %d\n", msgid, ftok(argv[1], 1));

	// Set the properties in the message struct
	strcpy(msg.pinfo.name, argv[3]);
	msg.pinfo.device = type;
    msg.pinfo.data = 0;
	msg.pinfo.pid = getpid();
	msg.pinfo.threshold = threshold;

	// Send the init and wait for ack signal
    send_init();    

    int range = threshold + 20;
	while(running) {
		// Look for quit message from controller
		if (check_for_stop()) {
			printf("[STOPPING] Stop signal received, shutting down...\n");
			break;
		}

		// Randomize temperature data
		int r = rand() % range;

		// Print the data to the screen for the sensor type
		if (type == TEMP_SENSOR_TYPE) {
		    printf("[DATA] Temperature sensor %s reads temperature %d (Threshold: %ld)\n",
		    		name, r, threshold);
		} else {
		    printf("[DATA] Smoke sensor %s reads smoke level %d (Threshold: %ld)\n",
		    		name, r, threshold);
		}
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
