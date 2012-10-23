#include <stdio.h>
#include "DynamicBuffer.h"

int main() {
  DynamicBuffer* buffer = createDynamicBuffer(2);
  int* out;
  int len;
  int* a = (int*) calloc(10, sizeof(int));
  putDynamicBuffer(&buffer, 10, a, 10);

  int* b = (int*) calloc(20, sizeof(int));
  putDynamicBuffer(&buffer, 2, a, 20);

  int* c = (int*) calloc(20, sizeof(int));
  putDynamicBuffer(&buffer, 3, c, 20);

  int* d = (int*) calloc(20, sizeof(int));
  putDynamicBuffer(&buffer, 4, d, 20);

  int* a2 = (int*) calloc(30, sizeof(int));
  putDynamicBuffer(&buffer, 10, a2, 30);
  len = getDynamicBuffer(buffer, 4, &out);
  out[10] = 11;


  printf("size: %d\n", buffer->size);
  printf("capacity: %d\n", buffer->capacity);
  len = getDynamicBuffer(buffer, 4, &out);
  printf("len: %d\n", len);
  printf("len: %d\n", out[0]);
  printf("len: %d\n", out[10]);

  return 0;
}
