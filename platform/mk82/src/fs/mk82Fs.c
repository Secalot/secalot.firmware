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
#include "mk82Fs.h"
#include "mk82FsInt.h"

#include "fsl_device_registers.h"
#include "fsl_flash.h"

#include "uffs_config.h"
#include "uffs/uffs_public.h"
#include "uffs/uffs_fs.h"
#include "uffs/uffs_utils.h"
#include "uffs/uffs_core.h"
#include "uffs/uffs_mtb.h"
#include "uffs/uffs_fd.h"

#include "stddef.h"

static struct uffs_StorageAttrSt mk82FsFlashStorage;
static uffs_Device mk82FsDevice;

static struct uffs_MountTableEntrySt mk82FsMountTable = {&mk82FsDevice, 0, (MK82_FS_TOTAL_BLOCKS - 1), "/", NULL};

static int
    mk82FsStaticMemoryPool[UFFS_STATIC_BUFF_SIZE(MK82_FS_PAGES_PER_BLOCK, MK82_FS_PAGE_SIZE, MK82_FS_TOTAL_BLOCKS) /
                           sizeof(int)];
static uint8_t mk82FsPageBuffer[MK82_FS_PAGE_SIZE];

static void mk82FsFatalError(void);
static int mk82FsReadPage(uffs_Device *dev, u32 block, u32 page, u8 *data, int data_len, u8 *ecc, u8 *spare,
                          int spare_len);
static int mk82FsWritePage(uffs_Device *dev, u32 block, u32 page, const u8 *data, int data_len, const u8 *spare,
                           int spare_len);
static int mk82FsEraseBlock(uffs_Device *dev, u32 block);
static int mk82FsInitFlash(uffs_Device *dev);
static int mk82FsReleaseFlash(uffs_Device *dev);
static URET mk82FsInitDevice(uffs_Device *dev);
static URET mk82FsReleaseDevice(uffs_Device *dev);
static int mk82FsCheckErasedBlock(uffs_Device *dev, u32 block);

static void mk82FsGetFileHandleAndOffset(uint8_t fileID, int *fileHandle, uint32_t *fileOffset);

static int mk82FsCountersFileHandle;
static int mk82FsCertificatesFileHandle;
static int mk82FsKeysFileHandle;
static int mk82FsDataFileHandle;

static uffs_FlashOps mk82FsFunctionPointers = {mk82FsInitFlash,     // InitFlash()
                                               mk82FsReleaseFlash,  // ReleaseFlash()
                                               mk82FsReadPage,      // ReadPage()
                                               NULL,                // ReadPageWithLayout
                                               mk82FsWritePage,     // WritePage()
                                               NULL,                // WirtePageWithLayout
                                               NULL,                // IsBadBlock(), let UFFS take care of it.
                                               NULL,                // MarkBadBlock(), let UFFS take care of it.
                                               mk82FsEraseBlock,    // EraseBlock()
                                               mk82FsCheckErasedBlock};

static void mk82FsOpenAndCheckAllFiles(void);

static void mk82FsFatalError(void) { mk82SystemFatalError(); }

static int mk82FsReadPage(uffs_Device *dev, u32 block, u32 page, u8 *data, int data_len, u8 *ecc, u8 *spare,
                          int spare_len)
{
    int ret = UFFS_FLASH_NO_ERR;

    if (data && data_len > 0)
    {
        uint32_t flashAddress;

        flashAddress = MK82_FLASH_FILE_SYSTEM_START + (block * MK82_FS_BLOCK_SIZE) + (page * MK82_FS_PAGE_SIZE);

        if (data_len > MK82_FS_PAGE_DATA_SIZE)
        {
            mk82FsFatalError();
        }

        mk82SystemMemCpy(data, (uint8_t *)flashAddress, data_len);
    }

    if (spare && spare_len > 0)
    {
        uint32_t flashAddress;

        flashAddress = MK82_FLASH_FILE_SYSTEM_START + (block * MK82_FS_BLOCK_SIZE) + (page * MK82_FS_PAGE_SIZE) +
                       MK82_FS_PAGE_DATA_SIZE;

        if (spare_len > MK82_FS_PAGE_SPARE_SIZE)
        {
            mk82FsFatalError();
        }

        mk82SystemMemCpy(spare, (uint8_t *)flashAddress, spare_len);
    }

    if (data == NULL && spare == NULL)
    {
        // Read bad block mark
    }

    return ret;
}

