#include "Headers/headers.h"
#include "time.h"

int main(int agrc, char *argv[]) {

    int runtime = atoi(argv[1]);
    while ((clock() / CLOCKS_PER_SEC) < runtime);

    exit(EXIT_SUCCESS);
}