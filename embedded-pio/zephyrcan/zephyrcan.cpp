#include <zephyr/device.h>
#include <zephyr/drivers/can.h>

#include "zephyrcan.h"

#undef USING_CAN_FD
#undef USING_CAN_FD_AND_BITRATE_SWITCHING

#ifdef USING_CAN_FD_AND_BITRATE_SWITCHING
    static const uint8_t _sendMessageFlag = CAN_FRAME_FDF | CAN_FRAME_BRS;
#elif defined(USING_CAN_FD)
    static const uint8_t _sendMessageFlag = CAN_FRAME_FDF;
#else
    static const uint8_t _sendMessageFlag = 0;
#endif


ZephyrCAN::ZephyrCAN(const struct device *canDevice, uint32_t targetIDList[], size_t targetIDListSize) : 
    _canDevice(canDevice), _canStatus(OK) 
{
    size_t i = 0;

    while (_canStatus == OK && i < targetIDListSize) {

        struct can_filter filter = {
            .id = targetIDList[i],
            .mask = CAN_STD_ID_MASK, 
            .flags = 0,
        };

        _canStatus = (can_add_rx_filter(_canDevice, rxCallbackBridge, this, &filter) < 0)? FAILED_TO_ADD_CAN_CALLBACK : OK;
        i++;
    }
}

void ZephyrCAN::rxCallbackBridge(const struct device *dev, struct can_frame *frame, void *user_data)
{
    ZephyrCAN *instance = static_cast<ZephyrCAN*>(user_data);
    if (instance != nullptr) {
        instance->readHandler(frame);
    }
}

ErrorCode ZephyrCAN::canStatus() {
    return _canStatus;
}

int ZephyrCAN::sendMessage(uint32_t messageID, const uint8_t * data, int length, int timeout)
{
    struct can_frame frame;
    frame.id = messageID;
    frame.flags = CAN_FRAME_FDF | CAN_FRAME_BRS;
    frame.dlc = can_bytes_to_dlc(length);
    memcpy(frame.data, data, length);

    return can_send(_canDevice, &frame, K_MSEC(timeout), nullptr, nullptr);
}
