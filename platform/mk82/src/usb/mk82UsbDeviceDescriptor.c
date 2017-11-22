/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* o Redistributions of source code must retain the above copyright notice, this list
*   of conditions and the following disclaimer.
*
* o Redistributions in binary form must reproduce the above copyright notice, this
*   list of conditions and the following disclaimer in the documentation and/or
*   other materials provided with the distribution.
*
* o Neither the name of Freescale Semiconductor, Inc. nor the names of its
*   contributors may be used to endorse or promote products derived from this
*   software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"

#include "ccidClassDescriptor.h"
#include "mk82USBDeviceDescriptor.h"

#ifdef FIRMWARE
#include "sfHidReportDescriptor.h"
#include "btcHidReportDescriptor.h"
#endif /* FIRMWARE */

/*******************************************************************************
* API
******************************************************************************/
/*!
 * @brief USB device callback function.
 *
 * This function handles the usb device specific requests.
 *
 * @param handle          The USB device handle.
 * @param event           The USB device event type.
 * @param param           The parameter of the device specific request.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
extern usb_status_t USB_DeviceCallback(usb_device_handle handle, uint32_t event, void *param);

/*******************************************************************************
* Variables
******************************************************************************/
static uint8_t mk82UsbCurrentConfiguration = 0;
static uint8_t mk82UsbInterfaces[MK82_USB_INTERFACE_COUNT];

#ifdef FIRMWARE
static uint8_t mk82UsbU2fReportDescriptor[] = {SF_REPORT_DESCRIPTOR};
static uint8_t mk82UsbBtcReportDescriptor[] = {BTC_REPORT_DESCRIPTOR};

static uint8_t mk82UsbKeyboardReportDescriptor[] = {
    0x05U, 0x01U, /* Usage Page (Generic Desktop)*/
    0x09U, 0x06U, /* Usage (Keyboard) */
    0xA1U, 0x01U, /* Collection (Application) */
    0x75U, 0x01U, /* Report Size (1U) */
    0x95U, 0x08U, /* Report Count (8U) */
    0x05U, 0x07U, /* Usage Page (Key Codes) */
    0x19U, 0xE0U, /* Usage Minimum (224U) */
    0x29U, 0xE7U, /* Usage Maximum (231U) */
    0x15U, 0x00U, /* Logical Minimum (0U) */
    0x25U, 0x01U, /* Logical Maximum (1U) */
    0x81U, 0x02U, /* Input(Data, Variable, Absolute) Modifier byte */

    0x95U, 0x01U, /* Report count (1U) */
    0x75U, 0x08U, /* Report Size (8U) */
    0x81U, 0x01U, /* Input (Constant), Reserved byte */

    0x95U, 0x05U, /* Report count (5U) */
    0x75U, 0x01U, /* Report Size (1U) */
    0x05U, 0x01U, /* Usage Page (Page# for LEDs) */
    0x19U, 0x01U, /* Usage Minimum (1U) */
    0x29U, 0x05U, /* Usage Maximum (5U) */
    0x91U, 0x02U, /* Output (Data, Variable, Absolute) LED report */

    0x95U, 0x01U, /* Report count (1U) */
    0x75U, 0x03U, /* Report Size (3U) */
    0x91U, 0x01U, /* Output (Constant), LED report padding */

    0x95U, 0x06U, /* Report count (6U) */
    0x75U, 0x08U, /* Report Size (8U) */
    0x15U, 0x00U, /* logical Minimum (0U) */
    0x25U, 0xFFU, /* logical Maximum (255U) */
    0x05U, 0x07U, /* Usage Page (Key Codes) */
    0x19U, 0x00U, /* Usage Minimum (0U) */
    0x29U, 0xFFU, /* Usage Maximum (255U) */
    0x81U, 0x00U, /* Input(Data, Array), Key arrays(6U bytes)*/

    0xC0U, /* end collection */
};
#endif /* FIRMWARE */

