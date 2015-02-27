#include "system.h"

#if defined(PLAT_PI_1_B_PLUS)
#define MMIO_BASE		0x20000000
#define PLAT_NAME		"Raspberry Pi"
#elif defined(PLAT_PI_2_B)
#define MMIO_BASE		0x3f000000
#define PLAT_NAME		"Raspberry Pi2"
#else
#error "Unknown platform"
#endif

const uint32_t mmio_base = MMIO_BASE;
const char * const platform_name = PLAT_NAME;