static int mk82FsWritePage(uffs_Device *dev, u32 block, u32 page, const u8 *data, int data_len, const u8 *spare,
                           int spare_len)
{
    int ret = UFFS_FLASH_NO_ERR;
    uint32_t primask;

    if (data && data_len > 0 && spare && spare_len > 0)
    {
        uint32_t flashAddress;
        status_t result;
        uint32_t failAddr, failDat;

        flashAddress = MK82_FLASH_FILE_SYSTEM_START + (block * MK82_FS_BLOCK_SIZE) + (page * MK82_FS_PAGE_SIZE);

        if (data_len != MK82_FS_PAGE_DATA_SIZE)
        {
            mk82FsFatalError();
        }

        if (spare_len > MK82_FS_PAGE_SPARE_SIZE)
        {
            mk82FsFatalError();
        }

        mk82SystemMemSet(mk82FsPageBuffer, 0x00, sizeof(mk82FsPageBuffer));
        mk82SystemMemCpy(mk82FsPageBuffer, (uint8_t *)data, data_len);
        mk82SystemMemCpy(mk82FsPageBuffer + MK82_FS_PAGE_DATA_SIZE, (uint8_t *)spare, spare_len);

        primask = DisableGlobalIRQ();

        result = FLASH_Program(&mk82FlashDriver, flashAddress, (uint32_t *)mk82FsPageBuffer, MK82_FS_PAGE_SIZE);

        if (kStatus_FLASH_Success != result)
        {
            mk82FsFatalError();
        }

        result = FLASH_VerifyProgram(&mk82FlashDriver, flashAddress, MK82_FS_PAGE_SIZE, (uint32_t *)mk82FsPageBuffer,
                                     kFLASH_marginValueUser, &failAddr, &failDat);

        if (kStatus_FLASH_Success != result)
        {
            mk82FsFatalError();
        }

        EnableGlobalIRQ(primask);
    }
    else
    {
        mk82FsFatalError();
    }

    return ret;
}

static int mk82FsEraseBlock(uffs_Device *dev, u32 block)
{
    int ret = UFFS_FLASH_NO_ERR;
    uint32_t flashAddress;
    status_t result;
    uint32_t primask;

    flashAddress = MK82_FLASH_FILE_SYSTEM_START + (block * MK82_FS_BLOCK_SIZE);

    primask = DisableGlobalIRQ();

    result = FLASH_Erase(&mk82FlashDriver, flashAddress, MK82_FS_BLOCK_SIZE, kFLASH_apiEraseKey);
    if (kStatus_FLASH_Success != result)
    {
        mk82FsFatalError();
    }

    /* Verify sector if it's been erased. */
    result = FLASH_VerifyErase(&mk82FlashDriver, flashAddress, MK82_FS_BLOCK_SIZE, kFLASH_marginValueUser);

    if (kStatus_FLASH_Success != result)
    {
        mk82FsFatalError();
    }

    EnableGlobalIRQ(primask);

    return ret;
}

static int mk82FsCheckErasedBlock(uffs_Device *dev, u32 block)
{
    int ret = UFFS_FLASH_NO_ERR;
    uint32_t flashAddress;
    status_t result;
    uint32_t primask;

    flashAddress = MK82_FLASH_FILE_SYSTEM_START + (block * MK82_FS_BLOCK_SIZE);

    primask = DisableGlobalIRQ();

    /* Verify sector if it's been erased. */
    result = FLASH_VerifyErase(&mk82FlashDriver, flashAddress, MK82_FS_BLOCK_SIZE, kFLASH_marginValueUser);

    if (kStatus_FLASH_Success != result)
    {
        ret = -1;
    }

    EnableGlobalIRQ(primask);

    return ret;
}

static int mk82FsInitFlash(uffs_Device *dev)
{
    int ret = UFFS_FLASH_NO_ERR;

    return ret;
}

static int mk82FsReleaseFlash(uffs_Device *dev)
{
    int ret = UFFS_FLASH_NO_ERR;

    return ret;
}

static URET mk82FsInitDevice(uffs_Device *dev)
{
    dev->attr = &mk82FsFlashStorage;     // NAND flash attributes
    dev->attr->_private = (void *)NULL;  // hook nand_chip data structure to attr->_private
    dev->ops = &mk82FsFunctionPointers;  // NAND driver

    return U_SUCC;
}

static URET mk82FsReleaseDevice(uffs_Device *dev) { return U_SUCC; }

