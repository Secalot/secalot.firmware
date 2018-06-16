/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdint.h>

#include "btcGlobal.h"
#include "btcGlobalInt.h"
#include "btcHal.h"
#include "btcTran.h"
#include "btcTranInt.h"

static uint8_t btcTranParse8BitInt(uint8_t** data, uint32_t* dataRemaining, uint16_t* parsingSuccessful);
static uint32_t btcTranParse32BitInt(uint8_t** data, uint32_t* dataRemaining, uint16_t* parsingSuccessful);
static int64_t btcTranParse64BitInt(uint8_t** data, uint32_t* dataRemaining, uint16_t* parsingSuccessful);
static uint32_t btcTranParseVarInt(uint8_t** data, uint32_t* dataRemaining, uint16_t* parsingSuccessful);
static void btcTranParseArray(uint8_t** data, uint32_t* dataRemaining, uint32_t arrayLength,
                              uint16_t* parsingSuccessful);
static uint16_t btcTranTIGenerationProcessTransactionElement(uint8_t** data, uint32_t* dataLength);
static uint16_t btcTranSigningProcessHeaderAndInputsElement(uint8_t** data, uint32_t* dataLength, uint16_t firstSignatureGenerated);
static uint16_t btcTranSigningProcessOutputsElement(uint8_t** data, uint32_t* dataLength);
static uint16_t btcTranSigningProcessHeaderAndInputsElementSegWit(uint8_t** data, uint32_t* dataLength,
                                                                  uint16_t initialProcessing);
static uint16_t btcTranSigningProcessOutputsElementSegWit(uint8_t** data, uint32_t* dataLength);

static void btcTranSigningClearInputAndOutputProcessingState(void);

static void btcTranUpdateTransactionToDisplay(uint8_t* data, uint16_t dataLength);

static void btcTranSigningClearStateInternal(uint16_t clearSegwitData, uint16_t clearInputAmountsData);

static BTC_TRAN_TI_GENERATION_CONTEXT btcTranTIGenerationContext;
static BTC_TRAN_SIGNING_CONTEXT btcTranSigningContext;
static BTC_TRAN_MESSAGE_SIGNING_CONTEXT btcTranMessageSigningContext;

void btcTranInit(void)
{
    btcTranTIGenerationClearState();
    btcTranSigningClearState();
    btcTranMessageSigningClearState();
}

void btcTranDeinit(void) {}

static void btcTranParseArray(uint8_t** data, uint32_t* dataRemaining, uint32_t arrayLength,
                              uint16_t* parsingSuccessful)
{
    *parsingSuccessful = BTC_FALSE;

    if (*dataRemaining < arrayLength)
    {
        *parsingSuccessful = BTC_FALSE;
        goto END;
    }

    *data += arrayLength;
    *dataRemaining -= arrayLength;

    *parsingSuccessful = BTC_TRUE;

END:
    return;
}

static uint8_t btcTranParse8BitInt(uint8_t** data, uint32_t* dataRemaining, uint16_t* parsingSuccessful)
{
    uint8_t retVal = 0;

    *parsingSuccessful = BTC_FALSE;

    if (*dataRemaining < 1)
    {
        *parsingSuccessful = BTC_FALSE;
        goto END;
    }

    retVal = (*data)[0];

    *data += 1;
    *dataRemaining -= 1;

    *parsingSuccessful = BTC_TRUE;

END:
    return retVal;
}

static uint32_t btcTranParse32BitInt(uint8_t** data, uint32_t* dataRemaining, uint16_t* parsingSuccessful)
{
    uint32_t retVal = 0;

    *parsingSuccessful = BTC_FALSE;

    if (*dataRemaining < 4)
    {
        *parsingSuccessful = BTC_FALSE;
        goto END;
    }

    retVal = BTC_MAKEDWORD(BTC_MAKEWORD((*data)[0], (*data)[1]), BTC_MAKEWORD((*data)[2], (*data)[3]));

    *data += 4;
    *dataRemaining -= 4;

    *parsingSuccessful = BTC_TRUE;

END:
    return retVal;
}

static int64_t btcTranParse64BitInt(uint8_t** data, uint32_t* dataRemaining, uint16_t* parsingSuccessful)
{
    uint32_t retVal = 0;

    *parsingSuccessful = BTC_FALSE;

    if (*dataRemaining < 8)
    {
        *parsingSuccessful = BTC_FALSE;
        goto END;
    }

    retVal = BTC_MAKEQWORD(BTC_MAKEDWORD(BTC_MAKEWORD((*data)[0], (*data)[1]), BTC_MAKEWORD((*data)[2], (*data)[3])),
                           BTC_MAKEDWORD(BTC_MAKEWORD((*data)[4], (*data)[5]), BTC_MAKEWORD((*data)[6], (*data)[7])));

    *data += 8;
    *dataRemaining -= 8;

    *parsingSuccessful = BTC_TRUE;

END:
    return retVal;
}

static uint32_t btcTranParseVarInt(uint8_t** data, uint32_t* dataRemaining, uint16_t* parsingSuccessful)
{
    uint32_t retVal = 0;

    *parsingSuccessful = BTC_FALSE;

    if (*dataRemaining < 1)
    {
        *parsingSuccessful = BTC_FALSE;
        goto END;
    }

    if ((*data)[0] < 0xFD)
    {
        retVal = (uint32_t)(*data)[0];
        (*dataRemaining)--;
        (*data) += 1;
    }
    else if ((*data)[0] == 0xFD)
    {
        if (*dataRemaining < 3)
        {
            *parsingSuccessful = BTC_FALSE;
            goto END;
        }

        retVal = BTC_MAKEWORD((*data)[1], (*data)[2]);
        *dataRemaining -= 3;
        (*data) += 3;
    }
    else if ((*data)[0] == 0xFE)
    {
        if (*dataRemaining < 5)
        {
            *parsingSuccessful = BTC_FALSE;
            goto END;
        }

        retVal = BTC_MAKEDWORD(BTC_MAKEWORD((*data)[1], (*data)[2]), BTC_MAKEWORD((*data)[3], (*data)[4]));
        *dataRemaining -= 5;
        (*data) += 5;
    }

    *parsingSuccessful = BTC_TRUE;

END:
    return retVal;
}

void btcTranTIGenerationClearState(void)
{
    btcTranTIGenerationContext.currentInputNumber = 0;
    btcTranTIGenerationContext.currentOutputNumber = 0;
    btcTranTIGenerationContext.totalNumberOfInputs = 0;
    btcTranTIGenerationContext.totalNumberOfOutputs = 0;
    btcTranTIGenerationContext.remainingScriptLength = 0;
    btcTranTIGenerationContext.state = BTC_TRAN_TI_GENERATION_STATE_WAITING_FOR_RESET;
    btcTranTIGenerationContext.outputNumberToGetAmountOf = BTC_TRAN_INVALID_INPUT_OUTPUT_NUMBER;
    btcTranTIGenerationContext.amount = 0;
    btcTranTIGenerationContext.outputWithRequestedIndexFound = BTC_FALSE;

    btcHalSha256Start(BTC_HAL_HASH_ID_TRUSTED_INPUT);
}

void btcTranTIGenerationInit(uint32_t outputNumberToGetAmountOf)
{
    btcTranTIGenerationClearState();
    btcTranTIGenerationContext.outputNumberToGetAmountOf = outputNumberToGetAmountOf;
    btcTranTIGenerationContext.state = BTC_TRAN_TI_GENERATION_STATE_PARSING_VERSION;
}

