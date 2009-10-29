// libexpat.cpp : Defines the entry point for the DLL or NLM
//

#ifdef _WIN32

#include <windows.h>


BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	if (reason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hModule);
	return TRUE;
}

#endif


//
// NetWare library entry point
//

#ifdef _NETWARE

extern "C" int _init(void)
{
   return 0;
}

extern "C" int _fini(void)
{
   return 0;
}

#endif
