#include "zephyrethernet.h"
#include <string.h>

static const int ALIGN_BYTES = 4;
static const int MAX_QUEUE_SIZE = 10;
static const int LOCAL_PORT = 12345;
static const int REMOTE_PORT = 12345;
static constexpr char MCU_IP_ADDR[] = "192.168.1.1";
static constexpr char MCU_NETMASK[] = "255.255.255.0";

struct EthMsg_t {
    void * fifo_reserved;   
    struct net_pkt * pkt;   
};

K_FIFO_DEFINE(ethPacketFifo);
K_MEM_SLAB_DEFINE(msgSlab, sizeof(struct EthMsg_t), MAX_QUEUE_SIZE, ALIGN_BYTES);

ZephyrEthernet::ZephyrEthernet(const char * peerIP) : _udpContext(nullptr), _ethernetStatus(ETH_OK) {
    if (strlen(peerIP) < NET_IPV4_ADDR_LEN) {
        strncpy(_peerIP, peerIP, NET_IPV4_ADDR_LEN);
    }
    else {
        _peerIP[0] = '\0';
    }
}

ZephyrEthernet::ZephyrEthernet() : _udpContext(nullptr), _ethernetStatus(ETH_OK) {
    _peerIP[0] = '\0';
}

ZephyrEthernet::~ZephyrEthernet() {
    if (_udpContext != nullptr) {
        net_context_put(_udpContext);
    }
}

EthernetErrorCode ZephyrEthernet::initEthernetDevice(bool setRemoteDestAddr){
    if (configureInterface() != ETH_OK) {
        return _ethernetStatus;
    }
    
    net_context * context;
    setUDPContext(context, setRemoteDestAddr);

    if (_ethernetStatus == ETH_OK && net_context_recv(context, rxCallbackBridge, K_NO_WAIT, this) < 0) {
        _ethernetStatus = FAILED_TO_SET_CALLBACK;
        net_context_put(context);
        return _ethernetStatus;
    }

    if (_ethernetStatus == ETH_OK){
        _udpContext = context;
    }

    return _ethernetStatus;
}

EthernetErrorCode ZephyrEthernet::getNextPacket(uint8_t * buffer, size_t bufferSize, size_t * bytesRead, k_timeout_t timeout) {
    if (bytesRead) {
        *bytesRead = 0;
    }

    if (_ethernetStatus != ETH_OK) {
        return _ethernetStatus;
    }
    
    struct EthMsg_t * ethMsg = (struct EthMsg_t *) k_fifo_get(&ethPacketFifo, timeout);

    if (!ethMsg) {
        return PACKET_READ_TIMEOUT;
    }

    EthernetErrorCode readErrorCode = ETH_OK;

    if (ethMsg->pkt) {
        struct net_pkt * pkt = ethMsg->pkt;
        size_t frameLenLeft = net_pkt_remaining_data(pkt);
        size_t bytesToRead = MIN(frameLenLeft, bufferSize);
        
        if (net_pkt_read(pkt, buffer, bytesToRead) != 0) {
            readErrorCode = PACKET_READ_ERROR;
        }
        else {
            if (bytesRead) {
                *bytesRead = bytesToRead;
            }
        }

        net_pkt_unref(pkt);
    }

    ethFreeMsg(ethMsg);

    return readErrorCode;
}

void ZephyrEthernet::rxCallbackBridge(struct net_context * context,
                                        struct net_pkt * pkt,
                                        union net_ip_header * ipHeader,
                                        union net_proto_header * protocolHeader,
                                        int status,
                                        void * user_data)
{
    ZephyrEthernet * instance = static_cast<ZephyrEthernet*>(user_data);
    
    if (status == 0 && instance != nullptr) {
        instance->readHandler(context, pkt, ipHeader, protocolHeader, status);
    }
    else if (pkt != nullptr) {
        net_pkt_unref(pkt);
    }
}

void ZephyrEthernet::readHandler(struct net_context * context,
                                    struct net_pkt * pkt,
                                    union net_ip_header * ipHeader,
                                    union net_proto_header * protocolHeader,
                                    int status) 
{
    if (!pkt) {
        return;
    }

    if (_ethernetStatus != ETH_OK) {
        net_pkt_unref(pkt);
        return;
    }

    struct EthMsg_t * msgWrapper;

    if (k_mem_slab_alloc(&msgSlab, (void **)&msgWrapper, K_NO_WAIT) != 0) {
        net_pkt_unref(pkt);
        return; 
    }

    msgWrapper->pkt = pkt;
    k_fifo_put(&ethPacketFifo, msgWrapper);
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

EthernetErrorCode ZephyrEthernet::getNextPacketImmediate(uint8_t * buffer, size_t bufferSize, size_t * bytesRead) {
    return getNextPacket(buffer, bufferSize, bytesRead, K_NO_WAIT);
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

        if (setRemoteDestAddr && (*_peerIP != '\0')) {
            struct sockaddr_in remoteAddr;
            memset(&remoteAddr, 0, sizeof(remoteAddr));
            remoteAddr.sin_family = AF_INET;
            remoteAddr.sin_port = htons(REMOTE_PORT);
             
            if (net_addr_pton(AF_INET, _peerIP, &remoteAddr.sin_addr) != 0) {
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

EthernetErrorCode ZephyrEthernet::configureInterface() {
    if (_ethernetStatus == ETH_OK) {
        struct net_if * iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));

        if (iface == nullptr) {
            _ethernetStatus = INTERFACE_NOT_FOUND;
            return _ethernetStatus;
        }

        struct in_addr addr;
        struct in_addr mask;

        if (net_addr_pton(AF_INET, MCU_IP_ADDR, &addr) != 0 ||
            net_addr_pton(AF_INET, MCU_NETMASK, &mask) != 0 ||
            net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0) == nullptr)
        {
            _ethernetStatus = FAILED_TO_SET_IP_ADDR;
            return _ethernetStatus;
        }

        net_if_ipv4_set_netmask_by_addr(iface, &addr, &mask);
    }
    return _ethernetStatus;
}