static uint16_t btcTranTIGenerationProcessTransactionElement(uint8_t** data, uint32_t* dataLength)
{
    uint16_t retVal = BTC_GENERAL_ERROR;
    uint16_t parsingSuccessful = BTC_FALSE;

    switch (btcTranTIGenerationContext.state)
    {
        case BTC_TRAN_TI_GENERATION_STATE_PARSING_VERSION:
        {
            btcTranParse32BitInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            btcTranTIGenerationContext.state = BTC_TRAN_TI_GENERATION_STATE_PARSING_NUMBER_OF_INPUTS;
        }
        break;
        case BTC_TRAN_TI_GENERATION_STATE_PARSING_NUMBER_OF_INPUTS:
        {
            btcTranTIGenerationContext.totalNumberOfInputs = btcTranParseVarInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            btcTranTIGenerationContext.state = BTC_TRAN_TI_GENERATION_STATE_PARSING_INPUT_PREVOUT;
        }
        break;
        case BTC_TRAN_TI_GENERATION_STATE_PARSING_INPUT_PREVOUT:
        {
            btcTranParseArray(data, dataLength, BTC_GLOBAL_OUTPOINT_LENGTH, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            btcTranTIGenerationContext.state = BTC_TRAN_TI_GENERATION_STATE_PARSING_INPUT_SIGSCRIPT_LENGTH;
        }
        break;
        case BTC_TRAN_TI_GENERATION_STATE_PARSING_INPUT_SIGSCRIPT_LENGTH:
        {
            btcTranTIGenerationContext.remainingScriptLength = btcTranParseVarInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            btcTranTIGenerationContext.state = BTC_TRAN_TI_GENERATION_STATE_PARSING_INPUT_SIGSCRIPT;
        }
        break;
        case BTC_TRAN_TI_GENERATION_STATE_PARSING_INPUT_SIGSCRIPT:
        {
            if ((*dataLength) >= btcTranTIGenerationContext.remainingScriptLength)
            {
                btcTranParseArray(data, dataLength, btcTranTIGenerationContext.remainingScriptLength,
                                  &parsingSuccessful);
                if (parsingSuccessful != BTC_TRUE)
                {
                    retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                    goto END;
                }

                btcTranTIGenerationContext.remainingScriptLength = 0;
                btcTranTIGenerationContext.state = BTC_TRAN_TI_GENERATION_STATE_PARSING_INPUT_SEQUENCE;
            }
            else
            {
                btcTranTIGenerationContext.remainingScriptLength -= (*dataLength);

                btcTranParseArray(data, dataLength, (*dataLength), &parsingSuccessful);
                if (parsingSuccessful != BTC_TRUE)
                {
                    retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                    goto END;
                }
            }
        }
        break;
        case BTC_TRAN_TI_GENERATION_STATE_PARSING_INPUT_SEQUENCE:
        {
            btcTranParse32BitInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            btcTranTIGenerationContext.currentInputNumber++;

            if (btcTranTIGenerationContext.currentInputNumber == btcTranTIGenerationContext.totalNumberOfInputs)
            {
                btcTranTIGenerationContext.state = BTC_TRAN_TI_GENERATION_STATE_PARSING_NUMBER_OF_OUTPUTS;
            }
            else
            {
                btcTranTIGenerationContext.state = BTC_TRAN_TI_GENERATION_STATE_PARSING_INPUT_PREVOUT;
            }
        }
        break;
        case BTC_TRAN_TI_GENERATION_STATE_PARSING_NUMBER_OF_OUTPUTS:
        {
            btcTranTIGenerationContext.totalNumberOfOutputs = btcTranParseVarInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            btcTranTIGenerationContext.state = BTC_TRAN_TI_GENERATION_STATE_PARSING_OUTPUT_VALUE;
        }
        break;
        case BTC_TRAN_TI_GENERATION_STATE_PARSING_OUTPUT_VALUE:
        {
            int64_t amount;

            amount = btcTranParse64BitInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            if (amount < 0)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            if (btcTranTIGenerationContext.outputNumberToGetAmountOf == BTC_TRAN_INVALID_INPUT_OUTPUT_NUMBER)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            if (btcTranTIGenerationContext.outputNumberToGetAmountOf == btcTranTIGenerationContext.currentOutputNumber)
            {
                btcTranTIGenerationContext.outputWithRequestedIndexFound = BTC_TRUE;
                btcTranTIGenerationContext.amount = amount;
            }

            btcTranTIGenerationContext.state = BTC_TRAN_TI_GENERATION_STATE_PARSING_OUTPUT_PKSCRIPT_LENGTH;
        }
        break;
        case BTC_TRAN_TI_GENERATION_STATE_PARSING_OUTPUT_PKSCRIPT_LENGTH:
        {
            btcTranTIGenerationContext.remainingScriptLength = btcTranParseVarInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            btcTranTIGenerationContext.state = BTC_TRAN_TI_GENERATION_STATE_PARSING_OUTPUT_PKSCRIPT;
        }
        break;
        case BTC_TRAN_TI_GENERATION_STATE_PARSING_OUTPUT_PKSCRIPT:
        {
            if ((*dataLength) >= btcTranTIGenerationContext.remainingScriptLength)
            {
                btcTranParseArray(data, dataLength, btcTranTIGenerationContext.remainingScriptLength,
                                  &parsingSuccessful);
                if (parsingSuccessful != BTC_TRUE)
                {
                    retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                    goto END;
                }

                btcTranTIGenerationContext.remainingScriptLength = 0;

                btcTranTIGenerationContext.currentOutputNumber++;

                if (btcTranTIGenerationContext.currentOutputNumber == btcTranTIGenerationContext.totalNumberOfOutputs)
                {
                    btcTranTIGenerationContext.state = BTC_TRAN_TI_GENERATION_STATE_PARSING_LOCKTIME;
                }
                else
                {
                    btcTranTIGenerationContext.state = BTC_TRAN_TI_GENERATION_STATE_PARSING_OUTPUT_VALUE;
                }
            }
            else
            {
                btcTranTIGenerationContext.remainingScriptLength -= (*dataLength);

                btcTranParseArray(data, dataLength, (*dataLength), &parsingSuccessful);
                if (parsingSuccessful != BTC_TRUE)
                {
                    retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                    goto END;
                }
            }
        }
        break;
        case BTC_TRAN_TI_GENERATION_STATE_PARSING_LOCKTIME:
        {
            btcTranParse32BitInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            btcTranTIGenerationContext.state = BTC_TRAN_TI_GENERATION_STATE_PARSING_FINISHED;
        }
        break;
        case BTC_TRAN_TI_GENERATION_STATE_PARSING_FINISHED:
        {
            retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
            goto END;
        }
        default:
            btcHalFatalError();
            break;
    }

    retVal = BTC_NO_ERROR;

END:

    if (retVal != BTC_NO_ERROR)
    {
        btcTranTIGenerationClearState();
    }
    return retVal;
}

uint16_t btcTranTIGenerationProcessTransaction(uint8_t* data, uint32_t dataLength, uint16_t* action)
{
    uint16_t retVal = BTC_GENERAL_ERROR;
    uint16_t calleeRetVal = BTC_GENERAL_ERROR;

    if ((data == NULL) || (action == NULL))
    {
        btcHalFatalError();
    }
    if (btcTranTIGenerationContext.state == BTC_TRAN_TI_GENERATION_STATE_WAITING_FOR_RESET)
    {
        retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
        goto END;
    }

    btcHalSha256Update(BTC_HAL_HASH_ID_TRUSTED_INPUT, data, dataLength);

    while (dataLength != 0)
    {
        calleeRetVal = btcTranTIGenerationProcessTransactionElement(&data, &dataLength);

        if (calleeRetVal != BTC_NO_ERROR)
        {
            retVal = calleeRetVal;
            goto END;
        }
    }

    if (btcTranTIGenerationContext.state == BTC_TRAN_TI_GENERATION_STATE_PARSING_FINISHED)
    {
        if (btcTranTIGenerationContext.outputWithRequestedIndexFound != BTC_TRUE)
        {
            retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
            goto END;
        }

        *action = BTC_TRAN_RETREIVE_OUTPUT;
    }
    else
    {
        *action = BTC_TRAN_GIVE_MORE_INPUT;
    }

    retVal = BTC_NO_ERROR;

END:
    if (retVal != BTC_NO_ERROR)
    {
        btcTranTIGenerationClearState();
    }

    return retVal;
}

void btcTranTIGenerationGetHashIndexAndAmount(uint8_t* data)
{
    if (data == NULL)
    {
        btcHalFatalError();
    }

    if (btcTranTIGenerationContext.state != BTC_TRAN_TI_GENERATION_STATE_PARSING_FINISHED)
    {
        btcHalFatalError();
    }

    btcHalSha256Finalize(BTC_HAL_HASH_ID_TRUSTED_INPUT, data);

    btcHalSha256Start(BTC_HAL_HASH_ID_TRUSTED_INPUT);
    btcHalSha256Update(BTC_HAL_HASH_ID_TRUSTED_INPUT, data, BTC_GLOBAL_SHA256_SIZE);
    btcHalSha256Finalize(BTC_HAL_HASH_ID_TRUSTED_INPUT, data);

    data[BTC_GLOBAL_SHA256_SIZE] = (btcTranTIGenerationContext.outputNumberToGetAmountOf);
    data[BTC_GLOBAL_SHA256_SIZE + 1] = (btcTranTIGenerationContext.outputNumberToGetAmountOf >> 8);
    data[BTC_GLOBAL_SHA256_SIZE + 2] = (btcTranTIGenerationContext.outputNumberToGetAmountOf >> 16);
    data[BTC_GLOBAL_SHA256_SIZE + 3] = (btcTranTIGenerationContext.outputNumberToGetAmountOf >> 24);

    data[BTC_GLOBAL_SHA256_SIZE + 4] = (btcTranTIGenerationContext.amount);
    data[BTC_GLOBAL_SHA256_SIZE + 5] = (btcTranTIGenerationContext.amount >> 8);
    data[BTC_GLOBAL_SHA256_SIZE + 6] = (btcTranTIGenerationContext.amount >> 16);
    data[BTC_GLOBAL_SHA256_SIZE + 7] = (btcTranTIGenerationContext.amount >> 24);
    data[BTC_GLOBAL_SHA256_SIZE + 8] = (btcTranTIGenerationContext.amount >> 32);
    data[BTC_GLOBAL_SHA256_SIZE + 9] = (btcTranTIGenerationContext.amount >> 40);
    data[BTC_GLOBAL_SHA256_SIZE + 10] = (btcTranTIGenerationContext.amount >> 48);
    data[BTC_GLOBAL_SHA256_SIZE + 11] = (btcTranTIGenerationContext.amount >> 56);

    btcTranTIGenerationClearState();
}

static void btcTranUpdateTransactionToDisplay(uint8_t* data, uint16_t dataLength)
{
	if( (btcTranSigningContext.transactionToDisplay.currentOffset +  dataLength) > BTC_TRAN_MAX_VIEWABLE_TRANSACTION_SIZE )
	{
		btcTranSigningContext.transactionToDisplay.transactionTooBigToDisplay = BTC_TRUE;
	}
	else
	{
		btcHalMemCpy(btcTranSigningContext.transactionToDisplay.transaction+btcTranSigningContext.transactionToDisplay.currentOffset, data, dataLength);
		btcTranSigningContext.transactionToDisplay.currentOffset += dataLength;
	}
}

void btcTranSigningClearState(void)
{
	btcTranSigningClearStateInternal(BTC_TRUE, BTC_TRUE);
}

static void btcTranSigningClearStateInternal(uint16_t clearSegwitData, uint16_t clearInputAmountsData)
{
    btcTranSigningContext.state = BTC_TRAN_SIGNING_STATE_WAITING_FOR_RESET;
    btcTranSigningContext.firstSignatureGenerated = BTC_FALSE;
    btcTranSigningContext.headerAndInputsProcessingState = BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_VERSION;
    btcTranSigningContext.outputsProcessingState = BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_NUMBER_OF_OUTPUTS;
    btcTranSigningContext.currentInputNumber = 0;
    btcTranSigningContext.totalNumberOfInputs = 0;
    btcTranSigningContext.currentOutputNumber = 0;
    btcTranSigningContext.totalNumberOfOutputs = 0;
    btcTranSigningContext.remainingScriptLength = 0;

    btcHalMemSet(btcTranSigningContext.segWitCurrentInputAmount, 0x00,
                 sizeof(btcTranSigningContext.segWitCurrentInputAmount));

    btcHalMemSet(btcTranSigningContext.hashOfTransactionWithoutInputScripts, 0x00, BTC_GLOBAL_SHA256_SIZE);

    if(clearSegwitData == BTC_TRUE)
    {
        btcTranSigningContext.segWit = BTC_FALSE;
        btcTranSigningContext.segwitSignatureNumber = 0;

        btcHalMemSet(btcTranSigningContext.segWitHashPrevouts, 0x00, BTC_GLOBAL_SHA256_SIZE);
        btcHalMemSet(btcTranSigningContext.segWitHashSequence, 0x00, BTC_GLOBAL_SHA256_SIZE);
        btcHalMemSet(btcTranSigningContext.segWitHashOutputs, 0x00, BTC_GLOBAL_SHA256_SIZE);
    }

    btcHalSha256Start(BTC_HAL_HASH_ID_TRANSACTION_SIGNING);
    btcHalSha256Start(BTC_HAL_HASH_ID_TRANSACTION_INTEGRITY_CHECK);

    btcHalSha256Start(BTC_HAL_HASH_ID_SEGWIT_PREVOUTS);
    btcHalSha256Start(BTC_HAL_HASH_ID_SEGWIT_SEQUENCE);
    btcHalSha256Start(BTC_HAL_HASH_ID_SEGWIT_OUTPUTS);

    btcTranSigningContext.transactionToDisplay.currentOffset = 0;
    btcTranSigningContext.transactionToDisplay.transactionTooBigToDisplay = BTC_FALSE;
    btcHalMemSet(btcTranSigningContext.transactionToDisplay.transaction, 0x00, BTC_TRAN_MAX_VIEWABLE_TRANSACTION_SIZE);

    if(clearInputAmountsData == BTC_TRUE)
    {
    	uint32_t i;

    	for(i=0; i<BTC_TRANS_MAX_NUMBER_OF_INPUTS; i++)
    	{
    		btcTranSigningContext.inputAmounts[i] = 0x00;
    	}

    	btcTranSigningContext.numberOfInputAmounts = 0;
    }
}

static void btcTranSigningClearInputAndOutputProcessingState(void)
{
    btcTranSigningContext.headerAndInputsProcessingState = BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_VERSION;
    btcTranSigningContext.outputsProcessingState = BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_NUMBER_OF_OUTPUTS;
    btcTranSigningContext.currentInputNumber = 0;
    btcTranSigningContext.totalNumberOfInputs = 0;
    btcTranSigningContext.currentOutputNumber = 0;
    btcTranSigningContext.totalNumberOfOutputs = 0;
    btcTranSigningContext.remainingScriptLength = 0;
}

void btcTranSigningInit(uint16_t segWit)
{
    btcTranSigningClearState();
    btcTranSigningContext.state = BTC_TRAN_SIGNING_STATE_PROCESSING_HEADER_AND_INPUTS;
    btcTranSigningContext.segWit = segWit;
}

static uint16_t btcTranSigningProcessOutputsElement(uint8_t** data, uint32_t* dataLength)
{
    uint16_t retVal = BTC_GENERAL_ERROR;
    uint16_t parsingSuccessful = BTC_FALSE;
    uint8_t* originalDataPointer = *data;
    uint32_t originalDataLength = *dataLength;

    switch (btcTranSigningContext.outputsProcessingState)
    {
        case BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_NUMBER_OF_OUTPUTS:
        {
            btcTranSigningContext.totalNumberOfOutputs = btcTranParseVarInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            btcTranSigningContext.outputsProcessingState = BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_AMOUNT;
        }
        break;
        case BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_AMOUNT:
        {
            int64_t amount;

            amount = btcTranParse64BitInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            if (amount < 0)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            btcTranSigningContext.outputsProcessingState =
                BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_PUBKEYSCRIPT_LENGTH;
        }
        break;
        case BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_PUBKEYSCRIPT_LENGTH:
        {
            btcTranSigningContext.remainingScriptLength = btcTranParseVarInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            btcTranSigningContext.outputsProcessingState = BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_PUBKEYSCRIPT;
        }
        break;
        case BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_PUBKEYSCRIPT:
        {
            if ((*dataLength) >= btcTranSigningContext.remainingScriptLength)
            {
                btcTranParseArray(data, dataLength, btcTranSigningContext.remainingScriptLength, &parsingSuccessful);
                if (parsingSuccessful != BTC_TRUE)
                {
                    retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                    goto END;
                }

                btcTranSigningContext.remainingScriptLength = 0;

                btcTranSigningContext.currentOutputNumber++;

                if (btcTranSigningContext.currentOutputNumber == btcTranSigningContext.totalNumberOfOutputs)
                {
                    btcTranSigningContext.outputsProcessingState = BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_FINISHED;
                }
                else
                {
                    btcTranSigningContext.outputsProcessingState = BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_AMOUNT;
                }
            }
            else
            {
                btcTranSigningContext.remainingScriptLength -= (*dataLength);

                btcTranParseArray(data, dataLength, (*dataLength), &parsingSuccessful);
                if (parsingSuccessful != BTC_TRUE)
                {
                    retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                    goto END;
                }
            }
        }
        break;
        case BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_FINISHED:
        {
            retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
            goto END;
        }
        break;
        default:
            btcHalFatalError();
            break;
    }

    btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_SIGNING, originalDataPointer, (originalDataLength - (*dataLength)));
    btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_INTEGRITY_CHECK, originalDataPointer,
                       (originalDataLength - (*dataLength)));

    retVal = BTC_NO_ERROR;

