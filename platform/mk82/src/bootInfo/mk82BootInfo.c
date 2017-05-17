/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "mk82Global.h"
#include "mk82GlobalInt.h"
#include "mk82System.h"
#include "mk82BootInfo.h"
#include "mk82BootInfoInt.h"

#include "fsl_crc.h"

#ifndef BOOTSTRAPPER

#include "fsl_flash.h"

#ifdef BOOTLOADER
const MK82_BOOT_INFO mk82BootInfoInitialState MK82_PLACE_IN_SECTION("BootInfo1") = {
    MK82_BOOT_INFO_FIRMWARE_VERSION,
    MK82_BOOT_INFO_FILE_SYSTEM_VERSION,
    MK82_BOOT_INFO_BOOTLOADER_VERSION,
    MK82_BOOT_INFO_START_FIRMWARE,
    MK82_TRUE,
    MK82_TRUE,
    MK82_FALSE,
    MK82_BOOT_INFO_INITIAL_STRUCTURE_CRC};
#endif /* BOOTLOADER */
#endif /* BOOTSTRAPPER */

MK82_BOOT_INFO* mk82BootInfoStructure1 = (MK82_BOOT_INFO*)MK82_BOOT_INFO_1_FLASH_AREA_START;
MK82_BOOT_INFO* mk82BootInfoStructure2 = (MK82_BOOT_INFO*)MK82_BOOT_INFO_2_FLASH_AREA_START;

#ifndef BOOTSTRAPPER
static void mk82BootInfoEraseStructure(MK82_BOOT_INFO* bootInfo);
static void mk82BootInfoProgramStructure(MK82_BOOT_INFO* address, MK82_BOOT_INFO* dataToProgram);
static void mk82BootInfoSetStructure(MK82_BOOT_INFO* structureToSet, MK82_BOOT_INFO* data);
#endif /* BOOTSTRAPPER */

static void mk82BootInfoCheckIfStructureIsErased(MK82_BOOT_INFO* bootInfo, uint16_t* structureIsErased);
static void mk82BootInfoCalculateStructureCRC(MK82_BOOT_INFO* bootInfo, uint32_t* crc32);
static void mk82BootInfoGetStructures(MK82_BOOT_INFO** structureToUse, MK82_BOOT_INFO** otherStructure);

static void mk82BootInfoCalculateStructureCRC(MK82_BOOT_INFO* bootInfo, uint32_t* crc32)
{
    crc_config_t crcConfig;

    CRC_GetDefaultConfig(&crcConfig);

    CRC_Init(CRC0, &crcConfig);
    CRC_WriteData(CRC0, (const uint8_t*)(bootInfo), sizeof(MK82_BOOT_INFO) - sizeof(uint32_t));
    *crc32 = CRC_Get32bitResult(CRC0);

    CRC_Deinit(CRC0);
}

static void mk82BootInfoGetStructures(MK82_BOOT_INFO** structureToUse, MK82_BOOT_INFO** otherStructure)
{
    uint32_t structure1CRC;
    uint32_t structure2CRC;

    mk82BootInfoCalculateStructureCRC(mk82BootInfoStructure1, &structure1CRC);
    mk82BootInfoCalculateStructureCRC(mk82BootInfoStructure2, &structure2CRC);

    if (structure1CRC == mk82BootInfoStructure1->crc)
    {
        *structureToUse = mk82BootInfoStructure1;
        *otherStructure = mk82BootInfoStructure2;
    }
    else if (structure2CRC == mk82BootInfoStructure2->crc)
    {
        *structureToUse = mk82BootInfoStructure2;
        *otherStructure = mk82BootInfoStructure1;
    }
    else
    {
        mk82SystemFatalError();
    }
}

#ifndef BOOTSTRAPPER

