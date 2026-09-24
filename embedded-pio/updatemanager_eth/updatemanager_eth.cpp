#include "updatemanager_eth.h"
#include "zephyrethernet.h"
#include <zephyr/sys/atomic.h>
#include <otap.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

LOG_MODULE_REGISTER(ethupd, LOG_LEVEL_INF);
static const int ETHERNET_BUFFER_SIZE = CONFIG_IMG_BLOCK_BUF_SIZE;

typedef enum {
    WAIT_FOR_ETH_UPDATE = 0,
    UPDATING_WITH_ETH = 1,
    ETH_UPDATE_DONE = 2,
} update_state_t;

atomic_t ethUpdateFlag = ATOMIC_INIT(0);

void acceptEthernetPayload() {
    atomic_set(&ethUpdateFlag, 1);
}

void stopEthernetPayload() {
    atomic_set(&ethUpdateFlag, 0);
}

void ethernetUpdateTask(void * p1, void * p2, void * p3) {
    initSwapping();
    update_state_t currState = WAIT_FOR_ETH_UPDATE;
    update_state_t nextState = WAIT_FOR_ETH_UPDATE;

    uint8_t ethBuffer[ETHERNET_BUFFER_SIZE];
    size_t bufferSize = 0;
    size_t bytesRead;

    ZephyrEthernet ze;

    if (ze.initEthernetDevice() != ETH_OK) {
        LOG_ERR("Etherenet init failed: %d", ze.ethernetStatus());
        return;
    }

    while (1) {
        ze.getNextPacket(ethBuffer, sizeof(ethBuffer), &bytesRead);
        switch(currState) {
            case WAIT_FOR_ETH_UPDATE:
                if (ethBuffer[0] == 2 && ethBuffer[1] == 1) {
                    memset(ethBuffer, 0, sizeof(ethBuffer));
                    nextState = UPDATING_WITH_ETH;
                }
                break;
            case UPDATING_WITH_ETH:
                writeToBackup(ethBuffer, bytesRead, false);
                bufferSize += bytesRead;

                if (ethBuffer[0] == 1 && ethBuffer[1] == 2) {
                    memset(ethBuffer, 0, sizeof(ethBuffer));
                    nextState = ETH_UPDATE_DONE;
                }
                break;
            case ETH_UPDATE_DONE:
                writeToBackup(NULL, 0, true);
                setWriteToBackupDone();
                k_msleep(1000);
                sys_reboot(SYS_REBOOT_COLD);
                break;
            default:
                break;
        }

        currState = nextState;
    }
}
