#include "strtools.h"
#include "hooks.h"
#include "onloadlib.h"
#include "PluginExports.h"

std::vector<std::pair<std::string, OnLoaded_t>> mapLibs;

void PluginExports::RegisterLoadLibCallbacks(std::string *libNames, int numLibs, OnLoaded_t callback)
{
	for (auto i = 0; i < numLibs; i++)
	{
		mapLibs.push_back(std::pair<std::string, OnLoaded_t>(libNames[i], callback));
	}
}

static std::unique_ptr<PLH::x86Detour> g_pLoadLibExHook;
static uint64_t g_LoadLibExOrig = NULL;

NOINLINE HMODULE WINAPI hkLoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
	HMODULE hLoadedModule = PLH::FnCast(g_LoadLibExOrig, hkLoadLibraryExA)(lpLibFileName, hFile, dwFlags);
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
	PLH::CapstoneDisassembler dis(PLH::Mode::x86);

	g_pLoadLibExHook =
		SetupDetourHook(reinterpret_cast<uintptr_t>(&LoadLibraryExA),
			&hkLoadLibraryExA, &g_LoadLibExOrig, dis);
	g_pLoadLibExHook->hook();
}
