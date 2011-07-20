#include "memutils.h"
#include "vtableinfo.h"
#include "classinfo.h"

// Our global map of the class info
CUtlMap<const char *, ClassInfo *> g_ClassInfoMap;

bool MemUtils::InitializeClassMap(char *error, size_t maxlength)
{
	SetDefLessFunc(g_ClassInfoMap);

	MEMORY_BASIC_INFORMATION mbi = {0};

	if (!VirtualQuery((void *)g_SMAPI->GetServerFactory(false), &mbi, sizeof(MEMORY_BASIC_INFORMATION)))
	{
		g_pSM->Format(error, maxlength, "Error querying server memory.");
		return false;
	}

	IMAGE_DOS_HEADER *dos;
	IMAGE_NT_HEADERS *pe;
	IMAGE_OPTIONAL_HEADER *opt;
	
	intptr_t dll_base = (intptr_t)mbi.AllocationBase;
	
	dos = reinterpret_cast<IMAGE_DOS_HEADER *>(dll_base);
	pe = reinterpret_cast<IMAGE_NT_HEADERS *>(dll_base + dos->e_lfanew);
	opt = &pe->OptionalHeader;
	
	// Finds the boundaries of the .rdata segment of the binary image
	SegmentInfo_t RDataSeg((intptr_t *)(opt->BaseOfData + dll_base), (intptr_t *)(opt->BaseOfData + dll_base + opt->SizeOfInitializedData - 4));
	// Finds the boundaries of the text segment
	SegmentInfo_t TextSeg((intptr_t *)(opt->BaseOfCode + dll_base), (intptr_t *)(opt->BaseOfData + dll_base));

	intptr_t *rdata_ptr = (intptr_t *)RDataSeg.base;
	RttiCompleteObjectLocator_t *rtti = NULL;
	VTableInfo *pVTableInfo = NULL;
	intptr_t *vtable = NULL;
	
	//FILE *fp = fopen("classes.txt", "wb");
	// Scan through the rdata segment
	while (rdata_ptr < RDataSeg.end)
	{
		// Get address at this address
		pVTableInfo = new VTableInfo();
		pVTableInfo->SetBinaryDataPosition(rdata_ptr);
		vtable = pVTableInfo->GetVTable();

		// If the reference is in the code segment, aka valid
		if ((void *)vtable[0] >= TextSeg.base && (void *)vtable[0] < TextSeg.end)
		{
			rtti = pVTableInfo->GetRTTI();

			// If this is a valid vtable, there should be a forwards reference to rtti info
			// RTTI signature should always be 0.
			if ((void *)rtti > rdata_ptr && rtti < RDataSeg.end && rtti->unSignature == 0)
			{
				pVTableInfo->CalculateVTableSize(&TextSeg);
				pVTableInfo->DemangleNames();
				pVTableInfo->AddToMap();

				//fprintf(fp, "%s\n", pVTableInfo->GetVTableName());
			}
		}

		rdata_ptr ++;
	}

	//fclose(fp);

	META_CONPRINTF("Class Count: %i\n", g_ClassInfoMap.Count());

	return true;
}

