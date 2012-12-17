#ifndef POINTERS_H_GUARD
#define POINTERS_H_GUARD

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "buffer/FixedIntCounter.h"
#include "buffer/FixedLongCounter.h"
#include "Config.h"

typedef struct Pointers Pointers;

struct Pointers {
  FixedIntCounter* df;
  FixedLongCounter* startPointers;
};

Pointers* createPointers(int size) {
  Pointers* pointers = (Pointers*) malloc(sizeof(Pointers));
  pointers->df = createFixedIntCounter(size, 0);
  pointers->startPointers = createFixedLongCounter(size, UNDEFINED_POINTER);
  return pointers;
}

void destroyPointers(Pointers* pointers) {
  destroyFixedLongCounter(pointers->startPointers);
  destroyFixedIntCounter(pointers->df);
}

int getDf(Pointers* pointers, int term) {
  return getFixedIntCounter(pointers->df, term);
}

void setDf(Pointers* pointers, int term, int df) {
  setFixedIntCounter(pointers->df, term, df);
}

long getStartPointer(Pointers* pointers, int term) {
  return getFixedLongCounter(pointers->startPointers, term);
}

void setStartPointer(Pointers* pointers, int term, long sp) {
  setFixedLongCounter(pointers->startPointers, term, sp);
}

int nextTerm(Pointers* pointers, int term) {
  return nextIndexFixedLongCounter(pointers->startPointers, term);
}

void writePointers(Pointers* pointers, FILE* fp) {
  int size = sizeFixedLongCounter(pointers->startPointers);
  fwrite(&size, sizeof(unsigned int), 1, fp);
  int term = -1;
  while((term = nextIndexFixedLongCounter(pointers->startPointers, term)) != -1) {
    fwrite(&term, sizeof(int), 1, fp);
    fwrite(&pointers->df->counter[term], sizeof(int), 1, fp);
    fwrite(&pointers->startPointers->counter[term], sizeof(long), 1, fp);
  }
}

Pointers* readPointers(FILE* fp) {
  Pointers* pointers = createPointers(DEFAULT_VOCAB_SIZE);

  unsigned int size = 0;
  fread(&size, sizeof(unsigned int), 1, fp);
  int i, term, value;
  long pointer;
  for(i = 0; i < size; i++) {
    fread(&term, sizeof(int), 1, fp);
    fread(&value, sizeof(int), 1, fp);
    fread(&pointer, sizeof(long), 1, fp);
    setFixedIntCounter(pointers->df, term, value);
    setFixedLongCounter(pointers->startPointers, term, pointer);
  }
  return pointers;
}

#endif
