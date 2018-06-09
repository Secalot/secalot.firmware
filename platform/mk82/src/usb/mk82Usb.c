/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * Copyright (c) 2013 - 2015, Freescale Semiconductor, Inc.
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

#include "mk82Global.h"
#include "mk82GlobalInt.h"
#include "mk82System.h"
#include "mk82Usb.h"
#include "mk82UsbInt.h"

#include "fsl_device_registers.h"
#include "fsl_mpu.h"
#include "fsl_pit.h"

#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"
#include "usb_device_ch9.h"

#include "mk82USBDeviceDescriptor.h"
#include "ccidGlobal.h"
#include "ccidCore.h"

#ifdef FIRMWARE
#include "opgpGlobal.h"
#include "opgpCore.h"

#include "sfGlobal.h"
#include "sfHid.h"

#include "btcGlobal.h"
#include "btcHid.h"
#endif /* FIRMWARE */

static usb_status_t USB_DeviceCcidBulkIn(usb_device_handle handle,
                                         usb_device_endpoint_callback_message_struct_t *message, void *callbackParam);
static usb_status_t USB_DeviceCcidBulkOut(usb_device_handle handle,
                                          usb_device_endpoint_callback_message_struct_t *message, void *callbackParam);
#ifdef FIRMWARE
static usb_status_t USB_DeviceU2fInterruptIn(usb_device_handle handle,
                                             usb_device_endpoint_callback_message_struct_t *message,
                                             void *callbackParam);
static usb_status_t USB_DeviceU2fInterruptOut(usb_device_handle handle,
                                              usb_device_endpoint_callback_message_struct_t *message,
                                              void *callbackParam);
static usb_status_t USB_DeviceKeyboardInterruptIn(usb_device_handle handle,
                                                  usb_device_endpoint_callback_message_struct_t *message,
                                                  void *callbackParam);
static usb_status_t USB_DeviceBtcInterruptOut(usb_device_handle handle,
                                              usb_device_endpoint_callback_message_struct_t *message,
                                              void *callbackParam);
#endif /* FIRMWARE */

static void mk82UsbFatalError(void);
static void mk82UsbCcidSendDataBlocking(uint8_t *buffer, uint32_t bufferLength);
#ifdef FIRMWARE
static void mk82UsbU2fSendPacketBlocking(void);
static void mk82UsbKeyboardSendPacketBlocking(void);
static void mk82UsbBtcSendPacketBlocking(void);
#endif /* FIRMWARE */
static uint16_t mk82UsbCheckForAnEvent(uint32_t dataTypesToProcess);

static CCID_CORE_HANDLE mk82UsbCcidHandle;
static usb_device_handle mk82UsbDeviceHandle;
static uint8_t mk82UsbCcidPacketBuffer[MK82_USB_CCID_BULK_ENDPOINTS_PACKET_SIZE];
static uint8_t mk82UsbCcidDataBuffer[CCID_MAX_MESSAGE_LENGTH];
static volatile uint16_t mk82UsbCcidDataSent = MK82_FALSE;
static volatile uint16_t mk82UsbCcidPacketReceived = MK82_FALSE;
static uint32_t mk82UsbCcidReceivedPacketLength = 0;

#ifdef FIRMWARE
static uint8_t mk82UsbU2fHidDataBuffer[SF_HID_MAX_DATA_SIZE];
static SF_HID_HANDLE mk82UsbSfHidHandle;
static uint8_t mk82UsbU2fIncomingPacketBuffer[MK82_USB_U2F_INTERRUPT_ENDPOINTS_PACKET_SIZE];
static uint8_t mk82UsbU2fOutgoingPacketBuffer[MK82_USB_U2F_INTERRUPT_ENDPOINTS_PACKET_SIZE];
static volatile uint16_t mk82UsbU2fPacketSent = MK82_FALSE;
static volatile uint16_t mk82UsbU2fPacketReceived = MK82_FALSE;
static volatile uint16_t mk82UsbU2fTimerExpired = MK82_FALSE;

static uint8_t mk82UsbKeyboardOutgoingPacketBuffer[MK82_USB_KBD_INTERRUPT_ENDPOINT_PACKET_SIZE];
static volatile uint16_t mk82UsbKeyboardPacketSent = MK82_FALSE;

static uint8_t mk82UsbBtcHidDataBuffer[BTC_HID_MAX_DATA_SIZE];
static BTC_HID_HANDLE mk82UsbBtcHidHandle;
static uint8_t mk82UsbBtcIncomingPacketBuffer[MK82_USB_BTC_INTERRUPT_ENDPOINTS_PACKET_SIZE];
static uint8_t mk82UsbBtcOutgoingPacketBuffer[MK82_USB_BTC_INTERRUPT_ENDPOINTS_PACKET_SIZE];
static volatile uint16_t mk82UsbBtcPacketSent = MK82_FALSE;
static volatile uint16_t mk82UsbBtcPacketReceived = MK82_FALSE;
#endif /* FIRMWARE */

