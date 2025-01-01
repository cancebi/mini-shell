# Makefile for the mini shell project

CC = gcc
CFLAGS = -Wall -g

# Source and object files
SRC = src/mysh.c src/executor.c src/parser.c src/wildcard.c src/myls.c src/myps.c src/redirection.c src/process_manager.c src/variable.c
OBJ = $(SRC:.c=.o)

# Output executable
TARGET = mysh

# Default rule
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

# Compile each .c file to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up object files and the executable
clean:
	rm -f $(OBJ) $(TARGET)

# Rebuild from scratch
rebuild: clean all

