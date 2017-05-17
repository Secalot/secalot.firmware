/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __SF_CCID_CORE_INT_H__
#define __SF_CCID_CORE_INT_H__

#include <ccidGlobal.h>
#include <ccidGlobalInt.h>

#define CCID_CORE_ATR_BASE                                         \
    {                                                              \
        0x3B, 0xD0, 0x18, 0xFF, 0x81, 0xB1, 0xFE, 0x75, 0x1F, 0x03 \
    }

#define CCID_CORE_STATE_RECEIVING_COMMAND (0x9999)
#define CCID_CORE_STATE_PROCESSING_RECEIVED_APDU (0x6666)
#define CCID_CORE_STATE_SENDING_RESPONSE (0xCCCC)

CCID_MAKE_PACKED(typedef struct)
{
    uint8_t messageType;
    uint8_t lengthLL;
    uint8_t lengthHL;
    uint8_t lengthLH;
    uint8_t lengthHH;
    uint8_t slot;
    uint8_t sequenceNumber;
    uint8_t messageSpecific1OrStatus;
    uint8_t messageSpecific2OrError;
    uint8_t messageSpecific3;
    uint8_t data[];
}
CCID_CORE_MESSAGE;

#define CCID_CORE_MESSAGE_LENGTH(messageStruct)                                      \
    (CCID_MAKEDWORD(CCID_MAKEWORD(messageStruct->lengthLL, messageStruct->lengthHL), \
                    CCID_MAKEWORD(messageStruct->lengthLH, messageStruct->lengthHH)))

#define CCID_CORE_ATR_MAX_HISTCHAR_LENGTH (15)

#define CCID_CORE_SLOT_NUMBER (0)

#define CCID_CORE_LEVEL_APDU_BEGINS_AND_ENDS_HERE (0x0000)

#define CCID_CORE_STATUS_ICC_PRESENT_AND_ACTIVE (0x00)
#define CCID_CORE_STATUS_ICC_PRESENT_AND_INACTIVE (0x01)
#define CCID_CORE_STATUS_ICC_NOT_PRESENT (0x02)

#define CCID_CORE_STATUS_COMMAND_SUCCEEDED (0x00)
#define CCID_CORE_STATUS_COMMAND_FAILED (0x40)
#define CCID_CORE_STATUS_COMMAND_WTX_REQUESTED (0x80)

#define CCID_CORE_ERROR_NO_ERROR (0x00)
#define CCID_CORE_ERROR_INVALID_COMMAND (0x00)
#define CCID_CORE_ERROR_INVALID_LENGTH (0x01)
#define CCID_CORE_ERROR_INVALID_SLOT (0x05)
#define CCID_CORE_ERROR_BAD_LEVEL_PARAMETER (0x08)
#define CCID_CORE_ERROR_SET_BWT_MULTIPLIER_TO_ONE (0x01)

#define CCID_CORE_CHAINING_RESPONSE_BEGINS_AND_ENDS_HERE (0x00)

#define CCID_CORE_CLOCK_STATUS_CLOCK_RUNNING (0x00)
#define CCID_CORE_CLOCK_STATUS_CLOCK_STOPPED_IN_LOW (0x01)
#define CCID_CORE_CLOCK_STATUS_CLOCK_STOPPED_IN_UNKNOWN (0x03)

#define CCID_CORE_COMMAND_ICC_POWER_ON (0x62)
#define CCID_CORE_COMMAND_ICC_POWER_OFF (0x63)
#define CCID_CORE_COMMAND_ICC_GET_PARAMETERS (0x6C)
#define CCID_CORE_COMMAND_XFR_BLOCK (0x6F)
#define CCID_CORE_COMMAND_ICC_SET_PARAMETERS (0x61)
#define CCID_CORE_COMMAND_ICC_GET_SLOT_STATUS (0x65)

#define CCID_CORE_RESPONSE_DATABLOCK (0x80)
#define CCID_CORE_RESPONSE_SLOTSTATUS (0x81)
#define CCID_CORE_RESPONSE_PARAMETERS (0x82)

#define CCID_CORE_T1_PARAMATERS_LENGTH (0x07)
#define CCID_CORE_PROTOCOL_TYPE_T1 (0x01)

#define CCID_CORE_T1_FIDI (0x18)
#define CCID_CORE_T1_TCCKS (0x10)
#define CCID_CORE_T1_GUARD_TIME (0xFF)
#define CCID_CORE_T1_WAITING_INTEGERS (0x75)
#define CCID_CORE_T1_CLOCK_STOP (0x00)
#define CCID_CORE_T1_IFSC (0xFE)
#define CCID_CORE_T1_NAD (0x00)

#endif /* __SF_CCID_CORE_INT_H__ */