#ifdef FIRMWARE
static const uint8_t mk82UsbKeyboardKeyTable[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2A, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x1e, 0x34, 0x20, 0x21, 0x22,
    0x24, 0x34, 0x26, 0x27, 0x25, 0x2e, 0x36, 0x2d, 0x37, 0x38, 0x27, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
    0x26, 0x33, 0x33, 0x36, 0x2e, 0x37, 0x38, 0x1f, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x2f, 0x31, 0x30, 0x23,
    0x2d, 0x35, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14,
    0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x2f, 0x31, 0x30, 0x35, 0x7f};

static const uint8_t mk82UsbKeyboardShiftModifierTable[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x00, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x02,
    0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x00};
#endif /* FIRMWARE */

static void mk82UsbFatalError(void) { mk82SystemFatalError(); }

/*!
 * @brief CCID bulk in pipe callback function.
 *
 * This function serves as the callback function for CCID bulk in pipe.
 *
 * @param handle The USB device handle.
 * @param message The endpoint callback message
 * @param callbackParam The parameter of the callback.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
static usb_status_t USB_DeviceCcidBulkIn(usb_device_handle handle,
                                         usb_device_endpoint_callback_message_struct_t *message, void *callbackParam)
{
    usb_status_t error = kStatus_USB_Error;

    if ((message->length != 0) && (!(message->length % MK82_USB_CCID_BULK_ENDPOINTS_PACKET_SIZE)))
    {
        /* If the last packet is the size of endpoint, then send also zero-ended packet,
         ** meaning that we want to inform the host that we do not have any additional
         ** data, so it can flush the output.
         */
        USB_DeviceSendRequest(handle, MK82_USB_CCID_BULK_IN_ENDPOINT, NULL, 0);
        error = kStatus_USB_Success;
    }
    else
    {
        mk82UsbCcidDataSent = MK82_TRUE;
        error = kStatus_USB_Success;
    }

    return error;
}

/*!
 * @brief CCID bulk out pipe callback function.
 *
 * This function serves as the callback function for CCID bulk out pipe.
 *
 * @param handle The USB device handle.
 * @param message The endpoint callback message
 * @param callbackParam The parameter of the callback.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
static usb_status_t USB_DeviceCcidBulkOut(usb_device_handle handle,
                                          usb_device_endpoint_callback_message_struct_t *message, void *callbackParam)
{
    if (message->length == USB_UNINITIALIZED_VAL_32)
    {
        return kStatus_USB_Error;
    }

    mk82UsbCcidReceivedPacketLength = message->length;
    mk82UsbCcidPacketReceived = MK82_TRUE;
    return kStatus_USB_Success;
}

#ifdef FIRMWARE

/*!
 * @brief U2F interrupt in pipe callback function.
 *
 * This function serves as the callback function for U2F interrupt in pipe.
 *
 * @param handle The USB device handle.
 * @param message The endpoint callback message
 * @param callbackParam The parameter of the callback.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
static usb_status_t USB_DeviceU2fInterruptIn(usb_device_handle handle,
                                             usb_device_endpoint_callback_message_struct_t *message,
                                             void *callbackParam)
{
    usb_status_t error = kStatus_USB_Error;

    mk82UsbU2fPacketSent = MK82_TRUE;
    error = kStatus_USB_Success;

    return error;
}

/*!
 * @brief U2F interrupt out pipe callback function.
 *
 * This function serves as the callback function for U2F interrupt out pipe.
 *
 * @param handle The USB device handle.
 * @param message The endpoint callback message
 * @param callbackParam The parameter of the callback.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
static usb_status_t USB_DeviceU2fInterruptOut(usb_device_handle handle,
                                              usb_device_endpoint_callback_message_struct_t *message,
                                              void *callbackParam)
{
    if (message->length != MK82_USB_U2F_INTERRUPT_ENDPOINTS_PACKET_SIZE)
    {
        if (message->length != USB_UNINITIALIZED_VAL_32)
        {
            mk82UsbFatalError();
        }
        else
        {
            return kStatus_USB_Error;
        }
    }

    mk82UsbU2fPacketReceived = MK82_TRUE;
    return kStatus_USB_Success;
}

/*!
 * @brief Keyboard interrupt in pipe callback function.
 *
 * This function serves as the callback function for U2F interrupt in pipe.
 *
 * @param handle The USB device handle.
 * @param message The endpoint callback message
 * @param callbackParam The parameter of the callback.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
static usb_status_t USB_DeviceKeyboardInterruptIn(usb_device_handle handle,
                                                  usb_device_endpoint_callback_message_struct_t *message,
                                                  void *callbackParam)
{
    usb_status_t error = kStatus_USB_Error;

    mk82UsbKeyboardPacketSent = MK82_TRUE;
    error = kStatus_USB_Success;

    return error;
}

/*!
 * @brief BTC interrupt in pipe callback function.
 *
 * This function serves as the callback function for BTC interrupt in pipe.
 *
 * @param handle The USB device handle.
 * @param message The endpoint callback message
 * @param callbackParam The parameter of the callback.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
static usb_status_t USB_DeviceBtcInterruptIn(usb_device_handle handle,
                                             usb_device_endpoint_callback_message_struct_t *message,
                                             void *callbackParam)
{
    usb_status_t error = kStatus_USB_Error;

    mk82UsbBtcPacketSent = MK82_TRUE;
    error = kStatus_USB_Success;

    return error;
}

/*!
 * @brief BTC interrupt out pipe callback function.
 *
 * This function serves as the callback function for BTC interrupt out pipe.
 *
 * @param handle The USB device handle.
 * @param message The endpoint callback message
 * @param callbackParam The parameter of the callback.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
static usb_status_t USB_DeviceBtcInterruptOut(usb_device_handle handle,
                                              usb_device_endpoint_callback_message_struct_t *message,
                                              void *callbackParam)
{
    if (message->length != MK82_USB_BTC_INTERRUPT_ENDPOINTS_PACKET_SIZE)
    {
        if (message->length != USB_UNINITIALIZED_VAL_32)
        {
            mk82UsbFatalError();
        }
        else
        {
            return kStatus_USB_Error;
        }
    }

    mk82UsbBtcPacketReceived = MK82_TRUE;
    return kStatus_USB_Success;
}

#endif /* FIRMWARE */

