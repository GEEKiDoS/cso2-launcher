// MainPatch.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <wincred.h>

PluginImports *g_pImports;
static std::string libList[] = { "engine.dll", "client.dll" };

#define PLUGIN_NAME "Login"
#define PLUGIN_AUTHOR "GEEKiDoS"
#define PLUGIN_VERSION "0.0.1"

void PatchCSO2_Engine(uintptr_t dwEngineBase);
void PatchCSO2_Client(uintptr_t dwClientBase);

void OnLibLoaded(std::string libName, uintptr_t dwLibraryBase)
{
	g_pImports->Print(va("%s loaded at %llx", libName.c_str(), dwLibraryBase));

	if (libList[0].compare(libName) == 0)
	{
		PatchCSO2_Engine(dwLibraryBase);
	}
	else if (libList[1].compare(libName) == 0)
	{
		PatchCSO2_Client(dwLibraryBase);
	}
}

char g_szUsername[CREDUI_MAX_USERNAME_LENGTH];
char g_szPassword[CREDUI_MAX_PASSWORD_LENGTH];
void Auth_VerifyIdentity();

extern "C" {
	__declspec(dllexport) bool _cdecl InitPlugin(PluginImports *pImports)
	{
		if (g_pImports) {
			return false;
		}

		// Import functions from launcher
		g_pImports = pImports;

		// Register the libaray loaded callback
		g_pImports->RegisterLoadLibCallbacks(libList, 2, OnLibLoaded);

		g_pImports->Print(va("%s Inited.\n", PLUGIN_NAME ));

		g_pImports->Print("Requesting username and password");
		Auth_VerifyIdentity();

		return true;
	}

	__declspec(dllexport) std::string _cdecl GetPluginName()
	{
		return PLUGIN_NAME;
	}

	__declspec(dllexport) std::string _cdecl GetPluginAuthor()
	{
		return PLUGIN_AUTHOR;
	}

	__declspec(dllexport) std::string _cdecl GetPluginVersion()
	{
		return PLUGIN_VERSION;
	}
}