static void mk82BootInfoCheckIfStructureIsErased(MK82_BOOT_INFO* bootInfo, uint16_t* structureIsErased)
{
    status_t calleeRetVal;
    uint32_t primask;

    primask = DisableGlobalIRQ();

    calleeRetVal =
        FLASH_VerifyErase(&mk82FlashDriver, (uint32_t)bootInfo, MK82_FLASH_PAGE_SIZE, kFLASH_marginValueUser);

    if (calleeRetVal != kStatus_FLASH_Success)
    {
        *structureIsErased = MK82_FALSE;
    }
    else
    {
        *structureIsErased = MK82_TRUE;
    }

    EnableGlobalIRQ(primask);
}

static void mk82BootInfoEraseStructure(MK82_BOOT_INFO* bootInfo)
{
    status_t calleeRetVal;
    uint32_t primask;

    primask = DisableGlobalIRQ();

    calleeRetVal = FLASH_Erase(&mk82FlashDriver, (uint32_t)bootInfo, MK82_FLASH_PAGE_SIZE, kFLASH_apiEraseKey);

    if (calleeRetVal != kStatus_FLASH_Success)
    {
        mk82SystemFatalError();
    }

    EnableGlobalIRQ(primask);
}

static void mk82BootInfoProgramStructure(MK82_BOOT_INFO* address, MK82_BOOT_INFO* dataToProgram)
{
    status_t calleeRetVal;
    uint32_t primask;

    primask = DisableGlobalIRQ();

    calleeRetVal = FLASH_Program(&mk82FlashDriver, (uint32_t)address, (uint32_t*)dataToProgram, sizeof(MK82_BOOT_INFO));

    if (calleeRetVal != kStatus_FLASH_Success)
    {
        mk82SystemFatalError();
    }

    EnableGlobalIRQ(primask);
}

static void mk82BootInfoSetStructure(MK82_BOOT_INFO* structureToSet, MK82_BOOT_INFO* bootInfo)
{
    uint16_t structureIsErased = MK82_FALSE;

    mk82BootInfoCheckIfStructureIsErased(structureToSet, &structureIsErased);

    if (structureIsErased != MK82_TRUE)
    {
        mk82BootInfoEraseStructure(structureToSet);
    }

    mk82BootInfoProgramStructure(structureToSet, bootInfo);
}

void mk82BootInfoSetData(MK82_BOOT_INFO* bootInfo)
{
    MK82_BOOT_INFO* structureToUse;
    MK82_BOOT_INFO* otherStructure;
    uint32_t crc;

    if (bootInfo == NULL)
    {
        mk82SystemFatalError();
    }

    mk82BootInfoGetStructures(&structureToUse, &otherStructure);

    mk82BootInfoCalculateStructureCRC(bootInfo, &crc);

    bootInfo->crc = crc;

    mk82BootInfoSetStructure(otherStructure, bootInfo);
    mk82BootInfoEraseStructure(structureToUse);
}

#endif /* BOOTSTRAPPER */

void mk82BootInfoGetData(MK82_BOOT_INFO* bootInfo)
{
    MK82_BOOT_INFO* structureToUse;
    MK82_BOOT_INFO* otherStructure;

    if (bootInfo == NULL)
    {
        mk82SystemFatalError();
    }

    mk82BootInfoGetStructures(&structureToUse, &otherStructure);

    mk82SystemMemCpy((uint8_t*)bootInfo, (uint8_t*)structureToUse, sizeof(MK82_BOOT_INFO));
}

void mk82BootInfoBootFirmware(void)
{
    MK82_BOOT_INFO_START_IMAGE mk82BootInfoStartFirmware =
        (MK82_BOOT_INFO_START_IMAGE)(*(uint32_t*)(MK82_FLASH_FIRMWARE_START + 4));

    mk82BootInfoStartFirmware();
}

void mk82BootInfoBootBootloader(void)
{
    MK82_BOOT_INFO_START_IMAGE mk82BootInfoStartBootloader =
        (MK82_BOOT_INFO_START_IMAGE)(*(uint32_t*)(MK82_FLASH_BOOTLOADER_START + 4));

    mk82BootInfoStartBootloader();
}

void mk82BootInfoInit(void) {}
