#include "examplecan.h"
#include <zephyr/logging/log.h>
#include "const.h"

LOG_MODULE_REGISTER(can_rx_example, LOG_LEVEL_INF);

ExampleCAN::ExampleCAN(const struct device * canDevice, const uint32_t targetIDList[], size_t targetIDListSize, uint32_t frequency) : 
    ZephyrCAN(canDevice, targetIDList, targetIDListSize, frequency) {}

void ExampleCAN::readHandler(struct can_frame * msg) {
    if ((msg->id) == EXAMPLE_ID) {
        uint8_t payload_len = can_dlc_to_bytes(msg->dlc);

        LOG_INF("Received CAN Frame! ID: 0x%X, Length: %d bytes", msg->id, payload_len);

        for (int i = 0; i < payload_len; i++) {
            uint8_t byte_data = msg->data[i];
            LOG_INF("Payload byte[%d]: 0x%02X", i, byte_data);
        }
    }
}

int ExampleCAN::sendExampleData() {
    return 0;
}