END:

    if (retVal != BTC_NO_ERROR)
    {
        btcTranSigningClearState();
    }
    return retVal;
}

static uint16_t btcTranSigningProcessHeaderAndInputsElement(uint8_t** data, uint32_t* dataLength, uint16_t firstSignatureGenerated)
{
    uint16_t retVal = BTC_GENERAL_ERROR;
    uint16_t parsingSuccessful = BTC_FALSE;
    uint8_t* originalDataPointer = *data;
    uint32_t originalDataLength = *dataLength;

    switch (btcTranSigningContext.headerAndInputsProcessingState)
    {
        case BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_VERSION:
        {
            btcTranParse32BitInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_SIGNING, originalDataPointer, sizeof(uint32_t));
            btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_INTEGRITY_CHECK, originalDataPointer, sizeof(uint32_t));

            if(firstSignatureGenerated == BTC_FALSE)
            {
            	btcTranUpdateTransactionToDisplay(originalDataPointer, sizeof(uint32_t));
            }

            btcTranSigningContext.headerAndInputsProcessingState =
                BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_NUMBER_OF_INPUTS;
        }
        break;
        case BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_NUMBER_OF_INPUTS:
        {
            btcTranSigningContext.totalNumberOfInputs = btcTranParseVarInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_SIGNING, originalDataPointer,
                               (originalDataLength - (*dataLength)));
            btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_INTEGRITY_CHECK, originalDataPointer,
                               (originalDataLength - (*dataLength)));

            if(firstSignatureGenerated == BTC_FALSE)
            {
            	btcTranUpdateTransactionToDisplay(originalDataPointer, (originalDataLength - (*dataLength)));
            }

            btcTranSigningContext.headerAndInputsProcessingState =
                BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_TI_OR_SWGWIT_FLAG;
        }
        break;
        case BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_TI_OR_SWGWIT_FLAG:
        {
            uint8_t trustedInputFlag;

            trustedInputFlag = btcTranParse8BitInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            if (trustedInputFlag != BTC_TRAN_TRUSTED_INPUT_FLAG)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            btcTranSigningContext.headerAndInputsProcessingState =
                BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_TI_OR_SEGWIT_PREVOUT;
        }
        break;

        case BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_TI_OR_SEGWIT_PREVOUT:
        {
            uint8_t trustedInputSize;
            uint16_t calleeRetVal = BTC_GENERAL_ERROR;
            uint8_t* blob;
            int64_t amount;

            trustedInputSize = btcTranParse8BitInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            if (trustedInputSize != BTC_GLOBAL_TRUSTED_INPUT_BLOB_LENGTH)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            btcTranParseArray(data, dataLength, BTC_GLOBAL_TRUSTED_INPUT_BLOB_LENGTH, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            blob = (*data) - BTC_GLOBAL_TRUSTED_INPUT_BLOB_LENGTH;

            calleeRetVal = btcHalCheckTrustedInputMAC(blob);

            if (calleeRetVal != BTC_NO_ERROR)
            {
                if (calleeRetVal == BTC_INVALID_MAC_ERROR)
                {
                    retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                    goto END;
                }
                else
                {
                    btcHalFatalError();
                }
            }

            amount = BTC_MAKEQWORD(BTC_MAKEDWORD(BTC_MAKEWORD(blob[BTC_GLOBAL_TRUSTED_INPUT_BLOB_AMOUNT_OFFSET],
                                                              blob[BTC_GLOBAL_TRUSTED_INPUT_BLOB_AMOUNT_OFFSET + 1]),
                                                 BTC_MAKEWORD(blob[BTC_GLOBAL_TRUSTED_INPUT_BLOB_AMOUNT_OFFSET + 2],
                                                              blob[BTC_GLOBAL_TRUSTED_INPUT_BLOB_AMOUNT_OFFSET + 3])),
                                   BTC_MAKEDWORD(BTC_MAKEWORD(blob[BTC_GLOBAL_TRUSTED_INPUT_BLOB_AMOUNT_OFFSET + 4],
                                                              blob[BTC_GLOBAL_TRUSTED_INPUT_BLOB_AMOUNT_OFFSET + 5]),
                                                 BTC_MAKEWORD(blob[BTC_GLOBAL_TRUSTED_INPUT_BLOB_AMOUNT_OFFSET + 6],
                                                              blob[BTC_GLOBAL_TRUSTED_INPUT_BLOB_AMOUNT_OFFSET + 7])));

            if (amount < 0)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_SIGNING, &blob[BTC_GLOBAL_TRUSTED_INPUT_BLOB_HASH_OFFSET],
                               BTC_GLOBAL_TRUSTED_INPUT_BLOB_HASH_LENGTH);
            btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_SIGNING, &blob[BTC_GLOBAL_TRUSTED_INPUT_BLOB_INDEX_OFFSET],
                               BTC_GLOBAL_TRUSTED_INPUT_BLOB_INDEX_LENGTH);
            btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_INTEGRITY_CHECK,
                               &blob[BTC_GLOBAL_TRUSTED_INPUT_BLOB_HASH_OFFSET],
                               BTC_GLOBAL_TRUSTED_INPUT_BLOB_HASH_LENGTH);
            btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_INTEGRITY_CHECK,
                               &blob[BTC_GLOBAL_TRUSTED_INPUT_BLOB_INDEX_OFFSET],
                               BTC_GLOBAL_TRUSTED_INPUT_BLOB_INDEX_LENGTH);

            if(firstSignatureGenerated == BTC_FALSE)
            {
            	if(btcTranSigningContext.currentInputNumber > BTC_TRANS_MAX_NUMBER_OF_INPUTS)
            	{
                    retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                    goto END;
            	}

            	btcTranSigningContext.inputAmounts[btcTranSigningContext.currentInputNumber] = amount;
            	btcTranSigningContext.numberOfInputAmounts++;

            	btcTranUpdateTransactionToDisplay(&blob[BTC_GLOBAL_TRUSTED_INPUT_BLOB_HASH_OFFSET], BTC_GLOBAL_TRUSTED_INPUT_BLOB_HASH_LENGTH);
            	btcTranUpdateTransactionToDisplay(&blob[BTC_GLOBAL_TRUSTED_INPUT_BLOB_INDEX_OFFSET],BTC_GLOBAL_TRUSTED_INPUT_BLOB_INDEX_LENGTH);
            }

            btcTranSigningContext.headerAndInputsProcessingState =
                BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_INPUT_SIGSCRIPT_LENGTH;
        }
        break;
        case BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_INPUT_SIGSCRIPT_LENGTH:
        {
            btcTranSigningContext.remainingScriptLength = btcTranParseVarInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_SIGNING, originalDataPointer,
                               (originalDataLength - (*dataLength)));

            if(firstSignatureGenerated == BTC_FALSE)
            {
            	btcTranUpdateTransactionToDisplay(originalDataPointer, (originalDataLength - (*dataLength)));
            }

            btcTranSigningContext.headerAndInputsProcessingState =
                BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_INPUT_SIGSCRIPT;
        }
        break;
        case BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_INPUT_SIGSCRIPT:
        {
            if ((*dataLength) >= btcTranSigningContext.remainingScriptLength)
            {
                btcTranParseArray(data, dataLength, btcTranSigningContext.remainingScriptLength, &parsingSuccessful);
                if (parsingSuccessful != BTC_TRUE)
                {
                    retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                    goto END;
                }

                btcTranSigningContext.remainingScriptLength = 0;
                btcTranSigningContext.headerAndInputsProcessingState =
                    BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_INPUT_SEQUENCE;
            }
            else
            {
                btcTranSigningContext.remainingScriptLength -= (*dataLength);

                btcTranParseArray(data, dataLength, (*dataLength), &parsingSuccessful);
                if (parsingSuccessful != BTC_TRUE)
                {
                    retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                    goto END;
                }
            }

            btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_SIGNING, originalDataPointer,
                               (originalDataLength - (*dataLength)));

            if(firstSignatureGenerated == BTC_FALSE)
            {
            	btcTranUpdateTransactionToDisplay(originalDataPointer, (originalDataLength - (*dataLength)));
            }

        }
        break;
        case BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_INPUT_SEQUENCE:
        {
            btcTranParse32BitInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_SIGNING, originalDataPointer, sizeof(uint32_t));
            btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_INTEGRITY_CHECK, originalDataPointer, sizeof(uint32_t));

            if(firstSignatureGenerated == BTC_FALSE)
            {
            	btcTranUpdateTransactionToDisplay(originalDataPointer, sizeof(uint32_t));
            }

            btcTranSigningContext.currentInputNumber++;

            if (btcTranSigningContext.currentInputNumber == btcTranSigningContext.totalNumberOfInputs)
            {
                btcTranSigningContext.headerAndInputsProcessingState =
                    BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_FINISHED;
            }
            else
            {
                btcTranSigningContext.headerAndInputsProcessingState =
                    BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_TI_OR_SWGWIT_FLAG;
            }
        }
        break;
        case BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_FINISHED:
        {
            retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
            goto END;
        }
        break;
        default:
            btcHalFatalError();
            break;
    }

    retVal = BTC_NO_ERROR;

