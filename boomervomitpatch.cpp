#include <cstdlib>
#include "memutils.h"
#include "boomervomitpatch.h"
#include "patchexceptions.h"


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
#ifdef _LINUX
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
#endif

BoomerVomitFrameTimePatch::BoomerVomitFrameTimePatch(IServerGameDLL * gamedll)
{
	// Mark that nothing is patched yet
	for(size_t i = 0; i < NUM_FRAMETIME_READS; i++) m_patches[i] = NULL;
	m_fpCVomitUpdateAbility = FindCVomitUpdateAbility(static_cast<void *>(gamedll));
	DevMsg("CVomitUpdateAbility at 0x%08x\n", m_fpCVomitUpdateAbility);
}

BoomerVomitFrameTimePatch::~BoomerVomitFrameTimePatch()
{
	for(size_t i = 0; i < NUM_FRAMETIME_READS; i++)
	{
		if(m_patches[i] != NULL)
		{
			delete m_patches[i];
		}
	} 
}

void BoomerVomitFrameTimePatch::Patch()
{
	if(!m_bInitialized)
	{
		InitializeBinPatches();
	}
	for(size_t i = 0; i < NUM_FRAMETIME_READS; i++)
	{
		m_patches[i]->Patch();
	}
}

void BoomerVomitFrameTimePatch::Unpatch() 
{
	if(!m_bInitialized)
	{
		InitializeBinPatches();
	}
	for(size_t i = 0; i < NUM_FRAMETIME_READS; i++)
	{
		m_patches[i]->Unpatch();
	}
}

void BoomerVomitFrameTimePatch::InitializeBinPatches()
{
	BYTE instr_buf[MAX_MOV_INSTR_LEN];

	if(!m_fpCVomitUpdateAbility)
	{
		throw PatchException("Couldn't find CVomit::UpdateAbility() in server memory.");
	}

	for(size_t i = 0; i < NUM_FRAMETIME_READS; i++)
	{
		if(m_patches[i] == NULL)
		{
			DevMsg("Setting up patch for frametime read %d (offs:0x%x).\n", i, g_FrameTimeReadOffsets[i]);

			// Calculate first offset target
			BYTE * pTarget = m_fpCVomitUpdateAbility + g_FrameTimeReadOffsets[i];

			int offs = mov_src_operand_offset(pTarget); // Find offset of disp32 in this particular mov instruction
			if(offs == 0)
			{
				// Throw an exception if we can't identify this offset (unexpected instruction!)
				// TODO: More useful exception here.
				throw PatchException("CVomit::UpdateAbility() Patch Offset incorrect.");
			}

			memcpy(instr_buf, pTarget, offs+4);
	
			// make this instruction read from an immediate address
			mov_to_disp32(instr_buf);

			// Plug in our super cool immediate address.
#if defined (_WIN32)
			*(fakeGlobals ***)(pTarget + offs) = &gp_FakeGlobals;
#elif defined (_LINUX)
			*(fakeGlobals ****)(pTarget + offs) = &gpp_FakeGlobals;
#endif
			
			// Generate BasicBinPatch
			m_patches[i] = new BasicStaticBinPatch<MAX_MOV_INSTR_LEN>(pTarget, instr_buf);
		}
	}
}

BYTE * BoomerVomitFrameTimePatch::FindCVomitUpdateAbility(void * gamedll)
{
	if(m_fpCVomitUpdateAbility) return m_fpCVomitUpdateAbility;
#if defined (_LINUX)
	return (BYTE *)SimpleResolve(gamedll, LIN_CVomit_UpdateAbility_SYMBOL);
#elif defined (_WIN32)
	return (BYTE*)g_MemUtils.FindLibPattern(gamedll, WIN_CVomit_UpdateAbility_SIG, WIN_CVomit_UpdateAbility_SIGLEN);
#endif
}
