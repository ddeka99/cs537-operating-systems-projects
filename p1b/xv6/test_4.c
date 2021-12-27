#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[]) {
    int num_proc = 20;
    int x1 = getiocount();

    int i, j, rc;
    
    for (i = 0; i < num_proc; i++) {
        rc = fork();
        if (rc == 0) {
            for (j = 0; j < 10000; j++) {
                char buf[100];
                if (j % 2 == 0) 
                    (void) read(4, buf, 1);
                else
                    (void) write(4, buf, 1);
            }
            exit();
        } else {
            wait();
        }
    }

   // https://wiki.osdev.org/Shutdown
    // (void) shutdown();
    int x2 = getiocount();
    printf(1, "XV6_TEST_OUTPUT %d\n", x2-x1);

    exit();
}
