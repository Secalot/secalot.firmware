/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __SF_CCID_CLASS_DESCRIPTOR_H__
#define __SF_CCID_CLASS_DESCRIPTOR_H__

#include "ccidGlobal.h"

#define CCID_DEVICE_CLASS_DESCRIPTOR_LENGTH (0x36)
#define CCID_DEVICE_CLASS_DESCRIPTOR_TYPE (0x21)
#define CCID_BCD_VERSION (0x0110)

#define CCID_MAX_SLOT_INDEX (0x00)                 // One slot
#define CCID_SUPPORTED_VOLTAGE (0x01)              // Supports 5.0 Volts only
#define CCID_SUPPORTED_PROTOCOLS (0x00000002)      // Supports T=1 only
#define CCID_DEFAULT_CLOCK (0x00000DFC)            // 3.58 MHz
#define CCID_MAXIMUM_CLOCK (0x00000DFC)            // 3.58 MHz
#define CCID_NUMBER_OF_SUPPORTED_CLOCKS (0x00)     // Supports one clock only
#define CCID_DATARATE (0x0001C200)                 //  9600 bps
#define CCID_MAXIMUM_DATARATE (0x0001C200)         //  9600 bps
#define CCID_NUMBER_OF_SUPPORTED_DATARATES (0x00)  // Supports one clock datarate
#define CCID_MAXIMUM_IFSD (0x000000FE)             // Ifsd = 254
#define CCID_SYNCH_PROTOCOLS (0x00000000)          // None supported
#define CCID_MECHANICAL (0x00000000)               // No special characteristics
/*
Support for the following:

00000002h Automatic parameter configuration based on ATR data
00000004h Automatic activation of ICC on inserting
00000008h Automatic ICC voltage selection
00000010h Automatic ICC clock frequency change   according to active parameters provided by the Host or self determine
00000020h Automatic baud rate change according to active parameters provided by the Host or self determined
00000040h Automatic parameters negotiation made by the CCID
00000400h Automatic IFSD exchange as first exchange
00040000h Extended APDU level exchange with CCID
*/
#define CCID_FEATURES (0x0004047E)
//#define CCID_FEATURES (0x00020840)
#define CCID_GET_RESPONSE_CLA (0xFF)    // Irrelivant, as onlt T=1 is supported
#define CCID_ENVELOPE_CLA (0xFF)        // Irrelivant, as onlt T=1 is supported
#define CCID_LCD_LAYOUT (0x0000)        // No LCD
#define CCID_PIN_SUPPORT (0x00)         // No Pin operations supported
#define CCID_MAXIMUM_BUSY_SLOTS (0x01)  // One slot is supported

