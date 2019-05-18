// MainPatch.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"

PluginImports *g_pImports;
static std::string libList[] = { "engine.dll", "vgui2.dll" };

#define PLUGIN_NAME "Main Patch for CSO2"
#define PLUGIN_AUTHOR "GEEKiDoS && Ochii"
#define PLUGIN_VERSION "0.1.0"

void PatchCSO2_Engine(uintptr_t dwEngineBase);
void PatchCSO2_VGUI2(uintptr_t dwVguiBase);

void OnLibLoaded(std::string libName, uintptr_t dwLibraryBase)
{
	if (libList[0].compare(libName) == 0)
	{
		PatchCSO2_Engine(dwLibraryBase);
	}
	else if (libList[1].compare(libName) == 0)
	{
		PatchCSO2_VGUI2(dwLibraryBase);
	}
}

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