#include "stdafx.h"
#include <wincred.h>
#include "hooks.h"

extern char g_szUsername[CREDUI_MAX_USERNAME_LENGTH];
extern char g_szPassword[CREDUI_MAX_PASSWORD_LENGTH];

HOOK_DETOUR_DECLARE(hkCSO2UIManager_InitMainUI);
NOINLINE bool __fastcall hkCSO2UIManager_InitMainUI(void* ecx, void* edx)
{
	g_pCSO2MsgHandler->Login(g_szUsername, g_szPassword, g_szUsername);
	
	return HOOK_DETOUR_GET_ORIG(hkCSO2UIManager_InitMainUI)(ecx, edx);
}


#pragma optimize( "", off )
void PatchCSO2_Client(uintptr_t dwClientBase)
{
	static bool loaded = false;

	if (loaded)
		return;

	g_pImports->Print("Login - Appling client patches. ");

	HOOK_DETOUR(dwClientBase + 0xAE4610, hkCSO2UIManager_InitMainUI);

	loaded = true;
}
#pragma optimize( "", on )