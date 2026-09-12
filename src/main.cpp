#include <stdio.h>
#include <zephyr/logging/log.h>
#include <zephyr/dfu/flash_img.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/dfu/mcuboot.h>
#include "const.h"
#include "examplecan.h"
#include "zephyrethernet.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void) {
    LOG_INF("Hello World %s\n", CONFIG_BOARD_TARGET);
    ExampleCAN canBus(canDev, CAN_IDS, sizeof(CAN_IDS)/sizeof(CAN_IDS[0]), CAN_FREQUENCY);
    canBus.begin();

    if (canBus.canStatus() != CAN_OK) {
        LOG_ERR("CAN init failed: %d", canBus.canStatus());
        return -1;
    }

    ZephyrEthernet ze;

    LOG_INF("STatus %d", ze.ethernetStatus());
    
    while(1) {
        if (canBus.canStatus() != CAN_OK)
        {
            break;
        }
        k_msleep(1000); 
    }
    
    return 0;
}
