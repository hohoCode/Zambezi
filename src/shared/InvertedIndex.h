/**
 * An inverted index data structure consisting of
 * the following components:
 *
 *  - PostingsPool, which contains all segments.
 *  - Dictionary, which is a mapping from term to term id
 *  - Pointers, which contains Document Frequency, StartPointers,
 *    etc.
 *
 * @author Nima Asadi
 */

#ifndef InvertedIndex_H_GUARD
#define InvertedIndex_H_GUARD

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "buffer/FixedIntCounter.h"
#include "buffer/FixedLongCounter.h"
#include "dictionary/Dictionary.h"
#include "PostingsPool.h"
#include "Pointers.h"
#include "Config.h"

typedef struct InvertedIndex InvertedIndex;

struct InvertedIndex {
  PostingsPool* pool;
  Dictionary** dictionary;
  Pointers* pointers;
};

InvertedIndex* createInvertedIndex(int bloomEnabled, unsigned int nbHash,
                                   unsigned int bitsPerElement) {
  InvertedIndex* index = (InvertedIndex*) malloc(sizeof(InvertedIndex));
  index->pool = createPostingsPool(NUMBER_OF_POOLS, bloomEnabled,
                                   nbHash, bitsPerElement);
  index->dictionary = initDictionary();
  index->pointers = createPointers(DEFAULT_VOCAB_SIZE);
  return index;
}

int hasValidPostingsList(InvertedIndex* index, int termid) {
  return getStartPointer(index->pointers, termid) != UNDEFINED_POINTER;
}

/**
 * An iterator over terms with a valid StartPointer.
 * Call this function as follows:
 *
 *   int term = -1;
 *   while((term = nextTerm(pointers, term)) != -1) {
 *     ...
 *   }
 */
int nextTermId(InvertedIndex* index, int currentTermId) {
  return nextIndexFixedLongCounter(index->pointers->startPointers, currentTermId);
}

int getDf_InvertedIndex(InvertedIndex* index, int term) {
  return getFixedIntCounter(index->pointers->df, term);
}

void destroyInvertedIndex(InvertedIndex* index) {
  destroyPostingsPool(index->pool);
  destroyDictionary(index->dictionary);
  destroyPointers(index->pointers);
}

InvertedIndex* readInvertedIndex(char* rootPath) {
  InvertedIndex* index = (InvertedIndex*) malloc(sizeof(InvertedIndex));

  char dicPath[1024];
  strcpy(dicPath, rootPath);
  strcat(dicPath, "/");
  strcat(dicPath, DICTIONARY_FILE);
  FILE* fp = fopen(dicPath, "rb");
  index->dictionary = readDictionary(fp);
  fclose(fp);

  char indexPath[1024];
  strcpy(indexPath, rootPath);
  strcat(indexPath, "/");
  strcat(indexPath, INDEX_FILE);
  fp = fopen(indexPath, "rb");
  index->pool = readPostingsPool(fp);
  fclose(fp);

  char pointerPath[1024];
  strcpy(pointerPath, rootPath);
  strcat(pointerPath, "/");
  strcat(pointerPath, POINTER_FILE);
  fp = fopen(pointerPath, "rb");
  index->pointers = readPointers(fp);
  fclose(fp);

  return index;
}

void writeInvertedIndex(InvertedIndex* index, char* rootPath) {
  char dicPath[1024];
  strcpy(dicPath, rootPath);
  strcat(dicPath, "/");
  strcat(dicPath, DICTIONARY_FILE);
  FILE* ofp = fopen(dicPath, "wb");
  writeDictionary(index->dictionary, ofp);
  fclose(ofp);

  char indexPath[1024];
  strcpy(indexPath, rootPath);
  strcat(indexPath, "/");
  strcat(indexPath, INDEX_FILE);
  ofp = fopen(indexPath, "wb");
  writePostingsPool(index->pool, ofp);
  fclose(ofp);

  char pointerPath[1024];
  strcpy(pointerPath, rootPath);
  strcat(pointerPath, "/");
  strcat(pointerPath, POINTER_FILE);
  ofp = fopen(pointerPath, "wb");
  writePointers(index->pointers, ofp);
  fclose(ofp);
}

#endif
