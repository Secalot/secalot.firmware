/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _BLDR_HAL_H__
#define _BLDR_HAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#define BLDR_GLOBAL_SIGNATURE_LENGTH (64)

BLDR_MAKE_PACKED(typedef struct)
{
    uint32_t deviceID;
    uint32_t firmwareVersion;
    uint32_t fileSystemVersion;
    uint16_t bootloaderVersion;

    uint8_t signature[BLDR_GLOBAL_SIGNATURE_LENGTH];
}
BLDR_GLOBAL_IMAGE_HEADER;

void bldrHalInit(void);
void bldrHalDeinit(void);

void bldrHalMemSet(uint8_t* dst, uint8_t value, uint16_t length);
void bldrHalMemCpy(uint8_t* dst, uint8_t* src, uint16_t length);

void bldrHalGetSerialNumber(uint32_t* serialNumber);
void bldrHalGetFirmwareVersion(uint32_t* firmwareVersion);
void bldrHalGetFileSystemVersion(uint32_t* fileSystemVersion);
void bldrHalGetBootloaderVersion(uint32_t* bootloaderVersion);
void bldrHalIsUpdateInterrupted(uint16_t* updateInterrupted);

#if defined(FIRMWARE)
void bldrHalSetBootloaderAsBootTarget(void);
void bldrHalIsBootloaderValid(uint16_t* bootloaderValid);
#elif defined(BOOTLOADER)
void bldrHalIsFirmwareValid(uint16_t* firmwareValid);
void bldrHalSetFirmwareAsBootTarget(void);
#endif

void bldrHalSetFileSystemUpdateInterruptedFlag(uint16_t fileSystemUpdateInterrupted);
void bldrHalSetFirmwareValidityFlag(uint16_t firmwareValid);
void bldrHalSetBootloaderValidityFlag(uint16_t bootloaderValid);
void bldrHalClearUpdateInterruptedFlagSetVersionsAndValidateFirmware(uint32_t firmwareVersion,
                                                                     uint32_t fileSystemVersion);
void bldrHalSetBootloaderVersionAndValidate(uint32_t bootloaderVersion);

void bldrHalGetCodeSize(uint32_t* codeSize);
void bldrHalGetFsSize(uint32_t* fsSize);

void bldrHalLoadImageData(uint8_t* data, uint32_t dataLength, uint32_t offset);
uint16_t bldrHalVerifyImageSignature(BLDR_GLOBAL_IMAGE_HEADER* imageHeader, uint32_t imageLength);
void bldrHalWriteImageData(uint32_t imageLength);
void bldrHalCalulateImageCRC32(uint32_t imageLength, uint32_t* crc32);
void bldrHalCalulateFlashContentsCRC32(uint32_t contentslength, uint32_t* crc32);

#ifdef FIRMWARE
void bldrHalEnableManufacturerBootloader(void);
#endif /* FIRMWARE */

void bldrHalFatalError(void);

#ifdef __cplusplus
}
#endif

#endif /* _BLDR_HAL_H__ */
