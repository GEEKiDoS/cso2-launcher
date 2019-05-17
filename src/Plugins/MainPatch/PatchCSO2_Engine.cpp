#include "stdafx.h"

#include <array>

#include "cdll_int.h"
#include "color.h"
#include "convar.h"
#include "tier0/icommandline.h"

#include <hooks.h>

using namespace std::literals::string_literals;

struct IpAddressInfo
{
	std::string szIpAddress;
	uint16_t iPort;
};

HOOK_DETOUR_DECLARE(hkGetServerIpAddressInfo);

//
// Allows the user to choose a specific master server's IP address and/or port
// number through command line arguments. Defaults to 127.0.0.1:30001 if no
// arguments are given Usage: "-masterip [desired master IP address]" and/or
// "-masterport [desired master port number]"
//
NOINLINE void __fastcall hkGetServerIpAddressInfo(IpAddressInfo& info)
{
	const char* szMasterIp = CommandLine()->ParmValue("-masterip");
	const char* szMasterPort = CommandLine()->ParmValue("-masterport");

	info.szIpAddress = szMasterIp ? szMasterIp : "127.0.0.1"s;
	info.iPort =
		szMasterPort ? static_cast<uint16_t>(atoi(szMasterPort)) : 30001;
}

HOOK_DETOUR_DECLARE(hkCon_ColorPrint);
NOINLINE void __fastcall hkCon_ColorPrint(Color &clr, const char *msg)
{
	g_pImports->Print(msg);

	return HOOK_DETOUR_GET_ORIG(hkCon_ColorPrint)(clr, msg);
}

//
// Restore original CanCheat check
// This fixes the use of sv_cheats as a host
// Credit goes to GEEKiDoS
//
ConVar* sv_cheats = nullptr;

ICvar *g_pCVar = nullptr;

HOOK_DETOUR_DECLARE(hkCanCheat);
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

void BytePatchEngine(const uintptr_t dwEngineBase)
{
	//
	// don't initialize BugTrap on engine
	//
	// jmp short 0x3C bytes forward
	const std::array<uint8_t, 5> btPatch = { 0xEB, 0x3C };
	WriteProtectedMemory(btPatch, (dwEngineBase + 0x15877B));

	//
	// skip nexon messenger login
	//
	// mov al, 1; retn 8
	const std::array<uint8_t, 5> nmPatch = { 0xB0, 0x01, 0xC2, 0x08, 0x00 };
	WriteProtectedMemory(nmPatch, (dwEngineBase + 0x289490));

	//
	// copy the password instead of a null string
	//
	// push edi; nops
	const std::array<uint8_t, 5> loginNMPatch = { 0x57, 0x90, 0x90, 0x90,
												  0x90 };
	WriteProtectedMemory(loginNMPatch, (dwEngineBase + 0x284786));

	//
	// don't null the username string
	//
	const std::array<uint8_t, 20> loginNMPatch2 = {
		0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,  // nops
		0x8B, 0x44, 0x24, 0x64,  // mov eax, [esp+64]
		0x8D, 0x4C, 0x24, 0x54,  // lea ecx, [esp+54]
		0x90, 0x90, 0x90         // nops
	};
	WriteProtectedMemory(loginNMPatch2, (dwEngineBase + 0x28499D));

	//
	// don't clear password string
	// TODO: this is DANGEROUS! find a better way to fix this!
	//
	// nops
	const std::array<uint8_t, 14> loginNMPatch3 = { 0x90, 0x90, 0x90, 0x90,
													0x90, 0x90, 0x90, 0x90,
													0x90, 0x90, 0x90, 0x90,
													0x90, 0x90 };
	WriteProtectedMemory(loginNMPatch3, dwEngineBase + 0x2849CB);

	//
	// don't allow nexon messenger to ovewrite our password
	//
	// nops
	const std::array<uint8_t, 10> loginNMPatch4 = {
		0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90
	};
	WriteProtectedMemory(loginNMPatch4, (dwEngineBase + 0x284A22));

	//
	// don't get the nexon username from NM
	//
	WriteProtectedMemory(loginNMPatch4, (dwEngineBase + 0x284A57));

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
	// don't send the filesystem hash
	// stops the weird blinking when you login,
	// but you don't get any client hash in the master server
	//
	// nops
	const std::array<uint8_t, 11> hashGenPatch = {
		0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
	};
	WriteProtectedMemory(hashGenPatch, (dwEngineBase + 0x2BC50D));

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

static IVEngineClient* g_pEngineClient = nullptr;

#pragma optimize( "", off )
void PatchCSO2_Engine(uintptr_t dwEngineBase)
{
	static bool loaded = false;

	if (loaded)
		return;

	g_pImports->Print("Appling engine patches. ");

	BytePatchEngine(dwEngineBase);

	HOOK_DETOUR(dwEngineBase + 0x285FE0, hkGetServerIpAddressInfo);
	HOOK_DETOUR(dwEngineBase + 0x1C4B40, hkCon_ColorPrint);
	HOOK_DETOUR(dwEngineBase + 0xCE8B0, hkCanCheat);

	loaded = true;
}
#pragma optimize( "", on )