static void mk82FsOpenAndCheckAllFiles(void)
{
    mk82FsCountersFileHandle = uffs_open("/1", UO_RDWR);

    if (mk82FsCountersFileHandle < 0)
    {
        mk82FsFatalError();
    }

    mk82FsCertificatesFileHandle = uffs_open("/2", UO_RDWR);

    if (mk82FsCertificatesFileHandle < 0)
    {
        mk82FsFatalError();
    }

    mk82FsKeysFileHandle = uffs_open("/3", UO_RDWR);

    if (mk82FsKeysFileHandle < 0)
    {
        mk82FsFatalError();
    }

    mk82FsDataFileHandle = uffs_open("/4", UO_RDWR);

    if (mk82FsDataFileHandle < 0)
    {
        mk82FsFatalError();
    }
}

static void mk82FsGetFileHandleAndOffset(uint8_t fileID, int *fileHandle, uint32_t *fileOffset)
{
    if (fileID == MK82_FS_FILE_ID_OPGP_COUNTERS)
    {
        *fileHandle = mk82FsCountersFileHandle;
        *fileOffset = offsetof(MK82_FS_COUNTERS, opgpCounters);
    }
    else if (fileID == MK82_FS_FILE_ID_OPGP_CERTIFICATES)
    {
        *fileHandle = mk82FsCertificatesFileHandle;
        *fileOffset = offsetof(MK82_FS_CERTIFICATES, opgpCertificates);
    }
    else if (fileID == MK82_FS_FILE_ID_OPGP_KEYS)
    {
        *fileHandle = mk82FsKeysFileHandle;
        *fileOffset = offsetof(MK82_FS_KEYS, opgpKeys);
    }
    else if (fileID == MK82_FS_FILE_ID_OPGP_DATA)
    {
        *fileHandle = mk82FsDataFileHandle;
        *fileOffset = offsetof(MK82_FS_DATA, opgpData);
    }
    else if (fileID == MK82_FS_FILE_ID_SF_COUNTERS)
    {
        *fileHandle = mk82FsCountersFileHandle;
        *fileOffset = offsetof(MK82_FS_COUNTERS, sfCounters);
    }
    else if (fileID == MK82_FS_FILE_ID_KEYSAFE_DATA)
    {
        *fileHandle = mk82FsDataFileHandle;
        *fileOffset = offsetof(MK82_FS_DATA, keysafeData);
    }
    else if (fileID == MK82_FS_FILE_ID_OTP_COUNTERS)
    {
        *fileHandle = mk82FsCountersFileHandle;
        *fileOffset = offsetof(MK82_FS_COUNTERS, otpCounters);
    }
    else if (fileID == MK82_FS_FILE_ID_OTP_KEYS)
    {
        *fileHandle = mk82FsKeysFileHandle;
        *fileOffset = offsetof(MK82_FS_KEYS, otpKeys);
    }
    else if (fileID == MK82_FS_FILE_ID_OTP_DATA)
    {
        *fileHandle = mk82FsDataFileHandle;
        *fileOffset = offsetof(MK82_FS_DATA, otpData);
    }
    else if (fileID == MK82_FS_FILE_ID_BTC_COUNTERS)
    {
        *fileHandle = mk82FsCountersFileHandle;
        *fileOffset = offsetof(MK82_FS_COUNTERS, btcCounters);
    }
    else if (fileID == MK82_FS_FILE_ID_BTC_KEYS)
    {
        *fileHandle = mk82FsKeysFileHandle;
        *fileOffset = offsetof(MK82_FS_KEYS, btcKeys);
    }
    else if (fileID == MK82_FS_FILE_ID_BTC_DATA)
    {
        *fileHandle = mk82FsDataFileHandle;
        *fileOffset = offsetof(MK82_FS_DATA, btcData);
    }
    else if (fileID == MK82_FS_FILE_ID_ETH_COUNTERS)
    {
        *fileHandle = mk82FsCountersFileHandle;
        *fileOffset = offsetof(MK82_FS_COUNTERS, ethCounters);
    }
    else if (fileID == MK82_FS_FILE_ID_ETH_KEYS)
    {
        *fileHandle = mk82FsKeysFileHandle;
        *fileOffset = offsetof(MK82_FS_KEYS, ethKeys);
    }
    else if (fileID == MK82_FS_FILE_ID_ETH_DATA)
    {
        *fileHandle = mk82FsDataFileHandle;
        *fileOffset = offsetof(MK82_FS_DATA, ethData);
    }
    else if (fileID == MK82_FS_FILE_ID_SSL_KEYS)
    {
        *fileHandle = mk82FsKeysFileHandle;
        *fileOffset = offsetof(MK82_FS_KEYS, sslKeys);
    }
    else if (fileID == MK82_FS_FILE_ID_XRP_COUNTERS)
    {
        *fileHandle = mk82FsCountersFileHandle;
        *fileOffset = offsetof(MK82_FS_COUNTERS, xrpCounters);
    }
    else if (fileID == MK82_FS_FILE_ID_XRP_KEYS)
    {
        *fileHandle = mk82FsKeysFileHandle;
        *fileOffset = offsetof(MK82_FS_KEYS, xrpKeys);
    }
    else if (fileID == MK82_FS_FILE_ID_XRP_DATA)
    {
        *fileHandle = mk82FsDataFileHandle;
        *fileOffset = offsetof(MK82_FS_DATA, xrpData);
    }
    else
    {
        mk82FsFatalError();
    }
}

