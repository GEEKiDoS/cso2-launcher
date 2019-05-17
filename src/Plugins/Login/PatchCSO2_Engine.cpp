#include "stdafx.h"
#include "CJsonObject.hpp"

#include "tier0/icommandline.h"

#include <hooks.h>
#include <string>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <WinSock2.h>

using namespace std::literals::string_literals;

struct IpAddressInfo
{
	std::string szIpAddress;
	uint16_t iPort;
};

HOOK_DETOUR_DECLARE(hkGetServerIpAddressInfo);

NOINLINE void __fastcall hkGetServerIpAddressInfo(IpAddressInfo& info)
{
	info.szIpAddress = MASTER_SERVER_ADDRESS;
	info.iPort = MASTER_SERVER_PORT;

	std::ifstream config("..\\cso2.json");

	if (config) {
		neb::CJsonObject cfgJson(std::string((std::istreambuf_iterator<char>(config)), std::istreambuf_iterator<char>()));
		config.close();

		std::string port;
		if (cfgJson.Get("masterport", port)) {
			info.iPort = atoi(port.c_str());
		}

		cfgJson.Get("masterserver", info.szIpAddress);

		uint32_t ipAddr[4];
		if (sscanf(info.szIpAddress.c_str(), "%d.%d.%d.%d", &ipAddr[0], &ipAddr[1], &ipAddr[2], &ipAddr[3]) == 4) 
		{
			if (ipAddr[0] < 255 && ipAddr[1] < 255 && ipAddr[2] < 255 && ipAddr[3] < 255)
				return; // A vaild ip address, good to go.
		}

		// Not a ip address, resolve dns name
		auto pHost = gethostbyname(info.szIpAddress.c_str());

		if (pHost == NULL) {
			auto error = WSAGetLastError();
			switch (error)
			{
			case WSAHOST_NOT_FOUND:
				printf("Host %s not found\n", info.szIpAddress.c_str());
				break;
			case WSANO_DATA:
				printf("No data record found\n");
				break;
			default:
				printf("Function failed with error: %ld\n", error);
				break;
			}

			info.szIpAddress = MASTER_SERVER_ADDRESS;
		} 
		else
		{
			int i = 0;
			if (pHost->h_addrtype == AF_INET)
			{
				in_addr addr;
				while (pHost->h_addr_list[i] != 0) {
					addr.s_addr = *(u_long *)pHost->h_addr_list[i++];
					info.szIpAddress = inet_ntoa(addr);

					return;
				}
			}
			else
			{
				printf("%s is not a vaild internet master server", info.szIpAddress.c_str());
				info.szIpAddress = MASTER_SERVER_ADDRESS;
			}
		}
	}
}
	

ICSO2MsgHandlerEngine* g_pCSO2MsgHandler;

#pragma optimize( "", off )
void PatchCSO2_Engine(uintptr_t dwEngineBase)
{
	static bool loaded = false;

	if (loaded)
		return;

	g_pImports->Print("Login - Appling engine patches. ");

	if (!g_pCSO2MsgHandler)
	{
		g_pCSO2MsgHandler = (ICSO2MsgHandlerEngine*)(dwEngineBase + 0xAA8190);
	}

	HOOK_DETOUR(dwEngineBase + 0x285FE0, hkGetServerIpAddressInfo);

	loaded = true;
}
#pragma optimize( "", on )