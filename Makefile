CC = gcc
FLAGS = -pedantic -Wall

memoryship: memoryship.c
	$(CC) $(FLAGS) memoryship.c -o memoryship

.PHONY: clean

clean:
	rm -f memoryship *.o

