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


ZephyrCAN::ZephyrCAN(const struct device * canDevice, const uint32_t targetIDList[], size_t targetIDListSize, uint32_t frequency) : 
    _canDevice(canDevice), _canStatus(OK) 
{
    if (device_is_ready(_canDevice))
    {
        size_t i = 0;

        while (_canStatus == OK && i < targetIDListSize) {

            struct can_filter filter = {
                .id = targetIDList[i],
                .mask = CAN_STD_ID_MASK, 
                .flags = 0,
            };

            int response = can_add_rx_filter(_canDevice, rxCallbackBridge, this, &filter);
            
            if(response < 0)
            {
                _canStatus = FAILED_TO_ADD_CAN_CALLBACK;
            }
            else
            {
                filterIDList.push_back(response);
            }

            i++;
        }

        if (_canStatus == OK && can_set_bitrate(canDevice, frequency) < 0) {
            _canStatus = FAILED_TO_SET_CAN_BITRATE;
        }
    }
    else
    {
        _canStatus = DEVICE_NOT_READY;
    }
}

ZephyrCAN::~ZephyrCAN() {
    for (auto const id : filterIDList) {
        can_remove_rx_filter(_canDevice, id);
    }
}

ErrorCode ZephyrCAN::begin() {
    if (_canStatus == OK && can_start(_canDevice) < 0) {
        _canStatus = FAILED_TO_START_CAN;
    }

    return _canStatus;
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

int ZephyrCAN::sendMessage(uint32_t messageID, const uint8_t * data, uint8_t length, int timeout)
{
    struct can_frame frame;


    if (length > sizeof(frame.data))
    {
        length = sizeof(frame.data);
    }

    frame.id = messageID;
    frame.flags = _sendMessageFlag;
    frame.dlc = can_bytes_to_dlc(length);
    memcpy(frame.data, data, length);

    return can_send(_canDevice, &frame, K_MSEC(timeout), nullptr, nullptr);
}
