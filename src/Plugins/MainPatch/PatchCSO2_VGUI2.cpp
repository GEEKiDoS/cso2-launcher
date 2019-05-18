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

void* GetLocalizedStringTable(const uintptr_t base)
{
	return reinterpret_cast<void*>(base + 0x96FB0);
}

static std::unique_ptr<PLH::VTableSwapHook> g_pStrTblHook;
static PLH::VFuncMap g_StrTblOrig;

NOINLINE int __fastcall hkLocalToUtf8(void* thisptr, void*,
	const char* szInput, char* szOutBuffer,
	std::uint32_t iOutBufferSize)
{
	return HandleLocalConvertion(szInput, szOutBuffer, iOutBufferSize);
}

NOINLINE int __fastcall hkUtf8ToLocal(void* thisptr, void*,
	const char* szInput, char* szOutBuffer,
	std::uint32_t iOutBufferSize)
{
	return HandleLocalConvertion(szInput, szOutBuffer, iOutBufferSize);
}

NOINLINE int __fastcall hkWideCharToUtf8(void* thisptr, void*,
	const wchar_t* szInput,
	char* szOutBuffer,
	std::uint32_t iOutBufferSize)
{
	return ConvertWideCharToUtf8(szInput, szOutBuffer, iOutBufferSize);
}

NOINLINE int __fastcall hkUtf8ToWideChar(void* thisptr, void*,
	const char* szInput,
	wchar_t* szOutBuffer,
	std::uint32_t iOutBufferSize)
{
	return ConvertUtf8ToWideChar(szInput, szOutBuffer, iOutBufferSize);
}

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

	g_pImports->Print("Main Patch - Appling VGui2 patches. ");
	const void* pTableInstance = reinterpret_cast<void*>(dwVguiBase + 0x96FB0);

	// does multiple hooks in CLocalizedStringTable 
	static const PLH::VFuncMap Redirects = {
		{ uint16_t(20), reinterpret_cast<uint64_t>(&hkWideCharToUtf8) },
		{ uint16_t(21), reinterpret_cast<uint64_t>(&hkUtf8ToLocal) },
		{ uint16_t(22), reinterpret_cast<uint64_t>(&hkLocalToUtf8) },
		{ uint16_t(23), reinterpret_cast<uint64_t>(&hkUtf8ToWideChar) },
		{ uint16_t(24), reinterpret_cast<uint64_t>(&hkWideCharToUtf8_2) },
	};

	g_pStrTblHook = SetupVtableSwap(pTableInstance, Redirects);
	g_pStrTblHook->hook();
	g_StrTblOrig = g_pStrTblHook->getOriginals();
}
#pragma optimize( "", on )