/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <bldrGlobal.h>
#include <bldrGlobalInt.h>
#include <bldrHal.h>
#include <hal/k82/bldrHalInt.h>

#include "mk82Global.h"
#include "mk82System.h"
#include "mk82BootInfo.h"

#include "fsl_flash.h"
#include "fsl_crc.h"

extern flash_config_t mk82FlashDriver;

#include "mbedtls/ecdsa.h"
#include "mbedtls/ecp.h"
#include "mbedtls/sha256.h"

#ifdef FIRMWARE
static uint8_t MK82_ALIGN(4) dataBuffer[MK82_FLASH_BOOTLOADER_SIZE];
#else
static uint8_t MK82_ALIGN(4) dataBuffer[MK82_FLASH_FIRMWARE_SIZE + MK82_FLASH_FILE_SYSTEM_SIZE];
#endif /* FIRMWARE */

uint8_t mk82BldrHalPublicKeyX[] = {0x6c, 0xdd, 0x30, 0xff, 0xb0, 0x4c, 0xa3, 0xec, 0x24, 0x28, 0x38,
                                   0x4d, 0x63, 0xe4, 0x2f, 0xb0, 0x2e, 0x47, 0x66, 0x9b, 0x21, 0x99,
                                   0xd1, 0xdc, 0xc9, 0x0e, 0x3a, 0xe9, 0xed, 0xb9, 0xaf, 0x4a};

uint8_t mk82BldrHalPublicKeyY[] = {0x36, 0xf7, 0xce, 0x11, 0x7d, 0x4c, 0xff, 0xf5, 0x8e, 0x7c, 0xac,
                                   0x74, 0x17, 0x7c, 0x04, 0x75, 0xf4, 0xcf, 0x47, 0x50, 0x11, 0x7c,
                                   0x27, 0xe9, 0x4b, 0x61, 0x3d, 0x6e, 0x21, 0xbd, 0x86, 0x6e};

void bldrHalInit(void) {}

void bldrHalDeinit(void) {}

void bldrHalMemSet(uint8_t* dst, uint8_t value, uint16_t length) { mk82SystemMemSet(dst, value, length); }

void bldrHalMemCpy(uint8_t* dst, uint8_t* src, uint16_t length) { mk82SystemMemCpy(dst, src, length); }

void bldrHalGetSerialNumber(uint32_t* serialNumber)
{
    if (serialNumber == NULL)
    {
        mk82SystemFatalError();
    }

    mk82SystemGetSerialNumber(serialNumber);
}

void bldrHalGetFirmwareVersion(uint32_t* firmwareVersion)
{
    MK82_BOOT_INFO bootInfo;

    if (firmwareVersion == NULL)
    {
        mk82SystemFatalError();
    }

    mk82BootInfoGetData(&bootInfo);

    *firmwareVersion = bootInfo.firmwareVersion;
}

void bldrHalGetFileSystemVersion(uint32_t* fileSystemVersion)
{
    MK82_BOOT_INFO bootInfo;

    if (fileSystemVersion == NULL)
    {
        mk82SystemFatalError();
    }

    mk82BootInfoGetData(&bootInfo);

    *fileSystemVersion = bootInfo.fileSystemVersion;
}

void bldrHalGetBootloaderVersion(uint32_t* bootloaderVersion)
{
    MK82_BOOT_INFO bootInfo;

    if (bootloaderVersion == NULL)
    {
        mk82SystemFatalError();
    }

    mk82BootInfoGetData(&bootInfo);

    *bootloaderVersion = bootInfo.bootloaderVersion;
}

void bldrHalClearUpdateInterruptedFlagSetVersionsAndValidateFirmware(uint32_t firmwareVersion,
                                                                     uint32_t fileSystemVersion)
{
    MK82_BOOT_INFO bootInfo;

    mk82BootInfoGetData(&bootInfo);

    bootInfo.firmwareValid = MK82_TRUE;
    bootInfo.fileSystemUpdateInterrupted = MK82_FALSE;
    bootInfo.firmwareVersion = firmwareVersion;
    bootInfo.fileSystemVersion = fileSystemVersion;

    mk82BootInfoSetData(&bootInfo);
}

void bldrHalSetBootloaderVersionAndValidate(uint32_t bootloaderVersion)
{
    MK82_BOOT_INFO bootInfo;

    mk82BootInfoGetData(&bootInfo);

    bootInfo.bootloaderValid = MK82_TRUE;
    bootInfo.bootloaderVersion = bootloaderVersion;

    mk82BootInfoSetData(&bootInfo);
}

