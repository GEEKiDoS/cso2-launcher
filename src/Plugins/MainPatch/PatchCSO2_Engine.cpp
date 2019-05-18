#include "stdafx.h"

#include "cdll_int.h"
#include "color.h"
#include "convar.h"
#include "tier0/icommandline.h"

#include <hooks.h>

#pragma optimize( "", off )
using namespace std::literals::string_literals;

//
// Restore original CanCheat check
// This fixes the use of sv_cheats as a host
// Credit goes to GEEKiDoS
//

ConVar* sv_cheats = nullptr;
ICvar *g_pCVar = nullptr;

static std::unique_ptr<PLH::x86Detour> g_pCanCheatHook;
static uint64_t g_CanCheatOrig = NULL;

NOINLINE bool __fastcall hkCanCheat()
{
	if (g_pCVar == nullptr)
		g_pCVar = g_pImports->GetCVar();

	if (!sv_cheats)
	{
		sv_cheats = g_pCVar->FindVar("sv_cheats");
		assert(sv_cheats != nullptr);  // this should always succeed
	}

	return sv_cheats->GetBool();
}

static std::unique_ptr<PLH::x86Detour> g_pColorPrintHook;
static uint64_t g_ColorPrintOrig = NULL;

NOINLINE void __fastcall hkCon_ColorPrint(Color& clr, const char* msg)
{
	g_pImports->Print(msg);

	return PLH::FnCast(g_ColorPrintOrig, &hkCon_ColorPrint)(clr, msg);
}

void BytePatchEngine(const uintptr_t dwEngineBase)
{
	//
	// don't initialize BugTrap on engine
	//
	// jmp short 0x3C bytes forward
	const std::array<uint8_t, 5> btPatch = { 0xEB, 0x3C };
	WriteProtectedMemory(btPatch, (dwEngineBase + 0x15877B));

	//
	// reenable UDP info packet
	//
	const std::array<uint8_t, 6> netPacketPatch = {
		0x89, 0xB0, 0x28, 0x01, 0x00, 0x00
	};  // mov [eax+128h], esi
	WriteProtectedMemory(netPacketPatch, (dwEngineBase + 0x283604));

	//
	// force direct UDP connection instead of relay connection
	//
	// mov dword ptr [eax], 2
	const std::array<uint8_t, 6> relayPatch = { 0xC7, 0x00, 0x02,
												0x00, 0x00, 0x00 };
	WriteProtectedMemory(relayPatch, (dwEngineBase + 0x2BE552));
	// mov dword ptr [eax+8], 2
	const std::array<uint8_t, 7> relayPatch2 = { 0xC7, 0x40, 0x08, 0x02,
												 0x00, 0x00, 0x00 };
	WriteProtectedMemory(relayPatch2, (dwEngineBase + 0x2BE56C));
	// mov dword ptr [eax+4], 2
	const std::array<uint8_t, 7> relayPatch3 = { 0xC7, 0x40, 0x04, 0x02,
												 0x00, 0x00, 0x00 };
	WriteProtectedMemory(relayPatch3, (dwEngineBase + 0x2BE587));

	//
	// always return true when checking if the user is over 18
	//
	// mov al, 01
	// ret
	const std::array<uint8_t, 11> isAdultPatch = {
		0xB0, 0x01, 0xC3
	};
	WriteProtectedMemory(isAdultPatch, (dwEngineBase + 0x288FF0));

	//
	// Patch CanCheat traps
	//
	// nops
	const std::array<uint8_t, 6> canCheatPatch1 = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
	WriteProtectedMemory(canCheatPatch1, (dwEngineBase + 0x1EFEF6));

	// jmp
	const std::array<uint8_t, 1> canCheatPatch2 = { 0xEB };
	WriteProtectedMemory(canCheatPatch2, (dwEngineBase + 0x19F4D2));
}

void PatchCSO2_Engine(uintptr_t dwEngineBase)
{
	static bool loaded = false;

	if (loaded)
		return;

	g_pImports->Print("Main Patch - Appling engine patches. ");

	BytePatchEngine(dwEngineBase);

	PLH::CapstoneDisassembler dis(PLH::Mode::x86);

	g_pCanCheatHook = SetupDetourHook(dwEngineBase + 0xCE8B0, &hkCanCheat,
		&g_CanCheatOrig, dis);

	g_pColorPrintHook = SetupDetourHook(
		dwEngineBase + 0x1C4B40, &hkCon_ColorPrint, &g_ColorPrintOrig, dis);

	g_pCanCheatHook->hook();
	g_pColorPrintHook->hook();
	loaded = true;
}
#pragma optimize( "", on )