END:

    if (retVal != BTC_NO_ERROR)
    {
        btcTranSigningClearState();
    }
    return retVal;
}

static uint16_t btcTranSigningProcessHeaderAndInputsElementSegWit(uint8_t** data, uint32_t* dataLength,
                                                                  uint16_t initialProcessing)
{
    uint16_t retVal = BTC_GENERAL_ERROR;
    uint16_t parsingSuccessful = BTC_FALSE;
    uint8_t* originalDataPointer = *data;
    uint32_t originalDataLength = *dataLength;

    switch (btcTranSigningContext.headerAndInputsProcessingState)
    {
        case BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_VERSION:
        {
            btcTranParse32BitInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            if (initialProcessing == BTC_TRUE)
            {
            	btcTranUpdateTransactionToDisplay(originalDataPointer, sizeof(uint32_t));
            }
            else
            {
                btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_SIGNING, originalDataPointer, sizeof(uint32_t));
                btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_SIGNING, btcTranSigningContext.segWitHashPrevouts,
                                   BTC_GLOBAL_SHA256_SIZE);
                btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_SIGNING, btcTranSigningContext.segWitHashSequence,
                                   BTC_GLOBAL_SHA256_SIZE);
            }

            btcTranSigningContext.headerAndInputsProcessingState =
                BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_NUMBER_OF_INPUTS;
        }
        break;
        case BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_NUMBER_OF_INPUTS:
        {
            btcTranSigningContext.totalNumberOfInputs = btcTranParseVarInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            if (initialProcessing == BTC_TRUE)
            {
				btcTranUpdateTransactionToDisplay(originalDataPointer, (originalDataLength - (*dataLength)));
            }
            else
            {
                if (btcTranSigningContext.totalNumberOfInputs != 1)
                {
                    retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                    goto END;
                }
            }

            btcTranSigningContext.headerAndInputsProcessingState =
                BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_TI_OR_SWGWIT_FLAG;
        }
        break;
        case BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_TI_OR_SWGWIT_FLAG:
        {
            uint8_t segwitFlag;

            segwitFlag = btcTranParse8BitInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            if (segwitFlag != BTC_TRAN_SEGWIT_FLAG)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            btcTranSigningContext.headerAndInputsProcessingState =
                BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_TI_OR_SEGWIT_PREVOUT;
        }
        break;

        case BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_TI_OR_SEGWIT_PREVOUT:
        {
            uint16_t calleeRetVal = BTC_GENERAL_ERROR;
            int64_t amount;

            btcTranParseArray(data, dataLength, BTC_GLOBAL_SHA256_SIZE, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            btcTranParse32BitInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            amount = btcTranParse64BitInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            if (amount < 0)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            if (initialProcessing == BTC_TRUE)
            {
            	if(btcTranSigningContext.currentInputNumber > BTC_TRANS_MAX_NUMBER_OF_INPUTS)
            	{
                    retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                    goto END;
            	}

            	btcTranSigningContext.inputAmounts[btcTranSigningContext.currentInputNumber] = amount;
            	btcTranSigningContext.numberOfInputAmounts++;

                btcHalSha256Update(BTC_HAL_HASH_ID_SEGWIT_PREVOUTS, originalDataPointer,
                                   BTC_GLOBAL_SHA256_SIZE + sizeof(uint32_t));

                btcTranUpdateTransactionToDisplay(originalDataPointer, BTC_GLOBAL_SHA256_SIZE + sizeof(uint32_t));
            }
            else
            {
            	if(btcTranSigningContext.segwitSignatureNumber > BTC_TRANS_MAX_NUMBER_OF_INPUTS)
            	{
                    retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                    goto END;
            	}

            	if(btcTranSigningContext.inputAmounts[btcTranSigningContext.segwitSignatureNumber] != amount)
            	{
                    retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                    goto END;
            	}

                btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_SIGNING, originalDataPointer,
                                   BTC_GLOBAL_SHA256_SIZE + sizeof(uint32_t));
                btcHalMemCpy(btcTranSigningContext.segWitCurrentInputAmount,
                             originalDataPointer + BTC_GLOBAL_SHA256_SIZE + sizeof(uint32_t),
                             sizeof(btcTranSigningContext.segWitCurrentInputAmount));
            }

            btcTranSigningContext.headerAndInputsProcessingState =
                BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_INPUT_SIGSCRIPT_LENGTH;
        }
        break;
        case BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_INPUT_SIGSCRIPT_LENGTH:
        {
            btcTranSigningContext.remainingScriptLength = btcTranParseVarInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            if (initialProcessing == BTC_TRUE)
            {
            	btcTranUpdateTransactionToDisplay(originalDataPointer, (originalDataLength - (*dataLength)));
            }
            else
            {
                btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_SIGNING, originalDataPointer,
                                   (originalDataLength - (*dataLength)));
            }

            btcTranSigningContext.headerAndInputsProcessingState =
                BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_INPUT_SIGSCRIPT;
        }
        break;
        case BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_INPUT_SIGSCRIPT:
        {
            if ((*dataLength) >= btcTranSigningContext.remainingScriptLength)
            {
                btcTranParseArray(data, dataLength, btcTranSigningContext.remainingScriptLength, &parsingSuccessful);
                if (parsingSuccessful != BTC_TRUE)
                {
                    retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                    goto END;
                }

                if (initialProcessing == BTC_TRUE)
                {
                	btcTranUpdateTransactionToDisplay(originalDataPointer, (originalDataLength - (*dataLength)));
                }
                else
                {
                    btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_SIGNING, originalDataPointer,
                                       (originalDataLength - (*dataLength)));
                }

                btcTranSigningContext.remainingScriptLength = 0;
                btcTranSigningContext.headerAndInputsProcessingState =
                    BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_INPUT_SEQUENCE;
            }
            else
            {
                btcTranSigningContext.remainingScriptLength -= (*dataLength);

                btcTranParseArray(data, dataLength, (*dataLength), &parsingSuccessful);
                if (parsingSuccessful != BTC_TRUE)
                {
                    retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                    goto END;
                }

                if (initialProcessing == BTC_TRUE)
                {
                	btcTranUpdateTransactionToDisplay(originalDataPointer, (originalDataLength - (*dataLength)));
                }
                else
                {
                    btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_SIGNING, originalDataPointer,
                                       (originalDataLength - (*dataLength)));
                }
            }
        }
        break;
        case BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_INPUT_SEQUENCE:
        {
            btcTranParse32BitInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            if (initialProcessing == BTC_TRUE)
            {
                btcHalSha256Update(BTC_HAL_HASH_ID_SEGWIT_SEQUENCE, originalDataPointer, sizeof(uint32_t));
                btcTranUpdateTransactionToDisplay(originalDataPointer, sizeof(uint32_t));
            }
            else
            {
                btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_SIGNING, btcTranSigningContext.segWitCurrentInputAmount,
                                   sizeof(btcTranSigningContext.segWitCurrentInputAmount));
                btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_SIGNING, originalDataPointer, sizeof(uint32_t));
                btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_SIGNING, btcTranSigningContext.segWitHashOutputs,
                                   BTC_GLOBAL_SHA256_SIZE);
            }

            btcTranSigningContext.currentInputNumber++;

            if (btcTranSigningContext.currentInputNumber == btcTranSigningContext.totalNumberOfInputs)
            {
                btcTranSigningContext.headerAndInputsProcessingState =
                    BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_FINISHED;
            }
            else
            {
                btcTranSigningContext.headerAndInputsProcessingState =
                    BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_TI_OR_SWGWIT_FLAG;
            }
        }
        break;
        case BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_FINISHED:
        {
            retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
            goto END;
        }
        break;
        default:
            btcHalFatalError();
            break;
    }

    retVal = BTC_NO_ERROR;

