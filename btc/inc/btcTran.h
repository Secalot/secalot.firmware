/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __BTC_TRAN_H__
#define __BTC_TRAN_H__

#ifdef __cplusplus
extern "C" {
#endif

#define BTC_TRAN_GIVE_MORE_INPUT (0x9999)
#define BTC_TRAN_RETREIVE_OUTPUT (0x6666)

void btcTranInit(void);
void btcTranDeinit(void);

void btcTranTIGenerationClearState(void);
void btcTranTIGenerationInit(uint32_t outputNumberToGetAmountOf);
uint16_t btcTranTIGenerationProcessTransaction(uint8_t* data, uint32_t dataLength, uint16_t* action);
void btcTranTIGenerationGetHashIndexAndAmount(uint8_t* data);
void btcTranSigningClearState(void);
void btcTranSigningInit(void);
uint16_t btcTranSigningProcessHeaderAndInputs(uint8_t* data, uint32_t dataLength);
uint16_t btcTranSigningProcessOutputs(uint8_t* outputAddress, int64_t amount, int64_t fees,
                                      uint16_t changeAddressPresent, uint8_t* changeAddress, uint8_t* outputData,
                                      uint32_t* outputLength);
uint16_t btcTranSigningSign(uint32_t* derivationIndexes, uint32_t numberOfKeyDerivations, uint32_t lockTime,
                            uint32_t signHashType, uint8_t* signature, uint32_t* signatureLength);
void btcTranIsFirstSignatureGenerated(uint16_t* firstSignatureGenerated);
void btcTranMessageSigningClearState(void);
void btcTranMessageSigningInit(void);
uint16_t btcTranMessageSigningProcessData(uint32_t* derivationIndexes, uint32_t numberOfKeyDerivations,
                                          uint8_t* dataToSign, uint8_t dataToSignLength);
uint16_t btcTranMessageSigningSign(uint8_t* signature, uint32_t* signatureLength);

#ifdef __cplusplus
}
#endif

#endif /* __BTC_TRAN_H__ */
