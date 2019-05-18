#include "stdafx.h"
#include "hooks.h"

#include <direct.h>
#include <filesystem>

std::filesystem::path GetCustomPath(const char* pFileName)
{
	char buff[FILENAME_MAX];
	_getcwd(buff, FILENAME_MAX);
	std::filesystem::path path(buff);

	while (*pFileName == '/' || *pFileName == '\\' ||
		*pFileName == '*')  // Clean path
		pFileName++;

	path /= "..\\custom";
	path /= pFileName;

	return path;
}

static std::unique_ptr<PLH::x86Detour> g_pOpenExHook;
static uint64_t g_OpenExOrig = NULL;

NOINLINE void* __fastcall hkFsOpenEx(void* ecx, void* edx,
	const char* pFileName,
	const char* pOptions, unsigned flags,
	const char* pathID,
	char** ppszResolvedFilename)
{
	auto path = GetCustomPath(pFileName);

	std::string_view curFile(pFileName);

	if (std::filesystem::exists(path))
	{
		curFile = path.string();
	}

	return PLH::FnCast(g_OpenExOrig, hkFsOpenEx)(ecx, edx, curFile.data(),
		pOptions, flags, pathID,
		ppszResolvedFilename);
}

static std::unique_ptr<PLH::x86Detour> g_pOpenExGfxHook;
static uint64_t g_OpenExGfxOrig = NULL;

NOINLINE void* __fastcall hkFsOpenExGfx(void* ecx, void* edx,
	const char* pFileName,
	const char* pOptions, unsigned flags,
	const char* pathID,
	char** ppszResolvedFilename)
{
	return hkFsOpenEx(ecx, edx, pFileName, pOptions, flags, pathID,
		ppszResolvedFilename);
}

const char* path = 0;

static std::unique_ptr<PLH::x86Detour> g_pFileEncHook;
static uint64_t g_FileEncOrig = NULL;

NOINLINE bool __fastcall hkIsFileEncrypted(void* ecx, void* edx)
{
	if (std::filesystem::exists(GetCustomPath(path)))
	{
		return false;
	}

	return PLH::FnCast(g_FileEncOrig, hkIsFileEncrypted)(ecx, edx);
}

static std::unique_ptr<PLH::x86Detour> g_pGetFileExtHook;
static uint64_t g_GetFileExtOrig = NULL;

//
// this will be called before call the IsFileEncrypted, and ecx is the pointer
// of file name.
//
NOINLINE int __fastcall hkGetFileExt(const char** ecx, void* edx,
	const char* file, size_t path_size,
	size_t ext_size)
{
	path = *ecx;
	return PLH::FnCast(g_GetFileExtOrig, hkGetFileExt)(ecx, edx, file,
		path_size, ext_size);
}

#pragma optimize( "", off )
void PatchCSO2_Filesystem(uintptr_t dwFsBase)
{
	static bool loaded = false;

	if (loaded)
		return;

	if (std::filesystem::exists(GetCustomPath("")))
	{
		g_pImports->Print("Filesystem Plugin - Custom exists, appling Filesystem patches. ");
		PLH::CapstoneDisassembler dis(PLH::Mode::x86);

		g_pOpenExHook = SetupDetourHook(dwFsBase + 0x46D90, &hkFsOpenEx,
			&g_OpenExOrig, dis);
		g_pOpenExGfxHook = SetupDetourHook(dwFsBase + 0x46C60, &hkFsOpenExGfx,
			&g_OpenExGfxOrig, dis);
		g_pFileEncHook = SetupDetourHook(
			dwFsBase + 0x6A3B0, &hkIsFileEncrypted, &g_FileEncOrig, dis);
		g_pGetFileExtHook = SetupDetourHook(dwFsBase + 0x6A4E0, &hkGetFileExt,
			&g_GetFileExtOrig, dis);

		g_pOpenExHook->hook();
		g_pOpenExGfxHook->hook();
		g_pFileEncHook->hook();
		g_pGetFileExtHook->hook();
	}

	loaded = true;
}
#pragma optimize( "", on )