END:

    if (retVal != BTC_NO_ERROR)
    {
        btcTranSigningClearState();
    }
    return retVal;
}

static uint16_t btcTranSigningProcessOutputsElementSegWit(uint8_t** data, uint32_t* dataLength)
{
    uint16_t retVal = BTC_GENERAL_ERROR;
    uint16_t parsingSuccessful = BTC_FALSE;
    uint8_t* originalDataPointer = *data;
    uint32_t originalDataLength = *dataLength;

    switch (btcTranSigningContext.outputsProcessingState)
    {
        case BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_NUMBER_OF_OUTPUTS:
        {
            btcTranSigningContext.totalNumberOfOutputs = btcTranParseVarInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            btcTranSigningContext.outputsProcessingState = BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_AMOUNT;
        }
        break;
        case BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_AMOUNT:
        {
            int64_t amount;

            amount = btcTranParse64BitInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            if (amount < 0)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            btcHalSha256Update(BTC_HAL_HASH_ID_SEGWIT_OUTPUTS, originalDataPointer, sizeof(uint64_t));

            btcTranSigningContext.outputsProcessingState =
                BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_PUBKEYSCRIPT_LENGTH;
        }
        break;
        case BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_PUBKEYSCRIPT_LENGTH:
        {
            btcTranSigningContext.remainingScriptLength = btcTranParseVarInt(data, dataLength, &parsingSuccessful);
            if (parsingSuccessful != BTC_TRUE)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }

            btcHalSha256Update(BTC_HAL_HASH_ID_SEGWIT_OUTPUTS, originalDataPointer,
                               (originalDataLength - (*dataLength)));

            btcTranSigningContext.outputsProcessingState = BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_PUBKEYSCRIPT;
        }
        break;
        case BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_PUBKEYSCRIPT:
        {
            if ((*dataLength) >= btcTranSigningContext.remainingScriptLength)
            {
                btcTranParseArray(data, dataLength, btcTranSigningContext.remainingScriptLength, &parsingSuccessful);
                if (parsingSuccessful != BTC_TRUE)
                {
                    retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                    goto END;
                }

                btcHalSha256Update(BTC_HAL_HASH_ID_SEGWIT_OUTPUTS, originalDataPointer,
                                   (originalDataLength - (*dataLength)));

                btcTranSigningContext.remainingScriptLength = 0;

                btcTranSigningContext.currentOutputNumber++;

                if (btcTranSigningContext.currentOutputNumber == btcTranSigningContext.totalNumberOfOutputs)
                {
                    btcTranSigningContext.outputsProcessingState = BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_FINISHED;
                }
                else
                {
                    btcTranSigningContext.outputsProcessingState = BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_AMOUNT;
                }
            }
            else
            {
                btcTranSigningContext.remainingScriptLength -= (*dataLength);

                btcTranParseArray(data, dataLength, (*dataLength), &parsingSuccessful);
                if (parsingSuccessful != BTC_TRUE)
                {
                    retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                    goto END;
                }

                btcHalSha256Update(BTC_HAL_HASH_ID_SEGWIT_OUTPUTS, originalDataPointer,
                                   (originalDataLength - (*dataLength)));
            }
        }
        break;
        case BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_FINISHED:
        {
            retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
            goto END;
        }
        break;
        default:
            btcHalFatalError();
            break;
    }

    retVal = BTC_NO_ERROR;

