#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[]) {
  int x1 = getiocount();
  int x2 = getiocount();
  char buf[100];
  (void) read(4, buf, 1);
  int x3 = getiocount();
  (void) write(4, buf, 1);  
  int x4 = getiocount();

  printf(1, "XV6_TEST_OUTPUT %d %d %d\n", x2-x1, x3-x2, x4-x3);
  exit();
}