/* Define device descriptor */
static uint8_t mk82UsbDeviceDescriptor[] = {
    /* Size of this descriptor in bytes */
    USB_DESCRIPTOR_LENGTH_DEVICE,
    /* DEVICE Descriptor Type */
    USB_DESCRIPTOR_TYPE_DEVICE,
    /* USB Specification Release Number in Binary-Coded Decimal (i.e., 2.10 is 210H). */
    USB_SHORT_GET_LOW(MK82_USB_DEVICE_SPECIFICATION_VERSION), USB_SHORT_GET_HIGH(MK82_USB_DEVICE_SPECIFICATION_VERSION),
    /* Class code (assigned by the USB-IF). */
    MK82_USB_DEVICE_CLASS,
    /* Subclass code (assigned by the USB-IF). */
    MK82_USB_DEVICE_SUBCLASS,
    /* Protocol code (assigned by the USB-IF). */
    MK82_USB_DEVICE_PROTOCOL,
    /* Maximum packet size for endpoint zero (only 8, 16, 32, or 64 are valid) */
    USB_CONTROL_MAX_PACKET_SIZE,
    /* Vendor ID (assigned by the USB-IF) */
    0x09, 0x12,
/* Product ID (assigned by the manufacturer) */
#ifdef FIRMWARE
    0x00, 0x70,
#else  /* FIRMWARE */
    0x01, 0x70,
#endif /* FIRMWARE */
    /* Device release number in binary-coded decimal */
    USB_SHORT_GET_LOW(MK82_USB_DEVICE_VERSION), USB_SHORT_GET_HIGH(MK82_USB_DEVICE_VERSION),
    /* Index of string descriptor describing manufacturer */
    0x01,
    /* Index of string descriptor describing product */
    0x02,
    /* Index of string descriptor describing the device's serial number */
    0x00,
    /* Number of possible configurations */
    MK82_USB_DEVICE_CONFIGURATION_COUNT,
};

