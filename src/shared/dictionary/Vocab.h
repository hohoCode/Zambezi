#ifndef VOCAB_H_GUARD
#define VOCAB_H_GUARD

#define DEFAULT_VOCAB_SIZE 33554432
#define LONG_CAPACITY 10
#define SHIFT 6
#define MASK 63

#define ENCODE(X) ((X>57) ? X-81l : X-44l)

inline unsigned long encodeInLong(char* chars, unsigned int len, int x) {
  if(x >= len) return 0L;
  int y = (LONG_CAPACITY < len - x) ? LONG_CAPACITY : (len - x);
  unsigned long z0, z1, z2, z3, z4, z5, z6, z7, z8, z9;
  switch(y) {
  case 1:
    return ENCODE(chars[x]);
  case 2:
    return ENCODE(chars[x]) | (ENCODE(chars[x + 1])<<SHIFT);
  case 3:
    z0 = ENCODE(chars[x]);
    z1 = ENCODE(chars[x + 1])<<SHIFT;
    z2 = ENCODE(chars[x + 2])<<(SHIFT*2);
    return (z0 | z1 | z2);
  case 4:
    z0 = ENCODE(chars[x]);
    z1 = ENCODE(chars[x + 1])<<SHIFT;
    z2 = ENCODE(chars[x + 2])<<(SHIFT*2);
    z3 = ENCODE(chars[x + 3])<<(SHIFT*3);
    return (z0 | z1 | z2 | z3);
  case 5:
    z0 = ENCODE(chars[x]);
    z1 = ENCODE(chars[x + 1])<<SHIFT;
    z2 = ENCODE(chars[x + 2])<<(SHIFT*2);
    z3 = ENCODE(chars[x + 3])<<(SHIFT*3);
    z4 = ENCODE(chars[x + 4])<<(SHIFT*4);
    return (z0 | z1 | z2 | z3 | z4);
  case 6:
    z0 = ENCODE(chars[x]);
    z1 = ENCODE(chars[x + 1])<<SHIFT;
    z2 = ENCODE(chars[x + 2])<<(SHIFT*2);
    z3 = ENCODE(chars[x + 3])<<(SHIFT*3);
    z4 = ENCODE(chars[x + 4])<<(SHIFT*4);
    z5 = ENCODE(chars[x + 5])<<(SHIFT*5);
    return (z0 | z1 | z2 | z3 | z4 | z5);
  case 7:
    z0 = ENCODE(chars[x]);
    z1 = ENCODE(chars[x + 1])<<SHIFT;
    z2 = ENCODE(chars[x + 2])<<(SHIFT*2);
    z3 = ENCODE(chars[x + 3])<<(SHIFT*3);
    z4 = ENCODE(chars[x + 4])<<(SHIFT*4);
    z5 = ENCODE(chars[x + 5])<<(SHIFT*5);
    z6 = ENCODE(chars[x + 6])<<(SHIFT*6);
    return (z0 | z1 | z2 | z3 | z4 | z5 | z6);
  case 8:
    z0 = ENCODE(chars[x]);
    z1 = ENCODE(chars[x + 1])<<SHIFT;
    z2 = ENCODE(chars[x + 2])<<(SHIFT*2);
    z3 = ENCODE(chars[x + 3])<<(SHIFT*3);
    z4 = ENCODE(chars[x + 4])<<(SHIFT*4);
    z5 = ENCODE(chars[x + 5])<<(SHIFT*5);
    z6 = ENCODE(chars[x + 6])<<(SHIFT*6);
    z7 = ENCODE(chars[x + 7])<<(SHIFT*7);
    return (z0 | z1 | z2 | z3 | z4 | z5 | z6 | z7);
  case 9:
    z0 = ENCODE(chars[x]);
    z1 = ENCODE(chars[x + 1])<<SHIFT;
    z2 = ENCODE(chars[x + 2])<<(SHIFT*2);
    z3 = ENCODE(chars[x + 3])<<(SHIFT*3);
    z4 = ENCODE(chars[x + 4])<<(SHIFT*4);
    z5 = ENCODE(chars[x + 5])<<(SHIFT*5);
    z6 = ENCODE(chars[x + 6])<<(SHIFT*6);
    z7 = ENCODE(chars[x + 7])<<(SHIFT*7);
    z8 = ENCODE(chars[x + 8])<<(SHIFT*8);
    return (z0 | z1 | z2 | z3 | z4 | z5 | z6 | z7 | z8);
  case 10:
  default:
    z0 = ENCODE(chars[x]);
    z1 = ENCODE(chars[x + 1])<<SHIFT;
    z2 = ENCODE(chars[x + 2])<<(SHIFT*2);
    z3 = ENCODE(chars[x + 3])<<(SHIFT*3);
    z4 = ENCODE(chars[x + 4])<<(SHIFT*4);
    z5 = ENCODE(chars[x + 5])<<(SHIFT*5);
    z6 = ENCODE(chars[x + 6])<<(SHIFT*6);
    z7 = ENCODE(chars[x + 7])<<(SHIFT*7);
    z8 = ENCODE(chars[x + 8])<<(SHIFT*8);
    z9 = ENCODE(chars[x + 9])<<(SHIFT*9);
    return (z0 | z1 | z2 | z3 | z4 | z5 | z6 | z7 | z8 | z9);
  }
}
#endif
