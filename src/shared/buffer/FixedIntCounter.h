#ifndef FIXED_INT_COUNTER_H_GUARD
#define FIXED_INT_COUNTER_H_GUARD

#include <stdlib.h>
#include <string.h>

typedef struct FixedIntCounter FixedIntCounter;

#ifndef DEFAULT_VALUE_ENUM_GUARD
#define DEFAULT_VALUE_ENUM_GUARD
typedef enum DefaultValue DefaultValue;
enum DefaultValue {
  ZERO = 0,
  NEGATIVE_ONE = -1
};
#endif

struct FixedIntCounter {
  int* counter;
  unsigned int vocabSize;
  DefaultValue defaultValue;
};

FixedIntCounter* createFixedIntCounter(int initialSize, DefaultValue defaultValue) {
  FixedIntCounter* counter = (FixedIntCounter*) malloc(sizeof(FixedIntCounter));
  counter->vocabSize = initialSize;
  counter->defaultValue = defaultValue;
  counter->counter = (int*) calloc(initialSize, sizeof(int));
  if(defaultValue) {
    memset(counter->counter, defaultValue, initialSize * sizeof(int));
  }
  return counter;
}

void destroyFixedIntCounter(FixedIntCounter* counter) {
  free(counter->counter);
  free(counter);
}

void expandFixedIntCounter(FixedIntCounter* counter) {
  int* temp = (int*) realloc(counter->counter, counter->vocabSize * 2 * sizeof(int));
  if(counter->defaultValue) {
    memset(&temp[counter->vocabSize], counter->defaultValue,
           counter->vocabSize * sizeof(int));
  }
  counter->vocabSize *= 2;
  counter->counter = temp;
}

unsigned int sizeFixedIntCounter(FixedIntCounter* counter) {
  unsigned int nbElements = 0;
  int i = 0;
  for(i = 0; i < counter->vocabSize; i++) {
    if(counter->counter[i] != counter->defaultValue) {
      nbElements++;
    }
  }
  return nbElements;
}

int getFixedIntCounter(FixedIntCounter* counter, unsigned int index) {
  while(index >= counter->vocabSize) {
    expandFixedIntCounter(counter);
  }
  return counter->counter[index];
}

void addFixedIntCounter(FixedIntCounter* counter, unsigned int index, int c) {
  while(index >= counter->vocabSize) {
    expandFixedIntCounter(counter);
  }
  counter->counter[index] += c;
}

void incrementFixedIntCounter(FixedIntCounter* counter, unsigned int index) {
  addFixedIntCounter(counter, index, 1);
}

void setFixedIntCounter(FixedIntCounter* counter, unsigned int index, int c) {
  while(index >= counter->vocabSize) {
    expandFixedIntCounter(counter);
  }
  counter->counter[index] = c;
}

void resetFixedIntCounter(FixedIntCounter* counter, unsigned int index) {
  setFixedIntCounter(counter, index, counter->defaultValue);
}

int nextIndexFixedIntCounter(FixedIntCounter* counter, int pos) {
  pos++;
  if(pos >= counter->vocabSize) {
    return -1;
  }

  while(counter->counter[pos] == counter->defaultValue) {
    pos++;
    if(pos >= counter->vocabSize) {
      return -1;
    }
  }
  return pos;
}
#endif
