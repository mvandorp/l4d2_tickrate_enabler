#include <cstdlib>
#include "memutils.h"
#include "igameevents.h"
#include "eiface.h"
#include "tier0/icommandline.h"

#define MODRM_SRC_TO_DISP32(modrm) (( modrm & 0x38) | 0x05 )

// Convert mov instruction of any type to mov from immediate address (disp32)
// @param instr: pointer to first byte of mov instruction
// @return true if instruction is now disp32 source
inline bool mov_to_disp32(BYTE * instr)
{
	switch(instr[0])
	{
	case 0x8B: // standard mov with modrm
		instr[1] = MODRM_SRC_TO_DISP32(instr[1]);
		return true;
	case 0xA1: // direct to eax mov
		return true;
	default: // unsupported or not mov
		return false;
	}
}

// offset of src operand in mov instruction
inline int mov_src_operand_offset(BYTE * instr)
{
	switch(instr[0])
	{
	case 0x8B:
		return 2;
	case 0xA1:
		return 1;
	default: 
		return 0;
	}
}

struct fakeGlobals {
	float padding[4];
	float frametime;
};

fakeGlobals g_FakeGlobals = { {0.0, 0.0, 0.0, 0.0}, 0.033333333};
fakeGlobals *gp_FakeGlobals = &g_FakeGlobals;

#if defined (_LINUX)
fakeGlobals **gpp_FakeGlobals = &gp_FakeGlobals; // olol

void * SimpleResolve(void * pBaseAddr, const char * symbol)
{
	Dl_info info;
	if (dladdr(pBaseAddr, &info) != 0)
    {
    	void *handle = dlopen(info.dli_fname, RTLD_NOW);
        if (handle)
        {
			void * pLocation = g_MemUtils.ResolveSymbol(handle, symbol);
        	dlclose(handle);
        	return pLocation;
        } else {
			Warning("Nohandle!\n");
			return NULL;
		}
	}
	else
	{
		Warning("No DLINFO!\n");
		return NULL;
	}
}


/* Linux L4D1+2 */

bool PatchBoomerVomit(IServerGameDLL * gamedll)
{
    const char CVomitUpdateAbility_Symbol[] = "_ZN6CVomit13UpdateAbilityEv";

#if defined (L4D1)
    const int firstFrameTimeReadOffset = 0x153; // mov edx, [ebx+offs gpGlobals]; 8B 93
    const int secondFrameTimeReadOffset = 0x303; // mov ecx, [eba+offs gpGlobals]; 8B 8B
#elif defined (L4D2)
    const int firstFrameTimeReadOffset = 0x158;
    const int secondFrameTimeReadOffset = 0x308; 
#endif
	// todo: search, not offsets... maybe
	
	
    BYTE * p_CVomitUpdateAbility = (BYTE *)SimpleResolve(gamedll, CVomitUpdateAbility_Symbol);
	if(!p_CVomitUpdateAbility)
	{
		Warning("Unable to find CVomitUpdateAbility\n");
		return false;
	}
	Msg("CVomitUpdateAbility at %p\n", p_CVomitUpdateAbility);

/* CVomit::UpdateAbility()+0x158

8B 93 00 F5 FF FF    mov     edx, ds:(gpGlobals_ptr - 0DCF5B0h)[ebx]
8B 02                mov     eax, [edx]
D9 40 10             fld     dword ptr [eax+10h] */
	
	BYTE * pTarget1 = p_CVomitUpdateAbility + firstFrameTimeReadOffset;
	if(pTarget1[0] != 0x8B)
	{
		// not mov we expect
		Warning("Bad Target 1: Not mov instruction (0x%02x).\n", pTarget1[0]);
		return false;
	}

	g_MemUtils.SetMemPatchable(pTarget1, 6);
	pTarget1[1] &= 0x38; // destroy first 2 and last 3 bits of MODR/M
	pTarget1[1] |= 0x05; // Convert source to mem32 immediate
	// Patch this read to read our fake gpGlobals
	*(fakeGlobals ****)(pTarget1 + 2) = &gpp_FakeGlobals;



/* CVomit::UpdateAbility()+0x308

8B 8B 00 F5 FF FF    mov     ecx, ds:(gpGlobals_ptr - 0DCF5B0h)[ebx]
8B 01                mov     eax, [ecx]
D9 40 10             fld     dword ptr [eax+10h] */

	BYTE * pTarget2 = p_CVomitUpdateAbility + secondFrameTimeReadOffset;
	if(pTarget2[0] != 0x8B)
	{
		// not mov we expect
		Warning("Bad Target 2: Not mov instruction (0x%02x).\n", pTarget2[0]);
		return false;
	}

	g_MemUtils.SetMemPatchable(pTarget2, 6);
	pTarget2[1] &= 0x38; // destroy first 2 and last 3 bits of MODR/M
	pTarget2[1] |= 0x05; // Convert source to mem32 immediate
	// Patch this read to read our fake gpGlobals
	*(fakeGlobals ****)(pTarget2 + 2) = &gpp_FakeGlobals;


	return true;
}

#elif defined (_WIN32)

/* Windows L4D1+2 */

