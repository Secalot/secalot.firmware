/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __MK82_BUTTON_H__
#define __MK82_BUTTON_H__

#ifdef __cplusplus
extern "C"
{
#endif

    typedef void (*MK82_BUTTON_ACTION_CALLBACK)(void);

    void mk82ButtonInit(void);

    void mk82ButtonRegisterButtonClickedCallback(MK82_BUTTON_ACTION_CALLBACK buttonClickedCallback);
    void mk82ButtonRegisterButtonDoubleClickedCallback(MK82_BUTTON_ACTION_CALLBACK buttonDoubleClickedCallback);
    void mk82ButtonRegisterButtonLongPressedCallback(MK82_BUTTON_ACTION_CALLBACK buttonLongPressedCallback);

    void mk82ButtonDeregisterButtonDoubleClickedCallback(void);

#ifdef __cplusplus
}
#endif

#endif /* __MK82_BUTTON_H__ */