void mk82FsReadFile(uint8_t fileID, uint32_t offset, uint8_t *buffer, uint32_t length)
{
    int calleeRetVal;
    int fileHandle;
    uint32_t fileOffset;

    if (buffer == NULL)
    {
        mk82FsFatalError();
    }

    mk82FsGetFileHandleAndOffset(fileID, &fileHandle, &fileOffset);

    offset += fileOffset;

    calleeRetVal = uffs_seek(fileHandle, offset, USEEK_SET);

    if (calleeRetVal != offset)
    {
        mk82FsFatalError();
    }

    calleeRetVal = uffs_read(fileHandle, buffer, length);

    if (calleeRetVal != length)
    {
        mk82FsFatalError();
    }
}

void mk82FsWriteFile(uint8_t fileID, uint32_t offset, uint8_t *buffer, uint32_t length)
{
    int calleeRetVal;
    int fileHandle;
    uint32_t fileOffset;

    if (buffer == NULL)
    {
        mk82FsFatalError();
    }

    mk82FsGetFileHandleAndOffset(fileID, &fileHandle, &fileOffset);

    offset += fileOffset;

    calleeRetVal = uffs_seek(fileHandle, offset, USEEK_SET);

    if (calleeRetVal != offset)
    {
        mk82FsFatalError();
    }

    calleeRetVal = uffs_write(fileHandle, buffer, length);

    if (calleeRetVal < 0)
    {
        mk82FsFatalError();
    }
}

void mk82FsCommitWrite(uint8_t fileID)
{
    int calleeRetVal;
    int fileHandle;
    uint32_t fileOffset;

    mk82FsGetFileHandleAndOffset(fileID, &fileHandle, &fileOffset);

    calleeRetVal = uffs_flush(fileHandle);

    if (calleeRetVal < 0)
    {
        mk82FsFatalError();
    }
}

