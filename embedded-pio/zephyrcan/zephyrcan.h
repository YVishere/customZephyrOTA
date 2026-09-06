#ifndef ZEPHYRCAN_H
#define ZEPHYRCAN_H

#include <stdint.h>
#include <stddef.h>
#include <vector>

using namespace std;

typedef enum {
    OK = 0,
    FAILED_TO_ADD_CAN_CALLBACK = 1,
    DEVICE_NOT_READY = 2,
    CAN_FAILED_TO_START = 3
} ErrorCode;

class ZephyrCAN {
    public:
        ZephyrCAN(const struct device *canDevice, uint32_t targetIDList[], size_t targetIDListSize);
        ~ZephyrCAN();
        
        static void rxCallbackBridge(const struct device *dev, struct can_frame *frame, void *user_data);

        virtual void readHandler(struct can_frame * msg) = 0;
        int sendMessage(uint32_t messageID, const uint8_t * data, uint8_t length, int timeout = 10);
        
        ErrorCode canStatus();

    private:
        const struct device *const _canDevice;
        ErrorCode _canStatus;
        vector<int> filterIDList;

};

#endif
