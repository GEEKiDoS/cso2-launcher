#pragma once
#include <PluginImport.h>

class PluginExports : public PluginImports
{
public:
	void RegisterLoadLibCallbacks(std::string *libNames, int numLibs, OnLoaded_t callback);
	void Print(std::string str);
	CreateInterfaceFn GetFactory(const char *pModuleName);
	ICvar *GetCVar();
};