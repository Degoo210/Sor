#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(int argc, char *argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 10;
    for (int i = 0; i < n; i++) {
        printf("%d\n", i);
        sleep(1);
        fflush(stdout);
    }
    return 0;
}