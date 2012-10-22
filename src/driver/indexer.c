#include <stdlib.h>
#include <zlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "dictionary/Dictionary.h"
#include "dictionary/Vocab.h"

#define LENGTH 8*4096
#define LINE_LENGTH 0x100000

inline int process(Dictionary* dic, char* line, int termid) {
  int docid = 0, consumed;
  sscanf(line, "%d%n", &docid, &consumed);

  char* token = strtok(line+consumed+1, " ");
  while(token) {
    int id = putIfNotPresent(dic, token, strlen(token), termid);
    if(id == termid) {
      termid++;
    }
    token = strtok(NULL, " ");
  }
  return termid;
}

int main (int argc, char** args) {
  char* outputPath = args[1];

  Dictionary* dic = createDictionary(DEFAULT_VOCAB_SIZE);
  int termid = 0;

  unsigned char oldBuffer[LINE_LENGTH * 2];
  unsigned char buffer[LENGTH];
  unsigned char line[LINE_LENGTH];
  gzFile * file;

  struct timeval start, end;
  gettimeofday(&start, NULL);

  int fp = 0;
  for(fp = 2; fp < argc; fp++) {
    file = gzopen(args[fp], "r");
    int oldBufferIndex = 0;

    while (1) {
      int bytes_read;
      bytes_read = gzread (file, buffer, LENGTH - 1);
      buffer[bytes_read] = '\0';

      int consumed;
      int start = 0;
      int c = sscanf(buffer, "%[^\n]\n%n", line, &consumed);
      while(c > 0) {
        if(buffer[start+consumed - 1] == '\n') {
          if(oldBufferIndex > 0) {
            memcpy(oldBuffer+oldBufferIndex, line, consumed);
            termid = process(dic, oldBuffer, termid);
            memset(oldBuffer, 0, oldBufferIndex);
            oldBufferIndex = 0;
          } else {
            termid = process(dic, line, termid);
          }
        } else {
          memcpy(oldBuffer+oldBufferIndex, line, consumed);
          oldBufferIndex += consumed;
        }

        start += consumed;
        c = sscanf(buffer+start, "%[^\n]\n%n", line, &consumed);
      }
      if (bytes_read < LENGTH - 1) {
        if (gzeof (file)) {
          break;
        }
      }
    }
    gzclose (file);

    gettimeofday(&end, NULL);
    printf("Files processed: %d Time: %6.0f\n", fp, ((float) (end.tv_sec - start.tv_sec)));
    fflush(stdout);
  }

  gettimeofday(&end, NULL);
  printf("Time: %6.0f\n", ((float) (end.tv_sec - start.tv_sec)));

  FILE* ofp = fopen(outputPath, "wb");
  writeDictionary(dic, ofp);
  fclose(ofp);

  destroyDictionary(dic);
  return 0;
}
