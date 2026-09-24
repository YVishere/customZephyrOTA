#include "updatemanager_eth.h"
#include "zephyrethernet.h"
#include <zephyr/sys/atomic.h>
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
    update_state_t currState = WAIT_FOR_ETH_UPDATE;
    update_state_t nextState = WAIT_FOR_ETH_UPDATE;

    uint8_t ethBuffer[ETHERNET_BUFFER_SIZE];
    size_t bufferSize = 0;
    size_t bytesRead;

    ZephyrEthernet ze(PC_IP);

    if (ze.initEthernetDevice(true) != ETH_OK) {   // connect to PC -> can send ACKs
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
                if (ethBuffer[0] == 2 && ethBuffer[1] == 1) {
                    LOG_INF("Saw start ........\n");
                    bufferSize = 0;
                    initSwapping();
                    ze.sendPacket(&ACK_OK, 1);
                    memset(ethBuffer, 0, sizeof(ethBuffer));
                    nextState = UPDATING_WITH_ETH;
                }
                break;
            case UPDATING_WITH_ETH:
                if (ethBuffer[0] == 1 && ethBuffer[1] == 2) {
                    LOG_INF("Saw end ........\n");
                    writeToBackup(NULL, 0, true);
                    setWriteToBackupDone();
                    k_msleep(1000);
                    sys_reboot(SYS_REBOOT_COLD);
                    break;
                }

                if (writeToBackup(ethBuffer, bytesRead, false) < 0) {
                    LOG_ERR("writeToBackup failed at %u bytes", (unsigned)bufferSize);
                    ze.sendPacket(&ACK_ERR, 1);
                    nextState = WAIT_FOR_ETH_UPDATE;
                } else {
                    bufferSize += bytesRead;
                    ze.sendPacket(&ACK_OK, 1);
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
