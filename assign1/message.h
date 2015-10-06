/*
 * message.h
 *
 *  Created on: Oct 3, 2015
 *      Author: nic
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_



#endif /* MESSAGE_H_ */
#define TEMP_SENSOR_TYPE 'a'
#define SMOKE_SENSOR_TYPE 'a'

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


