#ifndef BUFFER_WRITER_H
#define BUFFER_WRITER_H

#include <stdbool.h>

#define BUFFER_CAPACITY 256

// Structure for data buffer writer
typedef struct {
    int fd;
    char *buffer;
    int pos;
} dataBufferWriter;

// Function declarations
dataBufferWriter *createBufferWriter(char *fileName);
void destroyBufferWriter(dataBufferWriter *data);
void flush(dataBufferWriter *data);
void putBit(dataBufferWriter *data, bool val);
void putUnsignedChar(dataBufferWriter *data, unsigned char val);
void putUnsignedInt(dataBufferWriter *data, unsigned int val);

#endif // BUFFER_WRITER_H

