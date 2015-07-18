#include "system.h"

#if defined(PLAT_PI_1_B_PLUS)
#define MMIO_BASE		0x20000000
#define PLAT_NAME		"Raspberry Pi"
#define UART_CLOCK_STRING	"bcm2708.uart_clock="
#elif defined(PLAT_PI_2_B)
#define MMIO_BASE		0x3f000000
#define PLAT_NAME		"Raspberry Pi2"
#define UART_CLOCK_STRING	"bcm2709.uart_clock="
#else
#error "Unknown platform"
#endif

const uint32_t mmio_base = MMIO_BASE;
const char * const uart_clock_string = UART_CLOCK_STRING;
const char * const platform_name = PLAT_NAME;
const char * const libuart_version = VERSION;
