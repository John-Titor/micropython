/*
 * board-specific defines
 */

#define MICROPY_HW_BOARD_NAME       	"IP940"
//#define MICROPY_PY_MACHINE_UART   	  (1)
#define MICROPY_READER_VFS          	(1)
#define MICROPY_VFS                 	(1)
#define MICROPY_FATFS_ENABLE_LFN		(1)
#define MICROPY_FATFS_USE_LABEL			(1)
#define MICROPY_FATFS_RPATH				(2)
#define MICROPY_FATFS_MULTI_PARTITION	(1)
#define MICROPY_FATFS_EXFAT				(1)
#define MICROPY_FATFS_NORTC				(1)

#define M68K_EARLY_STARTUP_HOOK     	m68k_ip940_early_init
#define M68K_HAVE_FPU               	(1)
#define M68K_TICK_MS                	(5)     // use the 200Hz timer

#define QUART_CLOCK                 	33333333        // base clock is 66.667MHz
#define QUART_STRIDE                	(4)
#define QUART_BASE                  	(0x02110003)
#define QUART_CONSOLE_CHANNEL       	(0)
