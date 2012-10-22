#ifndef FIXED_BUFFER_H_GUARD
#define FIXED_BUFFER_H_GUARD

#include <stdlib.h>
#include <string.h>

typedef struct FixedBuffer FixedBuffer;

struct FixedBuffer {
  int* buffer;
  unsigned int bufferSize;
  unsigned int vocabSize;
};

FixedBuffer* createFixedBuffer(unsigned int initialSize, unsigned int bufferSize) {
  FixedBuffer* buffer = (FixedBuffer*) malloc(sizeof(FixedBuffer));
  buffer->vocabSize = initialSize;
  buffer->bufferSize = bufferSize;
  buffer->buffer = (int*) calloc(initialSize * bufferSize, sizeof(int));
  return buffer;
}

void destroyFixedBuffer(FixedBuffer* buffer) {
  free(buffer->buffer);
  free(buffer);
}

void expandFixedBuffer(FixedBuffer* buffer) {
  unsigned int len = buffer->vocabSize * buffer->bufferSize;
  int* temp = (int*) realloc(buffer->buffer, len * 2 * sizeof(int));
  memset(&temp[len], 0, len * sizeof(int));
  buffer->vocabSize *= 2;
  buffer->buffer = temp;
}

int getFixedBuffer(FixedBuffer* buffer, unsigned int index, unsigned int offset) {
  while(index >= buffer->vocabSize) {
    expandFixedBuffer(buffer);
  }
  return buffer->buffer[index * buffer->bufferSize + offset];
}

void setFixedBuffer(FixedBuffer* buffer, unsigned int index,
                   unsigned int offset, int value) {
  while(index >= buffer->vocabSize) {
    expandFixedBuffer(buffer);
  }
  buffer->buffer[index * buffer->bufferSize + offset] = value;
}

int* getStartFixedBuffer(FixedBuffer* buffer, unsigned int index) {
  return &buffer->buffer[index * buffer->bufferSize];
}
#endif
