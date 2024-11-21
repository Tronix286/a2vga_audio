#pragma once

#include <stdint.h>
#include <hardware/flash.h>

#ifdef HAVE_8MB_FLASH
#define FLASH_SIZE             (8*1024*1024)
#else
#define FLASH_SIZE             (2*1024*1024)
#endif
#define FLASH_TOP              (XIP_BASE + FLASH_SIZE)
#define FLASH_OTA_SIZE         (256*1024)
#define FLASH_OTA_AREA         (FLASH_TOP - FLASH_OTA_SIZE)

#define CONFIG_SIZE            (4*1024)
#define FLASH_CONFIG_ONETIME   (FLASH_OTA_AREA - CONFIG_SIZE)
#define FLASH_CONFIG_PRIMARY   (FLASH_CONFIG_ONETIME - CONFIG_SIZE)
#define FLASH_CONFIG_SECONDARY (FLASH_CONFIG_PRIMARY - CONFIG_SIZE)

#define FONT_SIZE              (4*1024)
#define FONT_COUNT             40
#define FLASH_FONT_BASE        (FLASH_CONFIG_SECONDARY - (FONT_SIZE * FONT_COUNT))
#define FLASH_FONT(n)          (FLASH_FONT_BASE + (FONT_SIZE * (n)))

#define FLASH_FONT_ROMXE00     FLASH_FONT(0x00)
#define FLASH_FONT_ROMXE01     FLASH_FONT(0x01)
#define FLASH_FONT_ROMXE02     FLASH_FONT(0x02)
#define FLASH_FONT_ROMXE03     FLASH_FONT(0x03)
#define FLASH_FONT_ROMXE04     FLASH_FONT(0x04)
#define FLASH_FONT_ROMXE05     FLASH_FONT(0x05)
#define FLASH_FONT_ROMXE06     FLASH_FONT(0x06)
#define FLASH_FONT_ROMXE07     FLASH_FONT(0x07)
#define FLASH_FONT_ROMXE08     FLASH_FONT(0x08)
#define FLASH_FONT_ROMXE09     FLASH_FONT(0x09)
#define FLASH_FONT_ROMXE0A     FLASH_FONT(0x0A)
#define FLASH_FONT_ROMXE0B     FLASH_FONT(0x0B)
#define FLASH_FONT_ROMXE0C     FLASH_FONT(0x0C)
#define FLASH_FONT_ROMXE0D     FLASH_FONT(0x0D)
#define FLASH_FONT_ROMXE0E     FLASH_FONT(0x0E)
#define FLASH_FONT_ROMXE0F     FLASH_FONT(0x0F)
#define FLASH_FONT_ROMXE10     FLASH_FONT(0x10)
#define FLASH_FONT_ROMXE11     FLASH_FONT(0x11)
#define FLASH_FONT_ROMXE12     FLASH_FONT(0x12)
#define FLASH_FONT_ROMXE13     FLASH_FONT(0x13)
#define FLASH_FONT_ROMXE14     FLASH_FONT(0x14)
#define FLASH_FONT_ROMXE15     FLASH_FONT(0x15)
#define FLASH_FONT_ROMXE16     FLASH_FONT(0x16)
#define FLASH_FONT_ROMXE17     FLASH_FONT(0x17)
#define FLASH_FONT_ROMXE18     FLASH_FONT(0x18)
#define FLASH_FONT_ROMXE19     FLASH_FONT(0x19)
#define FLASH_FONT_ROMXE1A     FLASH_FONT(0x1A)
#define FLASH_FONT_ROMXE1B     FLASH_FONT(0x1B)
#define FLASH_FONT_ROMXE1C     FLASH_FONT(0x1C)
#define FLASH_FONT_ROMXE1D     FLASH_FONT(0x1D)
#define FLASH_FONT_ROMXE1E     FLASH_FONT(0x1E)
#define FLASH_FONT_ROMXE1F     FLASH_FONT(0x1F)
#define FLASH_FONT_APPLE_II    FLASH_FONT(0x20)
#define FLASH_FONT_APPLE_IIE   FLASH_FONT(0x21)
#define FLASH_FONT_APPLE_IIGS  FLASH_FONT(0x22)
#define FLASH_FONT_PRAVETZ     FLASH_FONT(0x23)
#define FLASH_FONT_EXTRA24     FLASH_FONT(0x24)
#define FLASH_FONT_EXTRA25     FLASH_FONT(0x25)
#define FLASH_FONT_EXTRA26     FLASH_FONT(0x26)
#define FLASH_FONT_EXTRA27     FLASH_FONT(0x27)

// Videx Font
#define FLASH_VIDEX_SIZE       (4*1024)
#define FLASH_VIDEX_BASE       (FLASH_FONT_BASE - FLASH_VIDEX_SIZE)

// Firmware for $C000-$CFFF
#define FLASH_6502_SIZE       (4*1024)
#define FLASH_6502_BASE       (FLASH_VIDEX_BASE - FLASH_6502_SIZE)

extern void flash_reboot() __attribute__ ((noreturn));
