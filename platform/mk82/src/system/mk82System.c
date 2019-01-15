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
#include "mk82SystemInt.h"

#include "string.h"

#ifndef BOOTSTRAPPER

#include "fsl_ltc.h"
#include "fsl_pit.h"
#include "fsl_trng.h"
#include "fsl_flash.h"

#ifdef FIRMWARE
#include "mbedtls/ctr_drbg.h"
#endif /* FIRMWARE */

#include "mbedtls/sha256.h"

flash_config_t mk82FlashDriver;

#ifdef FIRMWARE
static uint32_t mk82SystemTickerPeriodsElapsed;
#endif /* FIRMWARE */

#ifdef FIRMWARE
static mbedtls_ctr_drbg_context mk82SystemCtrDrbg;
#endif /* FIRMWARE */

#ifdef FIRMWARE
static int mk82SystemHardwareRNGCallback(void* param, unsigned char* buffer, size_t bufferLength);
#endif /* FIRMWARE */

#ifdef FIRMWARE
static int mk82SystemHardwareRNGCallback(void* param, unsigned char* buffer, size_t bufferLength)
{
    status_t calleeRetVal;

    calleeRetVal = TRNG_GetRandomData(TRNG0, buffer, bufferLength);

    if (calleeRetVal != kStatus_Success)
    {
        mk82SystemFatalError();
    }

    return 0;
}
#endif /* FIRMWARE */

void mk82SystemInit(void)
{
    status_t calleeRetVal;
    trng_config_t trngConfig;
    uint32_t randomNumber;

    pit_config_t pitConfig;

    LTC_Init(LTC0);

    calleeRetVal = TRNG_GetDefaultConfig(&trngConfig);

    if (calleeRetVal != kStatus_Success)
    {
        mk82SystemFatalError();
    }

    calleeRetVal = TRNG_Init(TRNG0, &trngConfig);

    if (calleeRetVal != kStatus_Success)
    {
        mk82SystemFatalError();
    }

    calleeRetVal = TRNG_GetRandomData(TRNG0, &randomNumber, sizeof(randomNumber));

    if (calleeRetVal != kStatus_Success)
    {
        mk82SystemFatalError();
    }

    LTC_SetDpaMaskSeed(LTC0, randomNumber);

#ifdef FIRMWARE
    mbedtls_ctr_drbg_init(&mk82SystemCtrDrbg);

    calleeRetVal = mbedtls_ctr_drbg_seed(&mk82SystemCtrDrbg, mk82SystemHardwareRNGCallback, NULL, NULL, 0);
#endif /* FIRMWARE */

    if (calleeRetVal != kStatus_Success)
    {
        mk82SystemFatalError();
    }

    PIT_GetDefaultConfig(&pitConfig);
    PIT_Init(PIT0, &pitConfig);

#ifdef FIRMWARE
    mk82SystemTickerPeriodsElapsed = 0;
    PIT_SetTimerPeriod(PIT0, kPIT_Chnl_3,
                       MSEC_TO_COUNT(MK82_SYSTEM_TICKER_INTERRUPT_PERIOD_IN_MS, CLOCK_GetFreq(kCLOCK_BusClk)));
    PIT_EnableInterrupts(PIT0, kPIT_Chnl_3, kPIT_TimerInterruptEnable);
    NVIC_EnableIRQ(PIT3_IRQn);
    PIT_StartTimer(PIT0, kPIT_Chnl_3);
#endif /* FIRMWARE */

    mk82SystemMemSet((uint8_t*)&mk82FlashDriver, 0, sizeof(flash_config_t));

    calleeRetVal = FLASH_Init(&mk82FlashDriver);
    if (calleeRetVal != kStatus_FLASH_Success)
    {
        mk82SystemFatalError();
    }
}

#ifdef FIRMWARE

void PIT3_IRQHandler(void)
{
    PIT_ClearStatusFlags(PIT0, kPIT_Chnl_3, PIT_TFLG_TIF_MASK);
    mk82SystemTickerPeriodsElapsed++;
}

void mk82SystemTickerGetMsPassed(uint64_t* ms)
{
    if (ms == NULL)
    {
        mk82SystemFatalError();
    }

    while (1)
    {
        uint32_t currentTimerCount;
        uint32_t tickerPeriodsElapsedBackup;

        tickerPeriodsElapsedBackup = mk82SystemTickerPeriodsElapsed;

        currentTimerCount = PIT_GetCurrentTimerCount(PIT0, kPIT_Chnl_3);

        *ms = tickerPeriodsElapsedBackup * MK82_SYSTEM_TICKER_INTERRUPT_PERIOD_IN_MS +
              MK82_SYSTEM_TICKER_INTERRUPT_PERIOD_IN_MS -
              COUNT_TO_MSEC(currentTimerCount, CLOCK_GetFreq(kCLOCK_BusClk));

        if (tickerPeriodsElapsedBackup == mk82SystemTickerPeriodsElapsed)
        {
            break;
        }
    }
}

