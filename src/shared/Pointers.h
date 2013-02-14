#ifndef POINTERS_H_GUARD
#define POINTERS_H_GUARD

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "buffer/FixedIntCounter.h"
#include "buffer/FixedLongCounter.h"
#include "Config.h"

//typedef struct Pointers Pointers;

typedef struct Pointers {
  FixedIntCounter* df;
  FixedLongCounter* startPointers;
  FixedIntCounter* docLen;
  FixedIntCounter* maxTf;
  FixedIntCounter* maxTfDocLen;
  int totalDocs;
  unsigned long totalDocLen;
} Pointers;

Pointers* createPointers(int size) {
  Pointers* pointers = (Pointers*) malloc(sizeof(Pointers));
  pointers->df = createFixedIntCounter(size, 0);
  pointers->startPointers = createFixedLongCounter(size, UNDEFINED_POINTER);
  //pointers->docLen = createFixedIntCounter(size, 0);
  //pointers->maxTf = createFixedIntCounter(size, 0);
  //pointers->maxTfDocLen = createFixedIntCounter(size, 0);
  pointers->totalDocs = 0;
  pointers->totalDocLen = 0;
  return pointers;
}

void destroyPointers(Pointers* pointers) {
  destroyFixedLongCounter(pointers->startPointers);
  destroyFixedIntCounter(pointers->df);
  //destroyFixedIntCounter(pointers->docLen);
  //destroyFixedIntCounter(pointers->maxTf);
  //destroyFixedIntCounter(pointers->maxTfDocLen);
}

int getDf(Pointers* pointers, int term) {
  return getFixedIntCounter(pointers->df, term);
}

void setDf(Pointers* pointers, int term, int df) {
  setFixedIntCounter(pointers->df, term, df);
}

int getDocLen(Pointers* pointers, int docid) {
  return getFixedIntCounter(pointers->docLen, docid);
}

void setDocLen(Pointers* pointers, int docid, int docLen) {
  setFixedIntCounter(pointers->docLen, docid, docLen);
}

int getMaxTf(Pointers* pointers, int term) {
  return getFixedIntCounter(pointers->maxTf, term);
}

int getMaxTfDocLen(Pointers* pointers, int term) {
  return getFixedIntCounter(pointers->maxTfDocLen, term);
}

void setMaxTf(Pointers* pointers, int term, int tf, int dl) {
  setFixedIntCounter(pointers->maxTf, term, tf);
  setFixedIntCounter(pointers->maxTfDocLen, term, dl);
}

long getStartPointer(Pointers* pointers, int term) {
  return getFixedLongCounter(pointers->startPointers, term);
}

void setStartPointer(Pointers* pointers, int term, long sp) {
  setFixedLongCounter(pointers->startPointers, term, sp);
}

int nextTerm(Pointers* pointers, int currentTermId) {
  return nextIndexFixedLongCounter(pointers->startPointers, currentTermId);
}

void writePointers(Pointers* pointers, FILE* fp) {
  int size = sizeFixedLongCounter(pointers->startPointers);
  fwrite(&size, sizeof(unsigned int), 1, fp);
  int term = -1;
  while((term = nextIndexFixedLongCounter(pointers->startPointers, term)) != -1) {
    fwrite(&term, sizeof(int), 1, fp);
    fwrite(&pointers->df->counter[term], sizeof(int), 1, fp);
    fwrite(&pointers->startPointers->counter[term], sizeof(long), 1, fp);
    fwrite(&pointers->maxTf->counter[term], sizeof(int), 1, fp);
    fwrite(&pointers->maxTfDocLen->counter[term], sizeof(int), 1, fp);
  }

  size = sizeFixedIntCounter(pointers->docLen);
  fwrite(&size, sizeof(unsigned int), 1, fp);

  while((term = nextIndexFixedIntCounter(pointers->docLen, term)) != -1) {
    fwrite(&term, sizeof(int), 1, fp);
    fwrite(&pointers->docLen->counter[term], sizeof(int), 1, fp);
  }

  fwrite(&pointers->totalDocs, sizeof(int), 1, fp);
  fwrite(&pointers->totalDocLen, sizeof(unsigned long), 1, fp);
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
    setFixedIntCounter(pointers->df, term, value);
    fread(&pointer, sizeof(long), 1, fp);
    setFixedLongCounter(pointers->startPointers, term, pointer);
    fread(&value, sizeof(int), 1, fp);
    //setFixedIntCounter(pointers->maxTf, term, value);
    fread(&value, sizeof(int), 1, fp);
    //setFixedIntCounter(pointers->maxTfDocLen, term, value);
  }

  fread(&size, sizeof(unsigned int), 1, fp);
  for(i = 0; i < size; i++) {
    fread(&term, sizeof(int), 1, fp);
    fread(&value, sizeof(int), 1, fp);
    //setFixedIntCounter(pointers->docLen, term, value);
  }

  fread(&pointers->totalDocs, sizeof(int), 1, fp);
  fread(&pointers->totalDocLen, sizeof(unsigned long), 1, fp);
  return pointers;
}

#endif
