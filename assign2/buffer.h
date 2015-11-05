/*
 * buffer.h
 *
 *	Header file for buffer operations. Holds the struct that
 *	shared memory buffers should use to pass information
 *	and the bytes allowed per string in the struct.
 *
 *  Created on: November 4, 2015
 *      Author: Nicolas McCallum 100936816
 */

#ifndef BUFFER_H_
#define BUFFER_H_

#define TXTBUFSIZ 128

struct text_buf {
	int count;
	char buffer[TXTBUFSIZ];
};

#endif /* BUFFER_H_ */
