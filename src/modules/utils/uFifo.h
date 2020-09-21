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



template<typename T, size_t Size>
struct uFifo
{

	uFifo() : mOldV(0), mNewV(0), mIsEmpty(true), mIsFull(false)
	{}

	bool push(T data){
		if(mIsFull){
			return false;
		}
		size_t n = (mNewV+1)%Size;
		if(n == mOldV){
			mIsFull = true;
		}
		mElems[mNewV] = data;
		mNewV = n;
		mIsEmpty = false;
		return true;
	}

	T pop(){
		if(mIsEmpty){
			return T();
		}
		size_t n = mOldV;
		mOldV = (mOldV+1)%Size;
		if(mOldV == mNewV){
			mIsEmpty = true;
		}
		mIsFull = false;
		return mElems[n];
	}

	bool isEmpty(){
		return mIsEmpty;
	}

	bool isFull(){
		return mIsFull;
	}
	
	void flush(){
		mOldV = 0;
		mNewV = 0;
		mIsEmpty = true;
		mIsFull = false;
	}

private :

	size_t mOldV, mNewV;
	bool mIsEmpty:1;
	bool mIsFull:1;
	T mElems[Size];

};