bool MemUtils::CalculateOffsetFromAddress(const char *pszClassName, const void *pAddr, int *offset, const char *pszVTableName, int *vtable_offset)
{
	// Check the classname exists in our map
	unsigned short idx = g_ClassInfoMap.Find(pszClassName);
	if (idx == g_ClassInfoMap.InvalidIndex())
	{
		return false;
	}
	const ClassInfo *pClassInfo = g_ClassInfoMap.Element(idx);

	// Check if vtable name is given otherwise assume default
	if (pszVTableName == NULL)
	{
		pszVTableName = pszClassName;
	}

	// Find the vtable name in the classinfo map
	idx = pClassInfo->g_VTableInfoMap.Find(pszVTableName);
	if (idx == pClassInfo->g_VTableInfoMap.InvalidIndex())
	{
		return false;
	}
	const VTableInfo *pVTableInfo = pClassInfo->g_VTableInfoMap.Element(idx);

	intptr_t *vtable = pVTableInfo->GetVTable();
	intptr_t vfunc_index = 0;
	intptr_t vtable_len = pVTableInfo->GetVTableSize();
	intptr_t current_vfunc = NULL;

	// Scan through vtable until the address matches
	while (vfunc_index < vtable_len)
	{
		current_vfunc = vtable[vfunc_index];

		if ((void *)current_vfunc == pAddr)
		{
			*offset = vfunc_index;

			if (vtable_offset != NULL)
			{
				*vtable_offset = pVTableInfo->GetRTTI()->unVTableOffset;
			}

			return true;
		}

		vfunc_index ++;
	}

	return false;
}

// For name demangling, returns the length of a 'readable' section of the name
size_t GetValidStringLength(const char *pszString)
{
	size_t n = 0;

	while ((pszString[n] >= 'a' && pszString[n] <= 'z')
		|| (pszString[n] >= 'A' && pszString[n] <= 'Z')
		|| (pszString[n] >= '0' && pszString[n] <= '9')
		|| (pszString[n] == '_'))
	{
		n ++;
	}

	return n;
}

// A very weak method allowing us to ignore most of http://en.wikipedia.org/wiki/Microsoft_Visual_C%2B%2B_Name_Mangling
// This is completely fucked and broken. It will return names of basic classes like CTFPlayer however namespaces and templates are not
// demangled properly. Im not going to document this as it shouldnt be used - its a horrible bit of code that I wrote while pissed
// and not even I can decypher it now
void MemUtils::DemangleSymbol(char *pszSymbol, size_t unSize)
{

	bool bNameFound = false, bHasNameSpace = false, bHasTemplate = false;
	size_t unNamePos = 0, unNameSpacePos = 0;//, unTemplateNamePos;
	size_t unNameLen = 0, unNameSpaceLen = 0;//, unTemplateNameLen;
	size_t unCurrentPos = 0, unValidStringLen = 0;
/*
	while (unCurrentPos < unSize)
	{
		unValidStringLen = GetValidStringLength(&pszSymbol[unCurrentPos]);

		if (!bNameFound)
		{
			if (unValidStringLen)
			{
				bNameFound = true;
				unNamePos = unCurrentPos;
				unNameLen = unValidStringLen;
			}
		}
		else
		{
			if (!bHasNameSpace &&
				unValidStringLen &&
				pszSymbol[unNamePos + unValidStringLen] == '@' &&
				unCurrentPos - (unNamePos + unValidStringLen) == 2)
			{
				bHasNameSpace = true;
				unNameSpacePos = unCurrentPos;
				unNameSpaceLen = unValidStringLen;
			}

			if (!bHasTemplate &&
				pszSymbol[unNamePos - 1] == '$')
			{
				bHasTemplate = true;
				unTemplateNamePos = unCurrentPos;
				unTemplateNameLen = unValidStringLen;
			}
		}

		unCurrentPos ++;
	}

	size_t unTotalSize = unNameLen + unNameSpaceLen + unTemplateNameLen + (bHasTemplate ? 3 : 1);

	if (unTotalSize - 1 > unSize)
	{
		unTotalSize = unSize;
	}

	unCurrentPos = 0;

	if (bHasNameSpace)
	{
		V_strncpy(&pszSymbol[unCurrentPos], &pszSymbol[unNameSpacePos], unNameSpaceLen + 1);
		unCurrentPos += unNameSpaceLen;
	}
	
	V_strncpy(&pszSymbol[unCurrentPos], &pszSymbol[unNamePos], unNameLen + 1);
	unCurrentPos += unNameLen;

	if (bHasTemplate)
	{
		pszSymbol[unCurrentPos] = '<';
		unCurrentPos++;
		
		V_strncpy(&pszSymbol[unCurrentPos], &pszSymbol[unNamePos], unNameLen + 1);
		unCurrentPos += unNameLen;

		pszSymbol[unCurrentPos] = '<';
		unCurrentPos++;

		pszSymbol[unCurrentPos] = '\0';
	}
*/
	size_t s = 0, e = 0, t = 0, w = 0;

	while (s < unSize && pszSymbol[s] != '\0')
	{
		if ((pszSymbol[s] >= 'C' &&	// see http://en.wikipedia.org/wiki/Microsoft_Visual_C%2B%2B_Name_Mangling#Data_Type
			pszSymbol[s] <= 'O') ||
			(pszSymbol[s] >= 'T' &&
			pszSymbol[s] <= 'Z'))
		{
			e = 0;

			while (!e)
			{
				s ++;
				t = s + e;

				while ((pszSymbol[t] >= 'a' && pszSymbol[t] <= 'z')
					|| (pszSymbol[t] >= 'A' && pszSymbol[t] <= 'Z')
					|| (pszSymbol[t] >= '0' && pszSymbol[t] <= '9')
					|| (pszSymbol[t] == '_'))
				{
					t ++;
				}

				bNameFound = true;

				if (pszSymbol[t] == '?')	// Type modifier, Template parameter, Name starts after this
				{
					t ++;
					e = 0;

					bNameFound = false;
				}

				if (pszSymbol[t] == '$')	// Type modifier, Template parameter, Name starts after this
				{
					t ++;
					e = 0;

					bNameFound = false;
				}

				if (bNameFound)
				{
					e = t - s + 1;

					bHasTemplate = false;
				}
			}

			if (w)
			{
				bHasTemplate = true;
				pszSymbol[w] = '<';
				w ++;
			}

			if (w + e >= unSize)
			{
				e = unSize - w - 1;
			}

			V_strncpy(&pszSymbol[w], &pszSymbol[s], e);

			s += e;
			w += e;

			if (bHasTemplate)
			{
				pszSymbol[w] = '>';
				w ++;
			}
		}

		s ++;
	}

	return;
}

