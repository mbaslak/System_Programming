
output: main.o search_file.o
	gcc main.o search_file.o -o program

main.o: main.c
	gcc -c main.c

search_file.o: search_file.c search_file.h
	gcc -c search_file.c

clean:
	rm *.o program