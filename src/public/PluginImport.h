#include <string>
#include "convar.h"

typedef void(*OnLoaded_t)(std::string libName, uintptr_t);

class PluginImports {
public:
	virtual void RegisterLoadLibCallbacks(std::string *libNames, int numLibs, OnLoaded_t callback) = 0;
	virtual void Print(std::string str) = 0;
	virtual ICvar *GetCVar() = 0;
};

inline const char *va(const char *fmt, ...)
{
	va_list VarArgs;
	int32_t StringLength = 0;
	static char DestinationBuffer[8192];

	va_start(VarArgs, fmt);
	StringLength = _vsnprintf_s(DestinationBuffer, 8192, _TRUNCATE, fmt, VarArgs);
	va_end(VarArgs);

	return DestinationBuffer;
}
