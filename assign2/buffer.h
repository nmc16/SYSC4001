/*
 * buffer.h
 *
 *  Created on: Nov 4, 2015
 *      Author: nic
 */

#ifndef BUFFER_H_
#define BUFFER_H_

#define TXTBUFSIZ 128

struct text_buf {
	char buffer[TXTBUFSIZ];
	int count;
};

#endif /* BUFFER_H_ */
