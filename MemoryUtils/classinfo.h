#ifndef _INCLUDE_MEMUTILSCLASSINFO_H_
#define _INCLUDE_MEMUTILSCLASSINFO_H_

/*
 * Written by Paul Clothier (aka Wazz)
 * :3
 */

#include "vtableinfo.h"
#include <utlmap.h>

/* A class can have more than one vtable. This class is used to assign vtables to classes */
class ClassInfo
{
public :
	ClassInfo(const char *pszClassName);
	~ClassInfo();

	// Adds a vtable to the map
	void AddVTable(VTableInfo *pVTableInfo);

	// Returns class name
	inline const char *GetClassName() const;
	// Returns number of vtable this class has
	inline size_t GetNumVTables() const;

	// Our map of vtables
	CUtlMap<const char *, const VTableInfo *> g_VTableInfoMap;

private :
	char *m_pszClassName;
	size_t m_nNumVTables;
};

inline const char *ClassInfo::GetClassName() const
{
	return m_pszClassName;
}

inline size_t ClassInfo::GetNumVTables() const
{
	return m_nNumVTables;
}

#endif//_INCLUDE_MEMUTILSCLASSINFO_H_