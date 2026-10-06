#include "updatemanager_eth.h"
#include "zephyrethernet.h"
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <otap.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

LOG_MODULE_REGISTER(ethupd, LOG_LEVEL_INF);
static const int ETHERNET_BUFFER_SIZE = CONFIG_IMG_BLOCK_BUF_SIZE;
static const char PC_IP[] = "192.168.1.2";

static const uint8_t ACK_OK  = 0x00;
static const uint8_t ACK_ERR = 0x01;

typedef enum {
    WAIT_FOR_ETH_UPDATE = 0,
    UPDATING_WITH_ETH = 1,
} update_state_t;

atomic_t ethUpdateFlag = ATOMIC_INIT(0);

void acceptEthernetPayload() {
    atomic_set(&ethUpdateFlag, 1);
}

void stopEthernetPayload() {
    atomic_set(&ethUpdateFlag, 0);
}

void ethernetUpdateTask(void * p1, void * p2, void * p3) {
    update_state_t currState = WAIT_FOR_ETH_UPDATE;
    update_state_t nextState = WAIT_FOR_ETH_UPDATE;

    uint8_t ethBuffer[ETHERNET_BUFFER_SIZE];
    size_t bytesRead;
    uint32_t imageLen = 0;
    size_t remaining = 0;

    ZephyrEthernet ze(PC_IP);

    if (ze.initEthernetDevice(true) != ETH_OK) {
        LOG_ERR("Etherenet init failed: %d", ze.ethernetStatus());
        return;
    }

    if (!boot_is_img_confirmed()) {
        int rc = boot_write_img_confirmed();
        if (rc) {
            LOG_ERR("Image confirm failed: %d", rc);
        } else {
            LOG_INF("Image confirmed - update is now permanent");
        }
    }

    while (1) {
        ze.getNextPacket(ethBuffer, sizeof(ethBuffer), &bytesRead);
        switch(currState) {
            case WAIT_FOR_ETH_UPDATE:
                if (bytesRead >= 6 && ethBuffer[0] == 2 && ethBuffer[1] == 1) {
                    imageLen = sys_get_le32(&ethBuffer[2]);
                    remaining = imageLen;
                    LOG_INF("Saw update request, image %u bytes", (unsigned)imageLen);
                    initSwapping();
                    ze.sendPacket(&ACK_OK, 1);
                    nextState = UPDATING_WITH_ETH;
                }
                break;
            case UPDATING_WITH_ETH: {
                size_t toWrite = MIN(bytesRead, remaining);

                if (writeToBackup(ethBuffer, toWrite, false) < 0) {
                    LOG_ERR("writeToBackup failed at %u bytes", (unsigned)(imageLen - remaining));
                    ze.sendPacket(&ACK_ERR, 1);
                    nextState = WAIT_FOR_ETH_UPDATE;
                    break;
                }

                remaining -= toWrite;
                ze.sendPacket(&ACK_OK, 1);

                if (remaining == 0) {
                    LOG_INF("Image received (%u bytes), swapping", (unsigned)imageLen);
                    writeToBackup(NULL, 0, true);
                    setWriteToBackupDone();
                    k_msleep(1000);
                    sys_reboot(SYS_REBOOT_COLD);
                }
                break;
            }
            default:
                break;
        }

        currState = nextState;
    }
}
