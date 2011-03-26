/* -*-Mode: c++;-*-
   Copyright (c) 1994-2010 John Plevyak, All Rights Reserved
*/
#define EXTERN
#include "plib.h"

int main(int argc, char *argv[]) {
  INIT_RAND64(time(NULL));
  test_stat();
  test_list();
  test_vec();
  test_map();
  exit(0);
}
