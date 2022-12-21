#include <stdio.h>   // perror
#include <stdlib.h>  // exit

void error_handling(const char *message) {
  perror(message);
  exit(1);
}