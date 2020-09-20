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



#include "MemPool32.h"





uint8_t MemPool32<block_count, block_size>::mBlocks[block_count][block_size];
	
uint32_t MemPool32<block_count, block_size>::mMemoryMap;


template<uint8_t block_count, size_t block_size>
template<typename T>
bool MemPool32<block_count, block_size>::newBlock(T *t, blockID_t& ioBlockID)
{

	static_assert(sizeof(T) <= block_size, "Allocation error");
	
	// pool is full
	if(mMemoryMap == (1<<block_count)-1){ return false; }
	
	uint8_t i=0;

	do{
		if(!mMemoryMap&(1<<i)) // slot free
		{
			mMemoryMap |= (1<<i); // take slot

			ioBlockID.ID = i+1; // stores the index

			//t = reinterpret_cast<T *>(&mBlocks[i]);
			t = new(&mBlocks[i]) T();
			
			return true;
		}
	}while(++i<block_count);
	
	// allocation error : should not happen
	return false;
}

template<uint8_t block_count, size_t block_size>
template<typename T>
bool MemPool32<block_count, block_size>::getBlock(T *t, blockID_t inBlockID)
{
	static_assert(sizeof(T) <= block_size, "Allocation error");
	
	if(!isAllocated(inBLockID)){
		false;
	}
	
	t = reinterpret_cast<T>(&mBlocks[inBlockID.ID-1]);
	return true;
}


template<uint8_t block_count, size_t block_size>
bool MemPool32<block_count, block_size>::deleteBlock(blockID_t& ioBlockID)
{

	// pool is empty, nothing to free
	if(!mMemoryMap){ return false; }


	// security check : verify that the memory has been allocated
	// and that ID and map coincide
	if(!isAllocated(ioBlockID)){
		//ioIndex = 0; // should we?
		return false;
	}
		
	// release memory
	mMemoryMap &= ~(1<<(ioBlockID.ID-1));

	// delete index
	ioBlockID = 0;
	
	return true;

}



template<uint8_t block_count, size_t block_size>
size_t MemPool32<block_count, block_size>::getBlockSize(){
	return block_size;
}

template<uint8_t block_count, size_t block_size>
bool MemPool32<block_count, block_size>::isAllocated(blockID_t inBlockID){
	if(inBlockID.ID && inBlockID.ID <= block_count){
		return (mMemoryMap&(1<<(inBlockID.ID-1)));
	}
	return false;
}
	





