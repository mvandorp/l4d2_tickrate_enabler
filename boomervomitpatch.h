#ifndef __BOOMERVOMITPATCH_H__
#define __BOOMERVOMITPATCH_H__

#include "eiface.h"
#include "codepatch/icodepatch.h"
#include "basicbinpatch.h"

/* Platform specific offset+sig data */

/* CVomit::UpdateAbility() function finding */
// Same symbol on both l4d1+2
#define LIN_CVomit_UpdateAbility_SYMBOL "_ZN6CVomit13UpdateAbilityEv"
// search for "stopvomit" string in CVomit::StopVomitEffect() + ~0x1A0, xref StopVomitEffect + ~0xE0 (farther than other similar)
// This sig works for both l4d1+2
// 81 EC ? ? ? ? 53 55 56 57 8B F9 8B 87
#define WIN_CVomit_UpdateAbility_SIG "\x81\xEC\x2A\x2A\x2A\x2A\x53\x55\x56\x57\x8B\xF9\x8B\x87"
#define WIN_CVomit_UpdateAbility_SIGLEN 14

// 6 is as big as I recognize...
#define MAX_MOV_INSTR_LEN 6


/* gpGlobals read offsets into CVomit::UpdateAbility() */
const int g_FrameTimeReadOffsets[] =
#if defined (_LINUX)
#if defined (L4D1)
	{0x153, 0x303}; // L4D1 LINUX
#elif defined (L4D2)
	{0x158, 0x308}; // L4D2 LINUX
#endif
#elif defined (_WIN32)
#if defined (L4D1)
	{0x173, 0x2CD, 0x476};
#elif defined (L4D2)
	{0x168, 0x2C2, 0x45C};
#endif
#endif
#define NUM_FRAMETIME_READS (sizeof(g_FrameTimeReadOffsets)/sizeof(g_FrameTimeReadOffsets[0]))

class BoomerVomitFrameTimePatch : public ICodePatch
{
public:
	BoomerVomitFrameTimePatch(IServerGameDLL * gamedll);
	~BoomerVomitFrameTimePatch();
	void Patch();
	void Unpatch();
private:
	void InitializeBinPatches();
	BYTE * FindCVomitUpdateAbility(void * gamedll);
	BYTE * m_fpCVomitUpdateAbility;
	bool m_bInitialized;
	bool m_bIsReadPatched[NUM_FRAMETIME_READS];
	BasicStaticBinPatch<MAX_MOV_INSTR_LEN> * m_patches[NUM_FRAMETIME_READS];
};

// Deprecated
//bool PatchBoomerVomit(IServerGameDLL * gamedll);

#endif
