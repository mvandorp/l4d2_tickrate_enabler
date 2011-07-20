#ifndef _INCLUDE_VTABLEINFO_H_
#define _INCLUDE_VTABLEINFO_H_
/*
 * Written by Paul Clothier (aka Wazz)
 * :3
 */

#include "structs.h"

/* This class contains information of each vtable */
class VTableInfo
{
public :
	VTableInfo();
	~VTableInfo();

	/* 
	 * Sets the m_pBinaryData to given address. Its position should be
	 * at the address in the binary file where the class' RTTI pointer is given followed by the vtable list
	 */
	void SetBinaryDataPosition(void *pAddr);

	/* Adds the vtable to the global classinfo map */
	void AddToMap();

	// Calculates the number of functions in the vtable at the given segment
	void CalculateVTableSize(SegmentInfo_t *pTextSeg);

	// Tries to demangle the name
	void DemangleNames();

	// Some validation checks to see if the RTTI pointer is valid
	// Not complete DO NOT USE
	bool IsRTTIValid();
	
	// Checks the vtable is already mapped
	inline bool Mapped() const;

	// Returns vtable size
	inline size_t GetVTableSize() const;
	// Returns base class array size. ie inheritence length
	inline size_t GetBaseClassArraySize() const;

	// Returns RTTI pointer
	inline RttiCompleteObjectLocator_t *GetRTTI() const;
	// Returns VTable pointer
	inline intptr_t *GetVTable() const;

	// Returns class name
	inline const char *GetClassName();
	// Return vtable name
	inline const char *GetVTableName();

private :
	ClassInfo_BinaryData *m_pBinaryData;
	
	bool m_bMapped;

	size_t m_unVTableSize;

	char *m_pszClassName;
	char *m_pszVTableName;
};

inline bool VTableInfo::Mapped() const
{
	return m_bMapped;
}

inline size_t VTableInfo::GetVTableSize() const
{
	return m_unVTableSize;
}

inline RttiCompleteObjectLocator_t *VTableInfo::GetRTTI() const
{
	return m_pBinaryData->pRTTI;
}

inline intptr_t *VTableInfo::GetVTable() const
{
	return m_pBinaryData->vtable;
}

inline const char *VTableInfo::GetClassName()
{
	return m_pszClassName;
}

inline const char *VTableInfo::GetVTableName()
{
	return m_pszVTableName;
}

#endif//_INCLUDE_VTABLEINFO_H_
