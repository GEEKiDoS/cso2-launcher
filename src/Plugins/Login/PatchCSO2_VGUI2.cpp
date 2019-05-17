#include "stdafx.h"

#include <sstream>
#include <string>

#include "hooks.h"
#include "tier0/icommandline.h"

// the game converts the szInput (as an UTF8 string) to a wide char string
// and converts it back to a single byte string (using the OS's code page)
//
// example: converting string with hangul characters on an OS with a german code
// page would result in gibberish
//
// this fixes that by just copying the utf8 string to the out buffer
int HandleLocalConvertion(const char* szInput, char* szOutBuffer,
	const std::uint32_t iOutBufferSize)
{
	const std::uint32_t iStrLen = std::strlen(szInput) + 1;

	// make sure we don't overflow
	if (iStrLen > iOutBufferSize)
	{
		assert(false);
		return 0;
	}

	std::strncpy(szOutBuffer, szInput, iStrLen);

	return iStrLen;
}

int ConvertWideCharToUtf8(const wchar_t* szInput, char* szOutput,
	const std::uint32_t iOutBufferSize)
{
	int iLength = WideCharToMultiByte(CP_UTF8, NULL, szInput, -1, szOutput,
		iOutBufferSize, nullptr, nullptr);
	szOutput[iOutBufferSize - 1] = L'\0';
	return iLength;
}

int ConvertUtf8ToWideChar(const char* szInput, wchar_t* szOutput,
	const std::uint32_t iOutBufferSize)
{
	int iLength = MultiByteToWideChar(CP_UTF8, NULL, szInput, -1, szOutput,
		iOutBufferSize);
	szOutput[iOutBufferSize - 1] = L'\0';
	return iLength;
}

HOOK_DETOUR_DECLARE(hkLocalToUtf8);

NOINLINE int __fastcall hkLocalToUtf8(void* thisptr, void*,
	const char* szInput, char* szOutBuffer,
	std::uint32_t iOutBufferSize)
{
	return HandleLocalConvertion(szInput, szOutBuffer, iOutBufferSize);
}

HOOK_DETOUR_DECLARE(hkUtf8ToLocal);

NOINLINE int __fastcall hkUtf8ToLocal(void* thisptr, void*,
	const char* szInput, char* szOutBuffer,
	std::uint32_t iOutBufferSize)
{
	return HandleLocalConvertion(szInput, szOutBuffer, iOutBufferSize);
}

HOOK_DETOUR_DECLARE(hkWideCharToUtf8);

NOINLINE int __fastcall hkWideCharToUtf8(void* thisptr, void*,
	const wchar_t* szInput,
	char* szOutBuffer,
	std::uint32_t iOutBufferSize)
{
	return ConvertWideCharToUtf8(szInput, szOutBuffer, iOutBufferSize);
}

HOOK_DETOUR_DECLARE(hkUtf8ToWideChar);

NOINLINE int __fastcall hkUtf8ToWideChar(void* thisptr, void*,
	const char* szInput,
	wchar_t* szOutBuffer,
	std::uint32_t iOutBufferSize)
{
	return ConvertUtf8ToWideChar(szInput, szOutBuffer, iOutBufferSize);
}

HOOK_DETOUR_DECLARE(hkWideCharToUtf8_2);

NOINLINE int __fastcall hkWideCharToUtf8_2(void* thisptr, void*,
	const wchar_t* szInput,
	char* szOutBuffer,
	std::uint32_t iOutBufferSize)
{
	return ConvertWideCharToUtf8(szInput, szOutBuffer, iOutBufferSize);
}

#pragma optimize( "", off )
void PatchCSO2_VGUI2(uintptr_t dwVguiBase)
{
	static bool loaded = false;

	if (loaded)
		return;

	g_pImports->Print("Appling engine patches. ");
	HOOK_DETOUR(dwVguiBase + 0xB3F0, hkWideCharToUtf8);
	HOOK_DETOUR(dwVguiBase + 0xB420, hkUtf8ToLocal);
	HOOK_DETOUR(dwVguiBase + 0xB4A0, hkLocalToUtf8);
	HOOK_DETOUR(dwVguiBase + 0xB520, hkUtf8ToWideChar);
	HOOK_DETOUR(dwVguiBase + 0xB550, hkWideCharToUtf8_2);
}
#pragma optimize( "", on )