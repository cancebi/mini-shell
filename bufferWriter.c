#include "bufferWriter.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

dataBufferWriter *createBufferWriter(char *fileName) {
    dataBufferWriter *ret = (dataBufferWriter *)malloc(sizeof(dataBufferWriter));
    if (!ret) return NULL;

    ret->fd = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (ret->fd == -1) {
        perror("Cannot open file");
        free(ret);
        return NULL;
    }

    ret->buffer = (char *)malloc(sizeof(char) * BUFFER_CAPACITY);
    if (!ret->buffer) {
        perror("Memory allocation error");
        close(ret->fd);
        free(ret);
        return NULL;
    }

    ret->pos = 0;
    return ret;
}


// Free the memory and close the file, flushing the buffer
void destroyBufferWriter(dataBufferWriter *data) {
    if (data) {
        flush(data);  // Write any remaining data in the buffer
        if (data->fd != -1) close(data->fd);
        free(data->buffer);
        free(data);
    }
}

// Write all data in the buffer to the file
void flush(dataBufferWriter *data) {
    int i, cpt = 0;
    u_int8_t tmp = 0;

    for (i = 0; i < data->pos; i++) {
        if (cpt == 8) {
            write(data->fd, &tmp, sizeof(tmp));
            cpt = 0;
            tmp = 0;
        }
        tmp |= (data->buffer[i] & 1) << (7 - cpt);
        cpt++;
    }
    if (cpt) write(data->fd, &tmp, sizeof(tmp));
    data->pos = 0;
}

// Write a single bit to the buffer
void putBit(dataBufferWriter *data, bool val) {
    assert(data->pos < BUFFER_CAPACITY);
    data->buffer[data->pos++] = val & 1;

    // Flush if the buffer is full
    if (data->pos == BUFFER_CAPACITY) {
        flush(data);
    }
}

// Write an unsigned 8-bit char, bit-by-bit, to the buffer
void putUnsignedChar(dataBufferWriter *data, unsigned char val) {
    for (int i = 0; i < 8; i++) {
        putBit(data, (val >> (7 - i)) & 1);
    }
}

// Write an unsigned integer, byte-by-byte, to the buffer
void putUnsignedInt(dataBufferWriter *data, unsigned int val) {
    unsigned char *ptr = (unsigned char *)&val;
    for (int i = 0; i < sizeof(val); i++) {
        putUnsignedChar(data, ptr[i]);
    }
}