END:

    if (retVal != BTC_NO_ERROR)
    {
        btcTranSigningClearState();
    }
    return retVal;
}

uint16_t btcTranSigningProcessHeaderAndInputs(uint8_t* data, uint32_t dataLength)
{
    uint16_t retVal = BTC_GENERAL_ERROR;
    uint16_t calleeRetVal = BTC_GENERAL_ERROR;

    if (data == NULL)
    {
        btcHalFatalError();
    }

    if (btcTranSigningContext.segWit == BTC_TRUE)
    {
        if ((btcTranSigningContext.state != BTC_TRAN_SIGNING_STATE_PROCESSING_HEADER_AND_INPUTS) &&
            (btcTranSigningContext.state != BTC_TRAN_SIGNING_STATE_PROCESSING_AN_INPUT_FOR_SEGWIT))
        {
            retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
            goto END;
        }
    }
    else
    {
        if (btcTranSigningContext.state != BTC_TRAN_SIGNING_STATE_PROCESSING_HEADER_AND_INPUTS)
        {
            retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
            goto END;
        }
    }

    while (dataLength != 0)
    {
        if (btcTranSigningContext.segWit == BTC_TRUE)
        {
            if (btcTranSigningContext.state == BTC_TRAN_SIGNING_STATE_PROCESSING_HEADER_AND_INPUTS)
            {
                calleeRetVal = btcTranSigningProcessHeaderAndInputsElementSegWit(&data, &dataLength, BTC_TRUE);
            }
            else if (btcTranSigningContext.state == BTC_TRAN_SIGNING_STATE_PROCESSING_AN_INPUT_FOR_SEGWIT)
            {
                calleeRetVal = btcTranSigningProcessHeaderAndInputsElementSegWit(&data, &dataLength, BTC_FALSE);
            }
            else
            {
                btcHalFatalError();
            }
        }
        else
        {
            calleeRetVal = btcTranSigningProcessHeaderAndInputsElement(&data, &dataLength, btcTranSigningContext.firstSignatureGenerated);
        }

        if (calleeRetVal != BTC_NO_ERROR)
        {
            retVal = calleeRetVal;
            goto END;
        }
    }

    if (btcTranSigningContext.headerAndInputsProcessingState ==
        BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_FINISHED)
    {
        if (btcTranSigningContext.segWit == BTC_TRUE)
        {
            if (btcTranSigningContext.state == BTC_TRAN_SIGNING_STATE_PROCESSING_HEADER_AND_INPUTS)
            {
                btcHalSha256Finalize(BTC_HAL_HASH_ID_SEGWIT_PREVOUTS, btcTranSigningContext.segWitHashPrevouts);
                btcHalSha256(btcTranSigningContext.segWitHashPrevouts, sizeof(btcTranSigningContext.segWitHashPrevouts),
                             btcTranSigningContext.segWitHashPrevouts);
                btcHalSha256Finalize(BTC_HAL_HASH_ID_SEGWIT_SEQUENCE, btcTranSigningContext.segWitHashSequence);
                btcHalSha256(btcTranSigningContext.segWitHashSequence, sizeof(btcTranSigningContext.segWitHashSequence),
                             btcTranSigningContext.segWitHashSequence);
                btcTranSigningContext.state = BTC_TRAN_SIGNING_STATE_PROCESSING_OUTPUTS;
            }
            else if (btcTranSigningContext.state == BTC_TRAN_SIGNING_STATE_PROCESSING_AN_INPUT_FOR_SEGWIT)
            {
                btcTranSigningContext.state = BTC_TRAN_SIGNING_STATE_COMPUTING_SIGNATURE;
            }
            else
            {
                btcHalFatalError();
            }
        }
        else
        {
            btcTranSigningContext.state = BTC_TRAN_SIGNING_STATE_PROCESSING_OUTPUTS;
        }
    }

    retVal = BTC_NO_ERROR;

END:
    if (retVal != BTC_NO_ERROR)
    {
        btcTranSigningClearState();
    }

    return retVal;
}

