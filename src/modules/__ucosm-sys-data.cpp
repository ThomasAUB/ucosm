/*
 * Copyright (C) 2020 Thomas AUBERT <aubert.thms@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name ``Thomas AUBERT'' nor the name of any other
 *    contributor may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 
 * uCosmDev IS PROVIDED BY Thomas AUBERT ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Thomas AUBERT OR ANY OTHER CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ucosm-sys-data.h"

uint8_t SysKernelData::sCnt = 0;

//

#include "CPU_Usage_M.h"

tick_t CPU_Usage_M::sMaxTickValue = std::numeric_limits<tick_t>::max();
uint8_t CPU_Usage_M::sTimerOverFlow = 0;

//

#include "LinkedList_M.h"

template<int listIndex>
ListItem *LinkedList_M<listIndex>::sTopHandle = nullptr;

//

#include "MemPool32_M.h"

template <typename elem_t, uint16_t elem_count, bool auto_release>
elem_t MemPool32_M<elem_t, elem_count, auto_release>::mElems[elem_count];

template <typename elem_t, uint16_t elem_count, bool auto_release>
uint32_t MemPool32_M<elem_t, elem_count, auto_release>::mMemoryMap = 0;

//

#include "Stack_Usage_M.h"

template<uint16_t max_stack_usage>
uint32_t *Stack_Usage_M<max_stack_usage>::sSp;





