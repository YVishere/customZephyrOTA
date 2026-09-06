#include "examplecan.h"

ExampleCAN::ExampleCAN(const struct device * canDevice, uint32_t targetIDList[], size_t targetIDListSize) : 
    ZephyrCAN(canDevice, targetIDList, targetIDListSize) {};

void ExampleCAN::readHandler(struct can_frame * msg) {
    
}

int ExampleCAN::sendExampleData()
{
    return 0;
}
