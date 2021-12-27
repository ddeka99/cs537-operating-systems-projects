#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[]) {
  char buf[100];
  int i;
  int x1 = getiocount();
  for (i = 0; i < 1000; i++) {
    (void) read(4, buf, 1);
  }
  int x2 = getiocount();
  for (i = 0; i < 1000; i++) {
    (void) write(4, buf, 1);
  }
  int x3 = getiocount();
  for (i = 0; i < 1000; i++) {
    if (i % 2 == 0) 
      (void) read(4, buf, 1);
    else
      (void) write(4, buf, 1);
  }
  int x4 = getiocount();
  printf(1, "XV6_TEST_OUTPUT %d %d %d\n", x2-x1, x3-x2, x4-x3);
  exit();
}
