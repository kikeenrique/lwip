// swift-tools-version: 6.1
import PackageDescription

// lwIP as a Swift package, for one job: TCP over IPv6 in user space, on a
// point-to-point tunnel whose packets the caller carries (macOS).
//
// lwIP is configured when it is compiled, by `lwipopts.h`. A structure such
// as `ip_addr_t` or `struct tcp_pcb` changes shape with those options, so a
// client that included lwIP's own headers would have to be compiled with
// exactly the same options as the library, and Swift Package Manager gives a
// package no way to hand its C settings to the packages that depend on it.
// The product therefore exposes none of lwIP's headers: its one public
// header, `swiftpm/include/CLwIP.h`, is a small surface over the raw API
// that does not depend on the configuration.
//
// lwIP's own sources are used unchanged. The files under `swiftpm/` are the
// port: the configuration (`port/lwipopts.h`), the platform header
// (`port/arch/cc.h`) and the surface (`glue.c`).
let package = Package(
    name: "lwip",
    products: [.library(name: "CLwIP", targets: ["CLwIP"])],
    targets: [
        .target(
            name: "CLwIP",
            path: ".",
            sources: [
                "src/core/def.c", "src/core/inet_chksum.c", "src/core/init.c", "src/core/ip.c",
                "src/core/mem.c", "src/core/memp.c", "src/core/netif.c", "src/core/pbuf.c",
                "src/core/tcp.c", "src/core/tcp_in.c", "src/core/tcp_out.c", "src/core/timeouts.c",
                "src/core/ipv6/icmp6.c", "src/core/ipv6/inet6.c", "src/core/ipv6/ip6.c",
                "src/core/ipv6/ip6_addr.c", "src/core/ipv6/nd6.c",
                "swiftpm/glue.c"
            ],
            publicHeadersPath: "swiftpm/include",
            cSettings: [.headerSearchPath("swiftpm/port"), .headerSearchPath("src/include")]
        )
    ]
)
