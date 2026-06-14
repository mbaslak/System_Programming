CC = gcc

FLAGS = -Wall -Wextra -std=c11 -pthread

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

program: $(OBJ)
	$(CC) $(FLAGS) $(OBJ) -o program

%.o: %.c #RULE
	$(CC) $(FLAGS) -c $< -o $@

clean:
	rm -f *.o program