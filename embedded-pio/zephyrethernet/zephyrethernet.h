#ifndef ZEPHYRETHERNET_H
#define ZEPHYRETHERNET_H

#include <stddef.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>

typedef enum{
    OK = 0,
    FAILED_TO_SET_CALLBACK = 1,
    FAILED_TO_BIND_SOCKET = 2,
    ETHERNET_READ_ERROR = 3,
    NULL_PACKET_SEEN = 4,
} EthernetErrorCode;

class ZephyrEthernet {
    public:
        ZephyrEthernet() explicit;

        void initEthernetDevice();
        void getPacketQueue();
        void ethFreeMsg(struct eth_packet_msg *msg)
        EthernetErrorCode ethernetStatus() const;

        /**
         * Blocks thread until fifo buffer sees a packet
         */
        net_pkt getNextPacket();

        /**
         * Returns an empty packet immediately if there is no packet in the queue
         * 
         */
        net_pkt getNextPacketImmediate();
        bool packetReadyFifo();
    private:
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