#include <stdio.h>
#include <zephyr/logging/log.h>
#include "const.h"
#include "examplecan.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void) {
    LOG_INF("Hello World %s\n", CONFIG_BOARD_TARGET);
    ExampleCAN canBus(canDev, CAN_IDS, sizeof(CAN_IDS)/sizeof(CAN_IDS[0]), CAN_FREQUENCY);
    canBus.begin();

    if (canBus.canStatus() != OK) {
        LOG_ERR("CAN init failed: %d", canBus.canStatus());
        return -1;
    }
    
    while(1) {
        if (canBus.canStatus() != OK)
        {
            break;
        }
        k_msleep(1000); 
    }
    
    return 0;
}
