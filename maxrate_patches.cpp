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
	return (BYTE *)g_MemUtils.SimpleResolve(engine, "_ZN8CNetChan11SetDataRateEf");
#elif defined (_WIN32)
	// F3 0F 10 44 24 04 F3 0F 10 0D ? ? ? ? 0F 2F C1 76 10
	return (BYTE*)g_MemUtils.FindLibPattern(engine, "\xF3\x0F\x10\x44\x24\x04\xF3\x0F\x10\x0D\x2A\x2A\x2A\x2A\x0F\x2F\xC1\x76\x10", 19);
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
	// Change comparison jump at +0x20 to NOP2, removing upper bound check.
	if(pCNetChanSetDataRate[0x20] != JA_8_OPCODE)
	{
		throw PatchException("CNetChan::SetDataRate jump patch offset incorrect!");
	}
	return new BasicStaticBinPatch<sizeof(NOP_2)>(pCNetChanSetDataRate+0x20, NOP_2);
#endif
}



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
#if defined (_LINUX)
	return (BYTE *)g_MemUtils.SimpleResolve(engine, "_ZN11CGameClient7SetRateEib");
#elif defined (_WIN32)
	// 53 8B 5C 24 0C 84 DB 75 68
	return (BYTE*)g_MemUtils.FindLibPattern(engine, "\x53\x8B\x5C\x24\x0C\x84\xDB\x75\x68", 9);
#endif
}

ICodePatch * GameClientSetRatePatch::GeneratePatch(BYTE * pCGameClientSetRate)
{
	if(!pCGameClientSetRate)
	{
		throw PatchException("Unable to find CGameClient::SetRate!");
	}
#if defined _WIN32
	// Offset 0x15 should be cmp eax, 30000; jle +0x10
	// Change to JMP +0x15
	/*
	+0x15
	3D 30 75 00 00		cmp     eax, 7530h
	7E 10				jle     short loc_1004788C
	*/
	const BYTE replacement[JMP_8_INSTR_LEN] = {JMP_8_OPCODE, 0x15};
	return new BasicStaticBinPatch<sizeof(replacement)>(pCGameClientSetRate+0x15, replacement);
#elif defined _LINUX
	/*
	+0x4E
	B8 30 75 00 00 		mov     eax, 7530h
	81 FA 30 75 00 00	cmp     edx, 7530h
	0F 4E C2			cmovle  eax, edx
	*/
	BYTE replacement[MOV_R32_R32_INSTR_LEN+sizeof(NOP_9)] = {MOV_R32_RM32_OPCODE, MODRM_REG_EAX_EDX, 0,0,0,0,0,0,0,0,0};
	memcpy(&replacement[2], NOP_9, sizeof(NOP_9));
	if(pCGameClientSetRate[0x43] != MOV_R32_IMM32_OPCODE)
	{
		throw PatchException("CGameClient::SetRate jump patch offset incorrect!");
	}
	return new BasicStaticBinPatch<sizeof(replacement)>(pCGameClientSetRate+0x4E, replacement);
#endif
}