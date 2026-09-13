#include <stdio.h>
#include <zephyr/logging/log.h>
#include <zephyr/dfu/flash_img.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/dfu/mcuboot.h>
#include "const.h"
#include "OTAP.h"
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

    if (ze.initEthernetDevice() != ETH_OK) {
        LOG_ERR("Etherenet init failed: %d", ze.ethernetStatus());
        return -1;
    }

    uint8_t rxBuf[256];
    size_t rxLen;

    LOG_INF("Status %d", ze.ethernetStatus());
    
    while(1) {
        // if (canBus.canStatus() != CAN_OK)
        // {
        //     break;
        // }

        EthernetErrorCode rc = ze.getNextPacket(rxBuf, sizeof(rxBuf), &rxLen, K_MSEC(1000));

        if (rc == ETH_OK) {
            LOG_HEXDUMP_INF(rxBuf, rxLen, "UDP payload");
        } else if (rc != PACKET_READ_TIMEOUT) {
            LOG_WRN("Ethernet read error: %d", rc);
        }
    }
    
    return 0;
}