void MemUtils::GetPointerClassName(void *pAddr, char *pszBuffer, size_t unSize)
{
	pszBuffer[0] = '\0';

	// Similar start to MemUtils::InitializeClassMap
	MEMORY_BASIC_INFORMATION mbi = {0};

	if (!VirtualQuery((void *)g_SMAPI->GetServerFactory(false), &mbi, sizeof(MEMORY_BASIC_INFORMATION)))
	{
		return;
	}

	IMAGE_DOS_HEADER *dos;
	IMAGE_NT_HEADERS *pe;
	IMAGE_OPTIONAL_HEADER *opt;
	
	intptr_t dll_base = (intptr_t)mbi.AllocationBase;
	
	dos = reinterpret_cast<IMAGE_DOS_HEADER *>(dll_base);
	pe = reinterpret_cast<IMAGE_NT_HEADERS *>(dll_base + dos->e_lfanew);
	opt = &pe->OptionalHeader;
	
	SegmentInfo_t RDataSeg((intptr_t *)(opt->BaseOfData + dll_base), (intptr_t *)(opt->BaseOfData + dll_base + opt->SizeOfInitializedData - 4));
	SegmentInfo_t TextSeg((intptr_t *)(opt->BaseOfCode + dll_base), (intptr_t *)(opt->BaseOfData + dll_base));
	RttiCompleteObjectLocator_t *rtti = NULL;
	VTableInfo *pVTableInfo = NULL;

	// It is incorrectly(?) assumed here that if the ptr is a class, its first entry will be a vtable
	intptr_t *vtable = *(intptr_t **)pAddr;

	// if the vtable is potentially valid
	if (vtable >= RDataSeg.base && vtable < RDataSeg.end)
	{
		vtable = (intptr_t *)((intptr_t)vtable - 4);

		pVTableInfo = new VTableInfo();
		pVTableInfo->SetBinaryDataPosition(vtable);
		rtti = pVTableInfo->GetRTTI();
		
		// If this is a valid vtable, there should be a forwards reference to rtti info
		// RTTI signature should always be 0.
		if ((void *)rtti > vtable && rtti < RDataSeg.end && rtti->unSignature == 0)
		{
			pVTableInfo->DemangleNames();
			g_pSM->Format(pszBuffer, unSize, "%s", pVTableInfo->GetClassName());
		}
	}

	delete pVTableInfo;
}

