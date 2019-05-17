// ==========================================================
// alterOps project
// 
// Component: t5cli
// Sub-component: clientdll
// Purpose: Authorization to the aO web service.
//
// Initial author: NTAuthority
// Started: 2011-08-21 (finally...)
// ==========================================================

#undef UNICODE
#define NTDDI_VERSION 0x06000000
#include "stdafx.h"
#include <wincred.h>
#include <Shlwapi.h>
#include <shellapi.h>
#include "CJsonObject.hpp"

#include <string>
#include <fstream>
#include <streambuf>
#include <sstream>

extern char g_szUsername[CREDUI_MAX_USERNAME_LENGTH];
extern char g_szPassword[CREDUI_MAX_PASSWORD_LENGTH];

#define CURL_STATICLIB

typedef DWORD(WINAPI * CredUIPromptForWindowsCredentialsW_t)(
	__in_opt PCREDUI_INFOW pUiInfo,
	__in DWORD dwAuthError,
	__inout ULONG *pulAuthPackage,
	__in_bcount_opt(ulInAuthBufferSize) LPCVOID pvInAuthBuffer,
	__in ULONG ulInAuthBufferSize,
	__deref_out_bcount_full(*pulOutAuthBufferSize) LPVOID * ppvOutAuthBuffer,
	__out ULONG * pulOutAuthBufferSize,
	__inout_opt BOOL *pfSave,
	__in DWORD dwFlags
	);

typedef BOOL(WINAPI * CredUnPackAuthenticationBufferW_t)(
	__in DWORD                                      dwFlags,
	__in_bcount(cbAuthBuffer) PVOID                 pAuthBuffer,
	__in DWORD                                      cbAuthBuffer,
	__out_ecount_opt(*pcchMaxUserName) LPWSTR       pszUserName,
	__inout DWORD*                                  pcchMaxUserName,
	__out_ecount_opt(*pcchMaxDomainName) LPWSTR     pszDomainName,
	__inout_opt DWORD*                              pcchMaxDomainName,
	__out_ecount_opt(*pcchMaxPassword) LPWSTR       pszPassword,
	__inout DWORD*                                  pcchMaxPassword
	);

bool Auth_DisplayLoginDialog()
{
	static char username[CREDUI_MAX_USERNAME_LENGTH];
	static char password[CREDUI_MAX_PASSWORD_LENGTH];

	static bool doneFile = false;

	std::ifstream config("..\\cso2.json");

	if (!doneFile && config) {
		neb::CJsonObject cfgJson(std::string((std::istreambuf_iterator<char>(config)), std::istreambuf_iterator<char>()));
		config.close();

		strcpy(username, cfgJson["username"].ToString().c_str());
		strcpy(password, cfgJson["password"].ToString().c_str());

		strcpy(g_szUsername, username);
		strcpy(g_szPassword, password);

		doneFile = true;
		return true;
	}

	doneFile = true; //prevent opening file twice on wrong info

	HMODULE credUI = LoadLibrary("credui.dll");

	if (!credUI)
	{
		// wtf
		__asm int 3
	}

	CredUIPromptForWindowsCredentialsW_t credUIPromptForWindowsCredentialsW = (CredUIPromptForWindowsCredentialsW_t)GetProcAddress(credUI, "CredUIPromptForWindowsCredentialsW");

	CREDUI_INFOW info;
	memset(&info, 0, sizeof(info));
	info.cbSize = sizeof(info);
	info.pszCaptionText = L"Authentication for CSO2";
	info.pszMessageText = L"Please log in to play CSO2. Use your username and password.";

	CredUnPackAuthenticationBufferW_t credUnPackAuthenticationBufferW = (CredUnPackAuthenticationBufferW_t)GetProcAddress(credUI, "CredUnPackAuthenticationBufferW");

	DWORD uLen = CREDUI_MAX_USERNAME_LENGTH;
	DWORD pLen = CREDUI_MAX_PASSWORD_LENGTH;
	LPVOID outStuff;
	ULONG outSize = 0;
	ULONG outPackage = 0;
	static BOOL save = false;
	//CredUIPromptForCredentials(&info, _T("Target"), NULL, NULL, username, CREDUI_MAX_USERNAME_LENGTH, password, CREDUI_MAX_PASSWORD_LENGTH, &save, CREDUI_FLAGS_GENERIC_CREDENTIALS | CREDUI_FLAGS_SHOW_SAVE_CHECK_BOX | CREDUI_FLAGS_ALWAYS_SHOW_UI | CREDUI_FLAGS_DO_NOT_PERSIST | CREDUI_FLAGS_EXCLUDE_CERTIFICATES);

	HRESULT result = credUIPromptForWindowsCredentialsW(&info, 0, &outPackage, NULL, 0, &outStuff, &outSize, &save, CREDUIWIN_GENERIC | CREDUIWIN_CHECKBOX);

	static WCHAR usernamew[CREDUI_MAX_USERNAME_LENGTH * sizeof(WCHAR)] = { 0 };
	static WCHAR passwordw[CREDUI_MAX_PASSWORD_LENGTH * sizeof(WCHAR)] = { 0 };

	if (result == ERROR_SUCCESS)
	{
		credUnPackAuthenticationBufferW(0, outStuff, outSize, usernamew, &uLen, NULL, 0, passwordw, &pLen);

		WideCharToMultiByte(CP_UTF8, 0, usernamew, -1, username, sizeof(username), NULL, NULL);
		WideCharToMultiByte(CP_UTF8, 0, passwordw, -1, password, sizeof(password), NULL, NULL);

		strcpy(g_szUsername, username);
		strcpy(g_szPassword, password);

		std::ofstream configwrite("..\\cso2.json");

		if (save && configwrite) {
			std::ostringstream sstm;
			sstm << "{\"username\":\"" << username << "\", \"password\":\"" << password << "\"}";

			auto str = sstm.str();
			configwrite.write(str.c_str(), str.length());

			configwrite.close();
		}
	}

	return (result == ERROR_SUCCESS);
}

void Auth_Error(const char* message)
{
	//->int btn;
	static wchar_t buf[32768];
	MultiByteToWideChar(CP_UTF8, 0, message, -1, buf, sizeof(buf));

	MessageBoxW(NULL, buf, L"Error", MB_OK | MB_ICONSTOP);
}

size_t AuthDataReceived(void *ptr, size_t size, size_t nmemb, void *data)
{
	if ((strlen((char*)data) + (size * nmemb)) > 8192)
	{
		return (size * nmemb);
	}

	strncat((char*)data, (const char*)ptr, size * nmemb);
	return (size * nmemb);
}

#pragma optimize("", off)
void Auth_VerifyIdentity()
{
	bool canceled = false;

	while (!canceled)
	{
		canceled = !Auth_DisplayLoginDialog();

		if (!canceled)
		{
			return;
		}
	}

	ExitProcess(0x8000D3AD);
}
#pragma optimize("", on)