/*!
 * @brief Class specific callback function.
 *
 * This function handles class specific requests.
 *
 * @param handle The USB device handle.
 * @param setup The pointer to the setup packet.
 * @param length The pointer to the length of the data buffer.
 * @param buffer The pointer to the address of setup packet data buffer.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceProcessClassRequest(usb_device_handle handle, usb_setup_struct_t *setup, uint32_t *length,
                                           uint8_t **buffer)
{
    usb_status_t error = kStatus_USB_InvalidRequest;

#ifdef FIRMWARE
    if (setup->wIndex == MK82_USB_KBD_INTERFACE_NUMBER)
    {
        switch (setup->bRequest)
        {
            case MK82_USB_DEVICE_HID_REQUEST_GET_REPORT:
                break;
            case MK82_USB_DEVICE_HID_REQUEST_GET_IDLE:
                break;
            case MK82_USB_DEVICE_HID_REQUEST_GET_PROTOCOL:
                break;
            case MK82_USB_DEVICE_HID_REQUEST_SET_REPORT:
                break;
            case MK82_USB_DEVICE_HID_REQUEST_SET_IDLE:
                error = kStatus_USB_Success;
                break;
            case MK82_USB_DEVICE_HID_REQUEST_SET_PROTOCOL:
                break;
            default:
                break;
        }
    }
#endif /* FIRMWARE */

    return error;
}

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
usb_status_t USB_DeviceCallback(usb_device_handle handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;
    uint8_t *temp8 = (uint8_t *)param;

    switch (event)
    {
        case kUSB_DeviceEventBusReset:
        {
            USB_DeviceControlPipeInit(mk82UsbDeviceHandle);
        }
        break;
        case kUSB_DeviceEventSetConfiguration:
            if (param)
            {
                if (MK82_USB_DEFAULT_CONFIGURATION_VALUE == (*temp8))
                {
                    usb_device_endpoint_init_struct_t epInitStruct;
                    usb_device_endpoint_callback_struct_t endpointCallback;

                    /* CCID */

                    endpointCallback.callbackFn = USB_DeviceCcidBulkIn;
                    endpointCallback.callbackParam = handle;

                    epInitStruct.zlt = 0;
                    epInitStruct.transferType = USB_ENDPOINT_BULK;
                    epInitStruct.endpointAddress =
                        MK82_USB_CCID_BULK_IN_ENDPOINT | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT);
                    epInitStruct.maxPacketSize = MK82_USB_CCID_BULK_ENDPOINTS_PACKET_SIZE;

                    USB_DeviceInitEndpoint(mk82UsbDeviceHandle, &epInitStruct, &endpointCallback);

                    endpointCallback.callbackFn = USB_DeviceCcidBulkOut;
                    endpointCallback.callbackParam = handle;

                    epInitStruct.zlt = 0;
                    epInitStruct.transferType = USB_ENDPOINT_BULK;
                    epInitStruct.endpointAddress =
                        MK82_USB_CCID_BULK_OUT_ENDPOINT | (USB_OUT << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT);
                    epInitStruct.maxPacketSize = MK82_USB_CCID_BULK_ENDPOINTS_PACKET_SIZE;

                    USB_DeviceInitEndpoint(mk82UsbDeviceHandle, &epInitStruct, &endpointCallback);

#ifdef FIRMWARE
                    /* U2F */

                    endpointCallback.callbackFn = USB_DeviceU2fInterruptIn;
                    endpointCallback.callbackParam = handle;

                    epInitStruct.zlt = 0;
                    epInitStruct.transferType = USB_ENDPOINT_INTERRUPT;
                    epInitStruct.endpointAddress = MK82_USB_U2F_INTERRUPT_IN_ENDPOINT |
                                                   (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT);
                    epInitStruct.maxPacketSize = MK82_USB_U2F_INTERRUPT_ENDPOINTS_PACKET_SIZE;

                    USB_DeviceInitEndpoint(mk82UsbDeviceHandle, &epInitStruct, &endpointCallback);

                    endpointCallback.callbackFn = USB_DeviceU2fInterruptOut;
                    endpointCallback.callbackParam = handle;

                    epInitStruct.zlt = 0;
                    epInitStruct.transferType = USB_ENDPOINT_INTERRUPT;
                    epInitStruct.endpointAddress = MK82_USB_U2F_INTERRUPT_OUT_ENDPOINT |
                                                   (USB_OUT << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT);
                    epInitStruct.maxPacketSize = MK82_USB_U2F_INTERRUPT_ENDPOINTS_PACKET_SIZE;

                    USB_DeviceInitEndpoint(mk82UsbDeviceHandle, &epInitStruct, &endpointCallback);

                    /* Keyboard */

                    endpointCallback.callbackFn = USB_DeviceKeyboardInterruptIn;
                    endpointCallback.callbackParam = handle;

                    epInitStruct.zlt = 0;
                    epInitStruct.transferType = USB_ENDPOINT_INTERRUPT;
                    epInitStruct.endpointAddress = MK82_USB_KBD_INTERRUPT_IN_ENDPOINT |
                                                   (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT);
                    epInitStruct.maxPacketSize = MK82_USB_KBD_INTERRUPT_ENDPOINT_PACKET_SIZE;

                    USB_DeviceInitEndpoint(mk82UsbDeviceHandle, &epInitStruct, &endpointCallback);

                    /* BTC */

                    endpointCallback.callbackFn = USB_DeviceBtcInterruptIn;
                    endpointCallback.callbackParam = handle;

                    epInitStruct.zlt = 0;
                    epInitStruct.transferType = USB_ENDPOINT_INTERRUPT;
                    epInitStruct.endpointAddress = MK82_USB_BTC_INTERRUPT_IN_ENDPOINT |
                                                   (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT);
                    epInitStruct.maxPacketSize = MK82_USB_BTC_INTERRUPT_ENDPOINTS_PACKET_SIZE;

                    USB_DeviceInitEndpoint(mk82UsbDeviceHandle, &epInitStruct, &endpointCallback);

                    endpointCallback.callbackFn = USB_DeviceBtcInterruptOut;
                    endpointCallback.callbackParam = handle;

                    epInitStruct.zlt = 0;
                    epInitStruct.transferType = USB_ENDPOINT_INTERRUPT;
                    epInitStruct.endpointAddress = MK82_USB_BTC_INTERRUPT_OUT_ENDPOINT |
                                                   (USB_OUT << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT);
                    epInitStruct.maxPacketSize = MK82_USB_BTC_INTERRUPT_ENDPOINTS_PACKET_SIZE;

                    USB_DeviceInitEndpoint(mk82UsbDeviceHandle, &epInitStruct, &endpointCallback);
#endif /* FIRMWARE */

                    USB_DeviceRecvRequest(handle, MK82_USB_CCID_BULK_OUT_ENDPOINT, mk82UsbCcidPacketBuffer,
                                          MK82_USB_CCID_BULK_ENDPOINTS_PACKET_SIZE);
#ifdef FIRMWARE
                    USB_DeviceRecvRequest(handle, MK82_USB_U2F_INTERRUPT_OUT_ENDPOINT, mk82UsbU2fIncomingPacketBuffer,
                                          MK82_USB_U2F_INTERRUPT_ENDPOINTS_PACKET_SIZE);
                    USB_DeviceRecvRequest(handle, MK82_USB_BTC_INTERRUPT_OUT_ENDPOINT, mk82UsbBtcIncomingPacketBuffer,
                                          MK82_USB_BTC_INTERRUPT_ENDPOINTS_PACKET_SIZE);
#endif /* FIRMWARE */

                    error = kStatus_USB_Success;
                }
            }
            break;
        default:
            break;
    }

    return error;
}

