/* Author J. Zobel, April 2001.
   Permission to use this code is freely granted, provided that this
   statement is retained. */

int scmp(char *s1, char *s2) {
  while( *s1 != '\0' && *s1 == *s2 ) {
    s1++;
    s2++;
  }
  return (*s1-*s2);
}
