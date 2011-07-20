#include "classinfo.h"

ClassInfo::ClassInfo(const char *pszClassName)
{
	size_t length = strlen(pszClassName) + 1;
	m_pszClassName = new char[length];
	V_strncpy(m_pszClassName, pszClassName, length);

	m_nNumVTables = 1;

	SetDefLessFunc(g_VTableInfoMap);
}

ClassInfo::~ClassInfo()
{
	delete [] m_pszClassName;
}

void ClassInfo::AddVTable(VTableInfo *pVTableInfo)
{
	// if the vtable doesnt already exist in the map, add it
	unsigned short idx = g_VTableInfoMap.Find(pVTableInfo->GetVTableName());

	if (idx == g_VTableInfoMap.InvalidIndex())
	{
		g_VTableInfoMap.Insert(pVTableInfo->GetVTableName(), pVTableInfo);
		m_nNumVTables ++;
	}
}