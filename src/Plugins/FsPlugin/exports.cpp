// MainPatch.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"

PluginImports *g_pImports;
static std::string libList[] = { "filesystem_stdio.dll" };

#define PLUGIN_NAME "FileSystem Plugin"
#define PLUGIN_AUTHOR "GEEKiDoS"
#define PLUGIN_VERSION "0.0.1"

void PatchCSO2_Filesystem(uintptr_t dwEngineBase);

void OnLibLoaded(std::string libName, uintptr_t dwLibraryBase)
{
	if (libList[0].compare(libName) == 0)
	{
		PatchCSO2_Filesystem(dwLibraryBase);
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
		g_pImports->RegisterLoadLibCallbacks(libList, 1, OnLibLoaded);

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