#ifndef ZEPHYRETHERNET_H
#define ZEPHYRETHERNET_H

#include <stddef.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
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
    PACKET_READ_TIMEOUT = 10,
    INTERFACE_NOT_FOUND = 11,
    FAILED_TO_SET_IP_ADDR = 12,
} EthernetErrorCode;

class ZephyrEthernet {
    public:
        ZephyrEthernet(const char * peerIP);
        ZephyrEthernet();
        ~ZephyrEthernet();

        EthernetErrorCode initEthernetDevice(bool setRemoteDestAddr = true);
        EthernetErrorCode ethernetStatus() const;

        /**
         * Blocks thread until fifo buffer sees a packet
         * 
         */
        EthernetErrorCode getNextPacket(uint8_t * buffer, size_t bufferSize, size_t * bytesRead, k_timeout_t timeout = K_FOREVER);

        /**
         * Returns an empty packet immediately if there is no packet in the queue
         * 
         */
        EthernetErrorCode getNextPacketImmediate(uint8_t * buffer, size_t bufferSize, size_t * bytesRead);
        bool packetReadyFifo() const;
        
    private:
        EthernetErrorCode configureInterface(); 
        void ethFreeMsg(struct EthMsg_t *msg);
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
        
        char _peerIP[NET_IPV4_ADDR_LEN];
        struct net_context * _udpContext;
        EthernetErrorCode _ethernetStatus;
};

#endif
