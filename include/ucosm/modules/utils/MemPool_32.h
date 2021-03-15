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



template<uint8_t block_count, size_t block_size>
struct MemPool_32{

	static_assert(block_count < 32, "block count can't exceed 32");

	MemPool_32():mMap(0)
	{}

	template<typename T, typename... args_t>
	bool allocate(T** p, args_t... args){

		static_assert(sizeof(T) <= block_size, "Object is bigger than block size");

		if(*p != nullptr){
			return false;
		}

		// check space left
		if(mMap == (1<<block_count)-1){
			return false;
		}

		uint8_t i=0;

		do{
			// check if slot is free
			if(!(mMap&(1<<i))){

				// take slot
				mMap |= (1<<i);

				// allocate
				*p = new(mBlocks[i]) T(args...);

				return (*p != nullptr);
			}

		}while(++i < block_count);

		// couldn't allocate
		return false;
	}

	template<typename T>
	bool release(T** p){

		if(*p == nullptr){
			return false;
		}

		uint8_t i=0;

		void *ip = reinterpret_cast<void*>(*p);

		do{

			// search for the matching address
			if(mBlocks[i] == ip){

				// explicitely call destructor
				(*p)->T::~T();

				// release slot
				mMap &= ~(1<<i);

				*p = nullptr;

				return true;
			}

		}while(++i < block_count);

		return false;
	}

	uint8_t getSizeLeft(){
		uint8_t sizeLeft = 0;
		for(uint8_t i=0 ; i<block_count ; i++){
			if(!(mMap&(1<<i))){
				sizeLeft++;
			}
		}
		return sizeLeft;
	}

private:

	uint32_t mMap;
	uint8_t mBlocks[block_count][block_size];

};



