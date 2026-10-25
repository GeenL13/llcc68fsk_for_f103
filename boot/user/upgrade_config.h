#ifndef UPGRADE_CONFIG_H
#define UPGRADE_CONFIG_H

#include <stdint.h>
#include "flash_layout.h"

/* Boot currently starts App A. Receive an App B-linked .bin into the inactive slot. */
#define UPGRADE_TARGET_BASE        APP_B_BASE
#define UPGRADE_TARGET_SIZE        APP_SIZE

/* Send this byte during the entry window to stay in the serial receiver. */
#define UPGRADE_ENTRY_KEY          ((uint8_t)'u')
#define UPGRADE_ENTRY_WINDOW_MS    1500u

#define YMODEM_PACKET_TIMEOUT_MS   1000u
#define YMODEM_MAX_ERRORS          30u  /* ~30 s to select a file after 'u' */

#endif /* UPGRADE_CONFIG_H */