/* This is copy pasta at the moment
 * From: pRED's original CDetour
 *       AlliedModders SourceMod core 
 *		WHICH MEANS ITS UNDER GPL YOU SLAGS
 */

void MemUtils::ProtectMemory(void *pAddr, int nLength, int nProt)
{
#if defined _LINUX
	void *pAddr2 = (void *)ALIGN(pAddr);
	mprotect(pAddr2, sysconf(_SC_PAGESIZE), nProt);
#elif defined _WIN32
	DWORD old_prot;
	VirtualProtect(pAddr, nLength, nProt, &old_prot);
#endif

	return;
}

/* This is copy pasta at the moment
 * From: pRED's original CDetour
 *       AlliedModders SourceMod core 
 *		WHICH MEANS ITS UNDER GPL YOU SLAGS
 */
void MemUtils::SetMemPatchable(void *pAddr, size_t nSize)
{
	ProtectMemory(pAddr, (int)nSize, PAGE_EXECUTE_READWRITE);

	return;
}

// NOT COMPLETE
size_t MemUtils::CountBytesUntilJump(BYTE *pFunc, int *pnOffset)
{
	size_t bytecount = 0;

	while (*pFunc != 0xCC)
	{
		// prefixes F0h, F2h, F3h, 66h, 67h, D8h-DFh, 2Eh, 36h, 3Eh, 26h, 64h and 65h
		int operandSize = 4;
		int FPU = 0;
		int twoByte = 0;
		unsigned char opcode = 0x90;
		unsigned char modRM = 0xFF;
		while (*pFunc == 0xF0 ||
			  *pFunc == 0xF2 ||
			  *pFunc == 0xF3 ||
			 (*pFunc & 0xFC) == 0x64 ||
			 (*pFunc & 0xF8) == 0xD8 ||
			 (*pFunc & 0x7E) == 0x62)
		{
			if (*pFunc == 0x66)
			{
				operandSize = 2;
			}
			else if ((*pFunc & 0xF8) == 0xD8)
			{
				FPU = *pFunc;
				pFunc++;
				bytecount++;
				break;
			}

			pFunc++;
			bytecount++;
		}

		// two-byte opcode byte
		if (*pFunc == 0x0F)
		{
			twoByte = 1;
			pFunc++;
			bytecount++;
		}

		// opcode byte
		opcode = *pFunc++;
		bytecount++;

		// mod R/M byte
		modRM = 0xFF;
		if (FPU)
		{
			if ((opcode & 0xC0) != 0xC0)
			{
				modRM = opcode;
			}
		}
		else if (!twoByte)
		{
			if ((opcode & 0xC4) == 0x00 ||
			   (opcode & 0xF4) == 0x60 && ((opcode & 0x0A) == 0x02 || (opcode & 0x09) == 0x09) ||
			   (opcode & 0xF0) == 0x80 ||
			   (opcode & 0xF8) == 0xC0 && (opcode & 0x0E) != 0x02 ||
			   (opcode & 0xFC) == 0xD0 ||
			   (opcode & 0xF6) == 0xF6)
			{
				modRM = *pFunc++;
				bytecount++;
			}
		}
		else
		{
			if ((opcode & 0xF0) == 0x00 && (opcode & 0x0F) >= 0x04 && (opcode & 0x0D) != 0x0D ||
			   (opcode & 0xF0) == 0x30 ||
			   opcode == 0x77 ||
			   (opcode & 0xF0) == 0x80 ||
			   (opcode & 0xF0) == 0xA0 && (opcode & 0x07) <= 0x02 ||
			   (opcode & 0xF8) == 0xC8)
			{
				// No mod R/M byte
			}
			else
			{
				modRM = *pFunc++;
				bytecount++;
			}
		}

		// SIB
		if ((modRM & 0x07) == 0x04 &&
		   (modRM & 0xC0) != 0xC0)
		{
			pFunc++;
			bytecount++;
		}

		// mod R/M displacement

		// Dword displacement, no base
		if ((modRM & 0xC5) == 0x05)
		{
			pFunc += 4;
			bytecount += 4;
		}
	
		// Byte displacement
		if ((modRM & 0xC0) == 0x40)
		{
			pFunc++;
			bytecount++;
		}

		// Dword displacement
		if ((modRM & 0xC0) == 0x80)
		{
			pFunc += 4;
			bytecount += 4;
		}

		// immediate
		if (FPU)
		{
			// Can't have immediate operand
		}
		else if (!twoByte)
		{
			if ((opcode & 0xC7) == 0x04 ||
			   (opcode & 0xFE) == 0x6A ||   // PUSH/POP/IMUL
			   opcode == 0x80 ||
			   opcode == 0x83 ||
			   (opcode & 0xFD) == 0xA0 ||   // MOV
			   opcode == 0xA8 ||			// TEST
			   (opcode & 0xF8) == 0xB0 ||   // MOV
			   (opcode & 0xFE) == 0xC0 ||   // RCL
			   opcode == 0xC6 ||			// MOV
			   opcode == 0xCD ||			// INT
			   (opcode & 0xFE) == 0xD4 ||   // AAD/AAM
			   (opcode & 0xF8) == 0xE0 ||   // LOOP/JCXZ
			   opcode == 0xEB ||
			   opcode == 0xF6 && (modRM & 0x30) == 0x00)   // TEST
			{
				pFunc++;
				bytecount++;
			}
			else if ((opcode & 0xF0) == 0x70)   // Jcc
			{
				memcpy(pnOffset, pFunc, 1);
				*pnOffset += 2;
				bytecount -= 1;
				
				break;
			}
			else if((opcode & 0xF7) == 0xC2) // RET
			{
				*pnOffset = 0;
				bytecount -= 1;

				break;
			}
			else if ((opcode & 0xFC) == 0x80 ||
					(opcode & 0xC7) == 0x05 ||
					(opcode & 0xF8) == 0xB8 ||
					(opcode & 0xFE) == 0x68 ||
					(opcode & 0xFE) == 0xE8 ||  // CALL/Jcc
					(opcode & 0xFC) == 0xA0 ||
					(opcode & 0xEE) == 0xA8 ||
					opcode == 0xC7 ||
					opcode == 0xF7 && (modRM & 0x30) == 0x00)
			{
				pFunc += operandSize;
				bytecount += operandSize;
			}
		}
		else
		{
			if (opcode == 0xBA ||			// BT
			   opcode == 0x0F ||			// 3DNow!
			   (opcode & 0xFC) == 0x70 ||   // PSLLW
			   (opcode & 0xF7) == 0xA4 ||   // SHLD
			   opcode == 0xC2 ||
			   opcode == 0xC4 ||
			   opcode == 0xC5 ||
			   opcode == 0xC6)
			{
				pFunc++;
			}
			else if ((opcode & 0xF0) == 0x80) // Jcc -i
			{
				memcpy(pnOffset, pFunc, 4);
				*pnOffset += 6;
				bytecount -= 2;

				break;
			}
		}
	}

	return bytecount;
}

