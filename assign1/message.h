/*
 * message.h
 *
 * Holds constant message variables like the type of the sensor
 * and the message type that is sent in either the data or the
 * message type.
 *
 * Also holds the structure for the message to send on the
 * message queue for all devices and controllers.
 *
 *  Created on: Oct 3, 2015
 *      Author: Nicolas McCallum 100936816
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_

// Includes that all files in the project use
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include "error_types.h"

// Define character constants for device type
#define TEMP_SENSOR_TYPE 'a'
#define SMOKE_SENSOR_TYPE 'b'
#define AC_ACTUATOR_TYPE 'c'
#define BELL_ACTUATOR_TYPE 'd'

// Define constants for message type
#define INITCODE 1000
#define DATACODE 1001
#define STOPCODE 1002
#define ACKCODE 1003	// Sensor acknowledge code
#define PRNTCODE 1004
#define QUITCODE 1005
#define AACKCODE 1006 	// Actuator acknowledge code

// Define FIFO constants
#define SERVER_FIFO_NAME "/tmp/serv_fifo"
#define CLIENT_FIFO_NAME "/tmp/cli_%d_fifo"

// Define structures for message queue and device data
typedef struct proc_info {
	pid_t pid;
	char name[25];
	char action[50];
	char device;
	int data;
	long int threshold;
} proc_info;

struct proc_msg {
	long int msg_type;
	proc_info pinfo;
};

#endif /* MESSAGE_H_ */




