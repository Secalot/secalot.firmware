/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __BTC_TRAN_INT_H__
#define __BTC_TRAN_INT_H__

#define BTC_TRAN_TRUSTED_INPUT_FLAG (0x01)
#define BTC_TRAN_SEGWIT_FLAG (0x02)

#define BTC_TRAN_TI_GENERATION_STATE_WAITING_FOR_RESET (0x9999)
#define BTC_TRAN_TI_GENERATION_STATE_PARSING_VERSION (0x6666)
#define BTC_TRAN_TI_GENERATION_STATE_PARSING_NUMBER_OF_INPUTS (0xCCCC)
#define BTC_TRAN_TI_GENERATION_STATE_PARSING_INPUT_PREVOUT (0x3333)
#define BTC_TRAN_TI_GENERATION_STATE_PARSING_INPUT_SIGSCRIPT_LENGTH (0x5555)
#define BTC_TRAN_TI_GENERATION_STATE_PARSING_INPUT_SIGSCRIPT (0xAAAA)
#define BTC_TRAN_TI_GENERATION_STATE_PARSING_INPUT_SEQUENCE (0x7777)
#define BTC_TRAN_TI_GENERATION_STATE_PARSING_NUMBER_OF_OUTPUTS (0xBBBB)
#define BTC_TRAN_TI_GENERATION_STATE_PARSING_OUTPUT_VALUE (0xDDDD)
#define BTC_TRAN_TI_GENERATION_STATE_PARSING_OUTPUT_PKSCRIPT_LENGTH (0xEEEE)
#define BTC_TRAN_TI_GENERATION_STATE_PARSING_OUTPUT_PKSCRIPT (0x1111)
#define BTC_TRAN_TI_GENERATION_STATE_PARSING_LOCKTIME (0x2222)
#define BTC_TRAN_TI_GENERATION_STATE_PARSING_FINISHED (0x4444)

#define BTC_TRAN_INVALID_INPUT_OUTPUT_NUMBER (0xFFFFFFFF)

typedef struct
{
    uint16_t state;
    uint32_t currentInputNumber;
    uint32_t totalNumberOfInputs;
    uint32_t currentOutputNumber;
    uint32_t totalNumberOfOutputs;
    uint32_t remainingScriptLength;
    uint32_t outputNumberToGetAmountOf;
    int64_t amount;
    uint16_t outputWithRequestedIndexFound;
} BTC_TRAN_TI_GENERATION_CONTEXT;

#define BTC_TRAN_SIGNING_STATE_WAITING_FOR_RESET (0x9999)
#define BTC_TRAN_SIGNING_STATE_PROCESSING_HEADER_AND_INPUTS (0x6666)
#define BTC_TRAN_SIGNING_STATE_PROCESSING_OUTPUTS (0xCCCC)
#define BTC_TRAN_SIGNING_STATE_PROCESSING_AN_INPUT_FOR_SEGWIT (0x3333)
#define BTC_TRAN_SIGNING_STATE_COMPUTING_SIGNATURE (0x5555)

#define BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_VERSION (0x9999)
#define BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_NUMBER_OF_INPUTS (0x6666)
#define BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_TI_OR_SWGWIT_FLAG (0xCCCC)
#define BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_TI_OR_SEGWIT_PREVOUT (0x3333)
#define BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_INPUT_SIGSCRIPT_LENGTH (0x5555)
#define BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_INPUT_SIGSCRIPT (0xAAAA)
#define BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_INPUT_SEQUENCE (0x7777)
#define BTC_TRAN_HEADER_AND_INPUTS_PROCESSING_STATE_PARSING_FINISHED (0xBBBB)

#define BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_NUMBER_OF_OUTPUTS (0x9999)
#define BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_AMOUNT (0x6666)
#define BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_PUBKEYSCRIPT_LENGTH (0xCCCC)
#define BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_PUBKEYSCRIPT (0x3333)
#define BTC_TRAN_OUTPUTS_PROCESSING_STATE_PARSING_FINISHED (0x5555)

#define BTC_TRANS_MAX_NUMBER_OF_INPUTS (100)

typedef struct
{
    uint16_t state;
    uint16_t headerAndInputsProcessingState;
    uint16_t outputsProcessingState;
    uint16_t firstSignatureGenerated;
    uint8_t hashOfTransactionWithoutInputScripts[BTC_GLOBAL_SHA256_SIZE];

    uint32_t currentInputNumber;
    uint32_t totalNumberOfInputs;
    uint32_t currentOutputNumber;
    uint32_t totalNumberOfOutputs;
    uint32_t remainingScriptLength;

    uint32_t numberOfInputAmounts;
    int64_t inputAmounts[BTC_TRANS_MAX_NUMBER_OF_INPUTS];

    uint16_t segWit;
    uint32_t segwitSignatureNumber;
    uint8_t segWitCurrentInputAmount[8];
    uint8_t segWitHashPrevouts[BTC_GLOBAL_SHA256_SIZE];
    uint8_t segWitHashSequence[BTC_GLOBAL_SHA256_SIZE];
    uint8_t segWitHashOutputs[BTC_GLOBAL_SHA256_SIZE];

    BTC_TRAN_TRANSACTION_TO_DISPLAY transactionToDisplay;

} BTC_TRAN_SIGNING_CONTEXT;

#define BTC_TRAN_MESSAGE_SIGNING_STATE_WAITING_FOR_RESET (0x9999)
#define BTC_TRAN_MESSAGE_SIGNING_STATE_PROCESSING_DATA (0x6666)
#define BTC_TRAN_MESSAGE_SIGNING_STATE_GENERATING_SIGNATURE (0xCCCC)

typedef struct
{
    uint16_t state;
    uint32_t derivationIndexes[BTC_GLOBAL_MAXIMAL_NUMBER_OF_KEY_DERIVATIONS];
    uint32_t numberOfKeyDerivations;
} BTC_TRAN_MESSAGE_SIGNING_CONTEXT;

#define BTC_TRAN_OP_DUP (0x76)
#define BTC_TRAN_OP_HASH160 (0xA9)
#define BTC_TRAN_OP_OP_EQUALVERIFY (0x88)
#define BTC_TRAN_OP_OP_CHECKSIG (0xAC)

#define BTC_TRAN_MAXIMAL_MESSAGE_LENGTH (140)
#define BTC_TRAN_MESSAGE_TO_SIGN_MINIMAL_CHARACTER_VALUE (0x20)
#define BTC_TRAN_MESSAGE_TO_SIGN_MAXIMAL_CHARACTER_VALUE (0x7F)

#endif /* __BTC_TRAN_INT_H__ */
