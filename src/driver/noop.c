#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <zlib.h>

#define LENGTH 32*1024
#define LINE_LENGTH 0x100000

int grabword(char* t, char del) {
  char* s = t;
  int consumed = 0;
  while(*s != '\0' && *s != del) {
    consumed++;
    s++;
  }

  consumed += (*s == del);
  *s = '\0';
  return consumed;
}

int process(char* line, int termid) {
  int docid = 0, consumed;
  consumed = grabword(line, '\t');
  docid = atoi(line);
  line += consumed;

  consumed = grabword(line, ' ');
  while(consumed > 0) {
    termid++;
    line += consumed;
    consumed = grabword(line, ' ');
  }
  return termid;
}

int grabline(char* t, char* buffer, int* consumed) {
  int c = 0;
  char* s = t;
  *consumed = 0;
  while(*s != '\0' && *s != '\n') {
    (*consumed)++;
    s++;
  }
  if(*consumed == 0) return 0;

  memcpy(buffer, t, *consumed);
  buffer[*consumed] = '\0';
  *consumed += (*s == '\n');
  return *s == '\n';
}

int main (int argc, char** args) {
  int termid = 0;

  unsigned char* oldBuffer = (unsigned char*) calloc(LINE_LENGTH * 2, sizeof(unsigned char));
  unsigned char* iobuffer = (unsigned char*) calloc(LENGTH, sizeof(unsigned char));
  unsigned char* line = (unsigned char*) calloc(LINE_LENGTH, sizeof(unsigned char));
  gzFile* file;

  struct timeval start, end;
  gettimeofday(&start, NULL);

  int fp = 0;
  int len = 0;
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
        c = grabline(iobuffer, line+len, &consumed);
        len += consumed;
      }
      while(c > 0) {
        if(iobuffer[start+consumed - 1] == '\n') {
          if(oldBufferIndex > 0) {
            memcpy(oldBuffer+oldBufferIndex, line, consumed);
            termid = process(oldBuffer, termid);
            memset(oldBuffer, 0, oldBufferIndex);
            oldBufferIndex = 0;
            len = 0;
          } else {
            termid = process(line, termid);
            len = 0;
          }
        } else {
          memcpy(oldBuffer+oldBufferIndex, line, consumed);
          oldBufferIndex += consumed;
          len = 0;
        }

        start += consumed;
        c = grabline(iobuffer+start, line + len, &consumed);
        len += consumed;
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

  free(oldBuffer);
  free(iobuffer);
  free(line);

  return 0;
}
