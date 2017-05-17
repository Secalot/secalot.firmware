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
#include "../source/system/nt_system_prv.h"
#include "../source/system/nt_system_frmstr_prv.h"
#include "../source/controls/nt_controls_prv.h"
#include "nt.h"

int32_t nt_init(const struct nt_system *system, uint8_t *pool, const uint32_t size)
{
  NT_ASSERT(system != NULL);
  
  int32_t result = NT_SUCCESS;

  if (_nt_mem_init(pool, size) < NT_SUCCESS)
  {
    return NT_FAILURE;
  }  
  if (_nt_freemaster_init() < NT_SUCCESS)
  {
    return NT_FAILURE;
  }  
  if ((result = _nt_system_init(system)) < NT_SUCCESS)
  {
    return result;
  }
  if ((result = _nt_system_module_function(NT_SYSTEM_MODULE_INIT)) < NT_SUCCESS)
  {
    return result;
  }
  if ((result = _nt_system_control_function(NT_SYSTEM_CONTROL_INIT)) < NT_SUCCESS)
  {
    return result;
  }
  return result;
}

int32_t nt_trigger(void)
{
    int32_t result = NT_SUCCESS;

    _nt_system_increment_time_counter();
    if (_nt_system_module_function(NT_SYSTEM_MODULE_TRIGGER) < NT_SUCCESS) {
        result = NT_FAILURE;
    }
    if (_nt_system_control_function(NT_SYSTEM_CONTROL_OVERRUN) < NT_SUCCESS) {
        result = NT_FAILURE;
    }
    if (result == NT_FAILURE) {
        /* triggering is faster than measurement/processing */
        _nt_system_invoke_callback(NT_SYSTEM_EVENT_OVERRUN);
    }

    return result;
}

int32_t nt_task(void)
{
    int32_t ret;
  
		ret = _nt_system_module_function(NT_SYSTEM_MODULE_PROCESS);
    if (ret < NT_SUCCESS) {
        return ret;
    }
		ret = _nt_system_control_function(NT_SYSTEM_CONTROL_PROCESS);
    if (ret < NT_SUCCESS) {
        return ret;
    }
    return NT_SUCCESS;
}

void nt_error(char *file, uint32_t line)
{
  /* User's error handling */
  #if (NT_DEBUG != 0) 
  if(_nt_system_get()->error_callback)
  {
    _nt_system_get()->error_callback(file, line);
  }
  #endif
  while (1);
}