bool PatchBoomerVomit(IServerGameDLL * gamedll)
{
	BYTE * p_CVomitUpdateAbility = NULL;
	const CGlobalVarsBase ** p_gpGlobals_Addr = NULL;
	
	
	// Pattern to find CVomit::UpdateAblity()
	// search for "stopvomit" string in CVomit::StopVomitEffect() + ~0x1A0, xref StopVomitEffect + ~0xE0 (farther than other similar)
	// 81 EC ? ? ? ? 53 55 56 57 8B F9 8B 87
	const char CVomitUpdateAbility_pattern[] = "\x81\xEC\x2A\x2A\x2A\x2A\x53\x55\x56\x57\x8B\xF9\x8B\x87";

#if defined (L4D1)
	const int firstFrameTimeReadOffset = 0x173; // mov edx, gpGlobals; 8B 15 <ADDR>
	const int secondFrameTimeReadOffset = 0x2CD; // mov eax, gpGlobals; A1 <ADDR>
	const int thirdFrameTimeReadOffset = 0x476; // mov eax, gpGlobals; A1 <ADDR>
#elif defined (L4D2)
	const int firstFrameTimeReadOffset = 0x168; // mov edx, gpGlobals; 8B 15 <ADDR>
	const int secondFrameTimeReadOffset = 0x2C2; // mov eax, gpGlobals; A1 <ADDR>
	const int thirdFrameTimeReadOffset = 0x45C; // mov eax, gpGlobals; A1 <ADDR>
#endif
	
	p_CVomitUpdateAbility = (BYTE*)g_MemUtils.FindLibPattern(gamedll, CVomitUpdateAbility_pattern, sizeof(CVomitUpdateAbility_pattern)-1);
	if(!p_CVomitUpdateAbility)
	{
		Warning("Unable to find CVomitUpdateAbility\n");
		return false;
	}
	Msg("CVomitUpdateAbility at %08x\n", p_CVomitUpdateAbility);

	/* First gpGlobals read */

	// The first read of gp_globals should be at this offset into the func
	BYTE * pTarget1 = p_CVomitUpdateAbility + firstFrameTimeReadOffset;

	
	int offs = mov_src_operand_offset(pTarget1);
	if(offs == 0)
	{
		// if not "MOV r32,m32, fail
		Warning("Bad 1st read target: Not a mov read. (0x%02x)\n", pTarget1[0]);
		return false;
	}
	
	g_MemUtils.SetMemPatchable(pTarget1, 4+offs);
	
	// make this instruction read from an immediate address
	mov_to_disp32(pTarget1);

	// Pull the gp_globals read addr from this instruction
	p_gpGlobals_Addr = *(const CGlobalVarsBase ***)(pTarget1 + offs);
	DevMsg("gp_Globals at 0x%08x\n", p_gpGlobals_Addr);

	// Patch this read to read our fake gpGlobals
	*(fakeGlobals ***)(pTarget1 + offs) = &gp_FakeGlobals;

	DevMsg("Successfully patched the first read\n");

	/* Second gpGlobals read */

	BYTE * pTarget2 = p_CVomitUpdateAbility + secondFrameTimeReadOffset;
	
	// Should be mov eax, mem32
	offs = mov_src_operand_offset(pTarget2);
	if(offs == 0)
	{
		Warning("Bad 2nd read target: not mov eax (0x%02x)\n", pTarget2[0]);
		goto cleanup1;
	}

	g_MemUtils.SetMemPatchable(pTarget2, 4+offs);
	
	mov_to_disp32(pTarget2);
	
	// Test against gpGlobals addr from last read
	if(*(const CGlobalVarsBase ***)(pTarget2 + offs) != p_gpGlobals_Addr)
	{
		Warning("Bad 2nd read target: gpGlobals addr mismatch (0x%08x)\n", *(const CGlobalVarsBase ***)(pTarget2 + offs));
		goto cleanup1;
	}

	// Patch this read to read our fake globals
	*(fakeGlobals ***)(pTarget2+offs) = &gp_FakeGlobals;

	DevMsg("Successfully patched the second read\n");

	/* Third gpGlobals read */

	BYTE * pTarget3 = p_CVomitUpdateAbility + thirdFrameTimeReadOffset;
	g_MemUtils.SetMemPatchable(pTarget3, 6);

	offs = mov_src_operand_offset(pTarget3);
	if(offs == 0)
	{
		Warning("Bad 3rd read target: not mov eax (0x%02x)\n", pTarget3[0]);
		goto cleanup2;
	}

	g_MemUtils.SetMemPatchable(pTarget3, 4+offs);
	
	mov_to_disp32(pTarget3);
	
	// Test against gpGlobals addr from last read
	if(*(const CGlobalVarsBase ***)(pTarget3 + offs) != p_gpGlobals_Addr)
	{
		Warning("Bad 3rd read target: gpGlobals addr mismatch (0x%08x)\n", *(const CGlobalVarsBase ***)(pTarget3 + offs));
		goto cleanup2;
	}

	// Patch this read to read our fake globals
	*(fakeGlobals ***)(pTarget3+offs) = &gp_FakeGlobals;

	DevMsg("Successfully patched the third read\n");

	return true;

	// unpatch and return failure
cleanup2:
	*(const CGlobalVarsBase ***)(pTarget2 + mov_src_operand_offset(pTarget2)) = p_gpGlobals_Addr;
cleanup1:
	*(const CGlobalVarsBase ***)(pTarget1 + mov_src_operand_offset(pTarget1)) = p_gpGlobals_Addr;
	return false;
}

#endif // _WIN32

