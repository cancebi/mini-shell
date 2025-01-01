#ifndef BUFFER_READER_H
#define BUFFER_READER_H

#include <stdbool.h>

#define BUFFER_SIZE 256

// Structure for data buffer reader
typedef struct {
    int fd;
    char *buffer;
    int size;
    int pos;
    int posBit;
} dataBufferReader;

// Function declarations
dataBufferReader *createBufferReader(char *fileName);
void destroyBufferReader(dataBufferReader *data);
int getCurrentChar(dataBufferReader *data);
void consumeChar(dataBufferReader *data);
bool getCurrentBit(dataBufferReader *data);
void consumeBit(dataBufferReader *data);
bool eof(dataBufferReader *data);
void moveBeginning(dataBufferReader *data);

#endif // BUFFER_READER_H

