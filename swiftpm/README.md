# lwIP as a Swift package

`Package.swift` at the root of this repository builds lwIP for one job: TCP over IPv6 in user space, on a point-to-point link whose packets the caller carries. It is meant for a program that receives raw IPv6 packets from somewhere other than a network interface (a tunnel inside another connection, for example) and has no privilege to create one.

```swift
.package(url: "<this repository>", exact: "2.2.1-swiftpm.1")
// …
.product(name: "CLwIP", package: "lwip")
```

## What is in it

| Path | What |
| --- | --- |
| `src/` | lwIP's own sources, unchanged. The manifest compiles the seventeen files of `src/core` this configuration needs |
| `swiftpm/port/lwipopts.h` | The configuration: IPv6 and TCP only, `NO_SYS`, the raw API, the system allocator, an MSS of 1220 for an MTU of 1280 |
| `swiftpm/port/arch/cc.h` | The platform header for macOS |
| `swiftpm/glue.c`, `swiftpm/include/CLwIP.h` | The one public header and its implementation: bring the interface up, feed packets in, receive packets to send, open, write, acknowledge and close a TCP connection, run the timers |

## Why the package does not expose lwIP's headers

lwIP is configured when it is compiled. Structures such as `ip_addr_t` and `struct tcp_pcb` change shape with the options in `lwipopts.h`, so code that includes lwIP's headers must be compiled with the same options as the library. Swift Package Manager does not pass a package's C settings on to the packages that depend on it, so a client could not be relied on to match. The public header therefore uses only plain C types and opaque pointers, and does not depend on the configuration.

For the same reason the configuration is not offered as package traits. A trait can switch a C define inside this package, but not for a client that includes lwIP's headers; with the surface above there is nothing for a trait to change yet.

## Using it

lwIP keeps its state in globals: a process has one stack, and every function of `CLwIP.h` must be called from one thread at a time. None of them blocks. Call `ADLPoll` a few times a second for lwIP's timers.

## Licence

lwIP's own, in `COPYING`. The files under `swiftpm/` are offered under the same terms.
