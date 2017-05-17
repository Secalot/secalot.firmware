/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __MK82_GLOBAL_H__
#define __MK82_GLOBAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stddef.h"

#ifndef BOOTSTRAPPER
#include "fsl_flash.h"
extern flash_config_t mk82FlashDriver;
#endif /* BOOTSTRAPPER */

#ifndef NULL
#define NULL (0)
#endif

#ifdef __CC_ARM
#define MK82_MAKE_PACKED(x) __packed x
#define MK82_PLACE_IN_SECTION(sectionName) __attribute__((section(sectionName)))
#define MK82_ALIGN(alignment) __attribute__((aligned(alignment)))
#elif __GNUC__
#define MK82_MAKE_PACKED(x) x __attribute__((packed))
#define MK82_PLACE_IN_SECTION(sectionName) __attribute__((section(sectionName)))
#define MK82_ALIGN(alignment) __attribute__((aligned(alignment)))
#else
#error Unsupported platform
#endif

#define MK82_TRUE 0x9999
#define MK82_FALSE 0x6666

#define MK82_NO_ERROR 0x9999
#define MK82_WRAPPED_KEY_CORRUPTED_ERROR 0x6666


#define MK82_FLASH_BOOTLOADER_START 0x00002000
#define MK82_FLASH_BOOTLOADER_SIZE 0x00008000

#define MK82_FLASH_FIRMWARE_START 0x0000C000
#define MK82_FLASH_FIRMWARE_SIZE 0x0002C000

#define MK82_FLASH_FILE_SYSTEM_START 0x00038000
#define MK82_FLASH_FILE_SYSTEM_SIZE 0x00008000

#define MK82_FLASH_PAGE_SIZE 0x00001000

#define MK82_GLOBAL_READONLY_KEYS_STRUCTURE_ADDRESS (0x00001F80)

#define MK82_BOOT_INFO_1_FLASH_AREA_START 0x000A000
#define MK82_BOOT_INFO_2_FLASH_AREA_START 0x000B000
#define MK82_BOOT_INFO_FLASH_AREA_SIZE 0x00001000

#ifdef __cplusplus
}
#endif

#endif /* __MK82_GLOBAL_H__ */
