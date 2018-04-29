/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <symGlobal.h>
#include <symGlobalInt.h>
#include <symCore.h>
#include <core/symCoreInt.h>
#include <symHal.h>

#include <apduGlobal.h>
#include <apduCore.h>

static void symCoreProcessSetup(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void symCoreProcessGetInfo(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void symCoreProcessGetRandom(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void symCoreProcessGetWalletPublicKey(APDU_CORE_COMMAND_APDU* commandAPDU,
                                             APDU_CORE_RESPONSE_APDU* responseAPDU);
static void symCoreProcessSign(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void symCoreProcessWipeout(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);

void symCoreInit() { symHalInit(); }

void symCoreDeinit() { symHalDeinit(); }

static void symCoreProcessSetup(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint32_t offset = 0;

    if (commandAPDU->lcPresent != APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != SYM_CORE_P1P2_SETUP)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    if (commandAPDU->lc != SYM_GLOBAL_MASTER_KEY_SIZE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    symHalInitializeWallet(commandAPDU->data);

    responseAPDU->dataLength = 0;

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void symCoreProcessGetInfo(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint16_t walletState = SYM_GLOBAL_WALLET_STATE_INITIALIZATION;

    if (commandAPDU->lcPresent != APDU_FALSE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != SYM_CORE_P1P2_GET_INFO)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    walletState = symHalGetWalletState();

    responseAPDU->data[0] = SYM_CORE_VERSION_MAJOR;
    responseAPDU->data[1] = SYM_CORE_VERSION_MINOR;

    if (walletState == SYM_GLOBAL_WALLET_STATE_OPERATIONAL)
    {
        responseAPDU->data[2] |= SYM_CORE_GET_INFO_WALLET_OPERATIONAL;
    }

    responseAPDU->data[3] = SYM_CORE_GET_INFO_RFU;
    responseAPDU->data[4] = SYM_CORE_GET_INFO_RFU;
    responseAPDU->data[5] = SYM_CORE_GET_INFO_RFU;
    responseAPDU->data[6] = SYM_CORE_GET_INFO_RFU;
    responseAPDU->data[7] = SYM_CORE_GET_INFO_RFU;

    responseAPDU->dataLength = SYM_CORE_GET_INFO_RESPONSE_LENGTH;

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void symCoreProcessGetRandom(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint32_t dataLength;

    if (commandAPDU->lcPresent != APDU_FALSE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->lePresent != APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != SYM_CORE_P1P2_GET_RANDOM)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    dataLength = commandAPDU->le;

    if (dataLength == 0x00)
    {
        dataLength = 0x100;
    }

    symHalGetRandom(responseAPDU->data, dataLength);

    responseAPDU->dataLength = dataLength;

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void symCoreProcessGetWalletPublicKey(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint8_t tweakLength;

    if (commandAPDU->lcPresent != APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != SYM_CORE_P1P2_GET_WALLET_PUBLIC_KEY)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    tweakLength = commandAPDU->lc;

    if ((tweakLength < SYM_GLOBAL_MINIMAL_TWEAK_LENGTH) || (tweakLength > SYM_GLOBAL_MAXIMAL_TWEAK_LENGTH))
    {
        sw = APDU_CORE_SW_WRONG_DATA;
        goto END;
    }

    symHalDerivePublicKey(responseAPDU->data, commandAPDU->data, tweakLength);

    responseAPDU->dataLength = SYM_GLOBAL_ENCODED_FULL_POINT_SIZE;

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void symCoreProcessSign(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint8_t tweakLength;
    uint16_t calleeRetVal = SYM_GENERAL_ERROR;

    if (commandAPDU->lcPresent != APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != SYM_CORE_P1P2_SIGN)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    tweakLength = commandAPDU->data[0];

    if (commandAPDU->lc != (1 + tweakLength + SYM_GLOBAL_SHA256_SIZE))
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    symHalSignHash(commandAPDU->data + 1, tweakLength, commandAPDU->data + 1 + tweakLength, responseAPDU->data);

    responseAPDU->dataLength = ETH_GLOBAL_SIGNATURE_SIZE;

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void symCoreProcessWipeout(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint32_t dataLength;

    if (commandAPDU->lcPresent != APDU_FALSE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != SYM_CORE_P1P2_WIPEOUT)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    symHalWipeout();

    responseAPDU->dataLength = 0;

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

void symCoreGetAID(uint8_t* aid, uint32_t* aidLength)
{
    uint8_t aidTemplate[] = SYM_CORE_AID;

    if ((aid == NULL) || (aidLength == NULL))
    {
        symHalFatalError();
    }

    symHalMemCpy(aid, aidTemplate, SYM_CORE_AID_LENGTH);

    *aidLength = SYM_CORE_AID_LENGTH;
}

void symCoreProcessAPDU(uint8_t* apdu, uint32_t* apduLength)
{
    APDU_CORE_COMMAND_APDU commandAPDU;
    APDU_CORE_RESPONSE_APDU responseAPDU;
    uint16_t calleeRetVal = APDU_GENERAL_ERROR;
    uint16_t wipeoutInProgress = SYM_TRUE;
    uint16_t walletState = SYM_GLOBAL_WALLET_STATE_INITIALIZATION;

    if ((apdu == NULL) || (apduLength == NULL))
    {
        symHalFatalError();
    }

    apduCorePrepareResponseAPDUStructure(apdu, &responseAPDU);

    calleeRetVal = apduCoreParseIncomingAPDU(apdu, *apduLength, &commandAPDU);

    if (calleeRetVal != APDU_NO_ERROR)
    {
        if (calleeRetVal == APDU_GENERAL_ERROR)
        {
            responseAPDU.sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }
        else
        {
            symHalFatalError();
        }
    }

    wipeoutInProgress = symHalIsWipeoutInProgress();

    if (wipeoutInProgress != SYM_FALSE)
    {
        symHalWipeout();
    }

    walletState = symHalGetWalletState();

    if (commandAPDU.cla != SYM_CORE_CLA)
    {
        responseAPDU.sw = APDU_CORE_SW_CLA_NOT_SUPPORTED;
        goto END;
    }

    if (walletState == SYM_GLOBAL_WALLET_STATE_INITIALIZATION)
    {
        switch (commandAPDU.ins)
        {
            case SYM_CORE_INS_GET_INFO:
                symCoreProcessGetInfo(&commandAPDU, &responseAPDU);
                break;
            case SYM_CORE_INS_SETUP:
                symCoreProcessSetup(&commandAPDU, &responseAPDU);
                break;
            case SYM_CORE_INS_GET_RANDOM:
                symCoreProcessGetRandom(&commandAPDU, &responseAPDU);
                break;
            default:
                responseAPDU.sw = APDU_CORE_SW_INS_NOT_SUPPORTED;
                break;
        }
    }
    else if (walletState == SYM_GLOBAL_WALLET_STATE_OPERATIONAL)
    {
        switch (commandAPDU.ins)
        {
            case SYM_CORE_INS_GET_WALLET_PUBLIC_KEY:
                symCoreProcessGetWalletPublicKey(&commandAPDU, &responseAPDU);
                break;
            case SYM_CORE_INS_GET_INFO:
                symCoreProcessGetInfo(&commandAPDU, &responseAPDU);
                break;
            case SYM_CORE_INS_GET_RANDOM:
                symCoreProcessGetRandom(&commandAPDU, &responseAPDU);
                break;
            case SYM_CORE_INS_WIPEOUT:
                symCoreProcessWipeout(&commandAPDU, &responseAPDU);
                break;
            case SYM_CORE_INS_SIGN:
                symCoreProcessSign(&commandAPDU, &responseAPDU);
                break;
            default:
                responseAPDU.sw = APDU_CORE_SW_INS_NOT_SUPPORTED;
                break;
        }
    }
    else
    {
        symHalFatalError();
    }

END:
    apduCorePrepareOutgoingAPDU(apdu, apduLength, &responseAPDU);
}
