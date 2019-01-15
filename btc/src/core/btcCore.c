/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <btcGlobal.h>
#include <btcGlobalInt.h>
#include <btcCore.h>
#include <core/btcCoreInt.h>
#include <btcHal.h>
#include <btcPin.h>
#include <btcTran.h>
#include <btcBase58.h>

#include <apduGlobal.h>
#include <apduCore.h>

static void btcCoreProcessSetup(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void btcCoreProcessVerifyPin(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void btcCoreProcessGetWalletPublicKey(APDU_CORE_COMMAND_APDU* commandAPDU,
                                             APDU_CORE_RESPONSE_APDU* responseAPDU);
static void btcCoreProcessGetTrustedInput(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void btcCoreProcessUntrustedHashTransactionInputStart(APDU_CORE_COMMAND_APDU* commandAPDU,
                                                             APDU_CORE_RESPONSE_APDU* responseAPDU);
static void btcCoreProcessUntrustedHashTransactionInputFinalizeFull(APDU_CORE_COMMAND_APDU* commandAPDU,
                                                                    APDU_CORE_RESPONSE_APDU* responseAPDU);
static void btcCoreProcessUntrustedHashSign(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void btcCoreProcessReadTransaction(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void btcCoreProcessSignMessage(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void btcCoreProcessGetFirmwareVersion(APDU_CORE_COMMAND_APDU* commandAPDU,
                                             APDU_CORE_RESPONSE_APDU* responseAPDU);
static void btcCoreProcessSetKeyboardConfiguration(APDU_CORE_COMMAND_APDU* commandAPDU,
                                                   APDU_CORE_RESPONSE_APDU* responseAPDU);

static void btcCoreProcessGetRandom(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);

static uint16_t btcCoreWaitingForConfirmation;

void btcCoreInit()
{
    btcHalInit();
    btcPinInit();
    btcTranInit();
    btcBase58Init();

    btcCoreWaitingForConfirmation = BTC_FALSE;
}

void btcCoreDeinit()
{
    btcHalDeinit();
    btcPinDeinit();
    btcTranDeinit();
    btcBase58Deinit();
}

static void btcCoreProcessSetup(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint32_t offset = 0;
    uint8_t walletMode;
    uint8_t walletFeatures;
    uint8_t regularCoinVersion;
    uint8_t p2shCoinVersion;
    uint8_t pinLength;
    uint8_t secondaryPinLength;
    uint8_t seedLength;
    uint32_t pinOffset;
    uint32_t seedOffset;
    uint16_t createNewSeed = BTC_FALSE;
    uint8_t newSeed[BTC_GLOBAL_SEED_MAXIMUM_LENGTH];

    uint8_t pinHash[BTC_GLOBAL_PIN_HASH_LENGTH];

    if (commandAPDU->lcPresent != APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != BTC_CORE_P1P2_SETUP_REGULAR_SETUP)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    walletMode = commandAPDU->data[offset++];

    if (walletMode != BTC_CORE_MODE_WALLET_MODE)
    {
        sw = APDU_CORE_SW_WRONG_DATA;
        goto END;
    }

    walletFeatures = commandAPDU->data[offset++];

    if (walletFeatures != BTC_CORE_FEATURES_DETERMINISTIC_SIGNATURE)
    {
        sw = APDU_CORE_SW_WRONG_DATA;
        goto END;
    }

    regularCoinVersion = commandAPDU->data[offset++];
    p2shCoinVersion = commandAPDU->data[offset++];

    pinLength = commandAPDU->data[offset++];
    pinOffset = offset;
    offset += pinLength;

    if ((pinLength < BTC_GLOBAL_PIN_MINIMUM_LENGTH) || (pinLength > BTC_GLOBAL_PIN_MAXIMUM_LENGTH))
    {
        sw = APDU_CORE_SW_WRONG_DATA;
        goto END;
    }

    secondaryPinLength = commandAPDU->data[offset++];
    offset += secondaryPinLength;

    if (secondaryPinLength != 0)
    {
        sw = APDU_CORE_SW_WRONG_DATA;
        goto END;
    }

    seedLength = commandAPDU->data[offset++];
    seedOffset = offset;

    if (seedLength == 0)
    {
        createNewSeed = BTC_TRUE;
    }
    else
    {
        createNewSeed = BTC_FALSE;

        if ((seedLength < BTC_GLOBAL_SEED_MINIMUM_LENGTH) || (seedLength > BTC_GLOBAL_SEED_MAXIMUM_LENGTH))
        {
            sw = APDU_CORE_SW_WRONG_DATA;
            goto END;
        }
    }

    if ((seedOffset + seedLength + 1) != commandAPDU->lc)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (createNewSeed == BTC_TRUE)
    {
        btcHalGenerateNewSeed(newSeed, sizeof(newSeed));

        btcHalSetMasterKey(newSeed, sizeof(newSeed));
    }
    else
    {
        btcHalSetMasterKey(&commandAPDU->data[seedOffset], seedLength);
    }

    btcHalSetRandomTrustedInputKey();

    btcPinCheckNewPinAndGetPinHash(&commandAPDU->data[pinOffset], pinLength, pinHash);

    btcHalWriteSetupInfoAndFinalizeSetup(regularCoinVersion, p2shCoinVersion, pinHash);

    responseAPDU->data[0] = BTC_CORE_SEED_NOT_TYPED_TO_THE_USER;
    responseAPDU->dataLength = 1;

    if (createNewSeed == BTC_TRUE)
    {
        btcHalMemCpy(&responseAPDU->data[1], newSeed, sizeof(newSeed));
        responseAPDU->dataLength += sizeof(newSeed);
    }

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void btcCoreProcessVerifyPin(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint16_t calleeRetVal = BTC_GENERAL_ERROR;

    if (commandAPDU->lcPresent != APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 == BTC_CORE_P1P2_VERIFY_PIN)
    {
        calleeRetVal = btcPinVerifyPin(commandAPDU->data, commandAPDU->lc);

        if (calleeRetVal == BTC_INVALID_PIN_ERROR)
        {
            sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
            goto END;
        }
        else if (calleeRetVal == BTC_INVALID_PIN_LENGTH_ERROR)
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }
        else if (calleeRetVal == BTC_PIN_BLOCKED_ERROR)
        {
            sw = APDU_CORE_SW_PIN_BLOCKED;
            goto END;
        }
        else if (calleeRetVal != BTC_NO_ERROR)
        {
            btcHalFatalError();
        }
    }
    else if (commandAPDU->p1p2 == BTC_CORE_P1P2_VERIFY_PIN_GET_REMAINING_ATTEMPTS)
    {
        uint8_t errorCounter = BTC_GLOBAL_PIN_BLOCKED_ERROR_COUNTER_VALUE;

        btcHalGetPinErrorCounter(&errorCounter);

        sw = BTC_CORE_SW_REMAINING_PIN_TRIES + errorCounter;
        goto END;
    }
    else
    {
        btcHalFatalError();
    }

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void btcCoreProcessGetWalletPublicKey(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint8_t numberOfKeyDerivations;
    uint32_t i;
    uint16_t calleeRetVal = BTC_GENERAL_ERROR;
    uint32_t derivationIndexes[BTC_GLOBAL_MAXIMAL_NUMBER_OF_KEY_DERIVATIONS];
    uint8_t chainCode[BTC_GLOBAL_CHAIN_CODE_SIZE];
    uint8_t address[BTC_GLOBAL_RIPEMD160_SIZE];
    uint8_t compressedPublicKey[BTC_GLOBAL_ENCODED_COMPRESSED_POINT_SIZE];
    uint32_t encodedAddressLength = BTC_GLOBAL_MAXIMAL_BITCOIN_BASE58_ADDRESS_LENGTH;
    uint8_t regularCoinVersion;
    uint8_t p2shCoinVersion;

    if (commandAPDU->lcPresent != APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != BTC_CORE_P1P2_GET_WALLET_PUBLIC_KEY)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    if (btcPinIsPinVerified() != BTC_TRUE)
    {
        sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
        goto END;
    }

    numberOfKeyDerivations = commandAPDU->data[0];

    if ((numberOfKeyDerivations < BTC_GLOBAL_MINIMAL_NUMBER_OF_KEY_DERIVATIONS) ||
        (numberOfKeyDerivations > BTC_GLOBAL_MAXIMAL_NUMBER_OF_KEY_DERIVATIONS))
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
        derivationIndexes[i] = BTC_MAKEDWORD(BTC_MAKEWORD(commandAPDU->data[i * 4 + 4], commandAPDU->data[i * 4 + 3]),
                                             BTC_MAKEWORD(commandAPDU->data[i * 4 + 2], commandAPDU->data[i * 4 + 1]));
    }

    btcHalGetCoinVersions(&regularCoinVersion, &p2shCoinVersion);

    responseAPDU->data[0] = BTC_GLOBAL_ENCODED_FULL_POINT_SIZE;

    calleeRetVal = btcHalDerivePublicKey(derivationIndexes, numberOfKeyDerivations, &responseAPDU->data[1],
                                         compressedPublicKey, chainCode, BTC_TRUE, BTC_TRUE);

    if (calleeRetVal != BTC_NO_ERROR)
    {
        if (calleeRetVal == BTC_KEY_DERIVATION_ERROR)
        {
            sw = APDU_CORE_SW_WRONG_DATA;
            goto END;
        }
        else
        {
            btcHalFatalError();
        }
    }

    btcHalHash160(compressedPublicKey, BTC_GLOBAL_ENCODED_COMPRESSED_POINT_SIZE, address);

    btcBase58EncodeBitcoinAddress(address, BTC_GLOBAL_RIPEMD160_SIZE,
                                  &responseAPDU->data[2 + BTC_GLOBAL_ENCODED_FULL_POINT_SIZE], &encodedAddressLength,
                                  regularCoinVersion);

    responseAPDU->data[1 + BTC_GLOBAL_ENCODED_FULL_POINT_SIZE] = (uint8_t)encodedAddressLength;

    btcHalMemCpy(&responseAPDU->data[2 + BTC_GLOBAL_ENCODED_FULL_POINT_SIZE + encodedAddressLength], chainCode,
                 BTC_GLOBAL_CHAIN_CODE_SIZE);

    responseAPDU->dataLength =
        (2 + BTC_GLOBAL_ENCODED_FULL_POINT_SIZE + encodedAddressLength + BTC_GLOBAL_CHAIN_CODE_SIZE);

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void btcCoreProcessGetTrustedInput(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint16_t calleeRetVal = BTC_GENERAL_ERROR;
    uint16_t actionAfterParsing;
    uint32_t transactionPartOffset;
    uint32_t transactionPartLength;

    if (commandAPDU->lcPresent != APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if ((commandAPDU->p1p2 != BTC_CORE_P1P2_GET_TRUSTED_INPUT_FIRST_BLOCK) &&
        (commandAPDU->p1p2 != BTC_CORE_P1P2_GET_TRUSTED_INPUT_SUBSEQUENT_BLOCK))
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    if (btcPinIsPinVerified() != BTC_TRUE)
    {
        sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
        goto END;
    }

    if (commandAPDU->p1p2 == BTC_CORE_P1P2_GET_TRUSTED_INPUT_FIRST_BLOCK)
    {
        uint32_t outputNumberToGetAmountOf;

        if (commandAPDU->lc < sizeof(uint32_t))
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }

        outputNumberToGetAmountOf = BTC_MAKEDWORD(BTC_MAKEWORD(commandAPDU->data[3], commandAPDU->data[2]),
                                                  BTC_MAKEWORD(commandAPDU->data[1], commandAPDU->data[0]));

        btcTranTIGenerationInit(outputNumberToGetAmountOf);

        transactionPartOffset = sizeof(uint32_t);
        transactionPartLength = commandAPDU->lc - sizeof(uint32_t);
    }
    else
    {
        transactionPartOffset = 0;
        transactionPartLength = commandAPDU->lc;
    }

    calleeRetVal = btcTranTIGenerationProcessTransaction(&commandAPDU->data[transactionPartOffset],
                                                         transactionPartLength, &actionAfterParsing);

    if (calleeRetVal != BTC_NO_ERROR)
    {
        if (calleeRetVal == BTC_TRANSACTION_PARSING_FAILED_ERROR)
        {
            sw = APDU_CORE_SW_WRONG_DATA;
            goto END;
        }
        else
        {
            btcHalFatalError();
        }
    }

    if (actionAfterParsing == BTC_TRAN_RETREIVE_OUTPUT)
    {
        *(uint32_t*)(&responseAPDU->data[BTC_GLOBAL_TRUSTED_INPUT_BLOB_MAGIC_OFFSET]) =
            BTC_GLOBAL_TRUSTED_INPUT_BLOB_MAGIC;
        btcTranTIGenerationGetHashIndexAndAmount(
            &(responseAPDU->data[BTC_GLOBAL_TRUSTED_INPUT_BLOB_HASH_INDEX_AND_AMOUNT_OFFSET]));
        btcHalComputeTrustedInputMAC(responseAPDU->data);

        responseAPDU->dataLength = BTC_GLOBAL_TRUSTED_INPUT_BLOB_LENGTH;
    }
    else if (actionAfterParsing == BTC_TRAN_GIVE_MORE_INPUT)
    {
        responseAPDU->dataLength = 0;
    }
    else
    {
        btcHalFatalError();
    }

    sw = APDU_CORE_SW_NO_ERROR;

END:
    if (sw != APDU_CORE_SW_NO_ERROR)
    {
        btcTranTIGenerationClearState();
    }

    responseAPDU->sw = sw;
}

static void btcCoreProcessUntrustedHashTransactionInputStart(APDU_CORE_COMMAND_APDU* commandAPDU,
                                                             APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint16_t calleeRetVal = BTC_GENERAL_ERROR;

    if (commandAPDU->lcPresent != APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if ((commandAPDU->p1p2 != BTC_CORE_P1P2_UNTRUSTED_HASH_TRANSACTION_INPUT_START_START_NEW_TRANACTION_FIRST_BLOCK) &&
        (commandAPDU->p1p2 !=
         BTC_CORE_P1P2_UNTRUSTED_HASH_TRANSACTION_INPUT_START_START_NEW_TRANACTION_FIRST_BLOCK_SEGWIT) &&
        (commandAPDU->p1p2 !=
         BTC_CORE_P1P2_UNTRUSTED_HASH_TRANSACTION_INPUT_START_START_NEW_TRANACTION_SUBSEQUENT_BLOCK) &&
        (commandAPDU->p1p2 != BTC_CORE_P1P2_UNTRUSTED_HASH_TRANSACTION_INPUT_START_ANOTHER_TRANACTION_FIRST_BLOCK) &&
        (commandAPDU->p1p2 != BTC_CORE_P1P2_UNTRUSTED_HASH_TRANSACTION_INPUT_START_ANOTHER_TRANACTION_SUBSEQUENT_BLOCK))
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    if (btcPinIsPinVerified() != BTC_TRUE)
    {
        sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
        goto END;
    }

    if (commandAPDU->p1p2 == BTC_CORE_P1P2_UNTRUSTED_HASH_TRANSACTION_INPUT_START_START_NEW_TRANACTION_FIRST_BLOCK)
    {
        btcTranSigningInit(BTC_FALSE);
    }
    else if (commandAPDU->p1p2 ==
             BTC_CORE_P1P2_UNTRUSTED_HASH_TRANSACTION_INPUT_START_START_NEW_TRANACTION_FIRST_BLOCK_SEGWIT)
    {
        btcTranSigningInit(BTC_TRUE);
    }

    calleeRetVal = btcTranSigningProcessHeaderAndInputs(commandAPDU->data, commandAPDU->lc);

    if (calleeRetVal != BTC_NO_ERROR)
    {
        if (calleeRetVal == BTC_TRANSACTION_PARSING_FAILED_ERROR)
        {
            sw = APDU_CORE_SW_WRONG_DATA;
            goto END;
        }
        else
        {
            btcHalFatalError();
        }
    }

    responseAPDU->dataLength = 0;

    sw = APDU_CORE_SW_NO_ERROR;

END:
    if (sw != APDU_CORE_SW_NO_ERROR)
    {
        btcTranSigningClearState();
    }

    responseAPDU->sw = sw;
}

static void btcCoreProcessUntrustedHashTransactionInputFinalizeFull(APDU_CORE_COMMAND_APDU* commandAPDU,
                                                                    APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint16_t calleeRetVal = BTC_GENERAL_ERROR;

    if (commandAPDU->lcPresent != APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if ((commandAPDU->p1p2 != BTC_CORE_P1P2_UNTRUSTED_HASH_TRANSACTION_INPUT_FINALIZE_FULL_LAST_BLOCK) &&
        (commandAPDU->p1p2 != BTC_CORE_P1P2_UNTRUSTED_HASH_TRANSACTION_INPUT_FINALIZE_FULL_MORE_BLOCKS_TO_COME))
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    if (btcPinIsPinVerified() != BTC_TRUE)
    {
        sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
        goto END;
    }

    calleeRetVal = btcTranSigningProcessOutputs(commandAPDU->data, commandAPDU->lc);

    if (calleeRetVal != BTC_NO_ERROR)
    {
        if (calleeRetVal == BTC_TRANSACTION_PARSING_FAILED_ERROR)
        {
            sw = APDU_CORE_SW_WRONG_DATA;
            goto END;
        }
        else
        {
            btcHalFatalError();
        }
    }

    if (commandAPDU->p1p2 == BTC_CORE_P1P2_UNTRUSTED_HASH_TRANSACTION_INPUT_FINALIZE_FULL_LAST_BLOCK)
    {
        responseAPDU->dataLength = 1;
        responseAPDU->data[0] = BTC_GLOBAL_NO_USER_VALIDATION_REQUESTED;
    }
    else
    {
        responseAPDU->dataLength = 0;
    }

    sw = APDU_CORE_SW_NO_ERROR;

END:
    if (sw != APDU_CORE_SW_NO_ERROR)
    {
        btcTranSigningClearState();
    }

    responseAPDU->sw = sw;
}

static void btcCoreProcessUntrustedHashSign(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint16_t calleeRetVal = BTC_GENERAL_ERROR;
    uint32_t offset = 0;
    uint8_t numberOfKeyDerivations;
    uint32_t derivationIndexes[BTC_GLOBAL_MAXIMAL_NUMBER_OF_KEY_DERIVATIONS];
    uint32_t lockTime;
    uint32_t signHashType;
    uint32_t i = 0;
    uint32_t signatureLength;
    uint16_t confirmed = BTC_FALSE;
    uint16_t firstSignatureGenerated;

    if (commandAPDU->lcPresent != APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != BTC_CORE_P1P2_UNTRUSTED_HASH_SIGN)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    if (btcPinIsPinVerified() != BTC_TRUE)
    {
        sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
        goto END;
    }

    numberOfKeyDerivations = commandAPDU->data[offset++];

    if ((numberOfKeyDerivations < BTC_GLOBAL_MINIMAL_NUMBER_OF_KEY_DERIVATIONS) ||
        (numberOfKeyDerivations > BTC_GLOBAL_MAXIMAL_NUMBER_OF_KEY_DERIVATIONS))
    {
        sw = APDU_CORE_SW_WRONG_DATA;
        goto END;
    }

    for (i = 0; i < numberOfKeyDerivations; i++)
    {
        derivationIndexes[i] =
            BTC_MAKEDWORD(BTC_MAKEWORD(commandAPDU->data[offset + i * 4 + 3], commandAPDU->data[offset + i * 4 + 2]),
                          BTC_MAKEWORD(commandAPDU->data[offset + i * 4 + 1], commandAPDU->data[offset + i * 4]));
    }

    offset += numberOfKeyDerivations * sizeof(uint32_t);

    if (commandAPDU->data[offset++] != 0)
    {
        sw = APDU_CORE_SW_WRONG_DATA;
        goto END;
    }

    lockTime = BTC_MAKEDWORD(BTC_MAKEWORD(commandAPDU->data[offset + 3], commandAPDU->data[offset + 2]),
                             BTC_MAKEWORD(commandAPDU->data[offset + 1], commandAPDU->data[offset]));

    offset += sizeof(uint32_t);

    signHashType = (uint32_t)commandAPDU->data[offset++];

    if (offset != commandAPDU->lc)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    btcTranIsFirstSignatureGenerated(&firstSignatureGenerated);

    if (firstSignatureGenerated != BTC_TRUE)
    {
        btcCoreWaitingForConfirmation = BTC_TRUE;
        btcHalWaitForComfirmation(BTC_TRUE, &confirmed);
        btcCoreWaitingForConfirmation = BTC_FALSE;

        if (confirmed != BTC_TRUE)
        {
            sw = APDU_CORE_SW_CONDITIONS_NOT_SATISFIED;
            goto END;
        }
    }

    calleeRetVal = btcTranSigningSign(derivationIndexes, numberOfKeyDerivations, lockTime, signHashType,
                                      responseAPDU->data, &signatureLength);

    if (calleeRetVal != BTC_NO_ERROR)
    {
        if ((calleeRetVal == BTC_KEY_DERIVATION_ERROR) || (calleeRetVal == BTC_TRANSACTION_PARSING_FAILED_ERROR))
        {
            sw = APDU_CORE_SW_WRONG_DATA;
            goto END;
        }
        else
        {
            btcHalFatalError();
        }
    }

    responseAPDU->data[signatureLength] = (uint8_t)signHashType;

    responseAPDU->dataLength = signatureLength + 1;

    sw = APDU_CORE_SW_NO_ERROR;

END:
    if (sw != APDU_CORE_SW_NO_ERROR)
    {
        btcTranSigningClearState();
    }

    responseAPDU->sw = sw;
}

static void btcCoreProcessReadTransaction(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint32_t dataLength;
    uint16_t offset = 0;
    uint16_t i;
    uint64_t remainingTime;
    BTC_TRAN_TRANSACTION_TO_DISPLAY* transactionToDisplay;
    int64_t* inputAmounts;
    uint32_t numberOfInputs;

    if (btcCoreWaitingForConfirmation != BTC_TRUE)
    {
        sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
        goto END;
    }

    btcTranGetTransactionReadoutData(&transactionToDisplay, &inputAmounts, &numberOfInputs);

    if (commandAPDU->p1p2 == BTC_CORE_P1P2_READ_TRANSACTION_INFO)
    {
        if (commandAPDU->lcPresent != APDU_FALSE)
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }

        if (transactionToDisplay->transactionTooBigToDisplay == BTC_TRUE)
        {
            responseAPDU->data[offset++] = 0x01;
        }
        else
        {
            responseAPDU->data[offset++] = 0x00;
        }

        responseAPDU->data[offset++] = BTC_HIBYTE(transactionToDisplay->currentOffset);
        responseAPDU->data[offset++] = BTC_LOBYTE(transactionToDisplay->currentOffset);

        responseAPDU->data[offset++] = BTC_HIBYTE(BTC_HIWORD(numberOfInputs));
        responseAPDU->data[offset++] = BTC_LOBYTE(BTC_HIWORD(numberOfInputs));
        responseAPDU->data[offset++] = BTC_HIBYTE(BTC_LOWORD(numberOfInputs));
        responseAPDU->data[offset++] = BTC_LOBYTE(BTC_LOWORD(numberOfInputs));

        remainingTime = btcHalGetRemainingConfirmationTime();

        responseAPDU->data[offset++] = BTC_HIBYTE(BTC_HIWORD(remainingTime));
        responseAPDU->data[offset++] = BTC_LOBYTE(BTC_HIWORD(remainingTime));
        responseAPDU->data[offset++] = BTC_HIBYTE(BTC_LOWORD(remainingTime));
        responseAPDU->data[offset++] = BTC_LOBYTE(BTC_LOWORD(remainingTime));

        responseAPDU->dataLength = offset;
        sw = APDU_CORE_SW_NO_ERROR;
    }
    else if (commandAPDU->p1p2 == BTC_CORE_P1P2_READ_TRANSACTION_DATA)
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

        chunkNumber = BTC_MAKEWORD(commandAPDU->data[1], commandAPDU->data[0]);

        if (chunkNumber >= BTC_CORE_TRANSACTION_READ_NUMBER_OF_CHUNKS)
        {
            sw = APDU_CORE_SW_CONDITIONS_NOT_SATISFIED;
            goto END;
        }

        btcHalMemCpy(responseAPDU->data,
                     transactionToDisplay->transaction + (chunkNumber * BTC_CORE_TRANSACTION_READ_CHUNK_SIZE),
                     BTC_CORE_TRANSACTION_READ_CHUNK_SIZE);

        responseAPDU->dataLength = BTC_CORE_TRANSACTION_READ_CHUNK_SIZE;
        sw = APDU_CORE_SW_NO_ERROR;
    }
    else if (commandAPDU->p1p2 == BTC_CORE_P1P2_READ_TRANSACTION_AMOUNTS)
    {
        if (commandAPDU->lcPresent != APDU_FALSE)
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }

        for (uint32_t i = 0; i < numberOfInputs; i++)
        {
            responseAPDU->data[8 * i] = (uint8_t)(inputAmounts[i] >> 56);
            responseAPDU->data[8 * i + 1] = (uint8_t)(inputAmounts[i] >> 48);
            responseAPDU->data[8 * i + 2] = (uint8_t)(inputAmounts[i] >> 40);
            responseAPDU->data[8 * i + 3] = (uint8_t)(inputAmounts[i] >> 32);
            responseAPDU->data[8 * i + 4] = (uint8_t)(inputAmounts[i] >> 24);
            responseAPDU->data[8 * i + 5] = (uint8_t)(inputAmounts[i] >> 16);
            responseAPDU->data[8 * i + 6] = (uint8_t)(inputAmounts[i] >> 8);
            responseAPDU->data[8 * i + 7] = (uint8_t)(inputAmounts[i]);
        }

        responseAPDU->dataLength = numberOfInputs * sizeof(inputAmounts[0]);
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

static void btcCoreProcessSignMessage(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint16_t calleeRetVal = BTC_GENERAL_ERROR;

    if (commandAPDU->lcPresent != APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if ((commandAPDU->p1p2 != BTC_CORE_P1P2_SIGN_MESSAGE_PREPARE_MESSAGE) &&
        (commandAPDU->p1p2 != BTC_CORE_P1P2_SIGN_MESSAGE_SIGN_MESSAGE))
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    if (btcPinIsPinVerified() != BTC_TRUE)
    {
        sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
        goto END;
    }

    if (commandAPDU->p1p2 == BTC_CORE_P1P2_SIGN_MESSAGE_PREPARE_MESSAGE)
    {
        uint8_t numberOfKeyDerivations;
        uint32_t derivationIndexes[BTC_GLOBAL_MAXIMAL_NUMBER_OF_KEY_DERIVATIONS];
        uint8_t messageLength;
        uint8_t* message;
        uint32_t offset = 0;
        uint32_t i = 0;

        numberOfKeyDerivations = commandAPDU->data[offset++];

        if ((numberOfKeyDerivations < BTC_GLOBAL_MINIMAL_NUMBER_OF_KEY_DERIVATIONS) ||
            (numberOfKeyDerivations > BTC_GLOBAL_MAXIMAL_NUMBER_OF_KEY_DERIVATIONS))
        {
            sw = APDU_CORE_SW_WRONG_DATA;
            goto END;
        }

        for (i = 0; i < numberOfKeyDerivations; i++)
        {
            derivationIndexes[i] = BTC_MAKEDWORD(
                BTC_MAKEWORD(commandAPDU->data[offset + i * 4 + 3], commandAPDU->data[offset + i * 4 + 2]),
                BTC_MAKEWORD(commandAPDU->data[offset + i * 4 + 1], commandAPDU->data[offset + i * 4]));
        }

        offset += numberOfKeyDerivations * sizeof(uint32_t);

        messageLength = commandAPDU->data[offset++];

        message = &(commandAPDU->data[offset]);

        offset += messageLength;

        if (offset != commandAPDU->lc)
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }

        btcTranMessageSigningInit();

        calleeRetVal =
            btcTranMessageSigningProcessData(derivationIndexes, numberOfKeyDerivations, message, messageLength);

        if (calleeRetVal != BTC_NO_ERROR)
        {
            if (calleeRetVal == BTC_MESSAGE_SIGNING_ERROR)
            {
                sw = APDU_CORE_SW_WRONG_DATA;
                goto END;
            }
            else
            {
                btcHalFatalError();
            }
        }

        responseAPDU->data[0] = BTC_GLOBAL_NO_USER_VALIDATION_REQUESTED;
        responseAPDU->dataLength = 1;
    }
    else if (commandAPDU->p1p2 == BTC_CORE_P1P2_SIGN_MESSAGE_SIGN_MESSAGE)
    {
        uint16_t confirmed = BTC_FALSE;

        if (commandAPDU->lc != 1)
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }

        if (commandAPDU->data[0] != 0x00)
        {
            sw = APDU_CORE_SW_WRONG_DATA;
            goto END;
        }

        btcHalWaitForComfirmation(BTC_FALSE, &confirmed);

        if (confirmed != BTC_TRUE)
        {
            sw = APDU_CORE_SW_CONDITIONS_NOT_SATISFIED;
            goto END;
        }

        calleeRetVal = btcTranMessageSigningSign(responseAPDU->data, &(responseAPDU->dataLength));

        if (calleeRetVal != BTC_NO_ERROR)
        {
            if ((calleeRetVal == BTC_KEY_DERIVATION_ERROR) || (calleeRetVal == BTC_MESSAGE_SIGNING_ERROR))
            {
                sw = APDU_CORE_SW_WRONG_DATA;
                goto END;
            }
            else
            {
                btcHalFatalError();
            }
        }
    }
    else
    {
        btcHalFatalError();
    }

    sw = APDU_CORE_SW_NO_ERROR;

END:
    if (sw != APDU_CORE_SW_NO_ERROR)
    {
        btcTranMessageSigningClearState();
    }

    responseAPDU->sw = sw;
}

static void btcCoreProcessGetFirmwareVersion(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint8_t response[] = BTC_CORE_GET_FIRMWARE_VERSION_RESPONSE;
    uint16_t walletState = BTC_GLOBAL_WALLET_STATE_INITIALIZATION;

    if (commandAPDU->lcPresent != APDU_FALSE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != BTC_CORE_P1P2_GET_FIRMWARE_VERSION)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    walletState = btcHalGetWalletState();

    if (walletState == BTC_GLOBAL_WALLET_STATE_OPERATIONAL)
    {
        response[BTC_CORE_GET_FIRMWARE_VERSION_SPECIAL_VERSION_OFFSET] |=
            BTC_CORE_GET_FIRMWARE_VERSION_SPECIAL_VERSION_WALLET_OPERATIONAL;

        if (btcPinIsPinVerified() == BTC_TRUE)
        {
            response[BTC_CORE_GET_FIRMWARE_VERSION_SPECIAL_VERSION_OFFSET] |=
                BTC_CORE_GET_FIRMWARE_VERSION_SPECIAL_VERSION_PIN_VERIFIED;
        }
    }

    btcHalMemCpy(responseAPDU->data, response, sizeof(response));

    responseAPDU->dataLength = sizeof(response);

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void btcCoreProcessSetKeyboardConfiguration(APDU_CORE_COMMAND_APDU* commandAPDU,
                                                   APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    responseAPDU->sw = APDU_CORE_SW_NO_ERROR;
}

static void btcCoreProcessGetRandom(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
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

    if (commandAPDU->p1p2 != BTC_CORE_P1P2_GET_RANDOM)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    dataLength = commandAPDU->le;

    if (dataLength == 0x00)
    {
        dataLength = 0x100;
    }

    btcHalGetRandom(responseAPDU->data, dataLength);

    responseAPDU->dataLength = dataLength;

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

void btcCoreGetAID(uint8_t* aid, uint32_t* aidLength)
{
    uint8_t aidTemplate[] = BTC_CORE_AID;

    if ((aid == NULL) || (aidLength == NULL))
    {
        btcHalFatalError();
    }

    btcHalMemCpy(aid, aidTemplate, BTC_CORE_AID_LENGTH);

    *aidLength = BTC_CORE_AID_LENGTH;
}

void btcCoreProcessAPDU(uint8_t* apdu, uint32_t* apduLength)
{
    APDU_CORE_COMMAND_APDU commandAPDU;
    APDU_CORE_RESPONSE_APDU responseAPDU;
    uint16_t calleeRetVal = APDU_GENERAL_ERROR;
    uint16_t wipeoutInProgress = BTC_TRUE;
    uint16_t walletState = BTC_GLOBAL_WALLET_STATE_INITIALIZATION;

    if ((apdu == NULL) || (apduLength == NULL))
    {
        btcHalFatalError();
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
            btcHalFatalError();
        }
    }

    wipeoutInProgress = btcHalIsWipeoutInProgress();

    if (wipeoutInProgress != BTC_FALSE)
    {
        btcHalWipeout();
    }

    walletState = btcHalGetWalletState();

    if (walletState == BTC_GLOBAL_WALLET_STATE_OPERATIONAL)
    {
        uint16_t isPinBlocked = BTC_TRUE;

        isPinBlocked = btcPinIsPinBlocked();

        if (isPinBlocked == BTC_TRUE)
        {
            btcHalWipeout();
            walletState = BTC_GLOBAL_WALLET_STATE_INITIALIZATION;
        }
    }

    if (commandAPDU.cla != BTC_CORE_CLA)
    {
        responseAPDU.sw = APDU_CORE_SW_CLA_NOT_SUPPORTED;
        goto END;
    }

    if (walletState == BTC_GLOBAL_WALLET_STATE_INITIALIZATION)
    {
        switch (commandAPDU.ins)
        {
            case BTC_CORE_INS_GET_FIRMWARE_VERSION:
                btcCoreProcessGetFirmwareVersion(&commandAPDU, &responseAPDU);
                break;
            case BTC_CORE_INS_SETUP:
                btcCoreProcessSetup(&commandAPDU, &responseAPDU);
                break;
            case BTC_CORE_INS_GET_RANDOM:
                btcCoreProcessGetRandom(&commandAPDU, &responseAPDU);
                break;
            default:
                responseAPDU.sw = APDU_CORE_SW_INS_NOT_SUPPORTED;
                break;
        }
    }
    else if (walletState == BTC_GLOBAL_WALLET_STATE_OPERATIONAL)
    {
        if (btcCoreWaitingForConfirmation == BTC_TRUE)
        {
            switch (commandAPDU.ins)
            {
                case BTC_CORE_INS_READ_TRANSACTION:
                    btcCoreProcessReadTransaction(&commandAPDU, &responseAPDU);
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
                case BTC_CORE_INS_VERIFY_PIN:
                    btcCoreProcessVerifyPin(&commandAPDU, &responseAPDU);
                    break;
                case BTC_CORE_INS_GET_WALLET_PUBLIC_KEY:
                    btcCoreProcessGetWalletPublicKey(&commandAPDU, &responseAPDU);
                    break;
                case BTC_CORE_INS_GET_TRUSTED_INPUT:
                    btcCoreProcessGetTrustedInput(&commandAPDU, &responseAPDU);
                    break;
                case BTC_CORE_INS_UNTRUSTED_HASH_TRANSACTION_INPUT_START:
                    btcCoreProcessUntrustedHashTransactionInputStart(&commandAPDU, &responseAPDU);
                    break;
                case BTC_CORE_INS_UNTRUSTED_HASH_TRANSACTION_INPUT_FINALIZE_FULL:
                    btcCoreProcessUntrustedHashTransactionInputFinalizeFull(&commandAPDU, &responseAPDU);
                    break;
                case BTC_CORE_INS_UNTRUSTED_HASH_SIGN:
                    btcCoreProcessUntrustedHashSign(&commandAPDU, &responseAPDU);
                    break;
                case BTC_CORE_INS_SIGN_MESSAGE:
                    btcCoreProcessSignMessage(&commandAPDU, &responseAPDU);
                    break;
                case BTC_CORE_INS_GET_FIRMWARE_VERSION:
                    btcCoreProcessGetFirmwareVersion(&commandAPDU, &responseAPDU);
                    break;
                case BTC_CORE_INS_SET_KEYBOARD_CONFIGURATION:
                    btcCoreProcessSetKeyboardConfiguration(&commandAPDU, &responseAPDU);
                    break;
                case BTC_CORE_INS_GET_RANDOM:
                    btcCoreProcessGetRandom(&commandAPDU, &responseAPDU);
                    break;
                default:
                    responseAPDU.sw = APDU_CORE_SW_INS_NOT_SUPPORTED;
                    break;
            }
        }
    }
    else
    {
        btcHalFatalError();
    }

END:
    apduCorePrepareOutgoingAPDU(apdu, apduLength, &responseAPDU);
}
