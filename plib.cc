/* -*-Mode: c++;-*-
   Copyright (c) 1994-2010 John Plevyak, All Rights Reserved
*/
#define EXTERN
#include "plib.h"

int main(int argc, char *argv[]) {
  INIT_RAND64(time(NULL));
  test_stat();
  int p = 8;
  while (p < 32) {
    int n = 1 << p; 
    if (!(n&1)) n--;
    while (1) {
      if (miller_rabin(n)) {
        printf("%d\n", n); 
        break;
      }
      n -= 2;
    }
    p++;
  }
  exit(0);
}
