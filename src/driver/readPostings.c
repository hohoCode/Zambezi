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
#include "PostingsList.h"

int main (int argc, char** args) {
  char* inputPath = getValueCL(argc, args, "-index");
  char* inputTerm = getValueCL(argc, args, "-term");

  // Read the inverted index
  InvertedIndex* index = readInvertedIndex(inputPath);

  if(inputTerm) {
    // Read postings for the given term

    // Read term id for the input term
    int termid = getTermId(index->dictionary, inputTerm);

    // Check whether "termid" has valid postings (terms with
    // df < df-cutoff do not have postings stored in the index)
    if(!hasValidPostingsList(index, termid)) {
      printf("No postings for term %s\n", inputTerm);
      return -1;
    }

    // Retrieve PostingsList for "termid"
    PostingsList* list = getPostingsList(index, termid);

    // Read postings for the given term, one document id at a time
    while(hasNext(list)) {
      // Load the next document id and tf
      nextPosting(list);

      // Print docid and tf
      printf("%d:%d ", getDocumentId(list), getTermFrequency(list));
    }
    printf("\n");

    // Free the allocated space
    destroyPostingsList(list);
  } else {
    // Print df for all terms that have a valid StartPointer
    int term = -1;
    while((term = nextTermId(index, term)) != -1) {
      printf("termid: %d  df: %d\n", term, getDf_InvertedIndex(index, term));
    }
  }

  // Free the allocated space
  destroyInvertedIndex(index);
  return 0;
}
