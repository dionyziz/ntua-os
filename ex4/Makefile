#
# Makefile
#
# Operating Systems, Exercise 4
#

CC = gcc
#CFLAGS = -Wall -g
CFLAGS = -Wall -O2 -g

all: scheduler scheduler-solution scheduler-shell scheduler-shell-solution shell prog execve-example strace-test

scheduler: scheduler.o proc-common.o
	$(CC) -o scheduler scheduler.o proc-common.o

scheduler-solution: scheduler-solution.o proc-common.o
	$(CC) -o scheduler-solution scheduler-solution.o proc-common.o

scheduler-shell: scheduler-shell.o proc-common.o
	$(CC) -o scheduler-shell scheduler-shell.o proc-common.o

scheduler-shell-solution: scheduler-shell-solution.o proc-common.o
	$(CC) -o scheduler-shell-solution scheduler-shell-solution.o proc-common.o

shell: shell.o proc-common.o
	$(CC) -o shell shell.o proc-common.o

prog: prog.o proc-common.o
	$(CC) -o prog prog.o proc-common.o

execve-example: execve-example.o 
	$(CC) -o execve-example execve-example.o

strace-test: strace-test.o 
	$(CC) -o strace-test strace-test.o

proc-common.o: proc-common.c proc-common.h
	$(CC) $(CFLAGS) -o proc-common.o -c proc-common.c

shell.o: shell.c proc-common.h request.h
	$(CC) $(CFLAGS) -o shell.o -c shell.c

scheduler.o: scheduler.c proc-common.h request.h
	$(CC) $(CFLAGS) -o scheduler.o -c scheduler.c

scheduler-solution.o: scheduler-solution.c proc-common.h request.h
	$(CC) $(CFLAGS) -o scheduler-solution.o -c scheduler-solution.c

scheduler-shell.o: scheduler-shell.c proc-common.h request.h
	$(CC) $(CFLAGS) -o scheduler-shell.o -c scheduler-shell.c

scheduler-shell-solution.o: scheduler-shell-solution.c proc-common.h request.h
	$(CC) $(CFLAGS) -o scheduler-shell-solution.o -c scheduler-shell-solution.c

prog.o: prog.c
	$(CC) $(CFLAGS) -o prog.o -c prog.c

execve-example.o: execve-example.c
	$(CC) $(CFLAGS) -o execve-example.o -c execve-example.c

strace-test.o: strace-test.c
	$(CC) $(CFLAGS) -o strace-test.o -c strace-test.c

clean:
	rm -f scheduler scheduler-shell shell prog execve-example strace-test *.o
