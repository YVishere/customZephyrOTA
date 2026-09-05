#include <stdio.h>
#include "const.h"
#include "zephyrcan.h"

int main(void) {
    printf("Hello World\n", CONFIG_BOARD_TARGET);
    return 0;
}