void mk82SystemGetRandom(uint8_t* buffer, uint32_t bufferLength)
{
    int calleeRetVal;

    if (buffer == NULL)
    {
        mk82SystemFatalError();
    }

    calleeRetVal = mbedtls_ctr_drbg_random(&mk82SystemCtrDrbg, buffer, bufferLength);

    if (calleeRetVal != kStatus_Success)
    {
        mk82SystemFatalError();
    }
}

int mk82SystemGetRandomForTLS(void* param, unsigned char* buffer, size_t bufferLength)
{
    int calleeRetVal;

    if (buffer == NULL)
    {
        mk82SystemFatalError();
    }

    calleeRetVal = mbedtls_ctr_drbg_random(&mk82SystemCtrDrbg, buffer, bufferLength);

    if (calleeRetVal != kStatus_Success)
    {
        mk82SystemFatalError();
    }

    return 0;
}

#endif /* FIRMWARE */

void mk82SystemGetSerialNumber(uint32_t* serialNumber)
{
    mbedtls_sha256_context shaContext;
    uint8_t hash[32];
    uint8_t hash2[32];

    mbedtls_sha256_init(&shaContext);
    mbedtls_sha256_starts(&shaContext, false);
    mbedtls_sha256_update(&shaContext, (uint8_t*)&SIM->UIDL, sizeof(uint32_t));
    mbedtls_sha256_update(&shaContext, (uint8_t*)&SIM->UIDML, sizeof(uint32_t));
    mbedtls_sha256_update(&shaContext, (uint8_t*)&SIM->UIDMH, sizeof(uint32_t));
    mbedtls_sha256_update(&shaContext, (uint8_t*)&SIM->UIDH, sizeof(uint32_t));
    mbedtls_sha256_finish(&shaContext, hash);
    mbedtls_sha256_free(&shaContext);

    mbedtls_sha256_init(&shaContext);
    mbedtls_sha256_update(&shaContext, hash, sizeof(hash));
    mbedtls_sha256_finish(&shaContext, hash2);
    mbedtls_sha256_free(&shaContext);

    mk82SystemMemSet(hash, 0x00, sizeof(hash));

    *serialNumber = *(uint32_t*)hash2;
}

#endif /* BOOTSTRAPPER */

void mk82SystemMemCpy(uint8_t* dst, uint8_t* src, uint16_t length)
{
    if ((dst == NULL) || (src == NULL))
    {
        mk82SystemFatalError();
    }

    memmove((void*)dst, (void*)src, length);
}

void mk82SystemMemSet(uint8_t* dst, uint8_t value, uint16_t length)
{
    if (dst == NULL)
    {
        mk82SystemFatalError();
    }

    memset((void*)dst, value, length);
}

BEGIN_UNOPTIMIZED_FUNCTION

uint16_t mk82SystemMemCmp(uint8_t* array1, uint8_t* array2, uint16_t length)
{
    uint32_t i;
    uint8_t result = 0;

    if ((array1 == NULL) || (array2 == NULL))
    {
        mk82SystemFatalError();
    }

    for (i = 0; i < length; i++)
    {
        result |= array1[i] ^ array2[i];
    }

    if (result)
    {
        return MK82_CMP_NOT_EQUAL;
    }
    else
    {
        return MK82_CMP_EQUAL;
    }
}

END_UNOPTIMIZED_FUNCTION

void mk82SystemFatalError(void)
{
    while (1)
    {
    }
}

#ifdef FIRMWARE
BEGIN_UNOPTIMIZED_FUNCTION
void* _sbrk(ptrdiff_t incr)
{
    extern char end asm("end");
    extern char __heap_limit asm("__heap_limit");
    static char* heap_end;
    char* prev_heap_end;

    if (heap_end == NULL) heap_end = &end;

    prev_heap_end = heap_end;

    if (heap_end + incr > (char*)&__heap_limit)
    {
        return (void*)-1;
    }

    heap_end += incr;

    return (void*)prev_heap_end;
}
END_UNOPTIMIZED_FUNCTION
#endif /* FIRMWARE */
