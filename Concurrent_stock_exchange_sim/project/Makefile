CC = gcc

FLAGS = -Wall -Wextra -std=c11

TARGETS = ServerTrd ticker trader analyst

COMMON_SOURCES = get_command.c special_io.c
COMMON_OBJECTS = $(COMMON_SOURCES:.c=.o)

all: $(TARGETS)

ServerTrd: ServerTrd.o $(COMMON_OBJECTS)
	$(CC) $(FLAGS) $^ -o $@


ticker: ticker.o $(COMMON_OBJECTS)
	$(CC) $(FLAGS) $^ -o $@

trader: trader.o $(COMMON_OBJECTS)
	$(CC) $(FLAGS) $^ -o $@

analyst: analyst.o $(COMMON_OBJECTS)
	$(CC) $(FLAGS) $^ -o $@

%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@

clean:
	rm -f *o $(TARGETS)