/* This is copy pasta at the moment
 * From: pRED's original CDetour
 *       AlliedModders SourceMod core 
 *		WHICH MEANS ITS UNDER GPL YOU SLAGS
 */
void *MemUtils::FindPattern(const void *pLibPtr, const char *pchPattern, size_t nLen)
{
	DynLibInfo lib;
	bool bFound;
	char *pPtr, *pEnd;

	memset(&lib, 0, sizeof(DynLibInfo));

	if (!GetLibraryInfo(pLibPtr, lib))
	{
		return NULL;
	}

	pPtr = reinterpret_cast<char *>(lib.pBaseAddress);
	pEnd = pPtr + lib.nMemorySize;

	while (pPtr < pEnd)
	{
		bFound = true;
		for (register size_t i = 0; i < nLen; i++)
		{
			if (pchPattern[i] != '\x2A' && pchPattern[i] != pPtr[i])
			{
				bFound = false;
				break;
			}
		}

		if (bFound)
		{
			return pPtr;
		}

		pPtr++;
	}

	return NULL;
}

/* This is copy pasta at the moment
 * From: pRED's original CDetour
 *       AlliedModders SourceMod core 
 *		WHICH MEANS ITS UNDER GPL YOU SLAGS
 */
void *MemUtils::ResolveSymbol(void *pHandle, const char *pchSymbol)
{
#ifdef _WIN32

	return GetProcAddress((HMODULE)pHandle, pchSymbol);

#elif defined _LINUX
#if 0
	struct link_map *dlmap;
	struct stat dlstat;
	int dlfile;
	uintptr_t map_base;
	Elf32_Ehdr *file_hdr;
	Elf32_Shdr *sections, *shstrtab_hdr, *symtab_hdr, *strtab_hdr;
	Elf32_Sym *symtab;
	const char *shstrtab, *strtab;
	uint16_t section_count;
	uint32_t symbol_count;
	LibSymbolTable *libtable;
	SymbolTable *table;
	Symbol *symbol_entry;

	dlmap = (struct link_map *)handle;
	symtab_hdr = NULL;
	strtab_hdr = NULL;
	table = NULL;
	
	/* See if we already have a symbol table for this library */
	for (size_t i = 0; i < m_SymTables.size(); i++)
	{
		libtable = m_SymTables[i];
		if (libtable->lib_base == dlmap->l_addr)
		{
			table = &libtable->table;
			break;
		}
	}

	/* If we don't have a symbol table for this library, then create one */
	if (table == NULL)
	{
		libtable = new LibSymbolTable();
		libtable->table.Initialize();
		libtable->lib_base = dlmap->l_addr;
		libtable->last_pos = 0;
		table = &libtable->table;
		m_SymTables.push_back(libtable);
	}

	/* See if the symbol is already cached in our table */
	symbol_entry = table->FindSymbol(symbol, strlen(symbol));
	if (symbol_entry != NULL)
	{
		return symbol_entry->address;
	}

	/* If symbol isn't in our table, then we have open the actual library */
	dlfile = open(dlmap->l_name, O_RDONLY);
	if (dlfile == -1 || fstat(dlfile, &dlstat) == -1)
	{
		close(dlfile);
		return NULL;
	}

	/* Map library file into memory */
	file_hdr = (Elf32_Ehdr *)mmap(NULL, dlstat.st_size, PROT_READ, MAP_PRIVATE, dlfile, 0);
	map_base = (uintptr_t)file_hdr;
	if (file_hdr == MAP_FAILED)
	{
		close(dlfile);
		return NULL;
	}
	close(dlfile);

	if (file_hdr->e_shoff == 0 || file_hdr->e_shstrndx == SHN_UNDEF)
	{
		munmap(file_hdr, dlstat.st_size);
		return NULL;
	}

	sections = (Elf32_Shdr *)(map_base + file_hdr->e_shoff);
	section_count = file_hdr->e_shnum;
	/* Get ELF section header string table */
	shstrtab_hdr = &sections[file_hdr->e_shstrndx];
	shstrtab = (const char *)(map_base + shstrtab_hdr->sh_offset);

	/* Iterate sections while looking for ELF symbol table and string table */
	for (uint16_t i = 0; i < section_count; i++)
	{
		Elf32_Shdr &hdr = sections[i];
		const char *section_name = shstrtab + hdr.sh_name;

		if (strcmp(section_name, ".symtab") == 0)
		{
			symtab_hdr = &hdr;
		}
		else if (strcmp(section_name, ".strtab") == 0)
		{
			strtab_hdr = &hdr;
		}
	}

	/* Uh oh, we don't have a symbol table or a string table */
	if (symtab_hdr == NULL || strtab_hdr == NULL)
	{
		munmap(file_hdr, dlstat.st_size);
		return NULL;
	}

	symtab = (Elf32_Sym *)(map_base + symtab_hdr->sh_offset);
	strtab = (const char *)(map_base + strtab_hdr->sh_offset);
	symbol_count = symtab_hdr->sh_size / symtab_hdr->sh_entsize;

	/* Iterate symbol table starting from the position we were at last time */
	for (uint32_t i = libtable->last_pos; i < symbol_count; i++)
	{
		Elf32_Sym &sym = symtab[i];
		unsigned char sym_type = ELF32_ST_TYPE(sym.st_info);
		const char *sym_name = strtab + sym.st_name;
		Symbol *cur_sym;

		/* Skip symbols that are undefined or do not refer to functions or objects */
		if (sym.st_shndx == SHN_UNDEF || (sym_type != STT_FUNC && sym_type != STT_OBJECT))
		{
			continue;
		}

		/* Caching symbols as we go along */
		cur_sym = table->InternSymbol(sym_name, strlen(sym_name), (void *)(dlmap->l_addr + sym.st_value));
		if (strcmp(symbol, sym_name) == 0)
		{
			symbol_entry = cur_sym;
			libtable->last_pos = ++i;
			break;
		}
	}

	munmap(file_hdr, dlstat.st_size);
	return symbol_entry ? symbol_entry->address : NULL;
#endif
#endif
}