/* Define configuration descriptor */
static uint8_t mk82UsbDeviceConfigurationDescriptor[] = {
    /* Size of this descriptor in bytes */
    USB_DESCRIPTOR_LENGTH_CONFIGURE,
    /* CONFIGURATION Descriptor Type */
    USB_DESCRIPTOR_TYPE_CONFIGURE,
    /* Total length of data returned for this configuration. */
    USB_SHORT_GET_LOW(MK82_USB_TOTAL_CONFIGURATION_DESCRIPTOR_LENGTH),
    USB_SHORT_GET_HIGH(MK82_USB_TOTAL_CONFIGURATION_DESCRIPTOR_LENGTH),
    /* Number of interfaces supported by this configuration */
    MK82_USB_INTERFACE_COUNT,
    /* Value to use as an argument to the SetConfiguration() request to select this configuration */
    MK82_USB_DEFAULT_CONFIGURATION_VALUE,
    /* Index of string descriptor describing this configuration */
    0,
    /* Configuration characteristics D7: Reserved (set to one) D6: Self-powered D5: Remote Wakeup D4...0: Reserved
       (reset to zero) */
    (USB_DESCRIPTOR_CONFIGURE_ATTRIBUTE_D7_MASK) |
        (USB_DEVICE_CONFIG_SELF_POWER << USB_DESCRIPTOR_CONFIGURE_ATTRIBUTE_SELF_POWERED_SHIFT) |
        (USB_DEVICE_CONFIG_REMOTE_WAKEUP << USB_DESCRIPTOR_CONFIGURE_ATTRIBUTE_REMOTE_WAKEUP_SHIFT),
    /* Maximum power consumption of the USB * device from the bus in this specific * configuration when the device is
       fully * operational. Expressed in 2 mA units *  (i.e., 50 = 100 mA).  */
    MK82_USB_DEVICE_MAX_POWER,

    /* CCID Interface Descriptor */
    USB_DESCRIPTOR_LENGTH_INTERFACE, USB_DESCRIPTOR_TYPE_INTERFACE, MK82_USB_CCID_INTERFACE_NUMBER, 0x00,
    MK82_USB_CCID_NUMBER_OF_ENDPOINTS, MK82_USB_CCID_CLASS, MK82_USB_CCID_SUBCLASS, MK82_USB_CCID_PROTOCOL,
    0x00, /* Interface Description String Index*/

    CCID_DEVICE_CLASS_DESCRIPTOR,

    /*CCID Bulk IN Endpoint descriptor */
    USB_DESCRIPTOR_LENGTH_ENDPOINT, USB_DESCRIPTOR_TYPE_ENDPOINT, MK82_USB_CCID_BULK_IN_ENDPOINT | (USB_IN << 7U),
    USB_ENDPOINT_BULK, USB_SHORT_GET_LOW(MK82_USB_CCID_BULK_ENDPOINTS_PACKET_SIZE),
    USB_SHORT_GET_HIGH(MK82_USB_CCID_BULK_ENDPOINTS_PACKET_SIZE),
    0x00, /* The polling interval value is every 0 Frames */

    /*CCID Bulk OUT Endpoint descriptor */
    USB_DESCRIPTOR_LENGTH_ENDPOINT, USB_DESCRIPTOR_TYPE_ENDPOINT, MK82_USB_CCID_BULK_OUT_ENDPOINT | (USB_OUT << 7U),
    USB_ENDPOINT_BULK, USB_SHORT_GET_LOW(MK82_USB_CCID_BULK_ENDPOINTS_PACKET_SIZE),
    USB_SHORT_GET_HIGH(MK82_USB_CCID_BULK_ENDPOINTS_PACKET_SIZE),
    0x00 /* The polling interval value is every 0 Frames */

#ifdef FIRMWARE
    ,
    /* U2F Interface Descriptor */
    USB_DESCRIPTOR_LENGTH_INTERFACE, USB_DESCRIPTOR_TYPE_INTERFACE, MK82_USB_U2F_INTERFACE_NUMBER, 0x00,
    MK82_USB_U2F_NUMBER_OF_ENDPOINTS, MK82_USB_U2F_CLASS, MK82_USB_U2F_SUBCLASS, MK82_USB_U2F_PROTOCOL,
    0x00, /* Interface Description String Index*/

    MK82_USB_U2F_HID_DECRIPTOR_LENGTH,  // length of HID descriptor
    USB_DESCRIPTOR_TYPE_HID,            // descriptor type == HID  0x21
    USB_SHORT_GET_LOW(MK82_USB_U2F_HID_DECRIPTOR_SPEC_RELEASE),
    USB_SHORT_GET_HIGH(MK82_USB_U2F_HID_DECRIPTOR_SPEC_RELEASE),  // hid spec release
    MK82_USB_U2F_HID_DECRIPTOR_COUNTRY_CODE,                      // country code == Not Specified
    MK82_USB_U2F_HID_DECRIPTOR_NUMBER_OF_DESCRIPTORS,             // number of HID class descriptors
    USB_DESCRIPTOR_TYPE_HID_REPORT,                               // report descriptor type 0x22
    USB_SHORT_GET_LOW(sizeof(mk82UsbU2fReportDescriptor)),
    USB_SHORT_GET_HIGH(sizeof(mk82UsbU2fReportDescriptor)),  // total length of report descriptor

    /*U2F Interrupt IN Endpoint descriptor */
    USB_DESCRIPTOR_LENGTH_ENDPOINT, USB_DESCRIPTOR_TYPE_ENDPOINT, MK82_USB_U2F_INTERRUPT_IN_ENDPOINT | (USB_IN << 7U),
    USB_ENDPOINT_INTERRUPT, USB_SHORT_GET_LOW(MK82_USB_U2F_INTERRUPT_ENDPOINTS_PACKET_SIZE),
    USB_SHORT_GET_HIGH(MK82_USB_U2F_INTERRUPT_ENDPOINTS_PACKET_SIZE),
    MK82_USB_U2F_INTERRUPT_ENDPOINTS_POLLING_INTERVAL, /* The polling interval value is every 0 Frames */

    /*U2F Interrupt OUT Endpoint descriptor */
    USB_DESCRIPTOR_LENGTH_ENDPOINT, USB_DESCRIPTOR_TYPE_ENDPOINT, MK82_USB_U2F_INTERRUPT_OUT_ENDPOINT | (USB_OUT << 7U),
    USB_ENDPOINT_INTERRUPT, USB_SHORT_GET_LOW(MK82_USB_U2F_INTERRUPT_ENDPOINTS_PACKET_SIZE),
    USB_SHORT_GET_HIGH(MK82_USB_U2F_INTERRUPT_ENDPOINTS_PACKET_SIZE),
    MK82_USB_U2F_INTERRUPT_ENDPOINTS_POLLING_INTERVAL, /* The polling interval value is every 0 Frames */

    /* Keyboard Interface Descriptor */
    USB_DESCRIPTOR_LENGTH_INTERFACE, USB_DESCRIPTOR_TYPE_INTERFACE, MK82_USB_KBD_INTERFACE_NUMBER, 0x00,
    MK82_USB_KBD_NUMBER_OF_ENDPOINTS, MK82_USB_KBD_CLASS, MK82_USB_KBD_SUBCLASS, MK82_USB_KBD_PROTOCOL,
    0x00, /* Interface Description String Index*/

    MK82_USB_KBD_HID_DECRIPTOR_LENGTH,  // length of HID descriptor
    USB_DESCRIPTOR_TYPE_HID,            // descriptor type == HID  0x21
    USB_SHORT_GET_LOW(MK82_USB_KBD_HID_DECRIPTOR_SPEC_RELEASE),
    USB_SHORT_GET_HIGH(MK82_USB_KBD_HID_DECRIPTOR_SPEC_RELEASE),  // hid spec release
    MK82_USB_KBD_HID_DECRIPTOR_COUNTRY_CODE,                      // country code == Not Specified
    MK82_USB_KBD_HID_DECRIPTOR_NUMBER_OF_DESCRIPTORS,             // number of HID class descriptors
    USB_DESCRIPTOR_TYPE_HID_REPORT,                               // report descriptor type 0x22
    USB_SHORT_GET_LOW(sizeof(mk82UsbKeyboardReportDescriptor)),
    USB_SHORT_GET_HIGH(sizeof(mk82UsbKeyboardReportDescriptor)),  // total length of report descriptor

    /* Keyboard Interrupt IN Endpoint descriptor */
    USB_DESCRIPTOR_LENGTH_ENDPOINT, USB_DESCRIPTOR_TYPE_ENDPOINT, MK82_USB_KBD_INTERRUPT_IN_ENDPOINT | (USB_IN << 7U),
    USB_ENDPOINT_INTERRUPT, USB_SHORT_GET_LOW(MK82_USB_KBD_INTERRUPT_ENDPOINT_PACKET_SIZE),
    USB_SHORT_GET_HIGH(MK82_USB_KBD_INTERRUPT_ENDPOINT_PACKET_SIZE),
    MK82_USB_KBD_INTERRUPT_ENDPOINT_POLLING_INTERVAL, /* The polling interval value is every 0 Frames */

    /* BTC Interface Descriptor */
    USB_DESCRIPTOR_LENGTH_INTERFACE, USB_DESCRIPTOR_TYPE_INTERFACE, MK82_USB_BTC_INTERFACE_NUMBER, 0x00,
    MK82_USB_BTC_NUMBER_OF_ENDPOINTS, MK82_USB_BTC_CLASS, MK82_USB_BTC_SUBCLASS, MK82_USB_BTC_PROTOCOL,
    0x00, /* Interface Description String Index*/

    MK82_USB_BTC_HID_DECRIPTOR_LENGTH,  // length of HID descriptor
    USB_DESCRIPTOR_TYPE_HID,            // descriptor type == HID  0x21
    USB_SHORT_GET_LOW(MK82_USB_BTC_HID_DECRIPTOR_SPEC_RELEASE),
    USB_SHORT_GET_HIGH(MK82_USB_BTC_HID_DECRIPTOR_SPEC_RELEASE),  // hid spec release
    MK82_USB_BTC_HID_DECRIPTOR_COUNTRY_CODE,                      // country code == Not Specified
    MK82_USB_BTC_HID_DECRIPTOR_NUMBER_OF_DESCRIPTORS,             // number of HID class descriptors
    USB_DESCRIPTOR_TYPE_HID_REPORT,                               // report descriptor type 0x22
    USB_SHORT_GET_LOW(sizeof(mk82UsbBtcReportDescriptor)),
    USB_SHORT_GET_HIGH(sizeof(mk82UsbBtcReportDescriptor)),  // total length of report descriptor

    /*BTC Interrupt IN Endpoint descriptor */
    USB_DESCRIPTOR_LENGTH_ENDPOINT, USB_DESCRIPTOR_TYPE_ENDPOINT, MK82_USB_BTC_INTERRUPT_IN_ENDPOINT | (USB_IN << 7U),
    USB_ENDPOINT_INTERRUPT, USB_SHORT_GET_LOW(MK82_USB_BTC_INTERRUPT_ENDPOINTS_PACKET_SIZE),
    USB_SHORT_GET_HIGH(MK82_USB_BTC_INTERRUPT_ENDPOINTS_PACKET_SIZE),
    MK82_USB_BTC_INTERRUPT_ENDPOINTS_POLLING_INTERVAL, /* The polling interval value is every 0 Frames */

    /*BTC Interrupt OUT Endpoint descriptor */
    USB_DESCRIPTOR_LENGTH_ENDPOINT, USB_DESCRIPTOR_TYPE_ENDPOINT, MK82_USB_BTC_INTERRUPT_OUT_ENDPOINT | (USB_OUT << 7U),
    USB_ENDPOINT_INTERRUPT, USB_SHORT_GET_LOW(MK82_USB_BTC_INTERRUPT_ENDPOINTS_PACKET_SIZE),
    USB_SHORT_GET_HIGH(MK82_USB_BTC_INTERRUPT_ENDPOINTS_PACKET_SIZE),
    MK82_USB_BTC_INTERRUPT_ENDPOINTS_POLLING_INTERVAL, /* The polling interval value is every 0 Frames */
#endif                                                 /* FIRMWARE */
};

