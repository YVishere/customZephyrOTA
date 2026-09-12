#include "zephyrethernet.h"
#include <zephyr/logging/log.h>

static const int ALIGN_BYTES = 4;
static const int MAX_QUEUE_SIZE = 10;

struct EthMsg_t {
    void * fifo_reserved;   
    struct net_pkt * pkt;   
};

LOG_MODULE_REGISTER(zephyrethernet, CONFIG_ETH_SNIFFER_LOG_LEVEL);

K_FIFO_DEFINE(ethPacketFifo);
static struct net_if_cb ethListener;
K_MEM_SLAB_DEFINE(msgSlab, sizeof(struct EthMsg_t), MAX_QUEUE_SIZE, ALIGN_BYTES);

ZephyrEthernet::ZephyrEthernet() : _ethernetStatus(OK) {}

EthernetErrorCode ZephyrEthernet::initEthernetDevice(){
    app_fifo = shared_fifo;
    if (_ethernetStatus == OK && net_context_recv(context, ethernetReadCallback, K_NO_WAIT, this) < 0) {
        _ethernetStatus = FAILED_TO_SET_CALLBACK;
        return _ethernetStatus;
    }

    return _ethernetStatus;
}

net_pkt ZephyrEthernet::getNextPacket() {
    struct EthMsg_t * ethMsg = k_fifo_get(&ethPacketFifo, K_FOREVER);

    if (ethMsg && ethMsg->pkt)
    {
        
    }
}

void ZephyrEthernet::rxCallbackBridge(struct net_context * context,
                                        struct net_pkt * pkt,
                                        union net_ip_header * ipHeader,
                                        union net_proto_header * protocolHeader,
                                        int status,
                                        void * user_data)
{
    ZephyrEthernet *instance = static_cast<ZephyrEthernet*>(user_data);

    if ((_ethernetStatus == ETHERNET_READ_ERROR) && (status == 0)) {
        _ethernetStatus = OK;
    }

    if (_ethernetStatus == OK) {
        if (status == 0) {
            if (instance != nullptr) {
                instance->readHandler(context, pkt, ipHeader, protocolHeader, status);
            }
        }
        else {
            _ethernetStatus = ETHERNET_READ_ERROR;
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
        _ethernetStatus = OK;
    }
    else
    {
        return;
    }

    if (_ethernetStatus == OK)
    {
        if (pkt) {
            struct EthMsg_t * msgWrapper;

            if (k_mem_slab_alloc(&msgSlab, (void **)&msgWrapper, K_NO_WAIT) != 0) {
                LOG_WRN("Library wrapper slab exhausted. Dropping packet tracing wrapper.");
                return; 
            }

            net_pkt_ref(pkt);
            msgWrapper->pkt = pkt;
            k_fifo_put(app_fifo, msgWrapper);
        }
        else {
            _ethernetStatus = NULL_PACKET_SEEN;
            return;
        }
    }
}

EthernetErrorCode ZephyrEthernet::ethernetStatus() const{
    return _ethernetStatus;
}

void ZephyrEthernet::ethFreeMsg(struct EthMsg_t *msg)
{
    if (msg) {
        k_mem_slab_free(&msgSlab, (void *)msg);
    }
}
