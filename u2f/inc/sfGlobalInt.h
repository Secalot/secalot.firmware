/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __SF_GLOBAL_INT_H__
#define __SF_GLOBAL_INT_H__

#include "mk82Global.h"
#include "mk82Keysafe.h"

#define SF_CMP_EQUAL 0x9999
#define SF_CMP_NOT_EQUAL 0x6666

#define SF_GLOBAL_EC_POINT_COORDINATE_LENGTH (32)
#define SF_GLOBAL_PRIVATE_KEY_LENGTH (32)
#define SF_GLOBAL_ASN1_04_TAG (4)

#define SF_GLOBAL_KEY_HANDLE_PADDING_LENGTH 0x04

/* Padding is not needed, but is here to make U2f reference tests pass. They require key handles to be 64 or more bytes
 * in length. */
#define SF_GLOBAL_KEY_HANDLE_LENGTH                                                       \
    (SF_GLOBAL_PRIVATE_KEY_LENGTH + MK82_KEYSAFE_NONCE_LENGTH + MK82_KEYSAFE_TAG_LENGTH + \
     SF_GLOBAL_KEY_HANDLE_PADDING_LENGTH)
#define SF_GLOBAL_APPLICATION_ID_LENGTH (32)
#define SF_GLOBAL_CHALLENGE_LENGTH (32)
#define SF_GLOBAL_PUBLIC_KEY_LENGTH (SF_GLOBAL_EC_POINT_COORDINATE_LENGTH * 2 + 1)
#define SF_GLOBAL_MAXIMAL_SIGNATURE_LENGTH (72)

#endif /* __SF_GLOBAL_INT_H__ */
