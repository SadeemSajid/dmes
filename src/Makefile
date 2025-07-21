CC = gcc # The compiler
CFLAGS = -Wall

all: dmes

dmes: main.o setup.o
	$(CC) $(CFLAGS) -lpthread $^ -o $@

main.o: main.c setup.h
	$(CC) $(CFLAGS) -c main.c -o $@
setup.o: setup.c setup.h
	$(CC) $(CFLAGS) -c setup.c -o $@
clean:
	rm -rf dmes *.out *.o
