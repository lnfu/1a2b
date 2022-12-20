#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


bool game(char *ans, char *guess, int *a, int *b) {
  (*a) = (*b) = 0;
  printf("ANS: %s\n", ans);
  printf("GUESS: %s\n", guess);

  bool recorded_i[4] = {0};
  bool recorded_j[4] = {0};

  for (int i = 0; i < 4; i++) {
    if (ans[i] == guess[i]) {
      recorded_i[i] = true;
      recorded_j[i] = true;
      (*a)++;
    }
  }
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (recorded_i[i]) continue;
      if (recorded_j[j]) continue;
      if (ans[i] == guess[j]) {
        printf("i = %d\tj = %d\n", i, j);
        recorded_i[i] = true;
        recorded_j[j] = true;
        (*b)++;
      }
    }
  }



  printf("%dA%dB\n", *a, *b);
  return *a == 4;  // if a == 4 then bingo
}


void generate_answer(char *ans) {
  srand(time(NULL));
  char a[2];
  for (int i = 0; i < 4; i++) {
    sprintf(a, "%d", rand() % 10);
    ans[i] = *a;
  }
}