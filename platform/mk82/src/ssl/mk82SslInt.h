/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __MK82_SSL_INT_H__
#define __MK82_SSL_INT_H__

#define MK82_MAX_PAYLOAD_SIZE (1024)

#define MK82_SSL_CLA (0x80)
#define MK82_SSL_WRAPPED_APDU_CLA (0x84)

#define MK82_SSL_INS_HANDSHAKE (0x00)
#define MK82_SSL_INS_GET_PUBLIC_KEY (0x10)
#define MK82_SSL_INS_RESET (0x20)

#define MK82_SSL_INS_WRAPPED_COMMAND (0x00)

#define MK82_SSL_P1P2_HANDSHAKE (0x0000)
#define MK82_SSL_P1P2_GET_PUBLIC_KEY (0x000)
#define MK82_SSL_P1P2_RESET (0x000)

#define MK82_SSL_P1P2_WRAPPED_COMMAND (0x0000)

#define MK82_SSL_AID                                         \
    {                                                        \
        0x53, 0x53, 0x4C, 0x41, 0x50, 0x50, 0x4C, 0x45, 0x54 \
    }
#define MK82_SSL_AID_LENGTH (0x09)

#define MK82_SSL_WIPEOUT_BUFFER_SIZE (64)

#define MK82_SSL_PRIVATE_KEY_TEMPLATE_SIZE (121)
#define MK82_SSL_PRIVATE_KEY_TEMPLATE_PRIVATE_KEY_OFFSET (7)
#define MK82_SSL_PRIVATE_KEY_TEMPLATE_PUBLIC_KEY_OFFSET (56)

#define MK82_SSL_CERTIFICATE_TEMPLATE_SIZE (277)
#define MK82_SSL_CERTIFICATE_PUBLIC_KEY_OFFSET (127)

#endif /* __MK82_SSL_INT_H__ */
