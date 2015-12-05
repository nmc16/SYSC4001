#  Author: Nicolas McCallum 100936816
#    Date: October 11, 2015   
# Version: 1

# CC will be the compiler used, default is gcc
CC=gcc

# CFLAGS are the compiler options to be used for the project
CFLAGS=-c -Wall

# Default to run
all: controller actuator cloud sensor

controller: controller.o
	$(CC) controller.o -o controller

actuator: actuator.o
	$(CC) actuator.o -o actuator
	
cloud: cloud.o
	$(CC) cloud.o -o cloud
	
sensor: sensor.o
	$(CC) sensor.o -o sensor

controller.o: controller.c
	$(CC) $(CFLAGS) controller.c

actuator.o: actuator.c
	$(CC) $(CFLAGS) actuator.c

cloud.o: cloud.c
	$(CC) $(CFLAGS) cloud.c

sensor.o: sensor.c
	$(CC) $(CFLAGS) sensor.c

clean:
	rm *o IOT