/*!
 * @brief Get the setup packet buffer.
 *
 * This function provides the buffer for setup packet.
 *
 * @param handle The USB device handle.
 * @param setupBuffer The pointer to the address of setup packet buffer.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceGetSetupBuffer(usb_device_handle handle, usb_setup_struct_t **setupBuffer)
{
    static uint32_t setup[sizeof(usb_setup_struct_t)];
    if (NULL == setupBuffer)
    {
        return kStatus_USB_InvalidParameter;
    }
    *setupBuffer = (usb_setup_struct_t *)&setup;
    return kStatus_USB_Success;
}

/*!
 * @brief Get the setup packet data buffer.
 *
 * This function gets the data buffer for setup packet.
 *
 * @param handle The USB device handle.
 * @param setup The pointer to the setup packet.
 * @param length The pointer to the length of the data buffer.
 * @param buffer The pointer to the address of setup packet data buffer.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceGetClassReceiveBuffer(usb_device_handle handle, usb_setup_struct_t *setup, uint32_t *length,
                                             uint8_t **buffer)
{
    static uint8_t setupOut[8];
    if ((NULL == buffer) || ((*length) > sizeof(setupOut)))
    {
        return kStatus_USB_InvalidRequest;
    }
    *buffer = setupOut;
    return kStatus_USB_Success;
}

/*!
 * @brief Configure remote wakeup feature.
 *
 * This function configures the remote wakeup feature.
 *
 * @param handle The USB device handle.
 * @param enable 1: enable, 0: disable.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DevcieConfigureRemoteWakeup(usb_device_handle handle, uint8_t enable)
{
    return kStatus_USB_InvalidRequest;
}

/*!
 * @brief USB configure endpoint function.
 *
 * This function configure endpoint status.
 *
 * @param handle The USB device handle.
 * @param ep Endpoint address.
 * @param status A flag to indicate whether to stall the endpoint. 1: stall, 0: unstall.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DevcieConfigureEndpointStatus(usb_device_handle handle, uint8_t ep, uint8_t status)
{
    if (status)
    {
        return USB_DeviceStallEndpoint(handle, ep);
    }
    else
    {
        return USB_DeviceUnstallEndpoint(handle, ep);
    }
}

void USB0_IRQHandler(void) { USB_DeviceKhciIsrFunction(mk82UsbDeviceHandle); }

void PIT0_IRQHandler(void)
{
    uint8_t *wtxRequest;
    uint32_t wtxRequestLength;

    PIT_ClearStatusFlags(PIT0, kPIT_Chnl_0, PIT_TFLG_TIF_MASK);

    ccidCoreGetWTXRequest(&mk82UsbCcidHandle, &wtxRequest, &wtxRequestLength);

    mk82UsbCcidSendDataBlocking(wtxRequest, wtxRequestLength);
}

#ifdef FIRMWARE
void PIT1_IRQHandler(void)
{
    mk82UsbU2fTimerExpired = MK82_TRUE;

    PIT_StopTimer(PIT, kPIT_Chnl_1);
    PIT_ClearStatusFlags(PIT0, kPIT_Chnl_1, PIT_TFLG_TIF_MASK);
}
#endif /* FIRMWARE */

