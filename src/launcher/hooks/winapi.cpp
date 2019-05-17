#include "strtools.h"
#include "hooks.h"
#include "onloadlib.h"
#include "PluginExports.h"

std::map<std::string, OnLoaded_t> mapLibs;

void PluginExports::RegisterLoadLibCallbacks(std::string *libNames, int numLibs, OnLoaded_t callback)
{
	for (auto i = 0; i < numLibs; i++)
	{
		mapLibs.insert(std::pair<std::string, OnLoaded_t>(libNames[i], callback));
	}
}

HOOK_DETOUR_DECLARE(hkLoadLibraryExA);

NOINLINE HMODULE WINAPI hkLoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
	HMODULE hLoadedModule = HOOK_DETOUR_GET_ORIG(hkLoadLibraryExA)(lpLibFileName, hFile, dwFlags);
	CLoadLibCallbacks::OnLoadLibrary(V_GetFileName(lpLibFileName), (uintptr_t)hLoadedModule);	

	// convert the library path to just the library file name

	const uintptr_t dwLibraryBase = reinterpret_cast<uintptr_t>(hLoadedModule);

	for (auto libCallbackPair : mapLibs)
	{
		if (libCallbackPair.first.compare(V_GetFileName(lpLibFileName)) == 0)
			libCallbackPair.second(V_GetFileName(lpLibFileName), dwLibraryBase);
	}

	return hLoadedModule;
}

void HookWinapi()
{
	HOOK_DETOUR(LoadLibraryExA, hkLoadLibraryExA);
}
