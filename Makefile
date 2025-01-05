# Makefile for the mini shell project

CC = gcc
CFLAGS = -Wall -g

# Source and object files
SRC = src/mysh.c src/executor.c src/parser.c src/wildcard.c src/myls.c src/myps.c src/redirection.c src/process_manager.c src/variable.c
OBJ_DIR = build
OBJ = $(SRC:src/%.c=$(OBJ_DIR)/%.o)

# Output executable
TARGET = mysh

# Default rule
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

# Compile each .c file to .o in the build directory
$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Ensure the build directory exists
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Clean up object files and the executable
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# Rebuild from scratch
rebuild: clean all