/* Define string descriptor */
static uint8_t mk82UsbDeviceString0[MK82_USB_DESCRIPTOR_LENGTH_STRING0] = {sizeof(mk82UsbDeviceString0),
                                                                           USB_DESCRIPTOR_TYPE_STRING, 0x09, 0x04};

static uint8_t mk82UsbDeviceString1[MK82_USB_DESCRIPTOR_LENGTH_STRING1] = {
    sizeof(mk82UsbDeviceString1), USB_DESCRIPTOR_TYPE_STRING, 'S', 0, 'e', 0, 'c', 0, 'a', 0, 'l', 0, 'o', 0, 't', 0};

#ifdef FIRMWARE
static uint8_t mk82UsbDeviceString2[MK82_USB_DESCRIPTOR_LENGTH_STRING2] = {sizeof(mk82UsbDeviceString2),
                                                                           USB_DESCRIPTOR_TYPE_STRING,
                                                                           'S',
                                                                           0,
                                                                           'e',
                                                                           0,
                                                                           'c',
                                                                           0,
                                                                           'a',
                                                                           0,
                                                                           'l',
                                                                           0,
                                                                           'o',
                                                                           0,
                                                                           't',
                                                                           0,
                                                                           ' ',
                                                                           0,
                                                                           'D',
                                                                           0,
                                                                           'o',
                                                                           0,
                                                                           'n',
                                                                           0,
                                                                           'g',
                                                                           0,
                                                                           'l',
                                                                           0,
                                                                           'e',
                                                                           0};