/* This is copy pasta at the moment
 * From: pRED's original CDetour
 *       AlliedModders SourceMod core 
 *		WHICH MEANS ITS UNDER GPL YOU SLAGS
 */
bool MemUtils::GetLibraryInfo(const void *pLibPtr, DynLibInfo &lib)
{
	unsigned long baseAddr;

	if (pLibPtr == NULL)
	{
		return false;
	}

#ifdef _WIN32

	MEMORY_BASIC_INFORMATION info;
	IMAGE_DOS_HEADER *dos;
	IMAGE_NT_HEADERS *pe;
	IMAGE_FILE_HEADER *file;
	IMAGE_OPTIONAL_HEADER *opt;

	if (!VirtualQuery(pLibPtr, &info, sizeof(MEMORY_BASIC_INFORMATION)))
	{
		return false;
	}

	baseAddr = reinterpret_cast<unsigned long>(info.AllocationBase);

	/* All this is for our insane sanity checks :o */
	dos = reinterpret_cast<IMAGE_DOS_HEADER *>(baseAddr);
	pe = reinterpret_cast<IMAGE_NT_HEADERS *>(baseAddr + dos->e_lfanew);
	file = &pe->FileHeader;
	opt = &pe->OptionalHeader;

	/* Check PE magic and signature */
	if (dos->e_magic != IMAGE_DOS_SIGNATURE || pe->Signature != IMAGE_NT_SIGNATURE || opt->Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
	{
		return false;
	}

	/* Check architecture, which is 32-bit/x86 right now
	 * Should change this for 64-bit if Valve gets their act together
	 */
	if (file->Machine != IMAGE_FILE_MACHINE_I386)
	{
		return false;
	}

	/* For our purposes, this must be a dynamic library */
	if ((file->Characteristics & IMAGE_FILE_DLL) == 0)
	{
		return false;
	}

	/* Finally, we can do this */
	lib.nMemorySize = opt->SizeOfImage;

#elif defined _LINUX

	Dl_info info;
	Elf32_Ehdr *file;
	Elf32_Phdr *phdr;
	uint16_t phdrCount;

	if (!dladdr(pLibPtr, &info))
	{
		return false;
	}

	if (!info.dli_fbase || !info.dli_fname)
	{
		return false;
	}

	/* This is for our insane sanity checks :o */
	baseAddr = reinterpret_cast<unsigned long>(info.dli_fbase);
	file = reinterpret_cast<Elf32_Ehdr *>(baseAddr);

	/* Check ELF magic */
	if (memcmp(ELFMAG, file->e_ident, SELFMAG) != 0)
	{
		return false;
	}

	/* Check ELF version */
	if (file->e_ident[EI_VERSION] != EV_CURRENT)
	{
		return false;
	}

	/* Check ELF architecture, which is 32-bit/x86 right now
	 * Should change this for 64-bit if Valve gets their act together
	 */
	if (file->e_ident[EI_CLASS] != ELFCLASS32 || file->e_machine != EM_386 || file->e_ident[EI_DATA] != ELFDATA2LSB)
	{
		return false;
	}

	/* For our purposes, this must be a dynamic library/shared object */
	if (file->e_type != ET_DYN)
	{
		return false;
	}

	phdrCount = file->e_phnum;
	phdr = reinterpret_cast<Elf32_Phdr *>(baseAddr + file->e_phoff);
	
	/* Add up the memory sizes of segments marked as PT_LOAD as those are the only ones that should be in memory */
	for (uint16_t i = 0; i < phdrCount; i++)
	{
		Elf32_Phdr &hdr = phdr[i];

		if (hdr.p_type == PT_LOAD)
		{
			lib.nMemorySize += hdr.p_memsz;
		}
	}

#endif

	lib.pBaseAddress = reinterpret_cast<void *>(baseAddr);

	return true;
}

/*
size_t MemUtils::count_bytes_until_jmp(BYTE *pDest)
{
	size_t i = 0;
	bool end = false;

	while (!end)
	{
		switch (pDest[i])
		{
		// Cases we care about

		// 0 bytes
		case (0xC3) :
			{

			}

		// 1 byte
		case (0x70) : // ?
		case (0x71) : // ?
		case (0x72) :
		case (0x73) : // ?
		case (0x74) :
		case (0x75) :
		case (0x76) : // ?
		case (0x77) : // ?
		case (0x78) : // ?
		case (0x79) :
		case (0x7A) :
		case (0x7B) :
		case (0x7C) :
		case (0x7D) :
		case (0x7E) :
		case (0x7F) : // ?
		case (0xEB) :
		case (0xE3) : // ?
			{

			}

		// 2 bytes
		case (0xC2) : 
			{

			}

		// 4 bytes 
		case (0xE9) :
			{

			}

		// 5 bytes 
		case (0xE0) : // ?
		case (0xE1) : // ?
		case (0xE2) : // ?
			{

			}
			
		// 2 byte instruction
		case (0x0F) :
			{
				i ++;

				switch (pDest[i])
				{
				// 4 bytes 
				case (0x80) : // ?
				case (0x81) : // ?
				case (0x82) : // ?
				case (0x83) : // ?
				case (0x84) : // ?
				case (0x85) : // ?
				case (0x86) : // ?
				case (0x87) : // ?
				case (0x88) : // ?
				case (0x89) : // ?
				case (0x8A) : // ?
				case (0x8B) : // ?
				case (0x8C) : // ?
				case (0x8D) : // ?
				case (0x8E) : // ?
				case (0x8F) : // ?
					{

					}
				}

			}
		}

		i++;
	}

	return i;
}
*/