void bldrHalIsUpdateInterrupted(uint16_t* updateInterrupted)
{
    MK82_BOOT_INFO bootInfo;

    if (updateInterrupted == NULL)
    {
        mk82SystemFatalError();
    }

    mk82BootInfoGetData(&bootInfo);

    if ((bootInfo.fileSystemUpdateInterrupted) == MK82_TRUE)
    {
        (*updateInterrupted) = BLDR_TRUE;
    }
    else if ((bootInfo.fileSystemUpdateInterrupted) == MK82_FALSE)
    {
        (*updateInterrupted) = BLDR_FALSE;
    }
    else
    {
        mk82SystemFatalError();
    }
}

void bldrHalSetFileSystemUpdateInterruptedFlag(uint16_t fileSystemUpdateInterrupted)
{
    MK82_BOOT_INFO bootInfo;

    mk82BootInfoGetData(&bootInfo);

    if (fileSystemUpdateInterrupted == BLDR_TRUE)
    {
        bootInfo.fileSystemUpdateInterrupted = MK82_TRUE;
    }
    else if (fileSystemUpdateInterrupted == BLDR_FALSE)
    {
        bootInfo.fileSystemUpdateInterrupted = MK82_FALSE;
    }
    else
    {
        mk82SystemFatalError();
    }

    mk82BootInfoSetData(&bootInfo);
}

void bldrHalSetFirmwareValidityFlag(uint16_t firmwareValid)
{
    MK82_BOOT_INFO bootInfo;

    mk82BootInfoGetData(&bootInfo);

    if (firmwareValid == BLDR_TRUE)
    {
        bootInfo.firmwareValid = MK82_TRUE;
    }
    else if (firmwareValid == BLDR_FALSE)
    {
        bootInfo.firmwareValid = MK82_FALSE;
    }
    else
    {
        mk82SystemFatalError();
    }

    mk82BootInfoSetData(&bootInfo);
}

void bldrHalSetBootloaderValidityFlag(uint16_t bootloaderValid)
{
    MK82_BOOT_INFO bootInfo;

    mk82BootInfoGetData(&bootInfo);

    if (bootloaderValid == BLDR_TRUE)
    {
        bootInfo.bootloaderValid = MK82_TRUE;
    }
    else if (bootloaderValid == BLDR_FALSE)
    {
        bootInfo.bootloaderValid = MK82_FALSE;
    }
    else
    {
        mk82SystemFatalError();
    }

    mk82BootInfoSetData(&bootInfo);
}

#if defined(FIRMWARE)

void bldrHalSetBootloaderAsBootTarget(void)
{
    MK82_BOOT_INFO bootInfo;

    mk82BootInfoGetData(&bootInfo);

    bootInfo.bootTarget = MK82_BOOT_INFO_START_BOOTLOADER;

    mk82BootInfoSetData(&bootInfo);
}

void bldrHalIsBootloaderValid(uint16_t* bootloaderValid)
{
    MK82_BOOT_INFO bootInfo;

    if (bootloaderValid == NULL)
    {
        mk82SystemFatalError();
    }

    mk82BootInfoGetData(&bootInfo);

    if ((bootInfo.bootloaderValid) == MK82_TRUE)
    {
        (*bootloaderValid) = BLDR_TRUE;
    }
    else if ((bootInfo.bootloaderValid) == MK82_FALSE)
    {
        (*bootloaderValid) = BLDR_FALSE;
    }
    else
    {
        mk82SystemFatalError();
    }
}

#elif defined(BOOTLOADER)

void bldrHalSetFirmwareAsBootTarget(void)
{
    MK82_BOOT_INFO bootInfo;

    mk82BootInfoGetData(&bootInfo);

    bootInfo.bootTarget = MK82_BOOT_INFO_START_FIRMWARE;

    mk82BootInfoSetData(&bootInfo);
}

void bldrHalIsFirmwareValid(uint16_t* firmwareValid)
{
    MK82_BOOT_INFO bootInfo;

    if (firmwareValid == NULL)
    {
        mk82SystemFatalError();
    }

    mk82BootInfoGetData(&bootInfo);

    if ((bootInfo.firmwareValid) == MK82_TRUE)
    {
        (*firmwareValid) = BLDR_TRUE;
    }
    else if ((bootInfo.firmwareValid) == MK82_FALSE)
    {
        (*firmwareValid) = BLDR_FALSE;
    }
    else
    {
        mk82SystemFatalError();
    }
}

#endif

