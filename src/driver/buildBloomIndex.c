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
#include "bloom/BloomIndex.h"

int main (int argc, char** args) {
  char* inputPath = getValueCL(argc, args, "-index");
  char* outputPath = getValueCL(argc, args, "-output");
  int bitsPerElement = atoi(getValueCL(argc, args, "-r"));
  int nbHash = atoi(getValueCL(argc, args, "-k"));

  // Read the inverted index
  InvertedIndex* index = readInvertedIndex(inputPath);
  BloomIndex* bloom = createBloomIndex(DEFAULT_VOCAB_SIZE, bitsPerElement,
                                       nbHash);

  PostingsList* list;
  int term = -1;
  while((term = nextTermId(index, term)) != -1) {
    printf("term: %d\n", term);
    list = getPostingsList(index, term);
    createBloomFilterInIndex(bloom, term, getDocumentFrequency(list));

    while(hasNext(list)) {
      nextPosting(list);
      insertIntoBloomFilterInIndex(bloom, term, getDocumentId(list));
    }
    destroyPostingsList(list);
  }

  FILE* fp = fopen(outputPath, "wb");
  writeBloomIndex(bloom, fp);
  fclose(fp);

  // Free the allocated space
  destroyInvertedIndex(index);
  destroyBloomIndex(bloom);
  return 0;
}
