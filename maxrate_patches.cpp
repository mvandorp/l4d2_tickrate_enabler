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
#include "sigs.h"

NetChanDataRatePatch::NetChanDataRatePatch(BYTE * engine) : m_patch(NULL)
{
	m_patch = GeneratePatch(FindCNetChanSetDataRate(engine));
}

NetChanDataRatePatch::~NetChanDataRatePatch()
{
	if(m_patch != NULL) delete m_patch;
}

void NetChanDataRatePatch::Patch()
{
	m_patch->Patch();
}

void NetChanDataRatePatch::Unpatch()
{
	m_patch->Unpatch();
}

BYTE * NetChanDataRatePatch::FindCNetChanSetDataRate(BYTE * engine)
{
#if defined (_LINUX)
	return (BYTE *)g_MemUtils.SimpleResolve(engine, SIG_CNETCHAN_SETDATARATE);
#elif defined (_WIN32)
	return (BYTE*)g_MemUtils.FindLibPattern(engine, SIG_CNETCHAN_SETDATARATE, SIG_CNETCHAN_SETDATARATE_LEN);
#endif
}

ICodePatch * NetChanDataRatePatch::GeneratePatch(BYTE * pCNetChanSetDataRate)
{
	if(!pCNetChanSetDataRate)
	{
		throw PatchException("Unable to find CNetChan::SetDataRate!");
	}
#if defined _WIN32
	// Jump from just after first instruction (6 bytes, loads argument float to xmm0)
	// to the lower-bound comparison instructions (func+35 bytes, jump offset 35-6-2 = 27 = 0x1B)
	const BYTE replacement[JMP_8_INSTR_LEN] = {JMP_8_OPCODE, 0x1B};
	return new BasicStaticBinPatch<sizeof(replacement)>(pCNetChanSetDataRate+6, replacement);
#elif defined _LINUX
	// Change comparison jump at +0x18 to NOP2, removing upper bound check.
	if(pCNetChanSetDataRate[0x18] != JB_8_OPCODE)
	{
		throw PatchException("CNetChan::SetDataRate jump patch offset incorrect!");
	}
	return new BasicStaticBinPatch<sizeof(NOP_2)>(pCNetChanSetDataRate+0x18, NOP_2);
#endif
}


#if defined _WIN32
GameClientSetRatePatch::GameClientSetRatePatch(BYTE * engine) : m_patch(NULL)
{
	m_patch = GeneratePatch(FindCGameClientSetRate(engine));
}

GameClientSetRatePatch::~GameClientSetRatePatch()
{
	if(m_patch != NULL) delete m_patch;
}

void GameClientSetRatePatch::Patch()
{
	m_patch->Patch();
}

void GameClientSetRatePatch::Unpatch()
{
	m_patch->Unpatch();
}

BYTE * GameClientSetRatePatch::FindCGameClientSetRate(BYTE * engine)
{
	// A1 ? ? ? ? 8B 40 30 85 C0 7E 0C 8B
	return (BYTE*)g_MemUtils.FindLibPattern(engine, "\xA1\x2A\x2A\x2A\x2A\x8B\x40\x30\x85\xC0\x7E\x0C\x8B", 13);
}

ICodePatch * GameClientSetRatePatch::GeneratePatch(BYTE * pCGameClientSetRate)
{
	if(!pCGameClientSetRate)
	{
		throw PatchException("Unable to find CGameClient::SetRate!");
	}
	// Offset 0x2F should be cmp eax, 30000; jle +0x06
	// Change to JMP +0x0B
	/*
	+0x2F
	3D 30 75 00 00		cmp     eax, 7530h
	7E 06				jle     short loc_10180DFC
	*/
	const BYTE replacement[JMP_8_INSTR_LEN] = {JMP_8_OPCODE, 0x0B};
	return new BasicStaticBinPatch<sizeof(replacement)>(pCGameClientSetRate+0x2F, replacement);
}

#elif defined (_LINUX)

ClampClientRatePatch::ClampClientRatePatch(BYTE * engine) : m_patch(NULL)
{
	m_patch = GeneratePatch(FindClampClientRate(engine));
}

ClampClientRatePatch::~ClampClientRatePatch()
{
	if(m_patch != NULL) delete m_patch;
}

void ClampClientRatePatch::Patch()
{
	m_patch->Patch();
}

void ClampClientRatePatch::Unpatch()
{
	m_patch->Unpatch();
}

BYTE * ClampClientRatePatch::FindClampClientRate(BYTE * engine)
{
	return (BYTE *)g_MemUtils.SimpleResolve(engine, "_Z15ClampClientRatei");
}

ICodePatch * ClampClientRatePatch::GeneratePatch(BYTE * pClampClientRate)
{
	if(!pClampClientRate)
	{
		throw PatchException("Unable to find ClampClientRate!");
	}
	/*
	+0x38
	B8 30 75 00 00 		mov     eax, 7530h
	81 FA 30 75 00 00	cmp     edx, 7530h
	0F 4E C2			cmovle  eax, edx
	*/
	BYTE replacement[MOV_R32_R32_INSTR_LEN+sizeof(NOP_9)+sizeof(NOP_3)] = {MOV_R32_RM32_OPCODE, MODRM_REG_EAX_EDX, 0,0,0,0,0,0,0,0,0,0,0,0};
	memcpy(&replacement[2], NOP_9, sizeof(NOP_9));
	memcpy(&replacement[11], NOP_3, sizeof(NOP_3));
	if(pClampClientRate[0x38] != MOV_R32_IMM32_OPCODE)
	{
		throw PatchException("ClampClientRate jump patch offset incorrect!");
	}
	return new BasicStaticBinPatch<sizeof(replacement)>(pClampClientRate+0x38, replacement);
}
#endif