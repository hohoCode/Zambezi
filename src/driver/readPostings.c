#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "util/ParseCommandLine.h"
#include "dictionary/Dictionary.h"
#include "PostingsPool.h"
#include "Pointers.h"
#include "Config.h"
#include "InvertedIndex.h"

int main (int argc, char** args) {
  char* inputPath = getValueCL(argc, args, "-index");
  char* inputTerm = getValueCL(argc, args, "-term");

  // Read the inverted index
  InvertedIndex* index = readInvertedIndex(inputPath);

  if(inputTerm) {
    // Read postings for the given term

    // Read term id for the input term
    int termid = getTermId(index->dictionary, inputTerm);

    // Read StartPointer for the given term and check whether
    // the pointer is valid
    long pointer = getStartPointer(index->pointers, termid);

    if(pointer == UNDEFINED_POINTER) {
      printf("No postings for term %s\n", inputTerm);
      return -1;
    }

    // Read postings for the given term, one segment at a time
    int* docid = (int*) calloc(BLOCK_SIZE, sizeof(int));
    while(pointer != UNDEFINED_POINTER) {
      // Decompress one docid block. The return value is the number
      // of docids decompressed
      int count = decompressDocidBlock(index->pool, docid, pointer);

      // Print docids
      int i = 0;
      for(i = 0; i < count; i++) {
        printf("%d ", docid[i]);
      }

      // Set the pointer to the next pointer
      pointer = nextPointer(index->pool, pointer);
    }
    printf("\n");
    free(docid);
  } else {
    // Print df for all terms that have a valid StartPointer
    int term = -1;
    while((term = nextTerm(index->pointers, term)) != -1) {
      printf("termid: %d  df: %d\n", term, getDf(index->pointers, term));
    }
  }

  destroyInvertedIndex(index);
  return 0;
}
