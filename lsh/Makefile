CC=gcc
CFLAGS=-pedantic -Wall -std=gnu11 -g

all: main
main: main.o parser.o utility.o job.o

main.o: main.c parser.h job.h utility.h

parser.o: parser.c parser.h job.h utility.h

utility.o: utility.c utility.h

job.o: job.c job.h

clean:
	$(RM) *.o main
.PHONY: all clean
