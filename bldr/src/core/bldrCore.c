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
#include <bldrCore.h>
#include <bldrHal.h>
#include <core/bldrCoreInt.h>

#include <apduGlobal.h>
#include <apduCore.h>

static void bldrCoreInitContext(void);

static void bldrCoreProcessGetInfo(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
#ifdef BOOTLOADER
static void bldrCoreProcessSetFirmwareAsBootTarget(APDU_CORE_COMMAND_APDU* commandAPDU,
                                                   APDU_CORE_RESPONSE_APDU* responseAPDU);
#endif /* BOOTLOADER */
#ifdef FIRMWARE
static void bldrCoreProcessSetBootloaderAsBootTarget(APDU_CORE_COMMAND_APDU* commandAPDU,
                                                     APDU_CORE_RESPONSE_APDU* responseAPDU);
#endif /* FIRMWARE */
static void bldrCoreProcessSetImageInfo(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void bldrCoreProcessLoadImageData(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void bldrCoreProcessFinalizeLoading(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);

#ifdef FIRMWARE
static void bldrCoreProcessEnableManufacturerBootloader(APDU_CORE_COMMAND_APDU* commandAPDU,
                                                        APDU_CORE_RESPONSE_APDU* responseAPDU);
#endif /* FIRMWARE */

static BLDR_CORE_CONTEXT context;

void bldrCoreInit()
{
    bldrHalInit();
    bldrCoreInitContext();
}

void bldrCoreDeinit() { bldrHalDeinit(); }

#ifdef FIRMWARE

void bldrCoreGetAID(uint8_t* aid, uint32_t* aidLength)
{
    uint8_t aidTemplate[] = BLDR_CORE_AID;

    if ((aid == NULL) || (aidLength == NULL))
    {
        bldrHalFatalError();
    }

    bldrHalMemCpy(aid, aidTemplate, BLDR_CORE_AID_LENGTH);

    *aidLength = BLDR_CORE_AID_LENGTH;
}

#endif /* FIRMWARE */

static void bldrCoreInitContext(void)
{
    bldrHalMemSet((uint8_t*)&context, 0x00, sizeof(BLDR_CORE_CONTEXT));

    context.expectedCommand = BLDR_CORE_CONTEXT_EXPECTED_COMMAND_SET_IMAGE_INFO_OR_MISC_COMMANDS;
    context.fileSystemUpdateRequested = BLDR_FALSE;
}

static void bldrCoreProcessGetInfo(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint32_t firmwareVersion;
    uint32_t fileSystemVersion;
    uint32_t bootloaderVersion;
    uint32_t serialNumber;
    uint16_t fileSystemUpdateInterrupted;
    uint16_t firmwareValid;
    uint16_t bootloaderValid;

    if (commandAPDU->lcPresent != APDU_FALSE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != BLDR_CORE_P1P2_GET_INFO)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    if (context.expectedCommand != BLDR_CORE_CONTEXT_EXPECTED_COMMAND_SET_IMAGE_INFO_OR_MISC_COMMANDS)
    {
        sw = APDU_CORE_SW_CONDITIONS_NOT_SATISFIED;
        goto END;
    }

    bldrHalGetSerialNumber(&serialNumber);
    bldrHalGetFirmwareVersion(&firmwareVersion);
    bldrHalGetFileSystemVersion(&fileSystemVersion);
    bldrHalGetBootloaderVersion(&bootloaderVersion);
    bldrHalIsUpdateInterrupted(&fileSystemUpdateInterrupted);

#if defined(FIRMWARE)
    firmwareValid = BLDR_TRUE;
#elif defined(BOOTLOADER)
    bldrHalIsFirmwareValid(&firmwareValid);
#endif

#if defined(FIRMWARE)
    bldrHalIsBootloaderValid(&bootloaderValid);
#elif defined(BOOTLOADER)
    bootloaderValid = BLDR_TRUE;
#endif

    responseAPDU->data[0] = BLDR_HIBYTE(BLDR_HIWORD(DEVICE_ID));
    responseAPDU->data[1] = BLDR_LOBYTE(BLDR_HIWORD(DEVICE_ID));
    responseAPDU->data[2] = BLDR_HIBYTE(BLDR_LOWORD(DEVICE_ID));
    responseAPDU->data[3] = BLDR_LOBYTE(BLDR_LOWORD(DEVICE_ID));

    responseAPDU->data[4] = BLDR_HIBYTE(BLDR_HIWORD(serialNumber));
    responseAPDU->data[5] = BLDR_LOBYTE(BLDR_HIWORD(serialNumber));
    responseAPDU->data[6] = BLDR_HIBYTE(BLDR_LOWORD(serialNumber));
    responseAPDU->data[7] = BLDR_LOBYTE(BLDR_LOWORD(serialNumber));

    responseAPDU->data[8] = BLDR_HIBYTE(BLDR_HIWORD(firmwareVersion));
    responseAPDU->data[9] = BLDR_LOBYTE(BLDR_HIWORD(firmwareVersion));
    responseAPDU->data[10] = BLDR_HIBYTE(BLDR_LOWORD(firmwareVersion));
    responseAPDU->data[11] = BLDR_LOBYTE(BLDR_LOWORD(firmwareVersion));

    responseAPDU->data[12] = BLDR_HIBYTE(BLDR_HIWORD(fileSystemVersion));
    responseAPDU->data[13] = BLDR_LOBYTE(BLDR_HIWORD(fileSystemVersion));
    responseAPDU->data[14] = BLDR_HIBYTE(BLDR_LOWORD(fileSystemVersion));
    responseAPDU->data[15] = BLDR_LOBYTE(BLDR_LOWORD(fileSystemVersion));

    responseAPDU->data[16] = BLDR_HIBYTE(BLDR_HIWORD(bootloaderVersion));
    responseAPDU->data[17] = BLDR_LOBYTE(BLDR_HIWORD(bootloaderVersion));
    responseAPDU->data[18] = BLDR_HIBYTE(BLDR_LOWORD(bootloaderVersion));
    responseAPDU->data[19] = BLDR_LOBYTE(BLDR_LOWORD(bootloaderVersion));

    if (fileSystemUpdateInterrupted == BLDR_TRUE)
    {
        responseAPDU->data[10] = 1;
    }
    else
    {
        responseAPDU->data[20] = 0;
    }

    if (firmwareValid == BLDR_TRUE)
    {
        responseAPDU->data[21] = 1;
    }
    else
    {
        responseAPDU->data[21] = 0;
    }

    if (bootloaderValid == BLDR_TRUE)
    {
        responseAPDU->data[22] = 1;
    }
    else
    {
        responseAPDU->data[22] = 0;
    }

    responseAPDU->dataLength = 23;

    sw = APDU_CORE_SW_NO_ERROR;

END:

    if (sw != APDU_CORE_SW_NO_ERROR)
    {
        bldrCoreInitContext();
    }

    responseAPDU->sw = sw;
}

#ifdef BOOTLOADER
static void bldrCoreProcessSetFirmwareAsBootTarget(APDU_CORE_COMMAND_APDU* commandAPDU,
                                                   APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint16_t firmwareValid;

    if (commandAPDU->lcPresent != APDU_FALSE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != BLDR_CORE_P1P2_SET_FIRMWARE_AS_BOOT_TARGET)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    if (context.expectedCommand != BLDR_CORE_CONTEXT_EXPECTED_COMMAND_SET_IMAGE_INFO_OR_MISC_COMMANDS)
    {
        sw = APDU_CORE_SW_CONDITIONS_NOT_SATISFIED;
        goto END;
    }

    bldrHalIsFirmwareValid(&firmwareValid);

    if (firmwareValid == BLDR_TRUE)
    {
        bldrHalSetFirmwareAsBootTarget();
    }
    else
    {
        sw = APDU_CORE_SW_WRONG_DATA;
        goto END;
    }

    sw = APDU_CORE_SW_NO_ERROR;

END:

    if (sw != APDU_CORE_SW_NO_ERROR)
    {
        bldrCoreInitContext();
    }

    responseAPDU->sw = sw;
}
#endif /* BOOTLOADER */

#ifdef FIRMWARE
static void bldrCoreProcessSetBootloaderAsBootTarget(APDU_CORE_COMMAND_APDU* commandAPDU,
                                                     APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint16_t bootloaderValid;

    if (commandAPDU->lcPresent != APDU_FALSE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != BLDR_CORE_P1P2_SET_BOOTLOADER_AS_BOOT_TARGET)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    if (context.expectedCommand != BLDR_CORE_CONTEXT_EXPECTED_COMMAND_SET_IMAGE_INFO_OR_MISC_COMMANDS)
    {
        sw = APDU_CORE_SW_CONDITIONS_NOT_SATISFIED;
        goto END;
    }

    bldrHalIsBootloaderValid(&bootloaderValid);

    if (bootloaderValid == BLDR_TRUE)
    {
        bldrHalSetBootloaderAsBootTarget();
    }
    else
    {
        sw = APDU_CORE_SW_WRONG_DATA;
        goto END;
    }

    sw = APDU_CORE_SW_NO_ERROR;

END:

    if (sw != APDU_CORE_SW_NO_ERROR)
    {
        bldrCoreInitContext();
    }

    responseAPDU->sw = sw;
}
#endif /* FIRMWARE */

static void bldrCoreProcessSetImageInfo(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint32_t offset = 0;
    uint32_t codeSize;
    uint32_t fsSize;

    if (commandAPDU->lcPresent != APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != BLDR_CORE_P1P2_SET_IMAGE_INFO)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    if (context.expectedCommand != BLDR_CORE_CONTEXT_EXPECTED_COMMAND_SET_IMAGE_INFO_OR_MISC_COMMANDS)
    {
        sw = APDU_CORE_SW_CONDITIONS_NOT_SATISFIED;
        goto END;
    }

#if defined(FIRMWARE)

    if (commandAPDU->lc != (sizeof(uint32_t) + sizeof(uint32_t) + BLDR_GLOBAL_SIGNATURE_LENGTH))
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    context.imageHeader.deviceID =
        BLDR_MAKEDWORD(BLDR_MAKEWORD(commandAPDU->data[offset + 3], commandAPDU->data[offset + 2]),
                       BLDR_MAKEWORD(commandAPDU->data[offset + 1], commandAPDU->data[offset]));
    offset += 4;
    context.imageHeader.bootloaderVersion =
        BLDR_MAKEDWORD(BLDR_MAKEWORD(commandAPDU->data[offset + 3], commandAPDU->data[offset + 2]),
                       BLDR_MAKEWORD(commandAPDU->data[offset + 1], commandAPDU->data[offset]));
    offset += 4;

#elif defined(BOOTLOADER)

    if (commandAPDU->lc !=
        (sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + BLDR_GLOBAL_SIGNATURE_LENGTH))
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    context.imageHeader.deviceID =
        BLDR_MAKEDWORD(BLDR_MAKEWORD(commandAPDU->data[offset + 3], commandAPDU->data[offset + 2]),
                       BLDR_MAKEWORD(commandAPDU->data[offset + 1], commandAPDU->data[offset]));
    offset += 4;
    context.imageHeader.firmwareVersion =
        BLDR_MAKEDWORD(BLDR_MAKEWORD(commandAPDU->data[offset + 3], commandAPDU->data[offset + 2]),
                       BLDR_MAKEWORD(commandAPDU->data[offset + 1], commandAPDU->data[offset]));
    offset += 4;
    context.imageHeader.fileSystemVersion =
        BLDR_MAKEDWORD(BLDR_MAKEWORD(commandAPDU->data[offset + 3], commandAPDU->data[offset + 2]),
                       BLDR_MAKEWORD(commandAPDU->data[offset + 1], commandAPDU->data[offset]));
    offset += 4;
    context.imageHeader.bootloaderVersion =
        BLDR_MAKEDWORD(BLDR_MAKEWORD(commandAPDU->data[offset + 3], commandAPDU->data[offset + 2]),
                       BLDR_MAKEWORD(commandAPDU->data[offset + 1], commandAPDU->data[offset]));
    offset += 4;

#endif

    bldrHalMemCpy(context.imageHeader.signature, &commandAPDU->data[offset], BLDR_GLOBAL_SIGNATURE_LENGTH);

    bldrHalGetCodeSize(&codeSize);
    bldrHalGetFsSize(&fsSize);

    context.totalImageSize = codeSize + fsSize;

    context.expectedCommand = BLDR_CORE_CONTEXT_EXPECTED_COMMAND_LOAD_IMAGE_DATA;

    sw = APDU_CORE_SW_NO_ERROR;

END:

    if (sw != APDU_CORE_SW_NO_ERROR)
    {
        bldrCoreInitContext();
    }

    responseAPDU->sw = sw;
}

static void bldrCoreProcessLoadImageData(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;

    if (commandAPDU->lcPresent != APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != BLDR_CORE_P1P2_LOAD_IMAGE_DATA)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    if (context.expectedCommand != BLDR_CORE_CONTEXT_EXPECTED_COMMAND_LOAD_IMAGE_DATA)
    {
        sw = APDU_CORE_SW_CONDITIONS_NOT_SATISFIED;
        goto END;
    }

    if (commandAPDU->lc != BLDR_CORE_LOAD_IMAGE_DATA_PER_APDU)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if ((context.loadedImageSize + commandAPDU->lc) > context.totalImageSize)
    {
        sw = APDU_CORE_SW_WRONG_DATA;
        goto END;
    }

    bldrHalLoadImageData(commandAPDU->data, commandAPDU->lc, context.loadedImageSize);

    context.loadedImageSize += commandAPDU->lc;

    if (context.loadedImageSize == context.totalImageSize)
    {
        context.expectedCommand = BLDR_CORE_CONTEXT_EXPECTED_COMMAND_FINALIZE_LOADING;
    }

    sw = APDU_CORE_SW_NO_ERROR;

END:

    if (sw != APDU_CORE_SW_NO_ERROR)
    {
        bldrCoreInitContext();
    }

    responseAPDU->sw = sw;
}

static void bldrCoreProcessFinalizeLoading(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint16_t caleeRetVal;
    uint32_t bootloaderVersion;
    uint32_t firmwareVersion;
    uint32_t fileSystemVersion;
    uint16_t fileSystemUpdateRequested;
    uint32_t imageSizeToWrite;
    uint32_t codeSize;
    uint32_t fsSize;
    uint16_t updateInterrupted;
    uint32_t ramCRC32;
    uint32_t flashCRC32;

    if (commandAPDU->lcPresent != APDU_FALSE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 == BLDR_CORE_P1P2_FINALIZE_LOADING_DO_NOT_UPDATE_FILESYSTEM)
    {
        fileSystemUpdateRequested = BLDR_FALSE;
    }
#ifdef BOOTLOADER
    else if (commandAPDU->p1p2 == BLDR_CORE_P1P2_FINALIZE_LOADING_UPDATE_FILESYSTEM)
    {
        fileSystemUpdateRequested = BLDR_TRUE;
    }
#endif /* BOOTLOADER */
    else
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    if (context.expectedCommand != BLDR_CORE_CONTEXT_EXPECTED_COMMAND_FINALIZE_LOADING)
    {
        sw = APDU_CORE_SW_CONDITIONS_NOT_SATISFIED;
        goto END;
    }

    if (context.imageHeader.deviceID != DEVICE_ID)
    {
        sw = APDU_CORE_SW_WRONG_DATA;
        goto END;
    }

#if defined(FIRMWARE)

    bldrHalGetBootloaderVersion(&bootloaderVersion);

    if (context.imageHeader.bootloaderVersion < bootloaderVersion)
    {
        sw = APDU_CORE_SW_WRONG_DATA;
        goto END;
    }

#elif defined(BOOTLOADER)

    bldrHalGetBootloaderVersion(&bootloaderVersion);

    if (context.imageHeader.bootloaderVersion != bootloaderVersion)
    {
        sw = APDU_CORE_SW_WRONG_DATA;
        goto END;
    }

    bldrHalGetFirmwareVersion(&firmwareVersion);

    if (context.imageHeader.firmwareVersion < firmwareVersion)
    {
        sw = APDU_CORE_SW_WRONG_DATA;
        goto END;
    }

    bldrHalGetFileSystemVersion(&fileSystemVersion);

    if (context.imageHeader.fileSystemVersion < fileSystemVersion)
    {
        sw = APDU_CORE_SW_WRONG_DATA;
        goto END;
    }
    else if (context.imageHeader.fileSystemVersion > fileSystemVersion)
    {
        if (fileSystemUpdateRequested != BLDR_TRUE)
        {
            sw = APDU_CORE_SW_WRONG_DATA;
            goto END;
        }
    }

    bldrHalIsUpdateInterrupted(&updateInterrupted);

    if (updateInterrupted != BLDR_FALSE)
    {
        if (fileSystemUpdateRequested != BLDR_TRUE)
        {
            sw = APDU_CORE_SW_WRONG_DATA;
            goto END;
        }
    }

#endif

    bldrHalGetCodeSize(&codeSize);
    bldrHalGetFsSize(&fsSize);

    caleeRetVal = bldrHalVerifyImageSignature(&(context.imageHeader), context.loadedImageSize);

    if (caleeRetVal != BLDR_NO_ERROR)
    {
        if (caleeRetVal == BLDR_SIGNATURE_VERIFICATION_FAILED_ERROR)
        {
            sw = APDU_CORE_SW_WRONG_DATA;
            goto END;
        }
        else
        {
            bldrHalFatalError();
        }
    }

#ifdef BOOTLOADER
    if (fileSystemUpdateRequested == BLDR_TRUE)
    {
        imageSizeToWrite = codeSize + fsSize;
        bldrHalSetFileSystemUpdateInterruptedFlag(BLDR_TRUE);
    }
    else
#endif
    {
        imageSizeToWrite = codeSize;
    }

    bldrHalCalulateImageCRC32(imageSizeToWrite, &ramCRC32);

#if defined(FIRMWARE)
    bldrHalSetBootloaderValidityFlag(BLDR_FALSE);
    bldrHalWriteImageData(imageSizeToWrite);
#elif defined(BOOTLOADER)
    bldrHalSetFirmwareValidityFlag(BLDR_FALSE);
    bldrHalWriteImageData(imageSizeToWrite);
#endif

    bldrHalCalulateFlashContentsCRC32(imageSizeToWrite, &flashCRC32);

    if (ramCRC32 != flashCRC32)
    {
        bldrHalFatalError();
    }

#if defined(FIRMWARE)
    bldrHalSetBootloaderVersionAndValidate(context.imageHeader.bootloaderVersion);
#elif defined(BOOTLOADER)
    bldrHalClearUpdateInterruptedFlagSetVersionsAndValidateFirmware(context.imageHeader.firmwareVersion,
                                                                    context.imageHeader.fileSystemVersion);
#endif

    sw = APDU_CORE_SW_NO_ERROR;

END:

    bldrCoreInitContext();

    responseAPDU->sw = sw;
}

#ifdef FIRMWARE

static void bldrCoreProcessEnableManufacturerBootloader(APDU_CORE_COMMAND_APDU* commandAPDU,
                                                        APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;

    if (commandAPDU->lcPresent != APDU_FALSE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != BLDR_CORE_P1P2_ENABLE_MANUFACTURER_BOOTLOADER)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    bldrHalEnableManufacturerBootloader();

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

#endif /* FIRMWARE */

void bldrCoreProcessAPDU(uint8_t* apdu, uint32_t* apduLength)
{
    APDU_CORE_COMMAND_APDU commandAPDU;
    APDU_CORE_RESPONSE_APDU responseAPDU;
    uint16_t caleeRetVal = APDU_GENERAL_ERROR;

    if ((apdu == NULL) || (apduLength == NULL))
    {
        bldrHalFatalError();
    }

    apduCorePrepareResponseAPDUStructure(apdu, &responseAPDU);

    caleeRetVal = apduCoreParseIncomingAPDU(apdu, *apduLength, &commandAPDU);

    if (caleeRetVal != APDU_NO_ERROR)
    {
        if (caleeRetVal == APDU_GENERAL_ERROR)
        {
            responseAPDU.sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }
        else
        {
            bldrHalFatalError();
        }
    }

    if (commandAPDU.cla != BLDR_CORE_CLA)
    {
        responseAPDU.sw = APDU_CORE_SW_CLA_NOT_SUPPORTED;
        goto END;
    }

    switch (commandAPDU.ins)
    {
        case BLDR_CORE_INS_GET_INFO:
            bldrCoreProcessGetInfo(&commandAPDU, &responseAPDU);
            break;
#ifdef BOOTLOADER
        case BLDR_CORE_INS_SET_FIRMWARE_AS_BOOT_TARGET:
            bldrCoreProcessSetFirmwareAsBootTarget(&commandAPDU, &responseAPDU);
            break;
#endif /* BOOTLOADER */
        case BLDR_CORE_INS_SET_IMAGE_INFO:
            bldrCoreProcessSetImageInfo(&commandAPDU, &responseAPDU);
            break;
        case BLDR_CORE_INS_LOAD_IMAGE_DATA:
            bldrCoreProcessLoadImageData(&commandAPDU, &responseAPDU);
            break;
        case BLDR_CORE_INS_LOAD_FINALIZE_LOADING:
            bldrCoreProcessFinalizeLoading(&commandAPDU, &responseAPDU);
            break;
#ifdef FIRMWARE
        case BLDR_CORE_INS_SET_BOOTLOADER_AS_BOOT_TARGET:
            bldrCoreProcessSetBootloaderAsBootTarget(&commandAPDU, &responseAPDU);
            break;
        case BLDR_CORE_INS_ENABLE_MANUFACTURER_BOOTLOADER:
            bldrCoreProcessEnableManufacturerBootloader(&commandAPDU, &responseAPDU);
            break;
#endif /* FIRMWARE */
        default:
            responseAPDU.sw = APDU_CORE_SW_INS_NOT_SUPPORTED;
            break;
    }

END:

    if (responseAPDU.sw != APDU_CORE_SW_NO_ERROR)
    {
        bldrCoreInitContext();
    }

    apduCorePrepareOutgoingAPDU(apdu, apduLength, &responseAPDU);
}
