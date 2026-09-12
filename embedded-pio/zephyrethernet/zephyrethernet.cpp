#include "zephyrethernet.h"

static const int ALIGN_BYTES = 4;
static const int MAX_QUEUE_SIZE = 10;
static const int ETHERNET_HEADER_LEN = 14;
static const int LOCAL_PORT = 12345;
static const int REMOTE_PORT = 12345;
static constexpr char MCU_IP_ADDR[] = "192.168.1.67";

struct EthMsg_t {
    void * fifo_reserved;   
    struct net_pkt * pkt;   
};

K_FIFO_DEFINE(ethPacketFifo);
K_MEM_SLAB_DEFINE(msgSlab, sizeof(struct EthMsg_t), MAX_QUEUE_SIZE, ALIGN_BYTES);

ZephyrEthernet::ZephyrEthernet() : _ethernetStatus(ETH_OK) {}

EthernetErrorCode ZephyrEthernet::initEthernetDevice(bool setRemoteDestAddr){
    net_context * context;
    setUDPContext(context, setRemoteDestAddr);

    if (_ethernetStatus == ETH_OK && net_context_recv(context, rxCallbackBridge, K_NO_WAIT, this) < 0) {
        _ethernetStatus = FAILED_TO_SET_CALLBACK;
        return _ethernetStatus;
    }

    return _ethernetStatus;
}

EthernetErrorCode ZephyrEthernet::getNextPacket(uint8_t * buffer, size_t bufferSize, k_timeout_t timeout) {
    struct EthMsg_t * ethMsg = (struct EthMsg_t *) k_fifo_get(&ethPacketFifo, timeout);

    if (_ethernetStatus == PACKET_PARSING_ERROR || _ethernetStatus == PACKET_READ_ERROR) {
        _ethernetStatus = ETH_OK;
    }

    if ((_ethernetStatus == ETH_OK) && ethMsg && ethMsg->pkt) {
        net_pkt * pkt = ethMsg->pkt;
        size_t frameLen = net_pkt_get_len(pkt);
        net_pkt_cursor_init(pkt);

        if (net_pkt_skip(pkt, ETHERNET_HEADER_LEN) != 0) {
            _ethernetStatus = PACKET_PARSING_ERROR;
        }
        else {
            size_t bytesToRead = MIN(frameLen - ETHERNET_HEADER_LEN, bufferSize);
            
            if (net_pkt_read(pkt, buffer, bytesToRead) != 0) {
                _ethernetStatus = PACKET_READ_ERROR;
            }
        }
    }

    return _ethernetStatus;
}

void ZephyrEthernet::rxCallbackBridge(struct net_context * context,
                                        struct net_pkt * pkt,
                                        union net_ip_header * ipHeader,
                                        union net_proto_header * protocolHeader,
                                        int status,
                                        void * user_data)
{
    ZephyrEthernet * instance = static_cast<ZephyrEthernet*>(user_data);
    
    if (status == 0) {
        if (instance != nullptr) {
            instance->readHandler(context, pkt, ipHeader, protocolHeader, status);
        }
    }
}

void ZephyrEthernet::readHandler(struct net_context * context,
                                    struct net_pkt * pkt,
                                    union net_ip_header * ipHeader,
                                    union net_proto_header * protocolHeader,
                                    int status) 
{
    if ((_ethernetStatus == NULL_PACKET_SEEN) && pkt) {
        _ethernetStatus = ETH_OK;
    }
    else
    {
        return;
    }

    if (_ethernetStatus == ETH_OK)
    {
        if (pkt) {
            struct EthMsg_t * msgWrapper;

            if (k_mem_slab_alloc(&msgSlab, (void **)&msgWrapper, K_NO_WAIT) != 0) {
                return; 
            }

            net_pkt_ref(pkt);
            msgWrapper->pkt = pkt;
            k_fifo_put(&ethPacketFifo, msgWrapper);
        }
        else {
            _ethernetStatus = NULL_PACKET_SEEN;
            return;
        }
    }
}

EthernetErrorCode ZephyrEthernet::ethernetStatus() const {
    return _ethernetStatus;
}

void ZephyrEthernet::ethFreeMsg(struct EthMsg_t *msg)
{
    if (msg) {
        k_mem_slab_free(&msgSlab, (void *)msg);
    }
}

bool ZephyrEthernet::packetReadyFifo() const {
    return !k_fifo_is_empty(&ethPacketFifo);
}

EthernetErrorCode ZephyrEthernet::getNextPacketImmediate(uint8_t * buffer, size_t bufferSize) {
    return getNextPacket(buffer, bufferSize, K_NO_WAIT);
}

void ZephyrEthernet::setUDPContext(struct net_context *& udpContext, bool setRemoteDestAddr) {

    if (_ethernetStatus == ETH_OK || _ethernetStatus == FAILED_TO_BIND_CONTEXT ||
        _ethernetStatus == FAILED_TO_ALLOCATE_CONTEXT || _ethernetStatus == FAILED_TO_BIND_REMOTE_CONTEXT) 
    {
        if (net_context_get(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &udpContext) < 0) {
            _ethernetStatus = FAILED_TO_ALLOCATE_CONTEXT;
            return;
        }

        struct sockaddr_in localAddr;
        memset(&localAddr, 0, sizeof(localAddr));
        localAddr.sin_family = AF_INET;
        localAddr.sin_port = htons(LOCAL_PORT);

        if (net_context_bind(udpContext, (struct sockaddr *)&localAddr, sizeof(localAddr)) < 0) {
            _ethernetStatus = FAILED_TO_BIND_CONTEXT;
            net_context_put(udpContext);
            return;
        }

        if (setRemoteDestAddr) {
            struct sockaddr_in remoteAddr;
            memset(&remoteAddr, 0, sizeof(remoteAddr));
            remoteAddr.sin_family = AF_INET;
            remoteAddr.sin_port = htons(REMOTE_PORT);
             
            if (net_addr_pton(AF_INET, MCU_IP_ADDR, &remoteAddr.sin_addr) != 0) {
                _ethernetStatus = FAILED_TO_BIND_REMOTE_CONTEXT;
                net_context_put(udpContext);
                return;
            }

            if (net_context_connect(udpContext, (struct sockaddr *)&remoteAddr, sizeof(remoteAddr), NULL, K_NO_WAIT, NULL) < 0) {
                _ethernetStatus = FAILED_TO_BIND_REMOTE_CONTEXT;
                net_context_put(udpContext);
                return;
            }
        }

        _ethernetStatus = ETH_OK;
    }

    return;
}
