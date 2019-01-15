/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __MK82_TOUCH_H__
#define __MK82_TOUCH_H__

#ifdef __cplusplus
extern "C"
{
#endif

    typedef void (*MK82_TOUCH_CALLBACK)(void);

    void mk82TouchInit(void);
    void mk82TouchEnable(void);
    void mk82TouchDisable(void);
    void mk82TouchTask(void);
    void mk82TouchRegisterButton1PressedCallback(MK82_TOUCH_CALLBACK callback);
    void mk82TouchRegisterButton2PressedCallback(MK82_TOUCH_CALLBACK callback);
    void mk82TouchRegisterBothButtonsPressedCallback(MK82_TOUCH_CALLBACK callback);

    void mk82TouchDeregisterButton2PressedCallback(void);

#ifdef __cplusplus
}
#endif

#endif /* __MK82_TOUCH_H__ */
