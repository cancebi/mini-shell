# Makefile for the mini shell project

CC = gcc
CFLAGS = -Wall -g

# Source and object files
SRC = mysh.c executor.c parser.c wildcard.c myls.c myps.c redirection.c process_manager.c variable.c
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

