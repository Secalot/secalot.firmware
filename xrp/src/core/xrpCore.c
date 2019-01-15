/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <xrpGlobal.h>
#include <xrpGlobalInt.h>
#include <xrpCore.h>
#include <core/xrpCoreInt.h>
#include <xrpHal.h>
#include <xrpPin.h>

#include <apduGlobal.h>
#include <apduCore.h>

static void xrpCoreClearTransactionToDisplay(void);
static void xrpCoreUpdateTransactionToDisplay(uint8_t* data, uint16_t dataLength);

static void xrpCoreProcessSetup(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void xrpCoreProcessGetInfo(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void xrpCoreProcessGetRandom(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void xrpCoreProcessGetWalletPublicKey(APDU_CORE_COMMAND_APDU* commandAPDU,
                                             APDU_CORE_RESPONSE_APDU* responseAPDU);
static void xrpCoreProcessVerifyPin(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void xrpCoreProcessHashAndSign(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void xrpCoreProcessReadTransaction(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void xrpCoreProcessWipeout(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);

XRP_CORE_TRANSACTION_TO_DISPLAY transactionToDisplay;

static uint16_t xrpCoreHashingState;

void xrpCoreInit()
{
    xrpHalInit();
    xrpPinInit();

    xrpCoreHashingState = XRP_CORE_HASHING_STATE_IDLE;

    xrpCoreClearTransactionToDisplay();
}

void xrpCoreDeinit()
{
    xrpHalDeinit();
    xrpPinDeinit();
}

static void xrpCoreClearTransactionToDisplay(void)
{
    uint16_t i;

    transactionToDisplay.finalized = XRP_FALSE;
    transactionToDisplay.transactionTooBigToDisplay = XRP_FALSE;
    transactionToDisplay.currentOffset = 0;

    xrpHalMemSet(transactionToDisplay.transaction, 0x00, sizeof(transactionToDisplay.transaction));
}

static void xrpCoreUpdateTransactionToDisplay(uint8_t* data, uint16_t dataLength)
{
    if ((transactionToDisplay.currentOffset + dataLength) > XRP_CORE_MAX_VIEWABLE_TRANSACTION_SIZE)
    {
        transactionToDisplay.transactionTooBigToDisplay = XRP_TRUE;
    }
    else
    {
        xrpHalMemCpy(transactionToDisplay.transaction + transactionToDisplay.currentOffset, data, dataLength);
        transactionToDisplay.currentOffset += dataLength;
    }
}

static void xrpCoreProcessSetup(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint32_t offset = 0;
    uint8_t pinLength;
    uint32_t pinOffset;
    uint32_t privateKeyOffset;

    uint8_t pinHash[XRP_GLOBAL_PIN_HASH_LENGTH];

    if (commandAPDU->lcPresent != APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if ((commandAPDU->p1p2 != XRP_CORE_P1P2_SETUP_WITH_PRIVATE_KEY) &&
        (commandAPDU->p1p2 != XRP_CORE_P1P2_SETUP_WITH_SECRET))
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    pinLength = commandAPDU->data[offset++];
    pinOffset = offset;
    offset += pinLength;

    if ((pinLength < XRP_GLOBAL_PIN_MINIMUM_LENGTH) || (pinLength > XRP_GLOBAL_PIN_MAXIMUM_LENGTH))
    {
        sw = APDU_CORE_SW_WRONG_DATA;
        goto END;
    }

    privateKeyOffset = offset;

    if (commandAPDU->p1p2 == XRP_CORE_P1P2_SETUP_WITH_SECRET)
    {
        if ((privateKeyOffset + XRP_GLOBAL_SECRET_SIZE) != commandAPDU->lc)
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }

        xrpHalDerivePrivateKey(&commandAPDU->data[privateKeyOffset], &commandAPDU->data[privateKeyOffset]);
    }
    else
    {
        if ((privateKeyOffset + XRP_GLOBAL_PRIVATE_KEY_SIZE) != commandAPDU->lc)
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }
    }

    xrpHalSetPrivateKey(&commandAPDU->data[privateKeyOffset]);

    xrpPinCheckNewPinAndGetPinHash(&commandAPDU->data[pinOffset], pinLength, pinHash);

    xrpHalWriteSetupInfoAndFinalizeSetup(pinHash);

    responseAPDU->dataLength = 0;

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void xrpCoreProcessGetInfo(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint16_t walletState = XRP_GLOBAL_WALLET_STATE_INITIALIZATION;

    if (commandAPDU->lcPresent != APDU_FALSE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != XRP_CORE_P1P2_GET_INFO)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    walletState = xrpHalGetWalletState();

    responseAPDU->data[0] = XRP_CORE_VERSION_MAJOR;
    responseAPDU->data[1] = XRP_CORE_VERSION_MINOR;

    responseAPDU->data[2] = 0x00;

    if (walletState == XRP_GLOBAL_WALLET_STATE_OPERATIONAL)
    {
        responseAPDU->data[2] |= XRP_CORE_GET_INFO_WALLET_OPERATIONAL;

        if (xrpPinIsPinVerified() == XRP_TRUE)
        {
            responseAPDU->data[2] |= XRP_CORE_GET_INFO_PIN_VERIFIED;
        }
    }

    responseAPDU->data[3] = XRP_CORE_GET_INFO_RFU;
    responseAPDU->data[4] = XRP_CORE_GET_INFO_RFU;
    responseAPDU->data[5] = XRP_CORE_GET_INFO_RFU;
    responseAPDU->data[6] = XRP_CORE_GET_INFO_RFU;
    responseAPDU->data[7] = XRP_CORE_GET_INFO_RFU;

    responseAPDU->dataLength = XRP_CORE_GET_INFO_RESPONSE_LENGTH;

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void xrpCoreProcessGetRandom(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
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

    if (commandAPDU->p1p2 != XRP_CORE_P1P2_GET_RANDOM)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    dataLength = commandAPDU->le;

    if (dataLength == 0x00)
    {
        dataLength = 0x100;
    }

    xrpHalGetRandom(responseAPDU->data, dataLength);

    responseAPDU->dataLength = dataLength;

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void xrpCoreProcessGetWalletPublicKey(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;

    if (commandAPDU->lcPresent != APDU_FALSE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != XRP_CORE_P1P2_GET_WALLET_PUBLIC_KEY)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    if (xrpPinIsPinVerified() != XRP_TRUE)
    {
        sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
        goto END;
    }

    xrpHalDerivePublicKey(responseAPDU->data);

    responseAPDU->dataLength = XRP_GLOBAL_ENCODED_COMPRESSED_POINT_SIZE;

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void xrpCoreProcessVerifyPin(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint16_t calleeRetVal = XRP_GENERAL_ERROR;

    if (commandAPDU->p1p2 == XRP_CORE_P1P2_VERIFY_PIN)
    {
        if (commandAPDU->lcPresent != APDU_TRUE)
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }

        calleeRetVal = xrpPinVerifyPin(commandAPDU->data, commandAPDU->lc);

        if (calleeRetVal == XRP_INVALID_PIN_ERROR)
        {
            sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
            goto END;
        }
        else if (calleeRetVal == XRP_INVALID_PIN_LENGTH_ERROR)
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }
        else if (calleeRetVal == XRP_PIN_BLOCKED_ERROR)
        {
            sw = APDU_CORE_SW_PIN_BLOCKED;
            goto END;
        }
        else if (calleeRetVal != XRP_NO_ERROR)
        {
            xrpHalFatalError();
        }
    }
    else if (commandAPDU->p1p2 == XRP_CORE_P1P2_VERIFY_PIN_GET_REMAINING_ATTEMPTS)
    {
        uint8_t errorCounter = XRP_GLOBAL_PIN_BLOCKED_ERROR_COUNTER_VALUE;

        if (commandAPDU->lcPresent != APDU_FALSE)
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }

        xrpHalGetPinErrorCounter(&errorCounter);

        sw = XRP_CORE_SW_REMAINING_PIN_TRIES + errorCounter;
        goto END;
    }
    else
    {
        xrpHalFatalError();
    }

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void xrpCoreProcessHashAndSign(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint32_t dataLength;
    uint16_t confirmed = XRP_FALSE;

    if (xrpPinIsPinVerified() != XRP_TRUE)
    {
        sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
        goto END;
    }

    if (commandAPDU->p1p2 == XRP_CORE_P1P2_HASH_AND_SIGN_INIT)
    {
        if (commandAPDU->lcPresent != APDU_TRUE)
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }

        if (commandAPDU->lc < XRP_CORE_TRANSACTION_HEADER_SIZE)
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }

        if ((commandAPDU->data[0] != 'S') || (commandAPDU->data[1] != 'T') || (commandAPDU->data[2] != 'X') ||
            (commandAPDU->data[3] != '\0'))
        {
            sw = APDU_CORE_SW_WRONG_DATA;
            goto END;
        }

        xrpCoreClearTransactionToDisplay();

        xrpCoreUpdateTransactionToDisplay(commandAPDU->data + XRP_CORE_TRANSACTION_HEADER_SIZE,
                                          commandAPDU->lc - XRP_CORE_TRANSACTION_HEADER_SIZE);

        xrpHalHashInit();
        xrpHalHashUpdate(commandAPDU->data, commandAPDU->lc);

        xrpCoreHashingState = XRP_CORE_HASHING_STATE_STARTED;

        responseAPDU->dataLength = 0;

        sw = APDU_CORE_SW_NO_ERROR;
    }
    else if (commandAPDU->p1p2 == XRP_CORE_P1P2_HASH_AND_SIGN_UPDATE)
    {
        if (commandAPDU->lcPresent != APDU_TRUE)
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }

        if (xrpCoreHashingState != XRP_CORE_HASHING_STATE_STARTED)
        {
            sw = APDU_CORE_SW_CONDITIONS_NOT_SATISFIED;
            goto END;
        }

        xrpCoreUpdateTransactionToDisplay(commandAPDU->data, commandAPDU->lc);

        xrpHalHashUpdate(commandAPDU->data, commandAPDU->lc);

        responseAPDU->dataLength = 0;

        sw = APDU_CORE_SW_NO_ERROR;
    }
    else if (commandAPDU->p1p2 == XRP_CORE_P1P2_HASH_AND_SIGN_FINAL)
    {
        uint16_t signatureLength;
        uint8_t hash[XRP_GLOBAL_SHA256_SIZE];

        if (commandAPDU->lcPresent != APDU_FALSE)
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }

        if (xrpCoreHashingState != XRP_CORE_HASHING_STATE_STARTED)
        {
            sw = APDU_CORE_SW_CONDITIONS_NOT_SATISFIED;
            goto END;
        }

        xrpCoreHashingState = XRP_CORE_HASHING_STATE_IDLE;

        xrpHalHashFinal(hash);

        transactionToDisplay.finalized = XRP_TRUE;
        xrpHalWaitForComfirmation(&confirmed);
        xrpCoreClearTransactionToDisplay();

        if (confirmed != XRP_TRUE)
        {
            sw = APDU_CORE_SW_TIMEOUT;
            goto END;
        }

        xrpHalSignHash(hash, responseAPDU->data, &signatureLength);

        responseAPDU->dataLength = signatureLength;

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

static void xrpCoreProcessReadTransaction(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint32_t dataLength;
    uint16_t offset = 0;
    uint16_t i;
    uint64_t remainingTime;

    if (transactionToDisplay.finalized != XRP_TRUE)
    {
        sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
        goto END;
    }

    if (commandAPDU->p1p2 == XRP_CORE_P1P2_READ_TRANSACTION_INFO)
    {
        if (commandAPDU->lcPresent != APDU_FALSE)
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }

        if (transactionToDisplay.transactionTooBigToDisplay == XRP_TRUE)
        {
            responseAPDU->data[offset++] = 0x01;
        }
        else
        {
            responseAPDU->data[offset++] = 0x00;
        }

        responseAPDU->data[offset++] = XRP_HIBYTE(transactionToDisplay.currentOffset);
        responseAPDU->data[offset++] = XRP_LOBYTE(transactionToDisplay.currentOffset);

        remainingTime = xrpHalGetRemainingConfirmationTime();

        responseAPDU->data[offset++] = XRP_HIBYTE(XRP_HIWORD(remainingTime));
        responseAPDU->data[offset++] = XRP_LOBYTE(XRP_HIWORD(remainingTime));
        responseAPDU->data[offset++] = XRP_HIBYTE(XRP_LOWORD(remainingTime));
        responseAPDU->data[offset++] = XRP_LOBYTE(XRP_LOWORD(remainingTime));

        responseAPDU->dataLength = offset;
        sw = APDU_CORE_SW_NO_ERROR;
    }
    else if (commandAPDU->p1p2 == XRP_CORE_P1P2_READ_TRANSACTION_DATA)
    {
        uint16_t chunkNumber;

        if (commandAPDU->lcPresent != APDU_TRUE)
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }

        if (commandAPDU->lc != 2)
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }

        chunkNumber = XRP_MAKEWORD(commandAPDU->data[1], commandAPDU->data[0]);

        if (chunkNumber >= XRP_CORE_TRANSACTION_READ_NUMBER_OF_CHUNKS)
        {
            sw = APDU_CORE_SW_CONDITIONS_NOT_SATISFIED;
            goto END;
        }

        xrpHalMemCpy(responseAPDU->data,
                     transactionToDisplay.transaction + (chunkNumber * XRP_CORE_TRANSACTION_READ_CHUNK_SIZE),
                     XRP_CORE_TRANSACTION_READ_CHUNK_SIZE);

        responseAPDU->dataLength = XRP_CORE_TRANSACTION_READ_CHUNK_SIZE;
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

static void xrpCoreProcessWipeout(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint32_t dataLength;

    if (commandAPDU->lcPresent != APDU_FALSE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != XRP_CORE_P1P2_WIPEOUT)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    xrpHalWipeout();

    responseAPDU->dataLength = 0;

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

void xrpCoreGetAID(uint8_t* aid, uint32_t* aidLength)
{
    uint8_t aidTemplate[] = XRP_CORE_AID;

    if ((aid == NULL) || (aidLength == NULL))
    {
        xrpHalFatalError();
    }

    xrpHalMemCpy(aid, aidTemplate, XRP_CORE_AID_LENGTH);

    *aidLength = XRP_CORE_AID_LENGTH;
}

void xrpCoreProcessAPDU(uint8_t* apdu, uint32_t* apduLength)
{
    APDU_CORE_COMMAND_APDU commandAPDU;
    APDU_CORE_RESPONSE_APDU responseAPDU;
    uint16_t calleeRetVal = APDU_GENERAL_ERROR;
    uint16_t wipeoutInProgress = XRP_TRUE;
    uint16_t walletState = XRP_GLOBAL_WALLET_STATE_INITIALIZATION;

    if ((apdu == NULL) || (apduLength == NULL))
    {
        xrpHalFatalError();
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
            xrpHalFatalError();
        }
    }

    wipeoutInProgress = xrpHalIsWipeoutInProgress();

    if (wipeoutInProgress != XRP_FALSE)
    {
        xrpHalWipeout();
    }

    walletState = xrpHalGetWalletState();

    if (walletState == XRP_GLOBAL_WALLET_STATE_OPERATIONAL)
    {
        uint16_t isPinBlocked = XRP_TRUE;

        isPinBlocked = xrpPinIsPinBlocked();

        if (isPinBlocked == XRP_TRUE)
        {
            xrpHalWipeout();
            walletState = XRP_GLOBAL_WALLET_STATE_INITIALIZATION;
        }
    }

    if (commandAPDU.cla != XRP_CORE_CLA)
    {
        responseAPDU.sw = APDU_CORE_SW_CLA_NOT_SUPPORTED;
        goto END;
    }

    if (walletState == XRP_GLOBAL_WALLET_STATE_INITIALIZATION)
    {
        switch (commandAPDU.ins)
        {
            case XRP_CORE_INS_GET_INFO:
                xrpCoreProcessGetInfo(&commandAPDU, &responseAPDU);
                break;
            case XRP_CORE_INS_SETUP:
                xrpCoreProcessSetup(&commandAPDU, &responseAPDU);
                break;
            case XRP_CORE_INS_GET_RANDOM:
                xrpCoreProcessGetRandom(&commandAPDU, &responseAPDU);
                break;
            default:
                responseAPDU.sw = APDU_CORE_SW_INS_NOT_SUPPORTED;
                break;
        }
    }
    else if (walletState == XRP_GLOBAL_WALLET_STATE_OPERATIONAL)
    {
        if (transactionToDisplay.finalized == XRP_TRUE)
        {
            switch (commandAPDU.ins)
            {
                case XRP_CORE_INS_READ_TRANSACTION:
                    xrpCoreProcessReadTransaction(&commandAPDU, &responseAPDU);
                    break;
                default:
                    responseAPDU.sw = APDU_CORE_SW_INS_NOT_SUPPORTED;
                    break;
            }
        }
        else
        {
            switch (commandAPDU.ins)
            {
                case XRP_CORE_INS_VERIFY_PIN:
                    xrpCoreProcessVerifyPin(&commandAPDU, &responseAPDU);
                    break;
                case XRP_CORE_INS_GET_WALLET_PUBLIC_KEY:
                    xrpCoreProcessGetWalletPublicKey(&commandAPDU, &responseAPDU);
                    break;
                case XRP_CORE_INS_GET_INFO:
                    xrpCoreProcessGetInfo(&commandAPDU, &responseAPDU);
                    break;
                case XRP_CORE_INS_GET_RANDOM:
                    xrpCoreProcessGetRandom(&commandAPDU, &responseAPDU);
                    break;
                case XRP_CORE_INS_WIPEOUT:
                    xrpCoreProcessWipeout(&commandAPDU, &responseAPDU);
                    break;
                case XRP_CORE_INS_HASH_AND_SIGN:
                    xrpCoreProcessHashAndSign(&commandAPDU, &responseAPDU);
                    break;
                default:
                    responseAPDU.sw = APDU_CORE_SW_INS_NOT_SUPPORTED;
                    break;
            }
        }
    }
    else
    {
        xrpHalFatalError();
    }

END:
    apduCorePrepareOutgoingAPDU(apdu, apduLength, &responseAPDU);
}
