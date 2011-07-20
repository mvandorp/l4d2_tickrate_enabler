#include "vtableinfo.h"
#include "memutils.h"

VTableInfo::VTableInfo()
{
	m_pBinaryData = NULL;
	m_bMapped = false;
	m_unVTableSize = 0;
	m_pszClassName = NULL;
	m_pszVTableName = NULL;
};

VTableInfo::~VTableInfo()
{
	if (m_bMapped)
	{
		g_ClassInfoMap.Remove(m_pszClassName);
	}

	delete [] m_pszClassName;
	delete [] m_pszVTableName;
};

void VTableInfo::SetBinaryDataPosition(void *pAddr)
{
	this->m_pBinaryData = (ClassInfo_BinaryData *)pAddr;
}

void VTableInfo::AddToMap()
{
	if (m_bMapped)
	{
		return;
	}

	const char *pszClassName = NULL;
	ClassInfo *pClassInfo = NULL;

	// Check if the class has already been mapped
	// if so add this vtable the that class
	for (size_t i = 0; i < g_ClassInfoMap.Count(); i ++)
	{
		pszClassName = g_ClassInfoMap.Key(i);

		if (strcmp(pszClassName, m_pszClassName) == 0)
		{
			pClassInfo = g_ClassInfoMap.Element(i);
			pClassInfo->AddVTable(this);

			m_bMapped = true;
		}
	}

	// Otherwise add a new item to the global map
	if (!m_bMapped)
	{
		pClassInfo = new ClassInfo(m_pszClassName);
		pClassInfo->AddVTable(this);

		g_ClassInfoMap.Insert(m_pszClassName, pClassInfo);
	}

	m_bMapped = true;
}

void VTableInfo::CalculateVTableSize(SegmentInfo_t *pTextSeg)
{
	intptr_t *vtable = GetVTable();
	size_t unSize = 0;
	intptr_t current_vfunc = NULL;

	// Counts how many valid address are in the vtable
	// valid = within the .text segment
	while (true)
	{
		current_vfunc = (intptr_t)vtable[unSize];
		if (current_vfunc >= (intptr_t)pTextSeg->base && current_vfunc < (intptr_t)pTextSeg->end)
		{
			unSize ++;
		}
		else
		{
			break;
		}
	}

	m_unVTableSize = unSize;
}

void VTableInfo::DemangleNames()
{
	if (m_pszClassName != NULL)
	{
		return;
	}

	uint32 unVTableOffset = GetRTTI()->unVTableOffset;
	RttiBaseClassDescriptor_t *pBaseClass = NULL;
	ClassDescriptor_t *pClassDesc = NULL;
	int nVTableNameLength = 0;

	// This is something to do with demangling names of vtables not located at offset 0 :3
	// Seems to scan the inheritence array until it finds the class that related to the vtable
	if (unVTableOffset > 0)
	{
		pClassDesc = GetRTTI()->pClassDescriptor;

		if (pClassDesc != NULL)
		{
			uint32 unNumBaseClasses = pClassDesc->unNumBaseClasses;

			for (size_t i = 0; i < unNumBaseClasses; i++)
			{
				pBaseClass = pClassDesc->pBaseClassArray[i];

				if (pBaseClass->where.nMemberDisplacement == unVTableOffset)
				{
					nVTableNameLength = strlen(pBaseClass->pTypeDescriptor->aszName) + 1;
					break;
				}
				else
				{
					pBaseClass = NULL;
				}
			}
		}
	}

	// Then we demangle - currently commented out so it doesnt demangle
	// instead it will use the mangled name
	int nClassNameLength = strlen(m_pBinaryData->pRTTI->pTypeDescriptor->aszName) + 1;
	nVTableNameLength = nVTableNameLength ? nVTableNameLength : nClassNameLength;

	m_pszClassName = new char[nClassNameLength];

	V_strncpy(m_pszClassName, m_pBinaryData->pRTTI->pTypeDescriptor->aszName, nClassNameLength);
	//MemUtils::DemangleSymbol(m_pszClassName, nClassNameLength);
	
	if (pBaseClass != NULL)
	{
		m_pszVTableName = new char[nVTableNameLength];
		V_strncpy(m_pszVTableName, pBaseClass->pTypeDescriptor->aszName, nVTableNameLength);
		//MemUtils::DemangleSymbol(m_pszVTableName, nVTableNameLength);
	}
	else
	{
		m_pszVTableName = new char[nClassNameLength];
		V_strncpy(m_pszVTableName, m_pszClassName, nClassNameLength);
	}

	return;
}

// Well this isnt complete
bool VTableInfo::IsRTTIValid()
{
	ClassDescriptor_t *pClassDesc = NULL;
}