#else  /* FIRMWARE */
static uint8_t mk82UsbDeviceString2[MK82_USB_DESCRIPTOR_LENGTH_STRING2] = {sizeof(mk82UsbDeviceString2),
                                                                           USB_DESCRIPTOR_TYPE_STRING,
                                                                           'S',
                                                                           0,
                                                                           'e',
                                                                           0,
                                                                           'c',
                                                                           0,
                                                                           'a',
                                                                           0,
                                                                           'l',
                                                                           0,
                                                                           'o',
                                                                           0,
                                                                           't',
                                                                           0,
                                                                           ' ',
                                                                           0,
                                                                           'B',
                                                                           0,
                                                                           'o',
                                                                           0,
                                                                           'o',
                                                                           0,
                                                                           't',
                                                                           0,
                                                                           'l',
                                                                           0,
                                                                           'o',
                                                                           0,
                                                                           'a',
                                                                           0,
                                                                           'd',
                                                                           0,
                                                                           'e',
                                                                           0,
                                                                           'r',
                                                                           0};
#endif /* FIRMWARE */

static uint8_t *mk82UsbDeviceStringDescriptorArray[MK82_USB_DEVICE_STRING_COUNT] = {
    mk82UsbDeviceString0, mk82UsbDeviceString1, mk82UsbDeviceString2};

