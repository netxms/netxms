#include "libipfix.h"


//
// Mutex wrappers
//

LIBIPFIX_MUTEX mutex_create()
{
	return MutexCreate();
}

void mutex_destroy(LIBIPFIX_MUTEX mutex)
{
	MutexDestroy((MUTEX)mutex);
}

BOOL mutex_lock(LIBIPFIX_MUTEX mutex)
{
	return MutexLock((MUTEX)mutex);
}

void mutex_unlock(LIBIPFIX_MUTEX mutex)
{
	MutexUnlock((MUTEX)mutex);
}


//
// DLL entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif   /* _WIN32 */
