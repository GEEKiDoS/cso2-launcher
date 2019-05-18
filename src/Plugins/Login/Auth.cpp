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
#include "CJsonObject.hpp"

#include <wincred.h>
#include <Shlwapi.h>
#include <shellapi.h>

#include <curl/curl.h>
#include <curl/system.h>
#include <curl/easy.h>

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

		std::string sUsername, sPassword;

		if (cfgJson.Get("username", sUsername) && cfgJson.Get("password", sPassword)) {
			strcpy(g_szUsername, sUsername.c_str());
			strcpy(g_szPassword, sPassword.c_str());

			doneFile = true;
			return true;
		}
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

		if (save) {
			std::ofstream configwrite("..\\cso2.json");
			if (configwrite) {
				std::ostringstream sstm;
				sstm << "{\"username\":\"" << username << "\", \"password\":\"" << password << "\"}";

				auto str = sstm.str();
				configwrite.write(str.c_str(), str.length());

				configwrite.close();
			}
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

static int curlDebug(CURL *, curl_infotype type, char * data, size_t size, void *)
{
	if (type == CURLINFO_TEXT)
	{
		char text[8192];
		memcpy(text, data, size);
		text[size] = '\0';

		OutputDebugString(va("%s\n", text));
	}

	return 0;
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

std::wstring AsciiToUnicode(const std::string& str)
{
	int unicodeLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
	wchar_t *pUnicode = (wchar_t*)malloc(sizeof(wchar_t)*unicodeLen);
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, pUnicode, unicodeLen);
	std::wstring ret_str = pUnicode;
	free(pUnicode);
	return ret_str;
}

std::string UnicodeToUtf8(const std::wstring& wstr)
{  
	int ansiiLen = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	char *pAssii = (char*)malloc(sizeof(char)*ansiiLen);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, pAssii, ansiiLen, nullptr, nullptr);
	std::string ret_str = pAssii;
	free(pAssii);
	return ret_str;
}


std::string AsciiToUtf8(const std::string& str)
{
	return UnicodeToUtf8(AsciiToUnicode(str));
}

bool Auth_PerformSessionLogin(const char* username, const char* password)
{
	curl_global_init(CURL_GLOBAL_ALL);

	CURL* curl = curl_easy_init();

	if (curl)
	{
		std::ostringstream surl;
		surl << "http://" << MASTER_SERVER_ADDRESS << ":" << USER_SERVICE_PORT << "/users/session";

		char jsonToSend[128] = { 0 };
		snprintf(jsonToSend, 128, "{\"username\":\"%s\",\"password\":\"%s\"}", username, password);

		auto utf8Json = AsciiToUtf8(jsonToSend);

		char buf[8192] = { 0 };

		auto headers = curl_slist_append(NULL, "Content-Type:application/json;charset=UTF-8");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_URL, surl.str().c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, AuthDataReceived);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&buf);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "CSO2_LOGIN");
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, utf8Json.c_str());
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, curlDebug);

		CURLcode code = curl_easy_perform(curl);
		
		
		if (code == CURLE_OK)
		{
			long retcode = 0;
			if (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &retcode) == CURLE_OK) 
			{
				if (retcode == 201 || retcode == 200)
				{
					neb::CJsonObject retJson(buf);
					std::ostringstream sdelparams;
					sdelparams << "userId=" << retJson["userId"].ToString().c_str();

					curl_easy_cleanup(curl);
					curl = curl_easy_init();

					curl_easy_setopt(curl, CURLOPT_URL, surl.str().c_str());
					curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
					curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sdelparams.str().c_str());
					curl_easy_setopt(curl, CURLOPT_USERAGENT, "CSO2_LOGIN");
					curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
					curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

					code = curl_easy_perform(curl);
					if (code == CURLE_OK)
					{
						if (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &retcode) == CURLE_OK) 
						{
							if (retcode == 200)
							{
								curl_easy_cleanup(curl);
								curl_global_cleanup();
								return true;
							}
						}
					}

					curl_easy_cleanup(curl);
					Auth_Error("Failed to delete user session!");
					ExitProcess(0x8000D3AD);
				}
			}
		}
		else if (code == 0x16)
		{
			long retcode = 0;
			if (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &retcode) == CURLE_OK)
			{
				if (retcode == 409)
				{
					Auth_Error("You already logged in!");
				}
				else if (retcode == 401)
				{
					Auth_Error("Server denied your username and password, check them again!");
				}
				else if (retcode == 500)
				{
					Auth_Error("Server internal error");
				}
				else if (retcode == 400)
				{
					Auth_Error(va("WTF?\r\nPost url:%s\r\nPost json:%s", surl.str().c_str(), jsonToSend));
					throw "WTF that is impossible";
				}
			}
		}
		else Auth_Error(va("Could not reach the cso2 server. Error code from CURL: %x.", code));

		curl_easy_cleanup(curl);
		curl_global_cleanup();

		return false;
	}

	curl_global_cleanup();
	return false;
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
			if (!Auth_PerformSessionLogin(g_szUsername, g_szPassword))
			{
				continue;
			}
			return;
		}
	}

	ExitProcess(0x8000D3AD);
}
#pragma optimize("", on)