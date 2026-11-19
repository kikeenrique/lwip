// A small surface over lwIP's raw API for Swift Package Manager clients
// (see include/CLwIP.h).
#include "CLwIP.h"

#include <string.h>
#include <time.h>

#include "lwip/init.h"
#include "lwip/ip6.h"
#include "lwip/ip6_addr.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/timeouts.h"

// lwIP asks the port for a millisecond clock.
u32_t sys_now(void) {
    return (u32_t)(clock_gettime_nsec_np(CLOCK_MONOTONIC) / 1000000ULL);
}

static struct netif ADLInterface;
static int ADLInterfaceUp;
static ADLOutput ADLOutputFunction;
static void *ADLOutputContext;

static err_t ADLInterfaceOutput(struct netif *netif, struct pbuf *packet, const ip6_addr_t *destination) {
    (void)netif; (void)destination;
    if (!ADLOutputFunction) return ERR_IF;
    uint8_t buffer[2048];
    if (packet->tot_len > sizeof(buffer)) return ERR_BUF;
    pbuf_copy_partial(packet, buffer, packet->tot_len, 0);
    return ADLOutputFunction(ADLOutputContext, buffer, packet->tot_len) == 0 ? ERR_OK : ERR_IF;
}

static err_t ADLInterfaceInit(struct netif *netif) {
    netif->name[0] = 't'; netif->name[1] = 'n';
    netif->output_ip6 = ADLInterfaceOutput;
    netif->flags = NETIF_FLAG_UP | NETIF_FLAG_LINK_UP;
    return ERR_OK;
}

int ADLStart(const uint8_t localAddress[16], uint16_t mtu, ADLOutput output, void *context) {
    static int initialized;
    if (!initialized) { lwip_init(); initialized = 1; }
    if (ADLInterfaceUp) ADLStop();
    if (!localAddress || !output || mtu < 1280) return -1;
    ADLOutputFunction = output;
    ADLOutputContext = context;
    memset(&ADLInterface, 0, sizeof(ADLInterface));
    if (!netif_add_noaddr(&ADLInterface, NULL, ADLInterfaceInit, ip6_input)) return -1;
    ADLInterface.mtu = mtu;
    ADLInterface.mtu6 = mtu;
    ip6_addr_t address;
    memcpy(address.addr, localAddress, 16);
    ip6_addr_clear_zone(&address);
    netif_ip6_addr_set(&ADLInterface, 0, &address);
    netif_ip6_addr_set_state(&ADLInterface, 0, IP6_ADDR_PREFERRED);
    netif_set_default(&ADLInterface);
    netif_set_up(&ADLInterface);
    netif_set_link_up(&ADLInterface);
    ADLInterfaceUp = 1;
    return 0;
}

void ADLStop(void) {
    if (!ADLInterfaceUp) return;
    netif_set_down(&ADLInterface);
    netif_remove(&ADLInterface);
    ADLInterfaceUp = 0;
    ADLOutputFunction = NULL;
    ADLOutputContext = NULL;
}

int ADLInput(const uint8_t *packet, size_t length) {
    if (!ADLInterfaceUp || !packet || length < 40 || length > 0xffff) return -1;
    struct pbuf *buffer = pbuf_alloc(PBUF_RAW, (u16_t)length, PBUF_RAM);
    if (!buffer) return -1;
    pbuf_take(buffer, packet, (u16_t)length);
    if (ADLInterface.input(buffer, &ADLInterface) != ERR_OK) {
        pbuf_free(buffer);
        return -1;
    }
    return 0;
}

void ADLPoll(void) {
    sys_check_timeouts();
}

struct ADLConnection {
    struct tcp_pcb *pcb;
    ADLCallbacks callbacks;
};

static err_t ADLOnConnected(void *argument, struct tcp_pcb *pcb, err_t error) {
    (void)pcb;
    ADLConnection *connection = argument;
    if (error == ERR_OK && connection->callbacks.connected) connection->callbacks.connected(connection->callbacks.context);
    return ERR_OK;
}

static err_t ADLOnReceived(void *argument, struct tcp_pcb *pcb, struct pbuf *data, err_t error) {
    (void)pcb; (void)error;
    ADLConnection *connection = argument;
    if (!data) {
        if (connection->callbacks.finished) connection->callbacks.finished(connection->callbacks.context);
        return ERR_OK;
    }
    for (struct pbuf *part = data; part; part = part->next) {
        if (connection->callbacks.received) connection->callbacks.received(connection->callbacks.context, part->payload, part->len);
    }
    pbuf_free(data);
    return ERR_OK;
}

static void ADLOnError(void *argument, err_t error) {
    // lwIP has already freed the pcb when it reports an error.
    ADLConnection *connection = argument;
    connection->pcb = NULL;
    if (connection->callbacks.failed) connection->callbacks.failed(connection->callbacks.context, error);
}

ADLConnection *ADLConnect(const uint8_t remoteAddress[16], uint16_t port, ADLCallbacks callbacks) {
    if (!ADLInterfaceUp || !remoteAddress || port == 0) return NULL;
    ADLConnection *connection = calloc(1, sizeof(*connection));
    if (!connection) return NULL;
    connection->callbacks = callbacks;
    connection->pcb = tcp_new_ip6();
    if (!connection->pcb) { free(connection); return NULL; }
    tcp_arg(connection->pcb, connection);
    tcp_recv(connection->pcb, ADLOnReceived);
    tcp_err(connection->pcb, ADLOnError);
    tcp_nagle_disable(connection->pcb);
    ip_addr_t remote;
    memcpy(ip_2_ip6(&remote)->addr, remoteAddress, 16);
    ip6_addr_clear_zone(ip_2_ip6(&remote));
    if (tcp_connect(connection->pcb, &remote, port, ADLOnConnected) != ERR_OK) {
        tcp_arg(connection->pcb, NULL);
        tcp_abort(connection->pcb);
        free(connection);
        return NULL;
    }
    return connection;
}

long ADLWrite(ADLConnection *connection, const uint8_t *bytes, size_t length) {
    if (!connection || !connection->pcb || !bytes) return -1;
    size_t room = tcp_sndbuf(connection->pcb);
    size_t count = length < room ? length : room;
    if (count > 0xffff) count = 0xffff;
    if (count == 0) return 0;
    err_t error = tcp_write(connection->pcb, bytes, (u16_t)count, TCP_WRITE_FLAG_COPY);
    if (error == ERR_MEM) return 0;
    if (error != ERR_OK) return -1;
    tcp_output(connection->pcb);
    return (long)count;
}

void ADLAcknowledge(ADLConnection *connection, size_t length) {
    if (!connection || !connection->pcb) return;
    while (length > 0) {
        u16_t step = length > 0xffff ? 0xffff : (u16_t)length;
        tcp_recved(connection->pcb, step);
        length -= step;
    }
}

void ADLClose(ADLConnection *connection) {
    if (!connection) return;
    if (connection->pcb) {
        tcp_arg(connection->pcb, NULL);
        tcp_recv(connection->pcb, NULL);
        tcp_err(connection->pcb, NULL);
        if (tcp_close(connection->pcb) != ERR_OK) tcp_abort(connection->pcb);
        connection->pcb = NULL;
    }
    free(connection);
}