static void mk82UsbCcidSendDataBlocking(uint8_t *buffer, uint32_t bufferLength)
{
    usb_status_t error = kStatus_USB_Error;

    error = USB_DeviceSendRequest(mk82UsbDeviceHandle, MK82_USB_CCID_BULK_IN_ENDPOINT, buffer, bufferLength);

    if (error != kStatus_USB_Success)
    {
        mk82UsbFatalError();
    }

    while (mk82UsbCcidDataSent != MK82_TRUE)
    {
    };

    mk82UsbCcidDataSent = MK82_FALSE;
}

#ifdef FIRMWARE
static void mk82UsbU2fSendPacketBlocking(void)
{
    usb_status_t error = kStatus_USB_Error;

    error = USB_DeviceSendRequest(mk82UsbDeviceHandle, MK82_USB_U2F_INTERRUPT_IN_ENDPOINT,
                                  mk82UsbU2fOutgoingPacketBuffer, MK82_USB_U2F_INTERRUPT_ENDPOINTS_PACKET_SIZE);

    if (error != kStatus_USB_Success)
    {
        mk82UsbFatalError();
    }

    while (mk82UsbU2fPacketSent != MK82_TRUE)
    {
    };

    mk82UsbU2fPacketSent = MK82_FALSE;
}

static void mk82UsbKeyboardSendPacketBlocking(void)
{
    usb_status_t error = kStatus_USB_Error;

    error = USB_DeviceSendRequest(mk82UsbDeviceHandle, MK82_USB_KBD_INTERRUPT_IN_ENDPOINT,
                                  mk82UsbKeyboardOutgoingPacketBuffer, MK82_USB_KBD_INTERRUPT_ENDPOINT_PACKET_SIZE);

    if (error != kStatus_USB_Success)
    {
        mk82UsbFatalError();
    }

    while (mk82UsbKeyboardPacketSent != MK82_TRUE)
    {
    };

    mk82UsbKeyboardPacketSent = MK82_FALSE;
}

static void mk82UsbBtcSendPacketBlocking(void)
{
    usb_status_t error = kStatus_USB_Error;

    error = USB_DeviceSendRequest(mk82UsbDeviceHandle, MK82_USB_BTC_INTERRUPT_IN_ENDPOINT,
                                  mk82UsbBtcOutgoingPacketBuffer, MK82_USB_BTC_INTERRUPT_ENDPOINTS_PACKET_SIZE);

    if (error != kStatus_USB_Success)
    {
        mk82UsbFatalError();
    }

    while (mk82UsbBtcPacketSent != MK82_TRUE)
    {
    };

    mk82UsbBtcPacketSent = MK82_FALSE;
}
#endif /* FIRMWARE */

