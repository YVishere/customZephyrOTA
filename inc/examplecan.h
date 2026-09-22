#ifndef EXAMPLECAN_H
#define EXAMPLECAN_H

#include "zephyrcan.h"

class ExampleCAN : public ZephyrCAN {
    public:
        ExampleCAN(const struct device *canDevice, const uint32_t targetIDList[], size_t targetIDListSize, uint32_t frequency);
        void readHandler(struct can_frame * msg);
        int sendExampleData();
};

#endif