void bldrHalGetCodeSize(uint32_t* codeSize)
{
    if (codeSize == NULL)
    {
        mk82SystemFatalError();
    }

#if defined(FIRMWARE)
    *codeSize = MK82_FLASH_BOOTLOADER_SIZE;
#elif defined(BOOTLOADER)
    *codeSize = MK82_FLASH_FIRMWARE_SIZE;
#endif
}

void bldrHalGetFsSize(uint32_t* fsSize)
{
    if (fsSize == NULL)
    {
        mk82SystemFatalError();
    }

#if defined(FIRMWARE)
    *fsSize = 0;
#elif defined(BOOTLOADER)
    *fsSize = MK82_FLASH_FILE_SYSTEM_SIZE;
#endif
}

void bldrHalLoadImageData(uint8_t* data, uint32_t dataLength, uint32_t offset)
{
    if (data == NULL)
    {
        mk82SystemFatalError();
    }

    if ((offset + dataLength) > sizeof(dataBuffer))
    {
        mk82SystemFatalError();
    }

    mk82SystemMemCpy(&dataBuffer[offset], data, dataLength);
}

uint16_t bldrHalVerifyImageSignature(BLDR_GLOBAL_IMAGE_HEADER* imageHeader, uint32_t imageLength)
{
    uint16_t retVal = BLDR_GENERAL_ERROR;
    int caleeRetVal;
    mbedtls_sha256_context shaContext;
    mbedtls_ecdsa_context ecsdaContext;
    mbedtls_mpi r;
    mbedtls_mpi s;
    uint8_t one = 1;
#if defined(FIRMWARE)
    uint8_t header[sizeof(uint32_t) * 3];
#elif defined(BOOTLOADER)
    uint8_t header[sizeof(uint32_t) * 5];
#endif

    uint8_t imageHash[BLDR_HAL_SHA256_LENGTH];

    if (imageHeader == NULL)
    {
        mk82SystemFatalError();
    }
    mbedtls_sha256_init(&shaContext);
    mbedtls_ecdsa_init(&ecsdaContext);
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);

#if defined(FIRMWARE)
    header[0] = BLDR_HIBYTE(BLDR_HIWORD(BLDR_GLOBAL_IMAGE_TYPE_BOOTLOADER));
    header[1] = BLDR_LOBYTE(BLDR_HIWORD(BLDR_GLOBAL_IMAGE_TYPE_BOOTLOADER));
    header[2] = BLDR_HIBYTE(BLDR_LOWORD(BLDR_GLOBAL_IMAGE_TYPE_BOOTLOADER));
    header[3] = BLDR_LOBYTE(BLDR_LOWORD(BLDR_GLOBAL_IMAGE_TYPE_BOOTLOADER));
    header[4] = BLDR_HIBYTE(BLDR_HIWORD(imageHeader->deviceID));
    header[5] = BLDR_LOBYTE(BLDR_HIWORD(imageHeader->deviceID));
    header[6] = BLDR_HIBYTE(BLDR_LOWORD(imageHeader->deviceID));
    header[7] = BLDR_LOBYTE(BLDR_LOWORD(imageHeader->deviceID));
    header[8] = BLDR_HIBYTE(BLDR_HIWORD(imageHeader->bootloaderVersion));
    header[9] = BLDR_LOBYTE(BLDR_HIWORD(imageHeader->bootloaderVersion));
    header[10] = BLDR_HIBYTE(BLDR_LOWORD(imageHeader->bootloaderVersion));
    header[11] = BLDR_LOBYTE(BLDR_LOWORD(imageHeader->bootloaderVersion));
