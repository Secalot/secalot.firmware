/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ccidGlobal.h>
#include <ccidHal.h>

#include <stdint.h>
#include <string.h>

#include "mk82System.h"

void ccidhalInit() {}

void ccidHalDeinit() {}

void ccidHalMemCpy(uint8_t* dst, uint8_t* src, uint16_t length) { mk82SystemMemCpy(dst, src, length); }

void ccidHalMemSet(uint8_t* dst, uint8_t value, uint16_t length) { mk82SystemMemSet(dst, value, length); }

void ccidHalFatalError(void) { mk82SystemFatalError(); }