uint16_t btcTranSigningProcessOutputs(uint8_t* data, uint32_t dataLength)
{
    uint16_t retVal = BTC_GENERAL_ERROR;
    uint16_t calleeRetVal = BTC_GENERAL_ERROR;

    if (data == NULL)
    {
        btcHalFatalError();
    }

    if (btcTranSigningContext.state != BTC_TRAN_SIGNING_STATE_PROCESSING_OUTPUTS)
    {
        retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
        goto END;
    }

    while (dataLength != 0)
    {
        if (btcTranSigningContext.segWit == BTC_TRUE)
        {
			btcTranUpdateTransactionToDisplay(data, dataLength);

            calleeRetVal = btcTranSigningProcessOutputsElementSegWit(&data, &dataLength);
        }
        else
        {
	        if(btcTranSigningContext.firstSignatureGenerated == BTC_FALSE)
	        {
	        	btcTranUpdateTransactionToDisplay(data, dataLength);
	        }

            calleeRetVal = btcTranSigningProcessOutputsElement(&data, &dataLength);
        }

        if (calleeRetVal != BTC_NO_ERROR)
        {
            retVal = calleeRetVal;
            goto END;
        }
    }

    if (btcTranSigningContext.outputsProcessingState == BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_FINISHED)
    {
        if (btcTranSigningContext.segWit == BTC_TRUE)
        {
            btcHalSha256Finalize(BTC_HAL_HASH_ID_SEGWIT_OUTPUTS, btcTranSigningContext.segWitHashOutputs);
            btcHalSha256(btcTranSigningContext.segWitHashOutputs, sizeof(btcTranSigningContext.segWitHashOutputs),
                         btcTranSigningContext.segWitHashOutputs);
            btcTranSigningClearInputAndOutputProcessingState();
            btcTranSigningContext.state = BTC_TRAN_SIGNING_STATE_PROCESSING_AN_INPUT_FOR_SEGWIT;
        }
        else
        {
            btcTranSigningContext.state = BTC_TRAN_SIGNING_STATE_COMPUTING_SIGNATURE;
        }
    }

    retVal = BTC_NO_ERROR;

END:
    if (retVal != BTC_NO_ERROR)
    {
        btcTranSigningClearState();
    }

    return retVal;
}

uint16_t btcTranSigningSign(uint32_t* derivationIndexes, uint32_t numberOfKeyDerivations, uint32_t lockTime,
                            uint32_t signHashType, uint8_t* signature, uint32_t* signatureLength)
{
    uint16_t retVal = BTC_GENERAL_ERROR;
    uint16_t calleeRetVal = BTC_GENERAL_ERROR;
    uint8_t lockTimeArray[sizeof(uint32_t)];
    uint8_t signHashTypeArray[sizeof(uint32_t)];
    uint8_t hashToSign[BTC_GLOBAL_SHA256_SIZE];
    uint8_t integrityHash[BTC_GLOBAL_SHA256_SIZE];

    if ((derivationIndexes == NULL) || (signature == NULL) || (signatureLength == NULL))
    {
        btcHalFatalError();
    }

    if (btcTranSigningContext.state != BTC_TRAN_SIGNING_STATE_COMPUTING_SIGNATURE)
    {
        retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
        goto END;
    }

    lockTimeArray[0] = (uint8_t)(lockTime);
    lockTimeArray[1] = (uint8_t)(lockTime >> 8);
    lockTimeArray[2] = (uint8_t)(lockTime >> 16);
    lockTimeArray[3] = (uint8_t)(lockTime >> 24);

    signHashTypeArray[0] = (uint8_t)(signHashType);
    signHashTypeArray[1] = (uint8_t)(signHashType >> 8);
    signHashTypeArray[2] = (uint8_t)(signHashType >> 16);
    signHashTypeArray[3] = (uint8_t)(signHashType >> 24);

    btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_SIGNING, lockTimeArray, sizeof(lockTimeArray));
    btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_SIGNING, signHashTypeArray, sizeof(signHashTypeArray));

    if (btcTranSigningContext.segWit == BTC_FALSE)
    {
        btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_INTEGRITY_CHECK, lockTimeArray, sizeof(lockTimeArray));
        btcHalSha256Update(BTC_HAL_HASH_ID_TRANSACTION_INTEGRITY_CHECK, signHashTypeArray, sizeof(signHashTypeArray));
    }

    btcHalSha256Finalize(BTC_HAL_HASH_ID_TRANSACTION_SIGNING, hashToSign);
    btcHalSha256(hashToSign, sizeof(hashToSign), hashToSign);

    if (btcTranSigningContext.segWit == BTC_FALSE)
    {
        btcHalSha256Finalize(BTC_HAL_HASH_ID_TRANSACTION_INTEGRITY_CHECK, integrityHash);

        if (btcTranSigningContext.firstSignatureGenerated == BTC_TRUE)
        {
            uint16_t comparisonResult = BTC_CMP_NOT_EQUAL;

            comparisonResult = btcHalMemCmp(integrityHash, btcTranSigningContext.hashOfTransactionWithoutInputScripts,
                                            BTC_GLOBAL_SHA256_SIZE);

            if (comparisonResult != BTC_CMP_EQUAL)
            {
                retVal = BTC_TRANSACTION_PARSING_FAILED_ERROR;
                goto END;
            }
        }
    }

    calleeRetVal =
        btcHalSignHash(derivationIndexes, numberOfKeyDerivations, hashToSign, signature, signatureLength, BTC_TRUE);

    if (calleeRetVal != BTC_NO_ERROR)
    {
        retVal = calleeRetVal;
        goto END;
    }

    if (btcTranSigningContext.segWit == BTC_TRUE)
    {
        btcTranSigningClearStateInternal(BTC_FALSE, BTC_FALSE);
        btcTranSigningContext.state = BTC_TRAN_SIGNING_STATE_PROCESSING_AN_INPUT_FOR_SEGWIT;
        btcTranSigningContext.firstSignatureGenerated = BTC_TRUE;
        btcTranSigningContext.segwitSignatureNumber++;
    }
    else
    {
    	btcTranSigningClearStateInternal(BTC_TRUE, BTC_FALSE);
        btcHalMemCpy(btcTranSigningContext.hashOfTransactionWithoutInputScripts, integrityHash, BTC_GLOBAL_SHA256_SIZE);
        btcTranSigningContext.state = BTC_TRAN_SIGNING_STATE_PROCESSING_HEADER_AND_INPUTS;
        btcTranSigningContext.firstSignatureGenerated = BTC_TRUE;
    }

    retVal = BTC_NO_ERROR;