static uint16_t mk82UsbCheckForAnEvent(uint32_t dataTypesToProcess)
{
    uint16_t retVal;

    if ( ((dataTypesToProcess&MK82_GLOBAL_PROCESS_CCID_APDU) != 0) && (mk82UsbCcidPacketReceived == MK82_TRUE) )
    {
        mk82UsbCcidPacketReceived = MK82_FALSE;

        retVal = MK82_USB_EVENT_CCID_PACKET_RECEIVED;
    }
#ifdef FIRMWARE
    else if (((dataTypesToProcess&MK82_GLOBAL_PROCESS_U2F_MESSAGE) != 0) && (mk82UsbU2fPacketReceived == MK82_TRUE))
    {
        mk82UsbU2fPacketReceived = MK82_FALSE;

        retVal = MK82_USB_EVENT_U2F_PACKET_RECEIVED;
    }
    else if (((dataTypesToProcess&MK82_GLOBAL_PROCESS_U2F_MESSAGE) != 0) && (mk82UsbU2fTimerExpired == MK82_TRUE))
    {
        mk82UsbU2fTimerExpired = MK82_FALSE;

        retVal = MK82_USB_EVENT_U2F_TIMER_EXPIRED;
    }
    else if (((dataTypesToProcess&MK82_GLOBAL_PROCESS_BTC_MESSAGE) != 0) && (mk82UsbBtcPacketReceived == MK82_TRUE))
    {
        mk82UsbBtcPacketReceived = MK82_FALSE;

        retVal = MK82_USB_EVENT_BTC_PACKET_RECEIVED;
    }
#endif /* FIRMWARE */
    else
    {
        retVal = MK82_USB_EVENT_NOTHING_HAPPENED;
    }

    return retVal;
}

