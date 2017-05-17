/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __BLDR_CORE_INT_H__
#define __BLDR_CORE_INT_H__

#define BLDR_CORE_CLA (0x80)

#define BLDR_CORE_INS_GET_INFO (0x00)
#define BLDR_CORE_INS_SET_FIRMWARE_AS_BOOT_TARGET (0x01)
#define BLDR_CORE_INS_SET_IMAGE_INFO (0x02)
#define BLDR_CORE_INS_LOAD_IMAGE_DATA (0x03)
#define BLDR_CORE_INS_LOAD_FINALIZE_LOADING (0x04)
#define BLDR_CORE_INS_SET_BOOTLOADER_AS_BOOT_TARGET (0x05)

#define BLDR_CORE_INS_ENABLE_MANUFACTURER_BOOTLOADER (0x80)

#define BLDR_CORE_P1P2_GET_INFO (0x0000)
#define BLDR_CORE_P1P2_SET_FIRMWARE_AS_BOOT_TARGET (0x0000)
#define BLDR_CORE_P1P2_SET_IMAGE_INFO (0x0000)
#define BLDR_CORE_P1P2_LOAD_IMAGE_DATA (0x0000)
#define BLDR_CORE_P1P2_FINALIZE_LOADING_DO_NOT_UPDATE_FILESYSTEM (0x0000)
#define BLDR_CORE_P1P2_FINALIZE_LOADING_UPDATE_FILESYSTEM (0x0001)
#define BLDR_CORE_P1P2_SET_BOOTLOADER_AS_BOOT_TARGET (0x0000)
#define BLDR_CORE_P1P2_ENABLE_MANUFACTURER_BOOTLOADER (0x0000)

#define BLDR_CORE_LOAD_IMAGE_DATA_PER_APDU (0x80)

typedef struct
{
    uint16_t expectedCommand;
    uint16_t fileSystemUpdateRequested;

    uint32_t totalImageSize;
    uint32_t loadedImageSize;

    BLDR_GLOBAL_IMAGE_HEADER imageHeader;

} BLDR_CORE_CONTEXT;

#define BLDR_CORE_CONTEXT_EXPECTED_COMMAND_SET_IMAGE_INFO_OR_MISC_COMMANDS (0x9999)
#define BLDR_CORE_CONTEXT_EXPECTED_COMMAND_LOAD_IMAGE_DATA (0x6666)
#define BLDR_CORE_CONTEXT_EXPECTED_COMMAND_FINALIZE_LOADING (0xCCCC)

#define BLDR_CORE_AID                                              \
    {                                                              \
        0x42, 0x4C, 0x44, 0x52, 0x41, 0x50, 0x50, 0x4C, 0x45, 0x54 \
    }
#define BLDR_CORE_AID_LENGTH (0x0A)

#endif /* __BLDR_CORE_INT_H__ */
