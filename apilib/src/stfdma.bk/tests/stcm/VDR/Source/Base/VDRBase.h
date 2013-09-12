// FILE:			VDR/Source/Base/VDRBase.h
//
// AUTHOR:		Ulrich Sigmund
// COPYRIGHT:	(c) 2002 STMicroelectronics.  All Rights Reserved.
// CREATED:		06.12.2002
//
// PURPOSE:		Standard Implementation of the base interface
//
// SCOPE:		INTERNAL Header File

#ifndef VDRBASE_H
#define VDRBASE_H

#include "VDR/Interface/Base/IVDRBase.h"
#include "STF/Interface/STFSynchronisation.h"

//! Standard implementation of base interface
class VDRBase : virtual public IVDRBase
	{
	protected:
		//! Reference counter to control lifetime of the object exposing the interface
		STFInterlockedInt	refCount;
#if _DEBUG
		bool	destructed;
#endif
		virtual void ReleaseDestruct(void);
	public:
		VDRBase(void);
		virtual ~VDRBase(void);

		virtual STFResult QueryInterface(VDRIID iid, void *& ifp);

		virtual uint32 AddRef();
		virtual uint32 Release();
	};

///////////////////////////////////////////////////////////////////////////////
// Macros to simplify the implementation of QueryInterface
///////////////////////////////////////////////////////////////////////////////

//! Macro to begin checking for interfaces
#define VDRQI_BEGIN	\
	switch (iid) {

//! Macro to return pointer to a specific interface if it is supported
#define VDRQI_IMPLEMENT(qiid, qif)	\
	case qiid: ifp = (qif *)this; AddRef(); break

//! Macro to end checking for interfaces without calling a parent class
#define VDRQI_END_BASE	\
	default: ifp = NULL; STFRES_RAISE(STFRES_INTERFACE_NOT_FOUND); }

//! Macro to end checking for interfaces on the current level, and continuing the query in the parent class
#define VDRQI_END(super)	\
	default: STFRES_REASSERT(super::QueryInterface(iid, ifp)); }

#endif