#define CCID_DEVICE_CLASS_DESCRIPTOR                                                      \
    CCID_DEVICE_CLASS_DESCRIPTOR_LENGTH,                    /* bLength */                 \
        CCID_DEVICE_CLASS_DESCRIPTOR_TYPE,                  /* bDescriptorType */         \
        CCID_LOBYTE(CCID_BCD_VERSION),                      /* bcdCCID */                 \
        CCID_HIBYTE(CCID_BCD_VERSION),                      /* bcdCCID */                 \
        CCID_MAX_SLOT_INDEX,                                /*  bMaxSlotIndex */          \
        CCID_SUPPORTED_VOLTAGE,                             /* bVoltageSupport */         \
        CCID_LOBYTE(CCID_LOWORD(CCID_SUPPORTED_PROTOCOLS)), /* dwProtocols */             \
        CCID_HIBYTE(CCID_LOWORD(CCID_SUPPORTED_PROTOCOLS)), /* dwProtocols */             \
        CCID_LOBYTE(CCID_HIWORD(CCID_SUPPORTED_PROTOCOLS)), /* dwProtocols */             \
        CCID_HIBYTE(CCID_HIWORD(CCID_SUPPORTED_PROTOCOLS)), /* dwProtocols */             \
        CCID_LOBYTE(CCID_LOWORD(CCID_DEFAULT_CLOCK)),       /* dwDefaultClock */          \
        CCID_HIBYTE(CCID_LOWORD(CCID_DEFAULT_CLOCK)),       /* dwDefaultClock */          \
        CCID_LOBYTE(CCID_HIWORD(CCID_DEFAULT_CLOCK)),       /* dwDefaultClock */          \
        CCID_HIBYTE(CCID_HIWORD(CCID_DEFAULT_CLOCK)),       /* dwDefaultClock */          \
        CCID_LOBYTE(CCID_LOWORD(CCID_MAXIMUM_CLOCK)),       /* dwMaximumClock */          \
        CCID_HIBYTE(CCID_LOWORD(CCID_MAXIMUM_CLOCK)),       /* dwMaximumClock */          \
        CCID_LOBYTE(CCID_HIWORD(CCID_MAXIMUM_CLOCK)),       /* dwMaximumClock */          \
        CCID_HIBYTE(CCID_HIWORD(CCID_MAXIMUM_CLOCK)),       /* dwMaximumClock */          \
        CCID_NUMBER_OF_SUPPORTED_CLOCKS,                    /*  bNumClockSupported */     \
        CCID_LOBYTE(CCID_LOWORD(CCID_DATARATE)),            /* dwDataRate */              \
        CCID_HIBYTE(CCID_LOWORD(CCID_DATARATE)),            /* dwDataRate */              \
        CCID_LOBYTE(CCID_HIWORD(CCID_DATARATE)),            /* dwDataRate */              \
        CCID_HIBYTE(CCID_HIWORD(CCID_DATARATE)),            /* dwDataRate */              \
        CCID_LOBYTE(CCID_LOWORD(CCID_MAXIMUM_DATARATE)),    /* dwMaxDataRate */           \
        CCID_HIBYTE(CCID_LOWORD(CCID_MAXIMUM_DATARATE)),    /* dwMaxDataRate */           \
        CCID_LOBYTE(CCID_HIWORD(CCID_MAXIMUM_DATARATE)),    /* dwMaxDataRate */           \
        CCID_HIBYTE(CCID_HIWORD(CCID_MAXIMUM_DATARATE)),    /* dwMaxDataRate */           \
        CCID_NUMBER_OF_SUPPORTED_DATARATES,                 /*  bNumDataRatesSupported */ \
        CCID_LOBYTE(CCID_LOWORD(CCID_MAXIMUM_IFSD)),        /* dwMaxIFSD */               \
        CCID_HIBYTE(CCID_LOWORD(CCID_MAXIMUM_IFSD)),        /* dwMaxIFSD */               \
        CCID_LOBYTE(CCID_HIWORD(CCID_MAXIMUM_IFSD)),        /* dwMaxIFSD */               \
        CCID_HIBYTE(CCID_HIWORD(CCID_MAXIMUM_IFSD)),        /* dwMaxIFSD */               \
        CCID_LOBYTE(CCID_LOWORD(CCID_SYNCH_PROTOCOLS)),     /* dwSynchProtocols */        \
        CCID_HIBYTE(CCID_LOWORD(CCID_SYNCH_PROTOCOLS)),     /* dwSynchProtocols */        \
        CCID_LOBYTE(CCID_HIWORD(CCID_SYNCH_PROTOCOLS)),     /* dwSynchProtocols */        \
        CCID_HIBYTE(CCID_HIWORD(CCID_SYNCH_PROTOCOLS)),     /* dwSynchProtocols */        \
        CCID_LOBYTE(CCID_LOWORD(CCID_MECHANICAL)),          /* dwMechanical */            \
        CCID_HIBYTE(CCID_LOWORD(CCID_MECHANICAL)),          /* dwMechanical */            \
        CCID_LOBYTE(CCID_HIWORD(CCID_MECHANICAL)),          /* dwMechanical */            \
        CCID_HIBYTE(CCID_HIWORD(CCID_MECHANICAL)),          /* dwMechanical */            \
        CCID_LOBYTE(CCID_LOWORD(CCID_FEATURES)),            /* dwFeatures */              \
        CCID_HIBYTE(CCID_LOWORD(CCID_FEATURES)),            /* dwFeatures */              \
        CCID_LOBYTE(CCID_HIWORD(CCID_FEATURES)),            /* dwFeatures */              \
        CCID_HIBYTE(CCID_HIWORD(CCID_FEATURES)),            /* dwFeatures */              \
        CCID_LOBYTE(CCID_LOWORD(CCID_MAX_MESSAGE_LENGTH)),  /* dwMaxCCIDMessageLength */  \
        CCID_HIBYTE(CCID_LOWORD(CCID_MAX_MESSAGE_LENGTH)),  /* dwMaxCCIDMessageLength */  \
        CCID_LOBYTE(CCID_HIWORD(CCID_MAX_MESSAGE_LENGTH)),  /* dwMaxCCIDMessageLength */  \
        CCID_HIBYTE(CCID_HIWORD(CCID_MAX_MESSAGE_LENGTH)),  /* dwMaxCCIDMessageLength */  \
        CCID_GET_RESPONSE_CLA,                              /*  bClassGetResponse */      \
        CCID_ENVELOPE_CLA,                                  /*  bClassEnvelope */         \
        CCID_LOBYTE(CCID_LCD_LAYOUT),                       /*  wLcdLayout */             \
        CCID_HIBYTE(CCID_LCD_LAYOUT),                       /*  wLcdLayout */             \
        CCID_PIN_SUPPORT,                                   /*  bPINSupport */            \
        CCID_MAXIMUM_BUSY_SLOTS                             /* bMaxCCIDBusySlots */

#endif /* __SF_CCID_CLASS_DESCRIPTOR_H__ */