#if 0
void mk82FsCreateFileSystem(void)
{
	int calleeRetVal;
	
	MK82_FS_COUNTERS counters;
	MK82_FS_CERTIFICATES certificates;
	MK82_FS_KEYS keys;
	MK82_FS_DATA data;
	
	calleeRetVal = uffs_format("/");
	
	if(calleeRetVal < 0)
	{
		mk82FsFatalError();
	}	

	mk82FsCountersFileHandle = uffs_open("/1", UO_RDWR|UO_CREATE);

	if(mk82FsCountersFileHandle < 0)
	{
		mk82FsFatalError();
	} 
	
	mk82FsCertificatesFileHandle = uffs_open("/2", UO_RDWR|UO_CREATE);

	if(mk82FsCertificatesFileHandle < 0)
	{
		mk82FsFatalError();
	} 

	mk82FsKeysFileHandle = uffs_open("/3", UO_RDWR|UO_CREATE);

	if(mk82FsKeysFileHandle < 0)
	{
		mk82FsFatalError();
	} 

	mk82FsDataFileHandle = uffs_open("/4", UO_RDWR|UO_CREATE);

	if(mk82FsDataFileHandle < 0)
	{
		mk82FsFatalError();
	}
	
	mk82SystemMemSet((uint8_t*)&counters, 0x00, sizeof(counters));
	mk82SystemMemSet((uint8_t*)&certificates, 0x00, sizeof(certificates));
	mk82SystemMemSet((uint8_t*)&keys, 0x00, sizeof(keys));
	mk82SystemMemSet((uint8_t*)&data, 0x00, sizeof(data));
	
	calleeRetVal = uffs_write(mk82FsCountersFileHandle, &counters, sizeof(counters));
	
	if(calleeRetVal < 0)
	{
		mk82FsFatalError();
	}
	
	calleeRetVal = uffs_flush(mk82FsCountersFileHandle);
	
	if(calleeRetVal < 0)
	{
		mk82FsFatalError();
	}		
	
	calleeRetVal = uffs_write(mk82FsCertificatesFileHandle, &certificates, sizeof(certificates));
	
	if(calleeRetVal < 0)
	{
		mk82FsFatalError();
	}
	
	calleeRetVal = uffs_flush(mk82FsCertificatesFileHandle);
	
	if(calleeRetVal < 0)
	{
		mk82FsFatalError();
	}			
	
	calleeRetVal = uffs_write(mk82FsKeysFileHandle, &keys, sizeof(keys));
	
	if(calleeRetVal < 0)
	{
		mk82FsFatalError();
	}
	
	calleeRetVal = uffs_flush(mk82FsKeysFileHandle);
	
	if(calleeRetVal < 0)
	{
		mk82FsFatalError();
	}				
	
	calleeRetVal = uffs_write(mk82FsDataFileHandle, &data, sizeof(data));
	
	if(calleeRetVal < 0)
	{
		mk82FsFatalError();
	}
	
	calleeRetVal = uffs_flush(mk82FsDataFileHandle);
	
	if(calleeRetVal < 0)
	{
		mk82FsFatalError();
	}			
	
	uffs_close(mk82FsCountersFileHandle);
	
	if(calleeRetVal < 0)
	{
		mk82FsFatalError();
	}	
	
	uffs_close(mk82FsCertificatesFileHandle);
	
	if(calleeRetVal < 0)
	{
		mk82FsFatalError();
	}		
	
	uffs_close(mk82FsKeysFileHandle);
	
	if(calleeRetVal < 0)
	{
		mk82FsFatalError();
	}	
	
	uffs_close(mk82FsDataFileHandle);
	
	if(calleeRetVal < 0)
	{
		mk82FsFatalError();
	}
	
	mk82FsOpenAndCheckAllFiles();
	
	opgpHalWipeout();
	mk82KeysafeWipeout();
	sfHalWipeout();
	otpHalWipeout();
	btcHalWipeout();
	ethHalWipeout();
	
}
#endif

void mk82FsInit(void)
{
    URET calleeRetVal;

    uffs_SetupDebugOutput();

    mk82SystemMemSet((uint8_t *)&mk82FsFlashStorage, 0, sizeof(struct uffs_StorageAttrSt));

    // setup NAND flash attributes.
    mk82FsFlashStorage.total_blocks = MK82_FS_TOTAL_BLOCKS;       /* total blocks */
    mk82FsFlashStorage.page_data_size = MK82_FS_PAGE_DATA_SIZE;   /* page data size */
    mk82FsFlashStorage.pages_per_block = MK82_FS_PAGES_PER_BLOCK; /* pages per block */
    mk82FsFlashStorage.spare_size = MK82_FS_PAGE_SPARE_SIZE;      /* page spare size */
    mk82FsFlashStorage.block_status_offs = 4;                     /* block status offset is 5th byte in spare */
    mk82FsFlashStorage.ecc_opt = UFFS_ECC_NONE;                   /* ecc option */
    mk82FsFlashStorage.layout_opt = UFFS_LAYOUT_UFFS;             /* let UFFS do the spare layout */

    /* setup memory allocator */
    uffs_MemSetupStaticAllocator(&mk82FsMountTable.dev->mem, mk82FsStaticMemoryPool, sizeof(mk82FsStaticMemoryPool));

    /* register mount table */
    mk82FsMountTable.dev->Init = mk82FsInitDevice;
    mk82FsMountTable.dev->Release = mk82FsReleaseDevice;

    uffs_RegisterMountTable(&mk82FsMountTable);

    uffs_Mount("/");

    calleeRetVal = uffs_InitFileSystemObjects();

    if (calleeRetVal != U_SUCC)
    {
        mk82FsFatalError();
    }

#if 0
	mk82FsCreateFileSystem();
	while(1){}
#endif

    mk82FsOpenAndCheckAllFiles();
}
