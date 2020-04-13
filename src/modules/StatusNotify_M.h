#pragma once

#include "Status_M.h"


template<typename Callee_t>
struct StatusNotify_M : private Status_M // 1 byte
{
 
	enum eNotifyStatus
	{
		/*
		eRunning		= 0b00000001,
		eStarted		= 0b00000010
		eSuspended		= 0b00000100,
		eLocked			= 0b00001000,
		eStatusMask		= 0b00001111
		eNotifyStarted		= 0b00001000,
		eNotifySuspended	= 0b00100000,
		eNotifyLocked		= 0b01000000,
		eNotifyDeleted		= 0b10000000,
		eNotifyMask		= ~Status_M::eStatusMask
	*/
		eNotifyStarted		= Status_M::eStarted<<3,	// 0b00010000
		eNotifySuspended	= Status_M::eSuspended<<3,	// 0b00100000
		eNotifyLocked		= Status_M::eLocked<<3,	// 0b01000000
		eNotifyDeleted		= 0b10000000,				// 0b10000000
		eNotifyMask			= ~Status_M::eStatusMask
    };
	
	bool isStatus(uint8_t s){ return ((mStatus&s) == s); }
		
	void setStatus(uint8_t s, bool state)
	{
		if(isStatus(Status_M::eLocked) && s!=Status_M::eLocked){ return;}

		uint8_t prevNotifStatus = (mStatus&Status_M::eStatusMask)<<3;
		
		state ? mStatus|=s : mStatus&=~s;

		// is the new status notifiable
		uint8_t newNotifStatus = ((s&Status_M::eStatusMask)<<3)&(mStatus&eNotifyMask);
		
		if(newNotifStatus)// status notifiable: notify callee
		{
			Callee_t::notifyStatusChange(prevNotifStatus, newNotifStatus);
		}
	}
	
	void setStatus(Status_M::eStatus s, bool state)
	{
		setStatus(static_cast<uint8_t>(s), state);
	}
	
	void init()	{ mStatus = 0; }

	bool isExeReady() const { return !(mStatus&Status_M::eSuspended) ;}

	bool isDelReady() const { return !(mStatus&Status_M::eLocked);}

	void makePreExe()
	{  
		setStatus(Status_M::eRunning, true); 
	}

	void makePostExe()
	{
		setStatus(Status_M::eRunning, false); 
		setStatus(Status_M::eStarted, true);
	}

	void makePreDel()
	{
		if( isStatus(eNotifyDeleted) )
		{
			Callee_t::notifyStatusChange(mStatus, eNotifyDeleted);
		}
		mStatus = 0;
	}
	
};