#elif defined(BOOTLOADER)
    header[0] = BLDR_HIBYTE(BLDR_HIWORD(BLDR_GLOBAL_IMAGE_TYPE_FIRMWARE));
    header[1] = BLDR_LOBYTE(BLDR_HIWORD(BLDR_GLOBAL_IMAGE_TYPE_FIRMWARE));
    header[2] = BLDR_HIBYTE(BLDR_LOWORD(BLDR_GLOBAL_IMAGE_TYPE_FIRMWARE));
    header[3] = BLDR_LOBYTE(BLDR_LOWORD(BLDR_GLOBAL_IMAGE_TYPE_FIRMWARE));
    header[4] = BLDR_HIBYTE(BLDR_HIWORD(imageHeader->deviceID));
    header[5] = BLDR_LOBYTE(BLDR_HIWORD(imageHeader->deviceID));
    header[6] = BLDR_HIBYTE(BLDR_LOWORD(imageHeader->deviceID));
    header[7] = BLDR_LOBYTE(BLDR_LOWORD(imageHeader->deviceID));
    header[8] = BLDR_HIBYTE(BLDR_HIWORD(imageHeader->firmwareVersion));
    header[9] = BLDR_LOBYTE(BLDR_HIWORD(imageHeader->firmwareVersion));
    header[10] = BLDR_HIBYTE(BLDR_LOWORD(imageHeader->firmwareVersion));
    header[11] = BLDR_LOBYTE(BLDR_LOWORD(imageHeader->firmwareVersion));
    header[12] = BLDR_HIBYTE(BLDR_HIWORD(imageHeader->fileSystemVersion));
    header[13] = BLDR_LOBYTE(BLDR_HIWORD(imageHeader->fileSystemVersion));
    header[14] = BLDR_HIBYTE(BLDR_LOWORD(imageHeader->fileSystemVersion));
    header[15] = BLDR_LOBYTE(BLDR_LOWORD(imageHeader->fileSystemVersion));
    header[16] = BLDR_HIBYTE(BLDR_HIWORD(imageHeader->bootloaderVersion));
    header[17] = BLDR_LOBYTE(BLDR_HIWORD(imageHeader->bootloaderVersion));
    header[18] = BLDR_HIBYTE(BLDR_LOWORD(imageHeader->bootloaderVersion));
    header[19] = BLDR_LOBYTE(BLDR_LOWORD(imageHeader->bootloaderVersion));
#endif

    mbedtls_sha256_starts(&shaContext, false);
    mbedtls_sha256_update(&shaContext, header, sizeof(header));
    mbedtls_sha256_update(&shaContext, dataBuffer, imageLength);
    mbedtls_sha256_finish(&shaContext, imageHash);

    caleeRetVal = mbedtls_ecp_group_load(&ecsdaContext.grp, MBEDTLS_ECP_DP_SECP256R1);

    if (caleeRetVal != 0)
    {
        mk82SystemFatalError();
    }

    mbedtls_mpi_read_binary(&r, imageHeader->signature, BLDR_HAL_R_AND_S_LENGTH);

    if (caleeRetVal != 0)
    {
        mk82SystemFatalError();
    }

    mbedtls_mpi_read_binary(&s, imageHeader->signature + BLDR_HAL_R_AND_S_LENGTH, BLDR_HAL_R_AND_S_LENGTH);

    if (caleeRetVal != 0)
    {
        mk82SystemFatalError();
    }

    caleeRetVal = mbedtls_mpi_read_binary(&ecsdaContext.Q.X, mk82BldrHalPublicKeyX, sizeof(mk82BldrHalPublicKeyX));

    if (caleeRetVal != 0)
    {
        mk82SystemFatalError();
    }

    caleeRetVal = mbedtls_mpi_read_binary(&ecsdaContext.Q.Y, mk82BldrHalPublicKeyY, sizeof(mk82BldrHalPublicKeyY));

    if (caleeRetVal != 0)
    {
        mk82SystemFatalError();
    }

    caleeRetVal = mbedtls_mpi_read_binary(&ecsdaContext.Q.Z, &one, sizeof(one));

    if (caleeRetVal != 0)
    {
        mk82SystemFatalError();
    }

    caleeRetVal = mbedtls_ecdsa_verify(&ecsdaContext.grp, imageHash, BLDR_HAL_SHA256_LENGTH, &ecsdaContext.Q, &r, &s);

    if (caleeRetVal != 0)
    {
        if (caleeRetVal == MBEDTLS_ERR_ECP_VERIFY_FAILED)
        {
            retVal = BLDR_SIGNATURE_VERIFICATION_FAILED_ERROR;
            goto END;
        }
        else
        {
            mk82SystemFatalError();
        }
    }

    retVal = MK82_NO_ERROR;

END:
    mbedtls_sha256_free(&shaContext);
    mbedtls_ecdsa_free(&ecsdaContext);
    mbedtls_mpi_free(&r);
    mbedtls_mpi_free(&s);
    return retVal;
}