END:
    if (retVal != BTC_NO_ERROR)
    {
        btcTranSigningClearState();
    }

    return retVal;
}

void btcTranIsFirstSignatureGenerated(uint16_t* firstSignatureGenerated)
{
    if (firstSignatureGenerated == NULL)
    {
        btcHalFatalError();
    }

    *firstSignatureGenerated = btcTranSigningContext.firstSignatureGenerated;
}

void btcTranMessageSigningClearState(void)
{
    btcTranMessageSigningContext.state = BTC_TRAN_MESSAGE_SIGNING_STATE_WAITING_FOR_RESET;
    btcTranMessageSigningContext.numberOfKeyDerivations = 0;
    btcHalMemSet((uint8_t*)btcTranMessageSigningContext.derivationIndexes, 0x00,
                 sizeof(uint32_t) * BTC_GLOBAL_MAXIMAL_NUMBER_OF_KEY_DERIVATIONS);
    btcHalSha256Start(BTC_HAL_HASH_ID_MESSAGE_SIGNING);
}

void btcTranMessageSigningInit(void)
{
    btcTranMessageSigningClearState();
    btcTranMessageSigningContext.state = BTC_TRAN_MESSAGE_SIGNING_STATE_PROCESSING_DATA;
}

uint16_t btcTranMessageSigningProcessData(uint32_t* derivationIndexes, uint32_t numberOfKeyDerivations,
                                          uint8_t* dataToSign, uint8_t dataToSignLength)
{
    uint16_t retVal = BTC_GENERAL_ERROR;
    uint8_t messageHeader[] = {0x18, 0x42, 0x69, 0x74, 0x63, 0x6F, 0x69, 0x6E, 0x20, 0x53, 0x69, 0x67, 0x6E,
                               0x65, 0x64, 0x20, 0x4D, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65, 0x3A, 0x0A};
    uint32_t i;

    if (dataToSign == NULL)
    {
        btcHalFatalError();
    }

    if (btcTranMessageSigningContext.state != BTC_TRAN_MESSAGE_SIGNING_STATE_PROCESSING_DATA)
    {
        retVal = BTC_MESSAGE_SIGNING_ERROR;
        goto END;
    }

    if (dataToSignLength > BTC_TRAN_MAXIMAL_MESSAGE_LENGTH)
    {
        retVal = BTC_MESSAGE_SIGNING_ERROR;
        goto END;
    }

    for (i = 0; i < dataToSignLength; i++)
    {
        if ((dataToSign[i] < BTC_TRAN_MESSAGE_TO_SIGN_MINIMAL_CHARACTER_VALUE) ||
            (dataToSign[i] > BTC_TRAN_MESSAGE_TO_SIGN_MAXIMAL_CHARACTER_VALUE))
        {
            retVal = BTC_MESSAGE_SIGNING_ERROR;
            goto END;
        }
    }

    for (i = 0; i < BTC_GLOBAL_MAXIMAL_NUMBER_OF_KEY_DERIVATIONS; i++)
    {
        btcTranMessageSigningContext.derivationIndexes[i] = derivationIndexes[i];
    }

    btcTranMessageSigningContext.numberOfKeyDerivations = numberOfKeyDerivations;

    btcHalSha256Update(BTC_HAL_HASH_ID_MESSAGE_SIGNING, messageHeader, sizeof(messageHeader));
    btcHalSha256Update(BTC_HAL_HASH_ID_MESSAGE_SIGNING, &dataToSignLength, sizeof(uint8_t));
    btcHalSha256Update(BTC_HAL_HASH_ID_MESSAGE_SIGNING, dataToSign, dataToSignLength);

    btcTranMessageSigningContext.state = BTC_TRAN_MESSAGE_SIGNING_STATE_GENERATING_SIGNATURE;

    retVal = BTC_NO_ERROR;

END:
    if (retVal != BTC_NO_ERROR)
    {
        btcTranMessageSigningClearState();
    }

    return retVal;
}

uint16_t btcTranMessageSigningSign(uint8_t* signature, uint32_t* signatureLength)
{
    uint16_t retVal = BTC_GENERAL_ERROR;
    uint16_t calleeRetVal = BTC_GENERAL_ERROR;
    uint8_t hashToSign[BTC_GLOBAL_SHA256_SIZE];

    if ((signature == NULL) || (signatureLength == NULL))
    {
        btcHalFatalError();
    }

    if (btcTranMessageSigningContext.state != BTC_TRAN_MESSAGE_SIGNING_STATE_GENERATING_SIGNATURE)
    {
        retVal = BTC_MESSAGE_SIGNING_ERROR;
        goto END;
    }

    btcHalSha256Finalize(BTC_HAL_HASH_ID_MESSAGE_SIGNING, hashToSign);
    btcHalSha256(hashToSign, sizeof(hashToSign), hashToSign);

    calleeRetVal = btcHalSignHash(btcTranMessageSigningContext.derivationIndexes,
                                  btcTranMessageSigningContext.numberOfKeyDerivations, hashToSign, signature,
                                  signatureLength, BTC_FALSE);

    if (calleeRetVal != BTC_NO_ERROR)
    {
        retVal = calleeRetVal;
        goto END;
    }

    retVal = BTC_NO_ERROR;

END:
    btcTranMessageSigningClearState();

    return retVal;
}

void btcTranGetTransactionReadoutData(BTC_TRAN_TRANSACTION_TO_DISPLAY** transactionToDisplay, int64_t** amountsInInputs, uint32_t* numberOfInputs)
{
    if ((transactionToDisplay == NULL) || (amountsInInputs == NULL) || (numberOfInputs == NULL) )
    {
        btcHalFatalError();
    }

    if (btcTranSigningContext.state != BTC_TRAN_SIGNING_STATE_COMPUTING_SIGNATURE)
    {
    	btcHalFatalError();
    }

    if (btcTranSigningContext.firstSignatureGenerated != BTC_FALSE)
    {
    	btcHalFatalError();
    }

    *transactionToDisplay = &btcTranSigningContext.transactionToDisplay;
    *amountsInInputs = btcTranSigningContext.inputAmounts;
    *numberOfInputs= btcTranSigningContext.numberOfInputAmounts;
}
