/*
 * message.h
 *
 *  Created on: Oct 3, 2015
 *      Author: nic
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_



#endif /* MESSAGE_H_ */

// Define character constants for device type
#define TEMP_SENSOR_TYPE 'a'
#define SMOKE_SENSOR_TYPE 'b'
#define AC_ACTUATOR_TYPE 'c'
#define BELL_ACTUATOR_TYPE 'd'

// Define int constants for message type
#define INITCODE 1000
#define DATACODE 1001
#define STOPCODE 1002
#define ACKCODE 1003
#define PRNTCODE 1004


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


