/**
 * vim: set ts=4 :
 * =============================================================================
 * MaxRate Patches
 * Copyright (C) 2012 Michael "ProdigySim" Busby
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, the author(s) give you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, the author(s) grant
 * this exception to all derivative works.  The author(s) define further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */


#include "maxrate_patches.h"
#include "memutils.h"
#include "patchexceptions.h"
#include "basicbinpatch.h"

NetChanDataRatePatch::NetChanDataRatePatch(BYTE * engine) : m_patch(NULL)
{
	m_fpCNetChanSetDataRate = FindCNetChanSetDataRate(engine);
}

NetChanDataRatePatch::~NetChanDataRatePatch()
{
	if(m_patch != NULL) delete m_patch;
}

void NetChanDataRatePatch::Patch()
{
	if(!m_patch)
	{
		InitializePatch();
	}
	m_patch->Patch();
}

void NetChanDataRatePatch::Unpatch()
{
	if(!m_patch)
	{
		InitializePatch();
	}
	m_patch->Unpatch();
}

BYTE * NetChanDataRatePatch::FindCNetChanSetDataRate(BYTE * engine)
{
#if defined (_LINUX)
	return (BYTE *)g_MemUtils.SimpleResolve(engine, "_ZN8CNetChan11SetDataRateEf");
#elif defined (_WIN32)
	// F3 0F 10 44 24 04 F3 0F 10 0D ? ? ? ? 0F 2F C1 76 10
	return (BYTE*)g_MemUtils.FindLibPattern(engine, "\xF3\x0F\x10\x44\x24\x04\xF3\x0F\x10\x0D\x2A\x2A\x2A\x2A\x0F\x2F\xC1\x76\x10", 19);
#endif
}

void NetChanDataRatePatch::InitializePatch()
{
	if(!m_fpCNetChanSetDataRate)
	{
		throw PatchException("Unable to find CNetChan::SetDataRate!");
	}
#if defined _WIN32
	// Jump from just after first instruction (6 bytes, loads argument float to xmm0)
	// to the lower-bound comparison instructions (func+35 bytes, jump offset 35-6-2 = 27 = 0x1B)
	const BYTE replacement[JMP_8_INSTR_LEN] = {JMP_8_OPCODE, 0x1B};
	m_patch = new BasicStaticBinPatch<sizeof(replacement)>(m_fpCNetChanSetDataRate+6, replacement);
#elif defined _LINUX
	// Change comparison jump at +0x20 to NOP2, removing upper bound check.
	if(m_fpCNetChanSetDataRate[0x20] != JA_8_OPCODE)
	{
		throw PatchException("CNetChan::SetDataRate jump patch offset incorrect!");
	}
	m_patch = new BasicStaticBinPatch<sizeof(NOP_2)>(m_fpCNetChanSetDataRate+0x20, NOP_2);
#endif
}