// Compiler and platform glue for lwIP on macOS (arm64, little-endian).
#ifndef LWIP_SWIFTPM_ARCH_CC_H
#define LWIP_SWIFTPM_ARCH_CC_H

#include <stdio.h>
#include <stdlib.h>

// The system's headers already provide htons() and its relatives as macros;
// lwIP keeps its own lwip_htons() family and is told not to define the
// usual names over the system's.
#define LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS 1

// lwIP's arch.h defines LITTLE_ENDIAN and BIG_ENDIAN before it includes this
// file, and the system's headers define them again with the same values.
// With Clang modules that is reported as an ambiguous macro at every use,
// in every file; the two definitions agree, so the report is silenced for
// the files that include lwIP's headers.
#pragma clang diagnostic ignored "-Wambiguous-macro"

#define LWIP_RAND() ((u32_t)arc4random())

// lwIP's assertions mark a broken internal invariant. The port reports the
// text and stops rather than continue on a corrupted stack.
#define LWIP_PLATFORM_ASSERT(message) do { \
    fprintf(stderr, "lwIP assertion failed: %s (%s:%d)\n", message, __FILE__, __LINE__); \
    abort(); \
} while (0)

#define LWIP_PLATFORM_DIAG(message) do { printf message; } while (0)

#endif
