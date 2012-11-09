#include <stdlib.h>
#include <zlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "pfordelta/opt_p4.h"
#include "dictionary/hashtable.h"
#include "buffer/FixedBuffer.h"
#include "buffer/DynamicBuffer.h"
#include "buffer/FixedIntCounter.h"
#include "buffer/FixedLongCounter.h"
#include "buffer/IntSet.h"
#include "PostingsPool.h"

#define LENGTH 32*1024
#define LINE_LENGTH 0x100000
#define DF_CUTOFF 9
#define POS_PER_DOCID 10
#define EXPANSION_RATE 2
#define NUMBER_OF_POOLS 6
#define INDEX_FILE "index"
#define POINTER_FILE "pointers"
#define DICTIONARY_FILE "dictionary"
#define DEFAULT_VOCAB_SIZE 33554432

typedef struct IndexingData IndexingData;
struct IndexingData {
  Dictionary** dic;
  DynamicBuffer* buffer;
  DynamicBuffer* tfbuffer;
  DynamicBuffer* pbuffer;
  FixedLongCounter* startPointers;
  FixedIntCounter* df;
  FixedIntCounter* psum;
  IntSet* uniqueTerms;
  int expansionEnabled;
  int maxBlocks;
};

void destroyIndexingData(IndexingData* data) {
  destroyhashtable(data->dic);
  destroyDynamicBuffer(data->buffer);
  destroyDynamicBuffer(data->tfbuffer);
  destroyDynamicBuffer(data->pbuffer);
  destroyFixedLongCounter(data->startPointers);
  destroyFixedIntCounter(data->df);
  destroyFixedIntCounter(data->psum);
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

int process(PostingsPool* pool, IndexingData* data, char* line, int termid) {
  int docid = 0, consumed;
  grabword(line, '\t', &consumed);
  docid = atoi(line);
  line += consumed;

  int position = 1;
  clearIntSet(data->uniqueTerms);
  grabword(line, ' ', &consumed);
  while(consumed > 0) {
    int id = hashinsert(data->dic, line, termid);
    int added = addIntSet(&data->uniqueTerms, id);
    if(id == termid) {
      termid++;
    }

    int* curtfBuffer = getDynamicBuffer(data->tfbuffer, id);
    int* curBuffer = getDynamicBuffer(data->pbuffer, id);
    int ps = getFixedIntCounter(data->psum, id);
    if(!curBuffer) {
      curBuffer = (int*) calloc(DF_CUTOFF * POS_PER_DOCID, sizeof(int));
      putDynamicBuffer(data->pbuffer, id, curBuffer, DF_CUTOFF * POS_PER_DOCID);
      data->pbuffer->valuePosition[id] = 1;

      curtfBuffer = (int*) calloc(DF_CUTOFF, sizeof(int));
      putDynamicBuffer(data->tfbuffer, id, curtfBuffer, DF_CUTOFF);
    }
    if(data->pbuffer->valueLength[id] <= data->pbuffer->valuePosition[id] + 1) {
      int len = data->pbuffer->valueLength[id];
      int* tempCurBuffer = (int*) realloc(curBuffer, len * 2 * sizeof(int));
      memset(tempCurBuffer+len, 0, len * sizeof(int));
      data->pbuffer->value[id] = tempCurBuffer;
      data->pbuffer->valueLength[id] = len * 2;
      curBuffer = tempCurBuffer;
    }

    int pbufferpos = data->pbuffer->valuePosition[id];
    if(!added) {
      curBuffer[pbufferpos] = position - curBuffer[pbufferpos];
      pbufferpos++;
    } else {
      curBuffer[pbufferpos++] = position;
    }
    curBuffer[pbufferpos] = position;

    data->pbuffer->valuePosition[id]++;
    data->pbuffer->value[id][ps]++;
    curtfbuffer[data->tfbuffer->valuePosition[id]]++;

    position++;
    line += consumed;
    grabword(line, ' ', &consumed);
  }

  int keyPos = -1;
  while((keyPos = nextIndexIntSet(data->uniqueTerms, keyPos)) != -1) {
    int id = data->uniqueTerms->key[keyPos];

    data->pbuffer->value[id][data->pbuffer->valuePosition[id]] = 0;
    data->tfbuffer->valuePosition[id]++;

    int df = getFixedIntCounter(data->df, id);
    if(df < DF_CUTOFF) {
      int* curBuffer = getDynamicBuffer(data->buffer, id);
      if(!curBuffer) {
        curBuffer = (int*) calloc(DF_CUTOFF, sizeof(int));
        putDynamicBuffer(data->buffer, id, curBuffer, DF_CUTOFF);
      }
      data->buffer->value[id][df] = docid;
      data->df->counter[id]++;
      continue;
    }

    int* curBuffer = data->buffer->value[id];
    if(data->buffer->valueLength[id] < BLOCK_SIZE) {
      int* tempCurBuffer = (int*) realloc(curBuffer, BLOCK_SIZE * sizeof(int));
      memset(tempCurBuffer+DF_CUTOFF, 0, (BLOCK_SIZE - DF_CUTOFF) * sizeof(int));
      curBuffer = tempCurBuffer;
      data->buffer->value[id] = tempCurBuffer;
      data->buffer->valueLength[id] = BLOCK_SIZE;
      data->buffer->valuePosition[id] = DF_CUTOFF;

      //expand tfbuffer
      int* tempTfBuffer= (int*) realloc(data->tfbuffer->value[id], BLOCK_SIZE * sizeof(int));
      memset(tempTfBuffer+DF_CUTOFF, 0, (BLOCK_SIZE - DF_CUTOFF) * sizeof(int));
      data->tfbuffer->value[id] = tempTfBuffer;
    }

    curBuffer[data->buffer->valuePosition[id]++] = docid;
    data->df->counter[id]++;

    if(data->buffer->valuePosition[id] % BLOCK_SIZE == 0) {
      data->psum->counter[id] += data->pbuffer->valuePosition[id]++;
    }

    if(data->buffer->valuePosition[id] >= data->buffer->valueLength[id]) {
      int nb = data->buffer->valueLength[id] / BLOCK_SIZE;
      long pointer = data->buffer->tailPointer[id];
      if(nb == 1) {
        pointer = compressAndAdd(pool, curBuffer, data->tfbuffer->value[id],
                                 &data->pbuffer->value[id][1],
                                 BLOCK_SIZE, data->pbuffer->value[id][0],
                                 pointer);

        if(getFixedLongCounter(data->startPointers, id) == UNDEFINED_POINTER) {
          setFixedLongCounter(data->startPointers, id, pointer);
        }
      } else {
        int j, ps = 0;
        for(j = 0; j < nb; j++) {
          pointer = compressAndAdd(pool, &curBuffer[j * BLOCK_SIZE],
                                   &data->tfbuffer->value[id][j * BLOCK_SIZE],
                                   &data->pbuffer->value[id][ps + 1],
                                   BLOCK_SIZE, data->pbuffer->value[id][ps],
                                   pointer);
          ps += data->pbuffer->value[id][ps] + 1;
          if(getFixedLongCounter(data->startPointers, id) == UNDEFINED_POINTER) {
            setFixedLongCounter(data->startPointers, id, pointer);
          }
        }
      }
      data->buffer->tailPointer[id] = pointer;

      if((data->buffer->valueLength[id] < data->maxBlocks) && data->expansionEnabled) {
        int newLen = data->buffer->valueLength[id] * EXPANSION_RATE;
        int* newBuffer = (int*) realloc(curBuffer, newLen, sizeof(int));
        curBuffer = newBuffer;
        data->buffer->value[id] = curBuffer;
        data->buffer->valueLength[id] = newLen;

        int* newTfBuffer = (int*) realloc(data->tfbuffer->value[id], newLen, sizeof(int));
        memset(newTfBuffer, 0, newLen * sizeof(int));
        data->tfbuffer->value[id] = newTfBuffer;
        data->tfbuffer->valueLength[id] = newLen;
      }

      memset(curBuffer, 0, data->buffer->valueLength[id] * sizeof(int));
      data->buffer->valuePosition[id] = 0;
      data->tfbuffer->valuePosition[id] = 0;
      data->pbuffer->valuePosition[id] = 1;
      data->psum->counter[id] = 0;
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
  char* outputPath = args[1];
  int maxBlocks = atoi(args[2]) * BLOCK_SIZE;
  int contiguous = atoi(args[3]);

  IndexingData* data = (IndexingData*) malloc(sizeof(IndexingData));
  data->buffer = createDynamicBuffer(DEFAULT_VOCAB_SIZE);
  data->tfbuffer = createDynamicBuffer(DEFAULT_VOCAB_SIZE);
  data->pbuffer = createDynamicBuffer(DEFAULT_VOCAB_SIZE);
  data->dic = inithashtable();
  data->df = createFixedIntCounter(DEFAULT_VOCAB_SIZE, 0);
  data->psum = createFixedIntCounter(DEFAULT_VOCAB_SIZE, 0);
  data->startPointers = createFixedLongCounter(DEFAULT_VOCAB_SIZE, UNDEFINED_POINTER);
  data->uniqueTerms = createIntSet(2048);
  data->expansionEnabled = (maxBlocks > BLOCK_SIZE);
  data->maxBlocks = maxBlocks;
  PostingsPool* pool = createPostingsPool(NUMBER_OF_POOLS);

  int termid = 0;

  unsigned char* oldBuffer = (unsigned char*) calloc(LINE_LENGTH * 2, sizeof(unsigned char));
  unsigned char* iobuffer = (unsigned char*) calloc(LENGTH, sizeof(unsigned char));
  unsigned char* line = (unsigned char*) calloc(LINE_LENGTH, sizeof(unsigned char));
  gzFile * file;

  struct timeval start, end;
  gettimeofday(&start, NULL);

  int fp = 0;
  int len = 0;
  for(fp = 4; fp < argc; fp++) {
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
            termid = process(pool, data, oldBuffer, termid);
            memset(oldBuffer, 0, oldBufferIndex);
            oldBufferIndex = 0;
            len = 0;
          } else {
            termid = process(pool, data, line, termid);
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
    printf("Files processed: %d Time: %6.0f\n", (fp - 3), ((float) (end.tv_sec - start.tv_sec)));
    fflush(stdout);
  }

  int term = -1;
  while((term = nextIndexDynamicBuffer(data->buffer, term, BLOCK_SIZE)) != -1) {
    int pos = data->buffer->valuePosition[term];

    if(pos > 0) {
      int nb = pos / BLOCK_SIZE;
      int res = pos % BLOCK_SIZE;

      int* curBuffer = data->buffer->value[term];
      long pointer = data->buffer->tailPointer[term];
      int j;
      for(j = 0; j < nb; j++) {
        pointer = compressAndAdd(pool, &curBuffer[j * BLOCK_SIZE],
                                 BLOCK_SIZE, pointer);
        if(getFixedLongCounter(data->startPointers, term) == UNDEFINED_POINTER) {
          setFixedLongCounter(data->startPointers, term, pointer);
        }
      }

      if(res > 0) {
        pointer = compressAndAdd(pool, &curBuffer[nb * BLOCK_SIZE],
                                 res, pointer);
        if(getFixedLongCounter(data->startPointers, term) == UNDEFINED_POINTER) {
          setFixedLongCounter(data->startPointers, term, pointer);
        }
      }
    }
  }

  PostingsPool* contiguousPool;
  FixedLongCounter* contiguousStartPointers;
  if(contiguous) {
    contiguousPool = createPostingsPool(NUMBER_OF_POOLS);
    contiguousStartPointers = createFixedLongCounter(DEFAULT_VOCAB_SIZE,
                                                     UNDEFINED_POINTER);
    unsigned int* outBlock = (unsigned int*) calloc(BLOCK_SIZE*2, sizeof(unsigned int));
    term = -1;
    while((term = nextIndexFixedLongCounter(data->startPointers, term)) != -1) {
      long tailPointer = UNDEFINED_POINTER;
      long pointer = data->startPointers->counter[term];
      while(pointer != UNDEFINED_POINTER) {
        memset(outBlock, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
        int num = decompressDocidBlock(pool, outBlock, pointer);
        tailPointer = compressAndAdd(contiguousPool, outBlock, num, tailPointer);
        pointer = nextPointer(pool, pointer);
        if(getFixedLongCounter(contiguousStartPointers, term) == UNDEFINED_POINTER) {
          setFixedLongCounter(contiguousStartPointers, term, tailPointer);
        }
      }
    }
    free(outBlock);
  }

  gettimeofday(&end, NULL);
  printf("Time: %6.0f\n", ((float) (end.tv_sec - start.tv_sec)));
  printf("Terms in buffer: %u\n", data->buffer->size);
  fflush(stdout);

  char dicPath[1024];
  strcpy(dicPath, outputPath);
  strcat(dicPath, "/");
  strcat(dicPath, DICTIONARY_FILE);

  FILE* ofp = fopen(dicPath, "wb");
  writehashtable(data->dic, ofp);
  fclose(ofp);

  char indexPath[1024];
  strcpy(indexPath, outputPath);
  strcat(indexPath, "/");
  strcat(indexPath, INDEX_FILE);

  ofp = fopen(indexPath, "wb");
  if(contiguous) {
    writePostingsPool(contiguousPool, ofp);
  } else {
    writePostingsPool(pool, ofp);
  }
  fclose(ofp);

  char pointerPath[1024];
  strcpy(pointerPath, outputPath);
  strcat(pointerPath, "/");
  strcat(pointerPath, POINTER_FILE);

  ofp = fopen(pointerPath, "wb");
  if(contiguous) {
    int size = sizeFixedLongCounter(contiguousStartPointers);
    fwrite(&size, sizeof(unsigned int), 1, ofp);
    term = -1;
    while((term = nextIndexFixedLongCounter(contiguousStartPointers, term)) != -1) {
      fwrite(&term, sizeof(int), 1, ofp);
      fwrite(&data->df->counter[term], sizeof(int), 1, ofp);
      fwrite(&contiguousStartPointers->counter[term], sizeof(long), 1, ofp);
    }
  } else {
    int size = sizeFixedLongCounter(data->startPointers);
    fwrite(&size, sizeof(unsigned int), 1, ofp);
    term = -1;
    while((term = nextIndexFixedLongCounter(data->startPointers, term)) != -1) {
      fwrite(&term, sizeof(int), 1, ofp);
      fwrite(&data->df->counter[term], sizeof(int), 1, ofp);
      fwrite(&data->startPointers->counter[term], sizeof(long), 1, ofp);
    }
  }
  fclose(ofp);

  destroyPostingsPool(pool);
  destroyIndexingData(data);
  if(contiguous) {
    destroyPostingsPool(contiguousPool);
    destroyFixedLongCounter(contiguousStartPointers);
  }

  free(oldBuffer);
  free(iobuffer);
  free(line);
  return 0;
}
