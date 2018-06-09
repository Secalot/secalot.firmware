/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __MK82_SEC_APDU_H__
#define __MK82_SEC_APDU_H__

#ifdef __cplusplus
extern "C" {
#endif

void mk82SecApduInit(void);

void mk82SecApduSetPrimaryDataType(uint16_t primaryDataType);
uint16_t mk82SecApduGetPrimaryDataType(void);
void mk82SecApduProcessCommandIfAvailable(uint32_t dataTypesToProcess, uint32_t allowedCommands);


#ifdef __cplusplus
}
#endif

#endif /* __MK82_SEC_APDU_H__ */
