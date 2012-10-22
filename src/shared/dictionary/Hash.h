#ifndef HASH_H_GUARD
#define HASH_H_GUARD

inline unsigned long hash64shift(unsigned long key) {
  key = (~key) + (key << 21);
  key = key ^ (key >> 24);
  key = (key + (key << 3)) + (key << 8);
  key = key ^ (key >> 14);
  key = (key + (key << 2)) + (key << 4);
  key = key ^ (key >> 28);
  key = key + (key << 31);
  return key;
}

unsigned long jenkinsHash(unsigned long a) {
  a = (a+0x7ed55d16l) + (a<<12);
  a = (a^0xc761c23cl) ^ (a>>19);
  a = (a+0x165667b1l) + (a<<5);
  a = (a+0xd3a2646cl) ^ (a<<9);
  a = (a+0xfd7046c5l) + (a<<3);
  return (a^0xb55a4f09l) ^ (a>>16);
}

inline unsigned long murmurHash3(unsigned long x ) {
  x ^= x >> 33;
  x *= 0xff51afd7ed558ccdL;
  x ^= x >> 33;
  x *= 0xc4ceb9fe1a85ec53L;
  x ^= x >> 33;
  return x;
}

#endif
