#ifndef FIXED_LONG_COUNTER_H_GUARD
#define FIXED_LONG_COUNTER_H_GUARD

#include <stdlib.h>
#include <string.h>

typedef struct FixedLongCounter FixedLongCounter;

#ifndef DEFAULT_VALUE_ENUM_GUARD
#define DEFAULT_VALUE_ENUM_GUARD
typedef enum DefaultValue DefaultValue;
enum DefaultValue {
  ZERO = 0,
  NEGATIVE_ONE = -1
};
#endif

struct FixedLongCounter {
  long* counter;
  unsigned int vocabSize;
  DefaultValue defaultValue;
};

FixedLongCounter* createFixedLongCounter(int initialSize, DefaultValue defaultValue) {
  FixedLongCounter* counter = (FixedLongCounter*) malloc(sizeof(FixedLongCounter));
  counter->vocabSize = initialSize;
  counter->defaultValue = defaultValue;
  counter->counter = (long*) calloc(initialSize, sizeof(long));
  if(defaultValue) {
    memset(counter->counter, defaultValue, initialSize * sizeof(long));
  }
  return counter;
}

void destroyFixedLongCounter(FixedLongCounter* counter) {
  free(counter->counter);
  free(counter);
}

void expandFixedLongCounter(FixedLongCounter* counter) {
  long* temp = (long*) realloc(counter->counter, counter->vocabSize * 2 * sizeof(long));
  if(counter->defaultValue) {
    memset(&temp[counter->vocabSize], counter->defaultValue,
           counter->vocabSize * sizeof(long));
  }
  counter->vocabSize *= 2;
  counter->counter = temp;
}

unsigned int sizeFixedLongCounter(FixedLongCounter* counter) {
  unsigned int nbElements = 0;
  int i = 0;
  for(i = 0; i < counter->vocabSize; i++) {
    if(counter->counter[i] != counter->defaultValue) {
      nbElements++;
    }
  }
  return nbElements;
}

long getFixedLongCounter(FixedLongCounter* counter, unsigned int index) {
  while(index >= counter->vocabSize) {
    expandFixedLongCounter(counter);
  }

  return counter->counter[index];
}

void addFixedLongCounter(FixedLongCounter* counter, unsigned int index, long c) {
  while(index >= counter->vocabSize) {
    expandFixedLongCounter(counter);
  }
  counter->counter[index] += c;
}

void incrementFixedLongCounter(FixedLongCounter* counter, unsigned int index) {
  addFixedLongCounter(counter, index, 1);
}

void setFixedLongCounter(FixedLongCounter* counter, unsigned int index, long c) {
  while(index >= counter->vocabSize) {
    expandFixedLongCounter(counter);
  }
  counter->counter[index] = c;
}

void resetFixedLongCounter(FixedLongCounter* counter, unsigned int index) {
  setFixedLongCounter(counter, index, counter->defaultValue);
}

int nextIndexFixedLongCounter(FixedLongCounter* counter, int pos) {
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
