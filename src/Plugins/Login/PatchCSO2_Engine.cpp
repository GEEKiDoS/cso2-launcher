#include "stdafx.h"
#include "CJsonObject.hpp"

#include "tier0/icommandline.h"

#include <hooks.h>
#include <string>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <WinSock2.h>

using namespace std::literals::string_literals;

#pragma optimize( "", off )
struct IpAddressInfo
{
	std::string szIpAddress;
	uint16_t iPort;
};

static std::unique_ptr<PLH::x86Detour> g_pServerAddrHook;
static uint64_t g_ServerAddrOrig = NULL;

//
// Allows the user to choose a specific master server's IP address and/or port
// number through command line arguments. Defaults to 127.0.0.1:30001 if no
// arguments are given Usage: "-masterip [desired master IP address]" and/or
// "-masterport [desired master port number]"
//
NOINLINE void __fastcall hkGetServerInfo(IpAddressInfo& info)
{
	info.szIpAddress = std::string(MASTER_SERVER_ADDRESS);
	info.iPort = 30001;

	return;
}
	

void BytePatchEngine(const uintptr_t dwEngineBase)
{
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
	// Replace event images
	//
	DWORD dwOldProtect = NULL;
	VirtualProtect((void*)(dwEngineBase + 0x2879AA), 4, PAGE_EXECUTE_READWRITE, &dwOldProtect);
	*(DWORD *)(dwEngineBase + 0x2879AA) = (DWORD)SERVER_EVENT_URL;
	VirtualProtect((void*)(dwEngineBase + 0x2879AA), 4, dwOldProtect, &dwOldProtect);
}

ICSO2MsgHandlerEngine* g_pCSO2MsgHandler;

void PatchCSO2_Engine(uintptr_t dwEngineBase)
{
	static bool loaded = false;

	if (loaded)
		return;

	g_pImports->Print("Login - Appling engine patches. ");

	BytePatchEngine(dwEngineBase);

	if (!g_pCSO2MsgHandler)
	{
		CreateInterfaceFn pEngineFactory = g_pImports->GetFactory("engine.dll");
		g_pCSO2MsgHandler = reinterpret_cast<ICSO2MsgHandlerEngine*>(pEngineFactory(CSO2_MSGHANDLER_ENGINE_VERSION, nullptr));
	}

	PLH::CapstoneDisassembler dis(PLH::Mode::x86);

	g_pServerAddrHook = SetupDetourHook(
		dwEngineBase + 0x285FE0, &hkGetServerInfo, &g_ServerAddrOrig, dis);

	g_pServerAddrHook->hook();

	loaded = true;
}
#pragma optimize( "", on )