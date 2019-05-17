#pragma once
#include "PluginExports.h"
#include <vector>

void LoadPlugins();
void InitPlugins();

typedef bool(*InitPlugin_t)(PluginImports *pImports);

class Plugin
{
public:
	Plugin(std::string path);
	
	InitPlugin_t Init = nullptr;
	std::string name;
	std::string author;
	std::string version;

	bool loaded;
};

static std::vector<Plugin> g_Plugins;