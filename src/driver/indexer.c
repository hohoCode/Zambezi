#include <stdlib.h>
#include <zlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "pfordelta/opt_p4.h"
#include "dictionary/Dictionary.h"
#include "buffer/FixedBuffer.h"
#include "buffer/DynamicBuffer.h"
#include "buffer/FixedIntCounter.h"
#include "buffer/FixedLongCounter.h"
#include "buffer/IntSet.h"
#include "util/ParseCommandLine.h"
#include "PostingsPool.h"
#include "Pointers.h"
#include "Config.h"
#include "InvertedIndex.h"

#define LENGTH 32*1024
#define LINE_LENGTH 0x100000

typedef struct IndexingData IndexingData;
struct IndexingData {
  DynamicBuffer* buffer;
  FixedIntCounter* psum;
  IntSet* uniqueTerms;
  int positional;
  int expansionEnabled;
  int maxBlocks;
};

void destroyIndexingData(IndexingData* data) {
  destroyDynamicBuffer(data->buffer);
  if(data->psum) {
    destroyFixedIntCounter(data->psum);
  }
  destroyIntSet(data->uniqueTerms);
  free(data);
}

void grabword(char* t, char del, int* consumed) {
  char* s = t;
  *consumed = 0;
  while(*s != '\0' && *s != del) {
    (*consumed)++;
    s++;
  }

  (*consumed) += (*s == del);
  *s = '\0';
}

