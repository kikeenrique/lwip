// A small C surface over lwIP for one use: TCP connections over IPv6 on a
// point-to-point tunnel whose packets the caller carries.
//
// lwIP keeps its state in globals, so a process has one stack. Every function
// here must be called from one thread at a time; none of them blocks. The
// caller feeds received packets in with ADLInput, receives the packets to
// send through its output callback, and calls ADLPoll a few times a second
// for lwIP's timers.
#ifndef LWIP_SWIFTPM_CLWIP_H
#define LWIP_SWIFTPM_CLWIP_H

#include <stddef.h>
#include <stdint.h>

/// One IPv6 packet to send to the peer. Return zero when it was taken.
typedef int (*ADLOutput)(void *context, const uint8_t *packet, size_t length);

/// Brings the single interface up with the local address and MTU the tunnel
/// gave. Returns zero. Calling it again replaces the interface; connections
/// of the old one must have been closed.
int ADLStart(const uint8_t localAddress[16], uint16_t mtu, ADLOutput output, void *context);
/// Takes the interface down. Open connections are aborted first.
void ADLStop(void);
/// One received IPv6 packet. Returns zero when lwIP accepted it.
int ADLInput(const uint8_t *packet, size_t length);
/// Runs lwIP's timers (retransmission, delayed acknowledgement).
void ADLPoll(void);

typedef struct ADLConnection ADLConnection;

typedef struct {
    void *context;
    /// The connection is established.
    void (*connected)(void *context);
    /// Bytes arrived. The callee calls ADLAcknowledge once it has consumed them.
    void (*received)(void *context, const uint8_t *bytes, size_t length);
    /// The peer closed its side; no more bytes will arrive.
    void (*finished)(void *context);
    /// The connection failed or was reset; `error` is lwIP's err_t. The
    /// connection is gone: do not call ADLClose for it.
    void (*failed)(void *context, int error);
} ADLCallbacks;

/// Starts a TCP connection to `port` at `remoteAddress`. NULL when lwIP has
/// no memory for it or refuses the address.
ADLConnection *ADLConnect(const uint8_t remoteAddress[16], uint16_t port, ADLCallbacks callbacks);
/// Queues bytes to send and returns how many were accepted (possibly fewer
/// than `length`, or zero when the send buffer is full), or -1 on an error.
long ADLWrite(ADLConnection *connection, const uint8_t *bytes, size_t length);
/// Opens the receive window again by `length` consumed bytes.
void ADLAcknowledge(ADLConnection *connection, size_t length);
/// Closes and frees the connection. Callbacks stop.
void ADLClose(ADLConnection *connection);

#endif
