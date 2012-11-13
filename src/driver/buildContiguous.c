#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "dictionary/hashtable.h"
#include "buffer/FixedIntCounter.h"
#include "buffer/FixedLongCounter.h"
#include "util/ParseCommandLine.h"
#include "PostingsPool.h"
#include "Config.h"

#define TERMINAL_DOCID -1
#define NUMBER_OF_POOLS 4

int main (int argc, char** args) {
  char* inputPath = getValueCL(argc, args, "-input");
  char* outputPath = getValueCL(argc, args, "-output");

  char dicPath[1024];
  strcpy(dicPath, inputPath);
  strcat(dicPath, "/");
  strcat(dicPath, DICTIONARY_FILE);
  FILE* fp = fopen(dicPath, "rb");
  Dictionary** dic = readhashtable(fp);
  fclose(fp);

  FixedIntCounter* df = createFixedIntCounter(DEFAULT_VOCAB_SIZE, 0);
  FixedLongCounter* startPointers =
    createFixedLongCounter(DEFAULT_VOCAB_SIZE, UNDEFINED_POINTER);
  char pointerPath[1024];
  strcpy(pointerPath, inputPath);
  strcat(pointerPath, "/");
  strcat(pointerPath, POINTER_FILE);
  fp = fopen(pointerPath, "rb");
  unsigned int size = 0;
  fread(&size, sizeof(unsigned int), 1, fp);
  int i, term, value;
  long pointer;
  for(i = 0; i < size; i++) {
    fread(&term, sizeof(int), 1, fp);
    fread(&value, sizeof(int), 1, fp);
    fread(&pointer, sizeof(long), 1, fp);
    setFixedIntCounter(df, term, value);
    setFixedLongCounter(startPointers, term, pointer);
  }
  fclose(fp);

  //create a contiguous index
  char indexPath[1024];
  strcpy(indexPath, inputPath);
  strcat(indexPath, "/");
  strcat(indexPath, INDEX_FILE);
  fp = fopen(indexPath, "rb");

  PostingsPool* contiguousPool = createPostingsPool(NUMBER_OF_POOLS);
  FixedLongCounter* contiguousStartPointers =
    createFixedLongCounter(DEFAULT_VOCAB_SIZE, UNDEFINED_POINTER);

  term = -1;
  while((term = nextIndexFixedLongCounter(startPointers, term)) != -1) {
    long pointer = startPointers->counter[term];
    long newPointer = readPostingsForTerm(contiguousPool, pointer, fp);
    setFixedLongCounter(contiguousStartPointers, term, newPointer);
  }
  fclose(fp);
  //end sorting index


  //write output
  char odicPath[1024];
  strcpy(odicPath, outputPath);
  strcat(odicPath, "/");
  strcat(odicPath, DICTIONARY_FILE);

  fp = fopen(odicPath, "wb");
  writehashtable(dic, fp);
  fclose(fp);

  char oindexPath[1024];
  strcpy(oindexPath, outputPath);
  strcat(oindexPath, "/");
  strcat(oindexPath, INDEX_FILE);

  fp = fopen(oindexPath, "wb");
  writePostingsPool(contiguousPool, fp);
  fclose(fp);

  char opointerPath[1024];
  strcpy(opointerPath, outputPath);
  strcat(opointerPath, "/");
  strcat(opointerPath, POINTER_FILE);

  fp = fopen(opointerPath, "wb");
  size = sizeFixedLongCounter(contiguousStartPointers);
  fwrite(&size, sizeof(unsigned int), 1, fp);
  term = -1;
  while((term = nextIndexFixedLongCounter(contiguousStartPointers, term)) != -1) {
    fwrite(&term, sizeof(int), 1, fp);
    fwrite(&df->counter[term], sizeof(int), 1, fp);
    fwrite(&contiguousStartPointers->counter[term], sizeof(long), 1, fp);
  }
  fclose(fp);

  destroyhashtable(dic);
  destroyPostingsPool(contiguousPool);
  destroyFixedIntCounter(df);
  destroyFixedLongCounter(startPointers);
  destroyFixedLongCounter(contiguousStartPointers);

  return 0;
}
