#include <Windows.h>

#include "PluginLoader.h"
#include "hooks.h"

static bool ListFiles(std::string Path, std::vector<std::string>& Results, const char *Extension = nullptr)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	bool Result = false;

	// Append trailing slash.
	if (Path.back() != '\\')
	{
		Path.append("\\");
	}

	// Filename.
	Path.append("*");

	// Extension.
	if (Extension != nullptr)
	{
		if (*Extension != '.')
		{
			Path.append(".");
		}

		Path.append(Extension);
	}

	hFind = FindFirstFileA(Path.c_str(), &FindFileData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			// If not a directory.
			if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				Results.push_back(FindFileData.cFileName);
				Result = true;
			}
		} while (FindNextFileA(hFind, &FindFileData));
		FindClose(hFind);
	}

	return Result;
}

void LoadPlugins()
{
	std::vector<std::string> PluginFiles;
	if (!ListFiles("Plugins\\", PluginFiles, ".plg")) 
	{
		printf("Warning: no plugins installed.\n");
	}

	for (uint32_t i = 0; i < PluginFiles.size(); i++)
	{
		printf("Info: loading %s\n", PluginFiles[i].c_str());
		PluginFiles[i].insert(0, "Plugins\\");
		Plugin pluginToLoad(PluginFiles[i].c_str());

		if (pluginToLoad.loaded)
			g_Plugins.push_back(pluginToLoad);
	}
}

PluginExports *g_pExports = nullptr;

void InitPlugins()
{
	if (!g_pExports)
		g_pExports = new PluginExports;

	for (auto plugin : g_Plugins) 
	{
		plugin.Init(g_pExports);
	}
}

void PluginExports::Print(std::string str)
{
	if (str.c_str()[str.length() - 1] != '\n')
		printf("%s\n", str.c_str());
	else
		printf(str.c_str());
}

CreateInterfaceFn PluginExports::GetFactory(const char * pModuleName)
{
	return Sys_GetFactory(pModuleName);
}

ICvar * PluginExports::GetCVar()
{
	return g_pCVar;
}

Plugin::Plugin(std::string path)
{
	auto Library = LoadLibraryA(path.c_str());

	if (Library != NULL)
	{
		this->Init = (InitPlugin_t)GetProcAddress(Library, "InitPlugin");
		this->name = ((std::string(_cdecl *)(void))GetProcAddress(Library, "GetPluginName"))();
		this->author = ((std::string(_cdecl *)(void))GetProcAddress(Library, "GetPluginAuthor"))();
		this->version = ((std::string(_cdecl *)(void))GetProcAddress(Library, "GetPluginVersion"))();
		this->loaded = true;

		printf("Plugin %s v%s by %s loaded.\n", name.c_str(), version.c_str(), author.c_str());
	}
	else 
	{
		this->loaded = false;
		printf("Error loading plugin %s\n", path.c_str());
	}
}