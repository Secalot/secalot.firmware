/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ethGlobal.h>
#include <ethGlobalInt.h>
#include <ethCore.h>
#include <core/ethCoreInt.h>
#include <ethHal.h>
#include <ethPin.h>

#include <apduGlobal.h>
#include <apduCore.h>

static void ethCoreProcessSetup(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void ethCoreProcessGetInfo(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void ethCoreProcessGetRandom(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void ethCoreProcessGetWalletPublicKey(APDU_CORE_COMMAND_APDU* commandAPDU,
                                             APDU_CORE_RESPONSE_APDU* responseAPDU);
static void ethCoreProcessVerifyPin(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void ethCoreProcessHashAndSign(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void ethCoreProcessWipeout(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);

static uint16_t ethCoreHashingState;

void ethCoreInit()
{
    ethHalInit();
    ethPinInit();

    ethCoreHashingState = ETH_CORE_HASHING_STATE_IDLE;
}

void ethCoreDeinit()
{
    ethHalDeinit();
    ethPinDeinit();
}

static void ethCoreProcessSetup(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint32_t offset = 0;
    uint8_t pinLength;
    uint8_t seedLength;
    uint32_t pinOffset;
    uint32_t seedOffset;

    uint8_t pinHash[ETH_GLOBAL_PIN_HASH_LENGTH];

    if (commandAPDU->lcPresent != APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != ETH_CORE_P1P2_SETUP)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    pinLength = commandAPDU->data[offset++];
    pinOffset = offset;
    offset += pinLength;

    if ((pinLength < ETH_GLOBAL_PIN_MINIMUM_LENGTH) || (pinLength > ETH_GLOBAL_PIN_MAXIMUM_LENGTH))
    {
        sw = APDU_CORE_SW_WRONG_DATA;
        goto END;
    }

    seedLength = commandAPDU->data[offset++];
    seedOffset = offset;

    if ((seedLength < ETH_GLOBAL_SEED_MINIMUM_LENGTH) || (seedLength > ETH_GLOBAL_SEED_MAXIMUM_LENGTH))
    {
        sw = APDU_CORE_SW_WRONG_DATA;
        goto END;
    }

    if ((seedOffset + seedLength) != commandAPDU->lc)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    ethHalSetMasterKey(&commandAPDU->data[seedOffset], seedLength);

    ethPinCheckNewPinAndGetPinHash(&commandAPDU->data[pinOffset], pinLength, pinHash);

    ethHalWriteSetupInfoAndFinalizeSetup(pinHash);

    responseAPDU->dataLength = 0;

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void ethCoreProcessGetInfo(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint16_t walletState = ETH_GLOBAL_WALLET_STATE_INITIALIZATION;

    if (commandAPDU->lcPresent != APDU_FALSE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != ETH_CORE_P1P2_GET_INFO)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    walletState = ethHalGetWalletState();

    responseAPDU->data[0] = ETH_CORE_VERSION_MAJOR;
    responseAPDU->data[1] = ETH_CORE_VERSION_MINOR;

    if (walletState == ETH_GLOBAL_WALLET_STATE_OPERATIONAL)
    {
        responseAPDU->data[2] |= ETH_CORE_GET_INFO_WALLET_OPERATIONAL;

        if (ethPinIsPinVerified() == ETH_TRUE)
        {
            responseAPDU->data[2] |= ETH_CORE_GET_INFO_PIN_VERIFIED;
        }
    }

    responseAPDU->data[3] = ETH_CORE_GET_INFO_RFU;
    responseAPDU->data[4] = ETH_CORE_GET_INFO_RFU;
    responseAPDU->data[5] = ETH_CORE_GET_INFO_RFU;
    responseAPDU->data[6] = ETH_CORE_GET_INFO_RFU;
    responseAPDU->data[7] = ETH_CORE_GET_INFO_RFU;

    responseAPDU->dataLength = ETH_CORE_GET_INFO_RESPONSE_LENGTH;

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void ethCoreProcessGetRandom(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
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

    if (commandAPDU->p1p2 != ETH_CORE_P1P2_GET_RANDOM)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    dataLength = commandAPDU->le;

    if (dataLength == 0x00)
    {
        dataLength = 0x100;
    }

    ethHalGetRandom(responseAPDU->data, dataLength);

    responseAPDU->dataLength = dataLength;

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void ethCoreProcessGetWalletPublicKey(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint8_t numberOfKeyDerivations;
    uint32_t i;
    uint16_t calleeRetVal = ETH_GENERAL_ERROR;
    uint32_t derivationIndexes[ETH_GLOBAL_MAXIMAL_NUMBER_OF_KEY_DERIVATIONS];
    uint8_t chainCode[ETH_GLOBAL_CHAIN_CODE_SIZE];

    if (commandAPDU->lcPresent != APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != ETH_CORE_P1P2_GET_WALLET_PUBLIC_KEY)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    if (ethPinIsPinVerified() != ETH_TRUE)
    {
        sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
        goto END;
    }

    numberOfKeyDerivations = commandAPDU->data[0];

    if ((numberOfKeyDerivations < ETH_GLOBAL_MINIMAL_NUMBER_OF_KEY_DERIVATIONS) ||
        (numberOfKeyDerivations > ETH_GLOBAL_MAXIMAL_NUMBER_OF_KEY_DERIVATIONS))
    {
        sw = APDU_CORE_SW_WRONG_DATA;
        goto END;
    }

    if (commandAPDU->lc != (1 + numberOfKeyDerivations * 4))
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    for (i = 0; i < numberOfKeyDerivations; i++)
    {
        derivationIndexes[i] = ETH_MAKEDWORD(ETH_MAKEWORD(commandAPDU->data[i * 4 + 4], commandAPDU->data[i * 4 + 3]),
                                             ETH_MAKEWORD(commandAPDU->data[i * 4 + 2], commandAPDU->data[i * 4 + 1]));
    }

    calleeRetVal = ethHalDerivePublicKey(derivationIndexes, numberOfKeyDerivations, responseAPDU->data, chainCode);

    if (calleeRetVal != ETH_NO_ERROR)
    {
        if (calleeRetVal == ETH_KEY_DERIVATION_ERROR)
        {
            sw = APDU_CORE_SW_WRONG_DATA;
            goto END;
        }
        else
        {
            ethHalFatalError();
        }
    }

    ethHalMemCpy(&responseAPDU->data[ETH_GLOBAL_ENCODED_FULL_POINT_SIZE], chainCode, ETH_GLOBAL_CHAIN_CODE_SIZE);

    responseAPDU->dataLength = (ETH_GLOBAL_ENCODED_FULL_POINT_SIZE + ETH_GLOBAL_CHAIN_CODE_SIZE);

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void ethCoreProcessVerifyPin(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint16_t calleeRetVal = ETH_GENERAL_ERROR;

    if (commandAPDU->p1p2 == ETH_CORE_P1P2_VERIFY_PIN)
    {
        if (commandAPDU->lcPresent != APDU_TRUE)
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }

        calleeRetVal = ethPinVerifyPin(commandAPDU->data, commandAPDU->lc);

        if (calleeRetVal == ETH_INVALID_PIN_ERROR)
        {
            sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
            goto END;
        }
        else if (calleeRetVal == ETH_INVALID_PIN_LENGTH_ERROR)
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }
        else if (calleeRetVal == ETH_PIN_BLOCKED_ERROR)
        {
            sw = APDU_CORE_SW_PIN_BLOCKED;
            goto END;
        }
        else if (calleeRetVal != ETH_NO_ERROR)
        {
            ethHalFatalError();
        }
    }
    else if (commandAPDU->p1p2 == ETH_CORE_P1P2_VERIFY_PIN_GET_REMAINING_ATTEMPTS)
    {
        uint8_t errorCounter = ETH_GLOBAL_PIN_BLOCKED_ERROR_COUNTER_VALUE;

        if (commandAPDU->lcPresent != APDU_FALSE)
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }

        ethHalGetPinErrorCounter(&errorCounter);

        sw = ETH_CORE_SW_REMAINING_PIN_TRIES + errorCounter;
        goto END;
    }
    else
    {
        ethHalFatalError();
    }

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void ethCoreProcessHashAndSign(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint32_t dataLength;
    uint16_t calleeRetVal = ETH_GENERAL_ERROR;
    uint16_t confirmed = ETH_FALSE;

    if (commandAPDU->lcPresent != APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (ethPinIsPinVerified() != ETH_TRUE)
    {
        sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
        goto END;
    }

    if ((commandAPDU->p1p2 == ETH_CORE_P1P2_HASH_AND_SIGN_INIT_TRANSACTION) ||
        (commandAPDU->p1p2 == ETH_CORE_P1P2_HASH_AND_SIGN_INIT_MESSAGE))
    {
        ethHalHashInit();
        ethHalHashUpdate(commandAPDU->data, commandAPDU->lc);

        ethCoreHashingState = ETH_CORE_HASHING_STATE_STARTED;

        responseAPDU->dataLength = 0;

        sw = APDU_CORE_SW_NO_ERROR;
    }
    else if (commandAPDU->p1p2 == ETH_CORE_P1P2_HASH_AND_SIGN_UPDATE)
    {
        if (ethCoreHashingState != ETH_CORE_HASHING_STATE_STARTED)
        {
            sw = APDU_CORE_SW_CONDITIONS_NOT_SATISFIED;
            goto END;
        }

        ethHalHashUpdate(commandAPDU->data, commandAPDU->lc);

        responseAPDU->dataLength = 0;

        sw = APDU_CORE_SW_NO_ERROR;
    }
    else if (commandAPDU->p1p2 == ETH_CORE_P1P2_HASH_AND_SIGN_FINAL)
    {
        uint32_t derivationIndexes[ETH_GLOBAL_MAXIMAL_NUMBER_OF_KEY_DERIVATIONS];
        uint32_t numberOfKeyDerivations;
        uint8_t hash[ETH_GLOBAL_KECCAK_256_HASH_SIZE];
        uint32_t i;

        if (ethCoreHashingState != ETH_CORE_HASHING_STATE_STARTED)
        {
            sw = APDU_CORE_SW_CONDITIONS_NOT_SATISFIED;
            goto END;
        }

        ethCoreHashingState = ETH_CORE_HASHING_STATE_IDLE;

        ethHalHashFinal(hash);

        numberOfKeyDerivations = commandAPDU->data[0];

        if ((numberOfKeyDerivations < ETH_GLOBAL_MINIMAL_NUMBER_OF_KEY_DERIVATIONS) ||
            (numberOfKeyDerivations > ETH_GLOBAL_MAXIMAL_NUMBER_OF_KEY_DERIVATIONS))
        {
            sw = APDU_CORE_SW_WRONG_DATA;
            goto END;
        }

        if (commandAPDU->lc != (1 + numberOfKeyDerivations * 4))
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }

        for (i = 0; i < numberOfKeyDerivations; i++)
        {
            derivationIndexes[i] =
                ETH_MAKEDWORD(ETH_MAKEWORD(commandAPDU->data[i * 4 + 4], commandAPDU->data[i * 4 + 3]),
                              ETH_MAKEWORD(commandAPDU->data[i * 4 + 2], commandAPDU->data[i * 4 + 1]));
        }

        ethHalWaitForComfirmation(&confirmed);

        if (confirmed != ETH_TRUE)
        {
            sw = APDU_CORE_SW_CONDITIONS_NOT_SATISFIED;
            goto END;
        }

        calleeRetVal = ethHalSignHash(derivationIndexes, numberOfKeyDerivations, hash, responseAPDU->data);

        if (calleeRetVal != ETH_NO_ERROR)
        {
            if (calleeRetVal == ETH_KEY_DERIVATION_ERROR)
            {
                sw = APDU_CORE_SW_WRONG_DATA;
                goto END;
            }
            else
            {
                ethHalFatalError();
            }
        }

        responseAPDU->dataLength = ETH_GLOBAL_SIGNATURE_SIZE;

        sw = APDU_CORE_SW_NO_ERROR;
    }
    else
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

END:
    responseAPDU->sw = sw;
}

static void ethCoreProcessWipeout(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint32_t dataLength;

    if (commandAPDU->lcPresent != APDU_FALSE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != ETH_CORE_P1P2_WIPEOUT)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    ethHalWipeout();

    responseAPDU->dataLength = 0;

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

void ethCoreGetAID(uint8_t* aid, uint32_t* aidLength)
{
    uint8_t aidTemplate[] = ETH_CORE_AID;

    if ((aid == NULL) || (aidLength == NULL))
    {
        ethHalFatalError();
    }

    ethHalMemCpy(aid, aidTemplate, ETH_CORE_AID_LENGTH);

    *aidLength = ETH_CORE_AID_LENGTH;
}

void ethCoreProcessAPDU(uint8_t* apdu, uint32_t* apduLength)
{
    APDU_CORE_COMMAND_APDU commandAPDU;
    APDU_CORE_RESPONSE_APDU responseAPDU;
    uint16_t calleeRetVal = APDU_GENERAL_ERROR;
    uint16_t wipeoutInProgress = ETH_TRUE;
    uint16_t walletState = ETH_GLOBAL_WALLET_STATE_INITIALIZATION;

    if ((apdu == NULL) || (apduLength == NULL))
    {
        ethHalFatalError();
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
            ethHalFatalError();
        }
    }

    wipeoutInProgress = ethHalIsWipeoutInProgress();

    if (wipeoutInProgress != ETH_FALSE)
    {
        ethHalWipeout();
    }

    walletState = ethHalGetWalletState();

    if (walletState == ETH_GLOBAL_WALLET_STATE_OPERATIONAL)
    {
        uint16_t isPinBlocked = ETH_TRUE;

        isPinBlocked = ethPinIsPinBlocked();

        if (isPinBlocked == ETH_TRUE)
        {
            ethHalWipeout();
            walletState = ETH_GLOBAL_WALLET_STATE_INITIALIZATION;
        }
    }

    if (commandAPDU.cla != ETH_CORE_CLA)
    {
        responseAPDU.sw = APDU_CORE_SW_CLA_NOT_SUPPORTED;
        goto END;
    }

    if (walletState == ETH_GLOBAL_WALLET_STATE_INITIALIZATION)
    {
        switch (commandAPDU.ins)
        {
            case ETH_CORE_INS_GET_INFO:
                ethCoreProcessGetInfo(&commandAPDU, &responseAPDU);
                break;
            case ETH_CORE_INS_SETUP:
                ethCoreProcessSetup(&commandAPDU, &responseAPDU);
                break;
            case ETH_CORE_INS_GET_RANDOM:
                ethCoreProcessGetRandom(&commandAPDU, &responseAPDU);
                break;
            default:
                responseAPDU.sw = APDU_CORE_SW_INS_NOT_SUPPORTED;
                break;
        }
    }
    else if (walletState == ETH_GLOBAL_WALLET_STATE_OPERATIONAL)
    {
        switch (commandAPDU.ins)
        {
            case ETH_CORE_INS_VERIFY_PIN:
                ethCoreProcessVerifyPin(&commandAPDU, &responseAPDU);
                break;
            case ETH_CORE_INS_GET_WALLET_PUBLIC_KEY:
                ethCoreProcessGetWalletPublicKey(&commandAPDU, &responseAPDU);
                break;
            case ETH_CORE_INS_GET_INFO:
                ethCoreProcessGetInfo(&commandAPDU, &responseAPDU);
                break;
            case ETH_CORE_INS_GET_RANDOM:
                ethCoreProcessGetRandom(&commandAPDU, &responseAPDU);
                break;
            case ETH_CORE_INS_WIPEOUT:
                ethCoreProcessWipeout(&commandAPDU, &responseAPDU);
                break;
            case ETH_CORE_INS_HASH_AND_SIGN:
                ethCoreProcessHashAndSign(&commandAPDU, &responseAPDU);
                break;
            default:
                responseAPDU.sw = APDU_CORE_SW_INS_NOT_SUPPORTED;
                break;
        }
    }
    else
    {
        ethHalFatalError();
    }

END:
    apduCorePrepareOutgoingAPDU(apdu, apduLength, &responseAPDU);
}
