#   Author: Nicolas McCallum 100936816
#     Date: December 6th, 2015   
#  Version: 1

# CC will be the compiler used, default is gcc
CC=gcc

# Thread options needed for some distros
TFLAGS=-D_REENTRANT -lpthread

# CFLAGS are the compiler options to be used for the project
CFLAGS=-c -Wall

# Environment 
MKDIR=mkdir
CP=cp
CCADMIN=CCadmin

# Default to run
all: main

main: main.o
	$(CC) $(TFLAGS) -o main main.o
	
main.o: main.c
	$(CC) $(CFLAGS) main.c