void bldrHalWriteImageData(uint32_t imageLength)
{
    status_t caleeRetVal;
    uint32_t primask;
    uint32_t startAddress;
    uint32_t i;
    uint32_t numberOfSteps;

#if defined(FIRMWARE)
    startAddress = MK82_FLASH_BOOTLOADER_START;

    if (imageLength != MK82_FLASH_BOOTLOADER_SIZE)
    {
        mk82SystemFatalError();
    }
#elif defined(BOOTLOADER)
    startAddress = MK82_FLASH_FIRMWARE_START;

    if ((imageLength != MK82_FLASH_FIRMWARE_SIZE) &&
        (imageLength != (MK82_FLASH_FIRMWARE_SIZE + MK82_FLASH_FILE_SYSTEM_SIZE)))
    {
        mk82SystemFatalError();
    }
#endif

    numberOfSteps = imageLength / MK82_FLASH_PAGE_SIZE;

    for (i = 0; i < numberOfSteps; i++)
    {
        primask = DisableGlobalIRQ();

        caleeRetVal = FLASH_Erase(&mk82FlashDriver, startAddress + (i * MK82_FLASH_PAGE_SIZE), MK82_FLASH_PAGE_SIZE,
                                  kFLASH_apiEraseKey);

        if (caleeRetVal != kStatus_FLASH_Success)
        {
            mk82SystemFatalError();
        }

        EnableGlobalIRQ(primask);

        __ISB();
        __DSB();
    }

    for (i = 0; i < numberOfSteps; i++)
    {
        primask = DisableGlobalIRQ();

        caleeRetVal = FLASH_Program(&mk82FlashDriver, startAddress + (i * MK82_FLASH_PAGE_SIZE),
                                    (uint32_t*)(dataBuffer + (i * MK82_FLASH_PAGE_SIZE)), MK82_FLASH_PAGE_SIZE);

        if (caleeRetVal != kStatus_FLASH_Success)
        {
            mk82SystemFatalError();
        }

        EnableGlobalIRQ(primask);

        __ISB();
        __DSB();
    }
}

void bldrHalCalulateImageCRC32(uint32_t imageLength, uint32_t* crc32)
{
    crc_config_t crcConfig;

    if (crc32 == NULL)
    {
        mk82SystemFatalError();
    }

    CRC_GetDefaultConfig(&crcConfig);

    CRC_Init(CRC0, &crcConfig);
    CRC_WriteData(CRC0, dataBuffer, imageLength);
    *crc32 = CRC_Get32bitResult(CRC0);

    CRC_Deinit(CRC0);
}

void bldrHalCalulateFlashContentsCRC32(uint32_t contentslength, uint32_t* crc32)
{
    uint8_t* flashContentsStartAddress;
#if defined(FIRMWARE)
    flashContentsStartAddress = (uint8_t*)MK82_FLASH_BOOTLOADER_START;
#elif defined(BOOTLOADER)
    flashContentsStartAddress = (uint8_t*)MK82_FLASH_FIRMWARE_START;
#endif

    crc_config_t crcConfig;

    if (crc32 == NULL)
    {
        mk82SystemFatalError();
    }

    CRC_GetDefaultConfig(&crcConfig);

    CRC_Init(CRC0, &crcConfig);
    CRC_WriteData(CRC0, flashContentsStartAddress, contentslength);
    *crc32 = CRC_Get32bitResult(CRC0);

    CRC_Deinit(CRC0);
}

#ifdef FIRMWARE

void bldrHalEnableManufacturerBootloader(void)
{
    uint8_t pageContents[MK82_FLASH_PAGE_SIZE];
    uint8_t bootloaderEnabled[] = BLDR_HAL_MK82_BOOTLOADER_ENABLED;
    uint32_t primask;
    status_t calleeRetVal;
    uint32_t i;

    for (i = 0; i < MK82_FLASH_PAGE_SIZE; i++)
    {
        pageContents[i] = ((uint8_t*)BLDR_HAL_MK82_BOOTLOADER_SETTINGS_PAGE_ADDRESS)[i];
    }

    mk82SystemMemCpy(&pageContents[BLDR_HAL_MK82_BOOTLOADER_SETTINGS_PAGE_OFFSET], bootloaderEnabled,
                     sizeof(bootloaderEnabled));

    primask = DisableGlobalIRQ();

    calleeRetVal = FLASH_Erase(&mk82FlashDriver, BLDR_HAL_MK82_BOOTLOADER_SETTINGS_PAGE_ADDRESS, MK82_FLASH_PAGE_SIZE,
                               kFLASH_apiEraseKey);

    if (calleeRetVal != kStatus_FLASH_Success)
    {
        mk82SystemFatalError();
    }

    calleeRetVal = FLASH_Program(&mk82FlashDriver, BLDR_HAL_MK82_BOOTLOADER_SETTINGS_PAGE_ADDRESS,
                                 (uint32_t*)pageContents, MK82_FLASH_PAGE_SIZE);

    if (calleeRetVal != kStatus_FLASH_Success)
    {
        mk82SystemFatalError();
    }

    EnableGlobalIRQ(primask);
}

#endif /* FIRMWARE */

void bldrHalFatalError(void) { mk82SystemFatalError(); }