void mk82UsbInit(void)
{
    uint8_t histChars[MK82_USB_MAX_HISTCHARS_LENGTH];
    uint32_t histCharsLength = 0;

#ifdef FIRMWARE
    opgpCoreGetHistChars(histChars, &histCharsLength);
#endif /* FIRMWARE */

    ccidCoreInit(&mk82UsbCcidHandle, mk82UsbCcidDataBuffer, histChars, histCharsLength);

#ifdef FIRMWARE
    sfHidInit(&mk82UsbSfHidHandle, mk82UsbU2fHidDataBuffer);
    btcHidInit(&mk82UsbBtcHidHandle, mk82UsbBtcHidDataBuffer);
#endif /* FIRMWARE */

    SystemCoreClockUpdate();

    CLOCK_EnableUsbfs0Clock(kCLOCK_UsbSrcIrc48M, 48000000U);

    if (kStatus_USB_Success != USB_DeviceInit(kUSB_ControllerKhci0, USB_DeviceCallback, &mk82UsbDeviceHandle))
    {
        mk82UsbFatalError();
    }

    MPU_Enable(MPU, 0);

    NVIC_SetPriority(USB0_IRQn, MK82_USB_DEVICE_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ(USB0_IRQn);

    USB_DeviceRun(mk82UsbDeviceHandle);

    PIT_SetTimerPeriod(PIT0, kPIT_Chnl_0, MSEC_TO_COUNT(CCID_WTX_TIMEOUT_IN_MS, CLOCK_GetFreq(kCLOCK_BusClk)));
    PIT_EnableInterrupts(PIT0, kPIT_Chnl_0, kPIT_TimerInterruptEnable);
    NVIC_SetPriority(PIT0_IRQn, MK82_PIT_WTX_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ(PIT0_IRQn);

#ifdef FIRMWARE
    PIT_SetTimerPeriod(PIT0, kPIT_Chnl_1, MSEC_TO_COUNT(SF_HID_TIMER_TIMEOUT_IN_MS, CLOCK_GetFreq(kCLOCK_BusClk)));
    PIT_EnableInterrupts(PIT0, kPIT_Chnl_1, kPIT_TimerInterruptEnable);
    NVIC_SetPriority(PIT1_IRQn, MK82_PIT_U2F_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ(PIT1_IRQn);
#endif /* FIRMWARE */
}

uint16_t mk82UsbCheckForNewCommand(uint32_t dataTypesToProcess, uint8_t **data, uint32_t *dataLength, uint16_t *dataType)
{
    uint16_t event;
    uint16_t newCommandReceived = MK82_USB_COMMAND_NOT_RECEIVED;

    if ((data == NULL) || (dataLength == NULL))
    {
        mk82UsbFatalError();
    }

    event = mk82UsbCheckForAnEvent(dataTypesToProcess);

    if (event == MK82_USB_EVENT_CCID_PACKET_RECEIVED)
    {
        uint16_t requiredAction;

        ccidCoreProcessIncomingPacket(&mk82UsbCcidHandle, mk82UsbCcidPacketBuffer, mk82UsbCcidReceivedPacketLength,
                                      &requiredAction);

        if (requiredAction == CCID_CORE_ACTION_SEND_RESPOSE)
        {
            uint8_t *response;
            uint32_t responseLength;

            ccidCoreGetResponse(&mk82UsbCcidHandle, &response, &responseLength);

            mk82UsbCcidSendDataBlocking(response, responseLength);

            ccidCoreResponseSent(&mk82UsbCcidHandle);

            USB_DeviceRecvRequest(mk82UsbDeviceHandle, MK82_USB_CCID_BULK_OUT_ENDPOINT, mk82UsbCcidPacketBuffer,
                                  MK82_USB_CCID_BULK_ENDPOINTS_PACKET_SIZE);
        }
        else if (requiredAction == CCID_CORE_ACTION_PROCESS_RECEIVED_APDU)
        {
            ccidCoreGetAPDU(&mk82UsbCcidHandle, data, dataLength);

            PIT_StartTimer(PIT, kPIT_Chnl_0);

            *dataType = MK82_GLOBAL_DATATYPE_CCID_APDU;

            newCommandReceived = MK82_USB_COMMAND_RECEIVED;
        }
        else if (requiredAction == CCID_CORE_ACTION_DO_NOTHING)
        {
            USB_DeviceRecvRequest(mk82UsbDeviceHandle, MK82_USB_CCID_BULK_OUT_ENDPOINT, mk82UsbCcidPacketBuffer,
                                  MK82_USB_CCID_BULK_ENDPOINTS_PACKET_SIZE);
        }
        else
        {
            mk82UsbFatalError();
        }
    }
#ifdef FIRMWARE
    else if (event == MK82_USB_EVENT_U2F_PACKET_RECEIVED)
    {
        uint16_t requiredPostFrameProcessingAction;
        uint16_t timerAction;

        sfHidProcessIncomingFrame(&mk82UsbSfHidHandle, mk82UsbU2fIncomingPacketBuffer, mk82UsbU2fOutgoingPacketBuffer,
                                  &requiredPostFrameProcessingAction, &timerAction);

        if (timerAction == SF_HID_TIMER_ACTION_START)
        {
            PIT_StopTimer(PIT, kPIT_Chnl_1);
            mk82UsbU2fTimerExpired = MK82_FALSE;
            PIT_StartTimer(PIT, kPIT_Chnl_1);
        }
        else if (timerAction == SF_HID_TIMER_ACTION_STOP)
        {
            PIT_StopTimer(PIT, kPIT_Chnl_1);
            mk82UsbU2fTimerExpired = MK82_FALSE;
        }
        else if (timerAction == SF_HID_TIMER_ACTION_DO_NOTHING)
        {
        }
        else
        {
            mk82UsbFatalError();
        }

        if (requiredPostFrameProcessingAction == SF_HID_ACTION_SEND_IMMEDIATE_OUTGOING_FRAME)
        {
            mk82UsbU2fSendPacketBlocking();
            USB_DeviceRecvRequest(mk82UsbDeviceHandle, MK82_USB_U2F_INTERRUPT_OUT_ENDPOINT,
                                  mk82UsbU2fIncomingPacketBuffer, MK82_USB_U2F_INTERRUPT_ENDPOINTS_PACKET_SIZE);
        }
        else if (requiredPostFrameProcessingAction == SF_HID_ACTION_PROCESS_RECEIVED_COMMAND)
        {
            uint16_t incomingCommand;
            uint16_t incomingCommandLength;

            sfHidGetIncomingCommandAndDataSize(&mk82UsbSfHidHandle, &incomingCommand, &incomingCommandLength);

            if (incomingCommand == SF_HID_COMMAND_CODE_APDU)
            {
                *data = mk82UsbU2fHidDataBuffer;
                *dataLength = (uint16_t)incomingCommandLength;
                *dataType = MK82_GLOBAL_DATATYPE_U2F_MESSAGE;

                newCommandReceived = MK82_USB_COMMAND_RECEIVED;
            }
            else if (incomingCommand == SF_HID_COMMAND_CODE_PING)
            {
                uint16_t moreFramesAvailable;

                sfHidSetOutgoingDataLength(&mk82UsbSfHidHandle, incomingCommandLength);

                do
                {
                    sfHidProcessOutgoingData(&mk82UsbSfHidHandle, mk82UsbU2fOutgoingPacketBuffer, &moreFramesAvailable);

                    mk82UsbU2fSendPacketBlocking();

                } while (moreFramesAvailable == SF_TRUE);

                USB_DeviceRecvRequest(mk82UsbDeviceHandle, MK82_USB_U2F_INTERRUPT_OUT_ENDPOINT,
                                      mk82UsbU2fIncomingPacketBuffer, MK82_USB_U2F_INTERRUPT_ENDPOINTS_PACKET_SIZE);
            }
            else
            {
                mk82UsbFatalError();
            }
        }
        else if (requiredPostFrameProcessingAction == SF_HID_ACTION_DO_NOTHING)
        {
            USB_DeviceRecvRequest(mk82UsbDeviceHandle, MK82_USB_U2F_INTERRUPT_OUT_ENDPOINT,
                                  mk82UsbU2fIncomingPacketBuffer, MK82_USB_U2F_INTERRUPT_ENDPOINTS_PACKET_SIZE);
        }
        else
        {
            mk82UsbFatalError();
        }
    }
    else if (event == MK82_USB_EVENT_U2F_TIMER_EXPIRED)
    {
        sfHidTimeoutHandler(&mk82UsbSfHidHandle, mk82UsbU2fOutgoingPacketBuffer);

        mk82UsbU2fSendPacketBlocking();
    }
    else if (event == MK82_USB_EVENT_BTC_PACKET_RECEIVED)
    {
        uint16_t requiredPostFrameProcessingAction;

        btcHidProcessIncomingFrame(&mk82UsbBtcHidHandle, mk82UsbBtcIncomingPacketBuffer,
                                   &requiredPostFrameProcessingAction);

        if (requiredPostFrameProcessingAction == BTC_HID_ACTION_PROCESS_RECEIVED_COMMAND)
        {
            uint16_t incomingDataSize;

            btcHidGetIncomingDataSize(&mk82UsbBtcHidHandle, &incomingDataSize);

            *data = mk82UsbBtcHidDataBuffer;
            *dataLength = (uint16_t)incomingDataSize;
            *dataType = MK82_GLOBAL_DATATYPE_BTC_MESSAGE;

            newCommandReceived = MK82_USB_COMMAND_RECEIVED;
        }
        else if (requiredPostFrameProcessingAction == SF_HID_ACTION_DO_NOTHING)
        {
            USB_DeviceRecvRequest(mk82UsbDeviceHandle, MK82_USB_BTC_INTERRUPT_OUT_ENDPOINT,
                                  mk82UsbBtcIncomingPacketBuffer, MK82_USB_BTC_INTERRUPT_ENDPOINTS_PACKET_SIZE);
        }
        else
        {
            mk82UsbFatalError();
        }
    }
#endif /* FIRMWARE */
    else if (event == MK82_USB_EVENT_NOTHING_HAPPENED)
    {
    }
    else
    {
        mk82UsbFatalError();
    }

    return newCommandReceived;
}

void mk82UsbSendResponse(uint32_t dataLength, uint16_t dataType)
{
    if (dataType == MK82_GLOBAL_DATATYPE_CCID_APDU)
    {
        uint8_t *response;
        uint32_t responseLength;

        PIT_StopTimer(PIT, kPIT_Chnl_0);

        ccidCorePrepareResponseAPDU(&mk82UsbCcidHandle, dataLength, &response, &responseLength);

        mk82UsbCcidSendDataBlocking(response, responseLength);

        ccidCoreResponseSent(&mk82UsbCcidHandle);

        USB_DeviceRecvRequest(mk82UsbDeviceHandle, MK82_USB_CCID_BULK_OUT_ENDPOINT, mk82UsbCcidPacketBuffer,
                              MK82_USB_CCID_BULK_ENDPOINTS_PACKET_SIZE);
    }
#ifdef FIRMWARE
    else if (dataType == MK82_GLOBAL_DATATYPE_U2F_MESSAGE)
    {
        uint16_t moreFramesAvailable;

        sfHidSetOutgoingDataLength(&mk82UsbSfHidHandle, dataLength);

        do
        {
            sfHidProcessOutgoingData(&mk82UsbSfHidHandle, mk82UsbU2fOutgoingPacketBuffer, &moreFramesAvailable);

            mk82UsbU2fSendPacketBlocking();

        } while (moreFramesAvailable == SF_TRUE);

        USB_DeviceRecvRequest(mk82UsbDeviceHandle, MK82_USB_U2F_INTERRUPT_OUT_ENDPOINT, mk82UsbU2fIncomingPacketBuffer,
                              MK82_USB_U2F_INTERRUPT_ENDPOINTS_PACKET_SIZE);
    }
    else if (dataType == MK82_GLOBAL_DATATYPE_BTC_MESSAGE)
    {
        uint16_t moreFramesAvailable;

        btcHidSetOutgoingDataLength(&mk82UsbBtcHidHandle, dataLength);

        do
        {
            btcHidProcessOutgoingData(&mk82UsbBtcHidHandle, mk82UsbBtcOutgoingPacketBuffer, &moreFramesAvailable);

            mk82UsbBtcSendPacketBlocking();

        } while (moreFramesAvailable == SF_TRUE);

        USB_DeviceRecvRequest(mk82UsbDeviceHandle, MK82_USB_BTC_INTERRUPT_OUT_ENDPOINT, mk82UsbBtcIncomingPacketBuffer,
                              MK82_USB_BTC_INTERRUPT_ENDPOINTS_PACKET_SIZE);
    }
#endif /* FIRMWARE */
    else
    {
        mk82UsbFatalError();
    }
}

#ifdef FIRMWARE
void mk82UsbTypeStringWithAKeyboard(uint8_t *stringToType, uint32_t stringLength)
{
    MK82_USB_KEYBOARD_INPUT_REPORT *report = (MK82_USB_KEYBOARD_INPUT_REPORT *)mk82UsbKeyboardOutgoingPacketBuffer;
    uint32_t i;

    for (i = 0; i < stringLength; i++)
    {
        if (stringToType[i] < sizeof(mk82UsbKeyboardKeyTable))
        {
            mk82SystemMemSet((uint8_t *)report, 0x00, sizeof(MK82_USB_KEYBOARD_INPUT_REPORT));

            report->keyModifier = mk82UsbKeyboardShiftModifierTable[stringToType[i]];
            report->pressedKeys[0] = mk82UsbKeyboardKeyTable[stringToType[i]];

            mk82UsbKeyboardSendPacketBlocking();

            mk82SystemMemSet((uint8_t *)report, 0x00, sizeof(MK82_USB_KEYBOARD_INPUT_REPORT));

            mk82UsbKeyboardSendPacketBlocking();
        }
    }
}
#endif /* FIRMWARE */
