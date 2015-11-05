#  Author: Nicolas McCallum 100936816
#    Date: November 4, 2015   
# Version: 1

# CC will be the compiler used, default is gcc
CC=gcc

# CFLAGS are the compiler options to be used for the project
CFLAGS=-c -Wall

# Default to run
all: producer

producer: producer.o sem_helper.o 
	$(CC) -o producer producer.o sem_helper.o

producer.o: producer.c
	$(CC) $(CFLAGS) producer.c

sem_helper.o: sem_helper.c
	$(CC) $(CFLAGS) sem_helper.c

clean:
	rm *o
