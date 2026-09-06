#ifndef ZEPHYRCAN_H
#define ZEPHYRCAN_H

#include <stdint.h>
#include <stddef.h>

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>

typedef enum {
    OK = 0,
    FAILED_TO_ADD_CAN_CALLBACK = 1,
    FAILED_TO_START_CAN = 2,
    FAILED_TO_SET_CAN_BITRATE = 3,
} ErrorCode;

class ZephyrCAN {
    public:
        ZephyrCAN(const struct device *canDevice, const uint32_t targetIDList[], size_t targetIDListSize, uint32_t frequency);
        static void rxCallbackBridge(const struct device *dev, struct can_frame *frame, void *user_data);

        virtual void readHandler(struct can_frame * msg) = 0;
        int sendMessage(uint32_t messageID, const uint8_t * data, int length, int timeout = 10);
        
        ErrorCode canStatus();
        ErrorCode begin();

    private:
        const struct device *const _canDevice;
        ErrorCode _canStatus;
};

#endif
