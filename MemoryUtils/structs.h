#ifndef _INCLUDE_MEMUTILSSTRUCTS_H_
#define _INCLUDE_MEMUTILSSTRUCTS_H_
#include <unistd.h>
#include "blah_types.h"
/*
 * Written by Paul Clothier (aka Wazz)
 * :3
 */

/* 
 *These a variety of structs, some are 'official' RTTI structs
 * others are just for easy alignement of data
 * You shouldnt need to edit this ... famous last words
 */

struct SegmentInfo_t
{
	SegmentInfo_t(void *base, void *end)
	{
		this->base = base;
		this->end = end;
	};

	~SegmentInfo_t() {};

	void *base;
	void *end;
};

//#pragma warning(push)
//#pragma warning(disable:4200)
struct TypeDescriptor_t
{
	void *pVTable;	// nope, this is just a pointer to type_info :c
	uint32 unSpare;
	char aszName[];
};

struct PMD_t
{
	int nMemberDisplacement;
	int nVBTableDisplacement;
	int nVBTableDisplacement_Inside;
};

struct RttiBaseClassDescriptor_t
{
	TypeDescriptor_t *pTypeDescriptor;
	uint32 unNumContainedBases;
	PMD_t where;
	uint32 unAttributes;
};

struct ClassDescriptor_t
{
	uint32 unSignature;
	uint32 unAttributes;
	uint32 unNumBaseClasses;
	RttiBaseClassDescriptor_t **pBaseClassArray;
};

struct RttiCompleteObjectLocator_t
{
	uint32 unSignature;
	uint32 unVTableOffset;
	uint32 unCTorOffset;
	TypeDescriptor_t *pTypeDescriptor;
	ClassDescriptor_t *pClassDescriptor;
};

struct ClassInfo_BinaryData
{
	RttiCompleteObjectLocator_t *pRTTI;	
	intptr_t vtable[];
};
//#pragma warning(pop)

struct DynLibInfo
{
	void *pBaseAddress;
	size_t nMemorySize;
};

#endif//_INCLUDE_MEMUTILSSTRUCTS_H_
