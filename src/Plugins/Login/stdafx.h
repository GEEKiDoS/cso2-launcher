// stdafx.h: 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 项目特定的包含文件
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
// Windows 头文件
#include <windows.h>
#include <PluginImport.h>
#include "engine/cso2/icso2msgmanager.h"

extern PluginImports *g_pImports;
extern ICSO2MsgHandlerEngine* g_pCSO2MsgHandler;

#define MASTER_SERVER_ADDRESS "123.231.231.120"s
#define MASTER_SERVER_PORT 30001