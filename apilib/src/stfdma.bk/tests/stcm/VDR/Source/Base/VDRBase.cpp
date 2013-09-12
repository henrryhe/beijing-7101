// FILE:			VDR/Source/Base/VDRBase.cpp
//
// AUTHOR:		Ulrich Sigmund
// COPYRIGHT:	(c) 2002 STMicroelectronics.  All Rights Reserved.
// CREATED:		06.12.2002
//
// PURPOSE:		Standard Implementation of the base interface
//
// SCOPE:		INTERNAL Implementation File

#include "VDRBase.h"

VDRBase::VDRBase(void) : refCount(1)
	{
#if _DEBUG
	destructed = false;
#endif
	}

VDRBase::~VDRBase(void)
	{
#if _DEBUG
	assert(destructed);
#endif
	}

void VDRBase::ReleaseDestruct(void)
	{
#if _DEBUG
	assert(!destructed);
	destructed = true;
#endif
	}

//! Standard implementation of QueryInterface
STFResult VDRBase::QueryInterface(VDRIID iid, void *& ifp)
	{
	VDRQI_BEGIN
		VDRQI_IMPLEMENT(VDRIID_VDR_BASE, IVDRBase);
	VDRQI_END_BASE;

	STFRES_RAISE_OK;
	}

//! Standard implementation of AddRef
uint32 VDRBase::AddRef()
	{
	assert(refCount > 0);

	return ++refCount;
	}

//! Standard implementation of Release
uint32 VDRBase::Release()
	{
	uint32 res = --refCount;

	assert(res >= 0);

	if (!res)
		{
		this->ReleaseDestruct();
		delete this;
		}

	return res;
	}
