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

#pragma once



//#include "IMemPoolBase.h"

#include <limits>

// Fixed size dynamic memory allocation
// features :
//  - Allows to allocate and release a buffer of sizeof(elem_t) bytes
//  - Forbids task deletion if a buffer is allocated to avoid memory leakage
//  - The number of buffer per task types has a maximum value of 32
//	- If auto_release is "true", the module will automatically release allocated memory on deletion 




template<uint8_t block_count, size_t block_size>
struct MemPool32
{

	struct BlockID{
		BlockID() : ID(0) {}
		uint8_t ID;
	};

	using blockID_t = BlockID;

	static_assert(block_count <= 32, "size of pool must not exceed 32");
	
	template<typename T>
	static bool newBlock(T *t, BlockID& ioBlockID);

	template<typename T>
	static bool *getBlock(T *t, BlockID inBlockID);
	
	static bool deleteBlock(BlockID& ioBlockID);

	static size_t getBlockSize();

	static bool isAllocated(BlockID inBlockID);

private:

	static uint8_t mBlocks[block_count][block_size];
	
	static uint32_t mMemoryMap;

};