/* Define string descriptor size */
static uint32_t mk82UsbDeviceStringDescriptorLength[MK82_USB_DEVICE_STRING_COUNT] = {
    sizeof(mk82UsbDeviceString0), sizeof(mk82UsbDeviceString1), sizeof(mk82UsbDeviceString2)};
static usb_language_t mk82UsbDeviceLanguage[MK82_USB_DEVICE_LANGUAGE_COUNT] = {{
    mk82UsbDeviceStringDescriptorArray, mk82UsbDeviceStringDescriptorLength, (uint16_t)0x0409,
}};

static usb_language_list_t g_UsbDeviceLanguageList = {
    mk82UsbDeviceString0, sizeof(mk82UsbDeviceString0), mk82UsbDeviceLanguage, MK82_USB_DEVICE_LANGUAGE_COUNT,
};

/*******************************************************************************
* Code
******************************************************************************/
/*!
 * @brief Get the descritpor.
 *
 * The function is used to get the descritpor, including the device descritpor, configuration descriptor, and string
 * descriptor, etc.
 *
 * @param handle              The device handle.
 * @param setup               The setup packet buffer address.
 * @param length              It is an OUT parameter, return the data length need to be sent to host.
 * @param buffer              It is an OUT parameter, return the data buffer address.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceGetDescriptor(usb_device_handle handle, usb_setup_struct_t *setup, uint32_t *length,
                                     uint8_t **buffer)
{
    uint8_t descriptorType = (uint8_t)((setup->wValue & 0xFF00U) >> 8U);
    uint8_t descriptorIndex = (uint8_t)((setup->wValue & 0x00FFU));
    usb_status_t ret = kStatus_USB_Success;
    if (USB_REQUSET_STANDARD_GET_DESCRIPTOR != setup->bRequest)
    {
        return kStatus_USB_InvalidRequest;
    }
    switch (descriptorType)
    {
#ifdef FIRMWARE
        case USB_DESCRIPTOR_TYPE_HID_REPORT:
        {
            if (MK82_USB_U2F_INTERFACE_NUMBER == setup->wIndex)
            {
                /* Get HID report descriptor */
                *buffer = mk82UsbU2fReportDescriptor;
                *length = sizeof(mk82UsbU2fReportDescriptor);
            }
            else if (MK82_USB_KBD_INTERFACE_NUMBER == setup->wIndex)
            {
                /* Get HID report descriptor */
                *buffer = mk82UsbKeyboardReportDescriptor;
                *length = sizeof(mk82UsbKeyboardReportDescriptor);
            }
            else if (MK82_USB_BTC_INTERFACE_NUMBER == setup->wIndex)
            {
                /* Get HID report descriptor */
                *buffer = mk82UsbBtcReportDescriptor;
                *length = sizeof(mk82UsbBtcReportDescriptor);
            }
            else
            {
            }
        }
