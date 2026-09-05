#ifndef ZEPHYRCAN_H
#define ZEPHYRCAN_H

class ZephyrCAN {
    public:
        ZephyrCAN();

        virtual void readHandler(___ msg) = 0;
        bool sendMessage(int messageID, void* data, int length, int timeout = 10);
        void runQueue(int duration);
    
    private:
        
};

#endif
