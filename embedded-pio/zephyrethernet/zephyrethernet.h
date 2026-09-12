#ifndef ZEPHYRETHERNET_H
#define ZEPHYRETHERNET_H

#include <stddef.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_pkt.h>
extern "C" {
    #include <zephyr/net/net_if.h>
}
#include <zephyr/net/ethernet.h>

typedef enum{
    ETH_OK = 0,
    FAILED_TO_SET_CALLBACK = 1,
    FAILED_TO_BIND_SOCKET = 2,
    ETHERNET_READ_ERROR = 3,
    NULL_PACKET_SEEN = 4,
    PACKET_PARSING_ERROR = 5,
    PACKET_READ_ERROR = 6,
    FAILED_TO_ALLOCATE_CONTEXT = 7,
    FAILED_TO_BIND_CONTEXT = 8,
    FAILED_TO_BIND_REMOTE_CONTEXT = 9,
} EthernetErrorCode;

class ZephyrEthernet {
    public:
        explicit ZephyrEthernet();

        EthernetErrorCode initEthernetDevice(bool setRemoteDestAddr = true);
        void ethFreeMsg(struct EthMsg_t *msg);
        EthernetErrorCode ethernetStatus() const;

        /**
         * Blocks thread until fifo buffer sees a packet
         * 
         */
        EthernetErrorCode getNextPacket(uint8_t * buffer, size_t bufferSize, k_timeout_t timeout = K_FOREVER);

        /**
         * Returns an empty packet immediately if there is no packet in the queue
         * 
         */
        EthernetErrorCode getNextPacketImmediate(uint8_t * buffer, size_t bufferSize);
        bool packetReadyFifo() const;

        /**
         * Returns the pointer to the queue
         * 
         */
        void getPacketQueue();
        
    private:
        void setUDPContext(struct net_context *& udpContext, bool setRemoteDestAddr);
        static void rxCallbackBridge(struct net_context * context,
                                        struct net_pkt * pkt,
                                        union net_ip_header * ip_hdr,
                                        union net_proto_header * proto_hdr,
                                        int status,
                                        void * user_data);
        void readHandler(struct net_context * context,
                                    struct net_pkt * pkt,
                                    union net_ip_header * ipHeader,
                                    union net_proto_header * protocolHeader,
                                    int status);

        int _sockDescriptor;
        EthernetErrorCode _ethernetStatus;
};

#endif