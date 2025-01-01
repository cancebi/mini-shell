#include "../include/bufferReader.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

dataBufferReader *createBufferReader(char *fileName) {
    dataBufferReader *ret = (dataBufferReader *)malloc(sizeof(dataBufferReader));
    if (!ret) return NULL;

    ret->fd = open(fileName, O_RDONLY);
    if (ret->fd == -1) {
        perror("Cannot open file");
        free(ret);  // Free memory to prevent leaks
        return NULL;  // Return NULL to signal failure
    }

    ret->buffer = (char *)malloc(sizeof(char) * BUFFER_SIZE);
    if (!ret->buffer) {
        perror("Memory allocation error");
        close(ret->fd);  // Close the file descriptor to clean up
        free(ret);
        return NULL;
    }

    ret->size = read(ret->fd, ret->buffer, BUFFER_SIZE);
    if (ret->size == -1) {
        perror("Cannot read file");
        free(ret->buffer);
        close(ret->fd);
        free(ret);
        return NULL;
    }

    ret->pos = 0;
    ret->posBit = 0;
    return ret;
}

// Free the memory and close the file
void destroyBufferReader(dataBufferReader *data) {
    if (data) {
        if (data->fd != -1) close(data->fd);
        free(data->buffer);
        free(data);
    }
}

// Get the current character at the buffer position
int getCurrentChar(dataBufferReader *data) {
    if (eof(data)) return -1;
    return data->buffer[data->pos];
}

// Move to the next character in the buffer
void consumeChar(dataBufferReader *data) {
    data->pos++;
    if (data->pos >= data->size) {
        data->size = read(data->fd, data->buffer, BUFFER_SIZE);
        if (data->size == -1) {
            perror("Cannot read");
            exit(1);
        }
        data->pos = 0;
    }
    data->posBit = 0;
}

// Get the current bit from the current character
bool getCurrentBit(dataBufferReader *data) {
    return (data->buffer[data->pos] >> (7 - data->posBit)) & 1;
}

// Move to the next bit in the buffer
void consumeBit(dataBufferReader *data) {
    data->posBit++;
    if (data->posBit >= 8) {
        consumeChar(data);
        data->posBit = 0;
    }
}

// Check if the end of the buffer has been reached
bool eof(dataBufferReader *data) {
    return data->size == 0;
}

// Reset the buffer to the beginning of the file
void moveBeginning(dataBufferReader *data) {
    lseek(data->fd, 0, SEEK_SET);
    data->size = read(data->fd, data->buffer, BUFFER_SIZE);
    data->pos = 0;
    data->posBit = 0;
    if (data->size == -1) {
        perror("Cannot read");
        destroyBufferReader(data);
    }
}

