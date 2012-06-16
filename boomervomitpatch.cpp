#include <cstdlib>
#include "memutils.h"
#include "igameevents.h"
#include "eiface.h"
#include "tier0/icommandline.h"

struct fakeGlobals {
	float padding[4];
	float frametime;
};

fakeGlobals g_FakeGlobals = { {0.0, 0.0, 0.0, 0.0}, 0.033333333};
fakeGlobals *gp_FakeGlobals = &g_FakeGlobals;


#if defined _LINUX
bool PatchBoomerVomit(IServerGameDLL * gamedll)
{
	void * p_CVomitUpdateAbility = NULL;
    const char CVomitUpdateAbility_Symbol[] = "_ZN6CVomit13UpdateAbilityEv";
	Dl_info info;
    /* GNU only: returns 0 on error, inconsistent! >:[ */
    if (dladdr(gamedll, &info) != 0)
    {
    	void *handle = dlopen(info.dli_fname, RTLD_NOW);
        if (handle)
        {
			p_CVomitUpdateAbility = g_MemUtils.ResolveSymbol(handle, CVomitUpdateAbility_Symbol);
        	dlclose(handle);
        } else {
			Warning("Nohandle!\n");
			return false;
		}
	}
	else
	{
		Warning("No DLINFO!\n");
		return false;
	}
	if(!p_CVomitUpdateAbility)
	{
		Warning("Unable to find CVomitUpdateAbility\n");
		return false;
	}
	Msg("CVomitUpdateAbility at %p\n", p_CVomitUpdateAbility);

	void * end = (void *)(((char *)p_CVomitUpdateAbility) + 0x500);
/*	Msg("Searching for end of CVomit::UpdateAbility()\n");
	end = g_MemUtils.FindPattern(p_CVomitUpdateAbility, end, "\xe8\xf1\xe8\xec\xff\x90", 1);
	Msg("Found the end at %p\n", end);*/

	// mov e?x, ebp+gpGlobalsOffset
	const char movGpGlobals[] = "\x8B\x2a\xfc\xf4\xff\xff\x8b";

	Msg("Searching for from %p to %p for %d bytes\n", p_CVomitUpdateAbility, end, sizeof(movGpGlobals)-1);
	int patchcnt=0;
	while((p_CVomitUpdateAbility = g_MemUtils.FindPattern(p_CVomitUpdateAbility, 
			end, movGpGlobals, sizeof(movGpGlobals)-1)) != NULL)
	{
		++patchcnt;
		Msg("Found something at %p\n", p_CVomitUpdateAbility);
		unsigned char * test = (unsigned char *)p_CVomitUpdateAbility;
		Msg("It's %02x %02x %p\n", (uint32)test[0], (uint32)test[1], *(void **)(test+2));
		PatchGlobalsRead(p_CVomitUpdateAbility);
		p_CVomitUpdateAbility=(void *)(((char*)p_CVomitUpdateAbility)+1);
		Msg("Searching for from %p to %p for %d bytes\n", p_CVomitUpdateAbility, end, sizeof(movGpGlobals)-1);
	};
	Msg("Found %d instances\n", patchcnt);

	return true;


#elif defined _WIN32

bool PatchBoomerVomit(IServerGameDLL * gamedll)
{
	BYTE * p_CVomitUpdateAbility = NULL;
	const CGlobalVarsBase ** p_gpGlobals_Addr = NULL;
	
	
	// Pattern to find CVomit::UpdateAblity()
	// 81 EC ? ? ? ? 53 55 56 57 8B F9 8B 87 28 04
	const char CVomitUpdateAbility_pattern[] = "\x81\xEC\x2A\x2A\x2A\x2A\x53\x55\x56\x57\x8B\xF9\x8B\x87\x28\x04";
	const int firstFrameTimeReadOffset = 0x168; // mov edx, gpGlobals; 8B 15 <ADDR>
	// todo: search, not offsets... maybe
	const int secondFrameTimeReadOffset = 0x2C2; // mov eax, gpGlobals; A1 <ADDR>
	const int thirdFrameTimeReadOffset = 0x45C; // mov eax, gpGlobals; A1 <ADDR>
	
	p_CVomitUpdateAbility = (BYTE*)g_MemUtils.FindLibPattern(gamedll, CVomitUpdateAbility_pattern, sizeof(CVomitUpdateAbility_pattern));
	if(!p_CVomitUpdateAbility)
	{
		Warning("Unable to find CVomitUpdateAbility\n");
		return false;
	}
	Msg("CVomitUpdateAbility at %08x\n", p_CVomitUpdateAbility);

	/* First gpGlobals read */

	// The first read of gp_globals should be at this offset into the func
	BYTE * pTarget1 = p_CVomitUpdateAbility + firstFrameTimeReadOffset;

	if(pTarget1[0] != 0x8B)
	{
		// if not "MOV r32,m32, fail
		Warning("Bad 1st read target: Not a mov read. (0x%02x)\n", pTarget1[0]);
		return false;
	}
	
	// Pull the gp_globals read addr from this instruction
	p_gpGlobals_Addr = *(const CGlobalVarsBase ***)(pTarget1 + 2);
	DevMsg("gp_Globals at 0x%08x\n", p_gpGlobals_Addr);

	g_MemUtils.SetMemPatchable(pTarget1, 6);
	// Patch this read to read our fake gpGlobals
	*(fakeGlobals ***)(pTarget1 + 2) = &gp_FakeGlobals;

	DevMsg("Successfully patched the first read\n");

	/* Second gpGlobals read */

	BYTE * pTarget2 = p_CVomitUpdateAbility + secondFrameTimeReadOffset;

	// Should be mov eax, mem32
	if(pTarget2[0] != 0xA1)
	{
		Warning("Bad 2nd read target: not mov eax (0x%02x)\n", pTarget2[0]);
		goto cleanup1;
	}
	
	// Test against gpGlobals addr from last read
	if(*(const CGlobalVarsBase ***)(pTarget2 + 1) != p_gpGlobals_Addr)
	{
		Warning("Bad 2nd read target: gpGlobals addr mismatch (0x%08x)\n", *(const CGlobalVarsBase ***)(pTarget2 + 1));
		goto cleanup1;
	}

	g_MemUtils.SetMemPatchable(pTarget2, 5);
	// Patch this read to read our fake globals
	*(fakeGlobals ***)(pTarget2+1) = &gp_FakeGlobals;

	DevMsg("Successfully patched the second read\n");

	/* Third gpGlobals read */

	BYTE * pTarget3 = p_CVomitUpdateAbility + thirdFrameTimeReadOffset;

	// Should be mov eax, mem32
	if(pTarget3[0] != 0xA1)
	{
		Warning("Bad 3rd read target: not mov eax (0x%02x)\n", pTarget3[0]);
		goto cleanup2;
	}
	
	// Test against gpGlobals addr from last read
	if(*(const CGlobalVarsBase ***)(pTarget3 + 1) != p_gpGlobals_Addr)
	{
		Warning("Bad 3rd read target: gpGlobals addr mismatch (0x%08x)\n", *(const CGlobalVarsBase ***)(pTarget3 + 1));
		goto cleanup2;
	}

	g_MemUtils.SetMemPatchable(pTarget3, 5);
	// Patch this read to read our fake globals
	*(fakeGlobals ***)(pTarget3+1) = &gp_FakeGlobals;

	DevMsg("Successfully patched the third read\n");

	return true;

	// unpatch and return failure
cleanup2:
	*(const CGlobalVarsBase ***)(pTarget2 + 1) = p_gpGlobals_Addr;
cleanup1:
	*(const CGlobalVarsBase ***)(pTarget1 + 2) = p_gpGlobals_Addr;
	return false;
}
#endif

