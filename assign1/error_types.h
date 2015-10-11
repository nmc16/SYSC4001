/*
 * error_types.h
 *
 * Holds the constants for error codes for the program. Program
 * will exit with one of the codes below that correspond to
 * where the program crashed.
 *
 *  Created on: Oct 10, 2015
 *      Author: Nicolas McCallum 100936816
 */

#ifndef ERROR_TYPES_H_
#define ERROR_TYPES_H_

// Define error codes for the program
#define INITERR 1 	// Error during initialization of program
#define MQGERR 2	// Error during creation/connecting to the message queue
#define MQRERR 3	// Error during message receive
#define MQSERR 4	// Error during message sending



#endif /* ERROR_TYPES_H_ */
