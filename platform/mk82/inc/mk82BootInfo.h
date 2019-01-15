/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __MK82_BOOT_INFO_H__
#define __MK82_BOOT_INFO_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define MK82_BOOT_INFO_BOOTLOADER_VERSION 0x00000001
#define MK82_BOOT_INFO_FIRMWARE_VERSION 0x00000001
#define MK82_BOOT_INFO_FILE_SYSTEM_VERSION 0x00000001

#define MK82_BOOT_INFO_START_FIRMWARE 0x9999
#define MK82_BOOT_INFO_START_BOOTLOADER 0x6666

    typedef struct
    {
        uint32_t firmwareVersion;
        uint32_t fileSystemVersion;
        uint32_t bootloaderVersion;
        uint16_t bootTarget;
        uint16_t firmwareValid;
        uint16_t bootloaderValid;
        uint16_t fileSystemUpdateInterrupted;
        uint32_t crc;
    } MK82_BOOT_INFO;

#ifndef BOOTSTRAPPER
    void mk82BootInfoSetData(MK82_BOOT_INFO* bootInfo);
#endif /* BOOTSTRAPPER */

    void mk82BootInfoInit(void);

    void mk82BootInfoGetData(MK82_BOOT_INFO* bootInfo);

    void mk82BootInfoBootFirmware(void);
    void mk82BootInfoBootBootloader(void);

#ifdef __cplusplus
}
#endif

#endif /* __MK82_BOOT_INFO_H__ */
