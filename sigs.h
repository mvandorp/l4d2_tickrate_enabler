/**
 * vim: set ts=4 :
 * =============================================================================
 * BoomerVomitPatch
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
#ifndef _SIGS_H_
#define _SIGS_H_

/* Platform specific offset+sig data */

/* CVomit::UpdateAbility() function finding */
// Same symbol on both l4d1+2
#define LIN_CVomit_UpdateAbility_SYMBOL "_ZN6CVomit13UpdateAbilityEv"
// search for "stopvomit" string in CVomit::StopVomitEffect() + ~0x1A0, xref StopVomitEffect + ~0xE0 (farther than other similar)
// This sig works for both l4d1+2
// 81 EC ? ? ? ? 53 55 56 57 8B F9 8B 87
#define WIN_CVomit_UpdateAbility_SIG "\x81\xEC\x2A\x2A\x2A\x2A\x53\x55\x56\x57\x8B\xF9\x8B\x87"
#define WIN_CVomit_UpdateAbility_SIGLEN 14


/* gpGlobals read offsets into CVomit::UpdateAbility() */
const int g_FrameTimeReadOffsets[] =
#if defined (_LINUX)
#if defined (L4D1)
	{0x1DD, 0x38D}; // L4D1 LINUX
#elif defined (L4D2)
	{0x12D, 0x5FD}; // L4D2 LINUX
#endif
#elif defined (_WIN32)
#if defined (L4D1)
	{0x173, 0x2CD, 0x476};
#elif defined (L4D2)
	{0x168, 0x2C2, 0x45C};
#endif
#endif
#define NUM_FRAMETIME_READS (sizeof(g_FrameTimeReadOffsets)/sizeof(g_FrameTimeReadOffsets[0]))


#ifdef _WIN32
// F3 0F 10 44 24 04 F3 0F 10 0D ? ? ? ? 0F 2F C1 76 10
#define SIG_CNETCHAN_SETDATARATE "\xF3\x0F\x10\x44\x24\x04\xF3\x0F\x10\x0D\x2A\x2A\x2A\x2A\x0F\x2F\xC1\x76\x10"
#define SIG_CNETCHAN_SETDATARATE_LEN 19
#else
#define SIG_CNETCHAN_SETDATARATE "_ZN8CNetChan11SetDataRateEf"
#endif

#endif // _SIGS_H_