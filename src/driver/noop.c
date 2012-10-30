#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <zlib.h>

#define LENGTH 8*4096
#define LINE_LENGTH 0x100000

int process(char* line, int termid) {
  int docid = 0, consumed;
  sscanf(line, "%d%n", &docid, &consumed);

  char* token = strtok(line+consumed+1, " ");
  while(token) {
    termid++;
    token = strtok(NULL, " ");
  }
  return termid;
}

int main (int argc, char** args) {
  int termid = 0;

  unsigned char oldBuffer[LINE_LENGTH * 2];
  unsigned char iobuffer[LENGTH];
  unsigned char line[LINE_LENGTH];
  gzFile* file;

  struct timeval start, end;
  gettimeofday(&start, NULL);

  int fp = 0;
  for(fp = 1; fp < argc; fp++) {
    file = gzopen(args[fp], "r");
    int oldBufferIndex = 0;

    while (1) {
      int bytes_read;
      bytes_read = gzread (file, iobuffer, LENGTH - 1);
      iobuffer[bytes_read] = '\0';

      int consumed;
      int start = 0;
      int c;
      if(iobuffer[0] == '\n') {
        consumed = 1;
        c = 1;
      } else {
        c = sscanf(iobuffer, "%[^\n]\n%n", line, &consumed);
      }
      while(c > 0) {
        if(iobuffer[start+consumed - 1] == '\n') {
          if(oldBufferIndex > 0) {
            memcpy(oldBuffer+oldBufferIndex, line, consumed);
            termid = process(oldBuffer, termid);
            memset(oldBuffer, 0, oldBufferIndex);
            oldBufferIndex = 0;
          } else {
            termid = process(line, termid);
          }
        } else {
          memcpy(oldBuffer+oldBufferIndex, line, consumed);
          oldBufferIndex += consumed;
        }

        start += consumed;
        c = sscanf(iobuffer+start, "%[^\n]\n%n", line, &consumed);
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
  fflush(stdout);

  return 0;
}
