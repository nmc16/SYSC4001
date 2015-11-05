#  Author: Nicolas McCallum 100936816
#    Date: November 4, 2015   
# Version: 1

# CC will be the compiler used, default is gcc
CC=gcc

# CFLAGS are the compiler options to be used for the project
CFLAGS=-c -Wall

# Default to run
all: producer consumer producer_without_sem consumer_without_sem

producer: producer.o sem_helper.o 
	$(CC) -o producer producer.o sem_helper.o

consumer: consumer.o sem_helper.o
	$(CC) -o consumer consumer.o sem_helper.o

producer_without_sem: producer_without_sem.o sem_helper.o
	$(CC) -o producer_without_sem producer_without_sem.o sem_helper.o
	
consumer_without_sem: consumer_without_sem.o sem_helper.o
	$(CC) -o consumer_without_sem consumer_without_sem.o sem_helper.o

producer.o: producer.c
	$(CC) $(CFLAGS) producer.c

consumer.o: consumer.c
	$(CC) $(CFLAGS) consumer.c
	
producer_without_sem.o: producer_without_sem.c
	$(CC) $(CFLAGS) producer_without_sem.c

consumer_without_sem.o: consumer_without_sem.c
	$(CC) $(CFLAGS) consumer_without_sem.c
	
sem_helper.o: sem_helper.c
	$(CC) $(CFLAGS) sem_helper.c

clean:
	rm *o