#endif /* FIRMWARE */
        break;
        case USB_DESCRIPTOR_TYPE_STRING:
        {
            if (descriptorIndex == 0)
            {
                *buffer = (uint8_t *)g_UsbDeviceLanguageList.languageString;
                *length = g_UsbDeviceLanguageList.stringLength;
            }
            else
            {
                uint8_t langId = 0;
                uint8_t langIndex = MK82_USB_DEVICE_STRING_COUNT;

                for (; langId < MK82_USB_DEVICE_LANGUAGE_COUNT; langId++)
                {
                    if (setup->wIndex == g_UsbDeviceLanguageList.languageList[langId].languageId)
                    {
                        if (descriptorIndex < MK82_USB_DEVICE_STRING_COUNT)
                        {
                            langIndex = descriptorIndex;
                        }
                        break;
                    }
                }

                if (MK82_USB_DEVICE_STRING_COUNT == langIndex)
                {
                    langId = 0;
                }
                *buffer = (uint8_t *)g_UsbDeviceLanguageList.languageList[langId].string[langIndex];
                *length = g_UsbDeviceLanguageList.languageList[langId].length[langIndex];
            }
        }
        break;
        case USB_DESCRIPTOR_TYPE_DEVICE:
        {
            *buffer = mk82UsbDeviceDescriptor;
            *length = USB_DESCRIPTOR_LENGTH_DEVICE;
        }
        break;
        case USB_DESCRIPTOR_TYPE_CONFIGURE:
        {
            *buffer = mk82UsbDeviceConfigurationDescriptor;
            *length = sizeof(mk82UsbDeviceConfigurationDescriptor);
        }
        break;
        default:
            ret = kStatus_USB_InvalidRequest;
            break;
    } /* End Switch */
    return ret;
}

/*!
 * @brief Set the device configuration.
 *
 * The function is used to set the device configuration.
 *
 * @param handle              The device handle.
 * @param configure           The configuration value.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceSetConfigure(usb_device_handle handle, uint8_t configure)
{
    if (!configure)
    {
        return kStatus_USB_Error;
    }
    mk82UsbCurrentConfiguration = configure;
    return USB_DeviceCallback(handle, kUSB_DeviceEventSetConfiguration, &configure);
}

/*!
 * @brief Get the device configuration.
 *
 * The function is used to get the device configuration.
 *
 * @param handle The device handle.
 * @param configure It is an OUT parameter, save the current configuration value.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceGetConfigure(usb_device_handle handle, uint8_t *configure)
{
    *configure = mk82UsbCurrentConfiguration;
    return kStatus_USB_Success;
}

/*!
 * @brief Set an interface alternate setting.
 *
 * The function is used to set an interface alternate setting.
 *
 * @param handle The device handle.
 * @param interface The interface index.
 * @param alternateSetting The new alternate setting value.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceSetInterface(usb_device_handle handle, uint8_t interface, uint8_t alternateSetting)
{
    mk82UsbInterfaces[interface] = alternateSetting;
    return USB_DeviceCallback(handle, kUSB_DeviceEventSetInterface, &interface);
}

/*!
 * @brief Get an interface alternate setting.
 *
 * The function is used to get an interface alternate setting.
 *
 * @param handle The device handle.
 * @param interface The interface index.
 * @param alternateSetting It is an OUT parameter, save the new alternate setting value of the interface.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceGetInterface(usb_device_handle handle, uint8_t interface, uint8_t *alternateSetting)
{
    *alternateSetting = mk82UsbInterfaces[interface];
    return kStatus_USB_Success;
}
