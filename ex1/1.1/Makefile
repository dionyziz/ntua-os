all: compile link

compile: main.c
	gcc -g -Wall main.c -c -o main.o

link: main.o zing.o
	gcc -Wall main.o zing.o -o main

clean:
	rm -f main.o *~