int process(InvertedIndex* index, IndexingData* data, char* line, int termid) {
  int docid = 0, consumed;
  grabword(line, '\t', &consumed);
  docid = atoi(line);
  line += consumed;

  short position = 1;
  clearIntSet(data->uniqueTerms);
  grabword(line, ' ', &consumed);
  while(consumed > 0) {
    int id = setTermId(index->dictionary, line, termid);
    int added = addIntSet(&data->uniqueTerms, id);
    if(id == termid) {
      termid++;
    }

    if(data->positional == TFONLY) {
      short* curtfBuffer = getTfDynamicBuffer(data->buffer, id);
      if(!curtfBuffer) {
        curtfBuffer = (short*) calloc(DF_CUTOFF + 1, sizeof(short));
        data->buffer->tf[id] = curtfBuffer;
      }
      curtfBuffer[data->buffer->valuePosition[id]]++;
    } else if(data->positional == POSITIONAL) {
      short* curtfBuffer = getTfDynamicBuffer(data->buffer, id);
      short* curBuffer = data->buffer->position[id];
      int ps = getFixedIntCounter(data->psum, id);
      if(!curBuffer) {
        curBuffer = (short*) calloc(DF_CUTOFF, sizeof(short));
        data->buffer->position[id] = curBuffer;
        data->buffer->pvalueLength[id] = DF_CUTOFF;
        data->buffer->pvaluePosition[id] = 1;

        curtfBuffer = (short*) calloc(DF_CUTOFF + 1, sizeof(short));
        data->buffer->tf[id] = curtfBuffer;
      }

      if(data->buffer->pvalueLength[id] <= data->buffer->pvaluePosition[id] + 1) {
        int len = data->buffer->pvalueLength[id];
        int newLen = 2 * len;
        while(newLen <= data->buffer->pvaluePosition[id] + 1) {
          newLen *= 2;
        }
        short* tempCurBuffer = (short*) realloc(curBuffer, newLen * sizeof(short));
        memset(tempCurBuffer+len, 0, (newLen - len) * sizeof(short));
        data->buffer->position[id] = tempCurBuffer;
        data->buffer->pvalueLength[id] = newLen;
        curBuffer = data->buffer->position[id];
      }

      int pbufferpos = data->buffer->pvaluePosition[id];
      if(!added) {
        curBuffer[pbufferpos] = position - curBuffer[pbufferpos];
        pbufferpos++;
      } else {
        curBuffer[pbufferpos++] = position;
      }

      curBuffer[pbufferpos] = position;
      data->buffer->pvaluePosition[id]++;
      data->buffer->position[id][ps]++;
      curtfBuffer[data->buffer->valuePosition[id]]++;
      position++;
    }

    line += consumed;
    grabword(line, ' ', &consumed);
  }

  int keyPos = -1;
  while((keyPos = nextIndexIntSet(data->uniqueTerms, keyPos)) != -1) {
    int id = data->uniqueTerms->key[keyPos];

    if(data->positional == POSITIONAL) {
      data->buffer->position[id][data->buffer->pvaluePosition[id]] = 0;
    }

    int df = getDf(index->pointers, id);
    if(df < DF_CUTOFF) {
      int* curBuffer = getDocidDynamicBuffer(data->buffer, id);
      if(!curBuffer) {
        curBuffer = (int*) calloc(DF_CUTOFF, sizeof(int));
        data->buffer->docid[id] = curBuffer;
        data->buffer->valueLength[id] = DF_CUTOFF;
      }
      data->buffer->docid[id][df] = docid;
      data->buffer->valuePosition[id]++;
      index->pointers->df->counter[id]++;
      continue;
    }

    int* curBuffer = data->buffer->docid[id];
    if(data->buffer->valueLength[id] < BLOCK_SIZE) {
      int* tempCurBuffer = (int*) realloc(curBuffer, BLOCK_SIZE * sizeof(int));
      memset(tempCurBuffer+DF_CUTOFF, 0, (BLOCK_SIZE - DF_CUTOFF) * sizeof(int));
      data->buffer->docid[id] = tempCurBuffer;
      data->buffer->valueLength[id] = BLOCK_SIZE;
      data->buffer->valuePosition[id] = DF_CUTOFF;
      curBuffer = data->buffer->docid[id];

      if(data->positional == TFONLY || data->positional == POSITIONAL) {
        //expand tfbuffer
        short* tempTfBuffer = (short*) realloc(data->buffer->tf[id], BLOCK_SIZE * sizeof(short));
        memset(tempTfBuffer+DF_CUTOFF+1, 0, (BLOCK_SIZE - DF_CUTOFF - 1) * sizeof(short));
        data->buffer->tf[id] = tempTfBuffer;
      }

      if(data->positional == POSITIONAL) {
        //expand pbuffer
        int origLen = data->buffer->pvalueLength[id];
        int len = 2 * ((origLen / BLOCK_SIZE) + 1) * BLOCK_SIZE;
        short* tempPBuffer = (short*) realloc(data->buffer->position[id], len * sizeof(short));
        memset(tempPBuffer+origLen, 0, (len - origLen) * sizeof(short));
        data->buffer->position[id] = tempPBuffer;
        data->buffer->pvalueLength[id] = len;
      }
    }

    curBuffer[data->buffer->valuePosition[id]++] = docid;
    index->pointers->df->counter[id]++;

    if(data->positional == POSITIONAL) {
      if(data->buffer->valuePosition[id] % BLOCK_SIZE == 0) {
        data->psum->counter[id] = data->buffer->pvaluePosition[id]++;
      }
    }

    if(data->buffer->valuePosition[id] >= data->buffer->valueLength[id]) {
      int nb = data->buffer->valueLength[id] / BLOCK_SIZE;
      long pointer = data->buffer->tailPointer[id];
      if(nb == 1) {
        if(data->positional == TFONLY) {
          pointer = compressAndAddTfOnly(index->pool, curBuffer, data->buffer->tf[id],
                                             BLOCK_SIZE, pointer);
        } else if(data->positional == POSITIONAL) {
          pointer = compressAndAddPositional(index->pool, curBuffer, data->buffer->tf[id],
                                             &data->buffer->position[id][1],
                                             BLOCK_SIZE, data->buffer->position[id][0],
                                             pointer);
        } else {
          pointer = compressAndAddNonPositional(index->pool, curBuffer,
                                                BLOCK_SIZE, pointer);
        }
        if(getStartPointer(index->pointers, id) == UNDEFINED_POINTER) {
          setStartPointer(index->pointers, id, pointer);
        }
      } else {
        int j, ps = 0;
        for(j = 0; j < nb; j++) {
          if(data->positional == TFONLY) {
            pointer = compressAndAddTfOnly(index->pool, &curBuffer[j * BLOCK_SIZE],
                                               &data->buffer->tf[id][j * BLOCK_SIZE],
                                               BLOCK_SIZE, pointer);
          } else if(data->positional == POSITIONAL) {
            pointer = compressAndAddPositional(index->pool, &curBuffer[j * BLOCK_SIZE],
                                               &data->buffer->tf[id][j * BLOCK_SIZE],
                                               &data->buffer->position[id][ps + 1],
                                               BLOCK_SIZE, data->buffer->position[id][ps],
                                               pointer);
            ps += data->buffer->position[id][ps] + 1;
          } else {
            pointer = compressAndAddNonPositional(index->pool, &curBuffer[j * BLOCK_SIZE],
                                                  BLOCK_SIZE, pointer);
          }
          if(getStartPointer(index->pointers, id) == UNDEFINED_POINTER) {
            setStartPointer(index->pointers, id, pointer);
          }
        }
      }
      data->buffer->tailPointer[id] = pointer;

      if((data->buffer->valueLength[id] < data->maxBlocks) && data->expansionEnabled) {
        int newLen = data->buffer->valueLength[id] * EXPANSION_RATE;
        free(data->buffer->docid[id]);
        data->buffer->docid[id] = (int*) malloc(newLen * sizeof(int));
        data->buffer->valueLength[id] = newLen;

        if(data->positional == POSITIONAL || data->positional == TFONLY) {
          free(data->buffer->tf[id]);
          data->buffer->tf[id] = (short*) malloc(newLen * sizeof(short));
        }
      }

      memset(data->buffer->docid[id], 0, data->buffer->valueLength[id] * sizeof(int));

      if(data->positional == POSITIONAL || data->positional == TFONLY) {
        memset(data->buffer->tf[id], 0, data->buffer->valueLength[id] * sizeof(short));
      }
      if(data->positional == POSITIONAL) {
        memset(data->buffer->position[id], 0, data->buffer->pvalueLength[id] * sizeof(short));
        data->buffer->pvaluePosition[id] = 1;
        data->psum->counter[id] = 0;
      }

      data->buffer->valuePosition[id] = 0;
    }
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
  char* outputPath = getValueCL(argc, args, "-index");
  int maxBlocks = atoi(getValueCL(argc, args, "-mb")) * BLOCK_SIZE;
  int positional = NONPOSITIONAL;
  if(isPresentCL(argc, args, "-positional")) {
    positional = POSITIONAL;
  } else if(isPresentCL(argc, args, "-tf")) {
    positional = TFONLY;
  }
  int inputBeginIndex = isPresentCL(argc, args, "-input") + 1;

  InvertedIndex* index = createInvertedIndex();
  IndexingData* data = (IndexingData*) malloc(sizeof(IndexingData));
  data->buffer = createDynamicBuffer(DEFAULT_VOCAB_SIZE, positional);
  if(positional == POSITIONAL) {
    data->psum = createFixedIntCounter(DEFAULT_VOCAB_SIZE, 0);
  } else {
    data->psum = NULL;
  }
  data->uniqueTerms = createIntSet(2048);
  data->expansionEnabled = (maxBlocks > BLOCK_SIZE);
  data->maxBlocks = maxBlocks;
  data->positional = positional;

  int termid = 0;

  unsigned char* oldBuffer = (unsigned char*) calloc(LINE_LENGTH * 2, sizeof(unsigned char));
  unsigned char* iobuffer = (unsigned char*) calloc(LENGTH, sizeof(unsigned char));
  unsigned char* line = (unsigned char*) calloc(LINE_LENGTH, sizeof(unsigned char));
  gzFile * file;

  struct timeval start, end;
  gettimeofday(&start, NULL);

  int fp = 0;
  int len = 0;
  for(fp = inputBeginIndex; fp < argc; fp++) {
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
            termid = process(index, data, oldBuffer, termid);
            memset(oldBuffer, 0, oldBufferIndex);
            oldBufferIndex = 0;
            len = 0;
          } else {
            termid = process(index, data, line, termid);
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
    printf("Files processed: %d Time: %6.0f\n", (fp - inputBeginIndex + 1),
           ((float) (end.tv_sec - start.tv_sec)));
    fflush(stdout);
  }

  unsigned int termsInBuffer = 0;
  int term = -1;
  while((term = nextIndexDynamicBuffer(data->buffer, term, BLOCK_SIZE)) != -1) {
    termsInBuffer++;
    int pos = data->buffer->valuePosition[term];

    if(pos > 0) {
      int nb = pos / BLOCK_SIZE;
      int res = pos % BLOCK_SIZE;
      int ps = 0;

      int* curBuffer = data->buffer->docid[term];
      long pointer = data->buffer->tailPointer[term];
      int j;
      for(j = 0; j < nb; j++) {
        if(positional == TFONLY) {
          pointer =
            compressAndAddTfOnly(index->pool, &curBuffer[j * BLOCK_SIZE],
                                 &data->buffer->tf[term][j * BLOCK_SIZE],
                                 BLOCK_SIZE, pointer);
        } else if(positional == POSITIONAL) {
          pointer =
            compressAndAddPositional(index->pool, &curBuffer[j * BLOCK_SIZE],
                                     &data->buffer->tf[term][j * BLOCK_SIZE],
                                     &data->buffer->position[term][ps + 1],
                                     BLOCK_SIZE, data->buffer->position[term][ps],
                                     pointer);
          ps += data->buffer->position[term][ps] + 1;
        } else {
          pointer =
            compressAndAddNonPositional(index->pool, &curBuffer[j * BLOCK_SIZE],
                                        BLOCK_SIZE, pointer);
        }
        if(getStartPointer(index->pointers, term) == UNDEFINED_POINTER) {
          setStartPointer(index->pointers, term, pointer);
        }
      }

      if(res > 0) {
        if(positional == TFONLY) {
          pointer =
            compressAndAddTfOnly(index->pool, &curBuffer[nb * BLOCK_SIZE],
                                 &data->buffer->tf[term][nb * BLOCK_SIZE],
                                 res, pointer);
        } else if(positional == POSITIONAL) {
          pointer =
            compressAndAddPositional(index->pool, &curBuffer[nb * BLOCK_SIZE],
                                     &data->buffer->tf[term][nb * BLOCK_SIZE],
                                     &data->buffer->position[term][ps + 1],
                                     res, data->buffer->position[term][ps],
                                     pointer);
        } else {
          pointer =
            compressAndAddNonPositional(index->pool, &curBuffer[nb * BLOCK_SIZE],
                                        res, pointer);
        }
        if(getStartPointer(index->pointers, term) == UNDEFINED_POINTER) {
          setStartPointer(index->pointers, term, pointer);
        }
      }
    }
  }

  gettimeofday(&end, NULL);
  printf("Time: %6.0f\n", ((float) (end.tv_sec - start.tv_sec)));
  printf("Terms in buffer: %u\n", termsInBuffer);
  fflush(stdout);

  writeInvertedIndex(index, outputPath);

  destroyInvertedIndex(index);
  destroyIndexingData(data);
  free(oldBuffer);
  free(iobuffer);
  free(line);
  return 0;
}
