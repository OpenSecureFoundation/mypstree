CC = gcc
CFLAGS = -Wall -Wextra -g -O2
OBJ = main.o options.o proc_reader.o tree_builder.o tree_printer.o
TARGET = mypstree

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
