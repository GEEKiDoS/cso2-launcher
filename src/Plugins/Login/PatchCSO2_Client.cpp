#include "stdafx.h"
#include <wincred.h>
#include "hooks.h"

#pragma optimize( "", off )
extern char g_szUsername[CREDUI_MAX_USERNAME_LENGTH];
extern char g_szPassword[CREDUI_MAX_PASSWORD_LENGTH];

static std::unique_ptr<PLH::x86Detour> g_pInitUIHook;
static uint64_t g_InitUIOrig = NULL;

NOINLINE bool __fastcall hkCSO2UIManager_InitMainUI(void* ecx, void* edx)
{
	g_pCSO2MsgHandler->Login(g_szUsername, g_szPassword, g_szUsername);

	return PLH::FnCast(g_InitUIOrig, &hkCSO2UIManager_InitMainUI)(ecx, edx);
}

void PatchCSO2_Client(uintptr_t dwClientBase)
{
	static bool loaded = false;

	if (loaded)
		return;

	g_pImports->Print("Login - Appling client patches. ");

	PLH::CapstoneDisassembler dis(PLH::Mode::x86);

	g_pInitUIHook =
		SetupDetourHook(dwClientBase + 0xAE4610, &hkCSO2UIManager_InitMainUI, &g_InitUIOrig, dis);
	g_pInitUIHook->hook();

	loaded = true;
}
#pragma optimize( "", on )