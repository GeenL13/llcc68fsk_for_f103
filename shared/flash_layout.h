#ifndef FLASH_LAYOUT_H
#define FLASH_LAYOUT_H
#define BOOT_BASE    0x08000000u
#define BOOT_SIZE    (12u * 1024u)
#define APP_A_BASE   0x08003000u
#define APP_B_BASE   0x08009400u
#define APP_SIZE     (25u * 1024u)
#define META0_BASE   0x0800F800u
#define META1_BASE   0x0800FC00u
#define PAGE_SIZE    1024u
#define RAM_FIRST    0x20000000u
#define RAM_END      0x20005000u
_Static_assert(APP_A_BASE == BOOT_BASE + BOOT_SIZE, "boot/A boundary");
_Static_assert(APP_B_BASE == APP_A_BASE + APP_SIZE, "A/B boundary");
_Static_assert(META0_BASE == APP_B_BASE + APP_SIZE, "B/meta boundary");
_Static_assert(META1_BASE + PAGE_SIZE == 0x08010000u, "flash end");
#endif