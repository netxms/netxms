/*
  windll.cpp - Windows-specific stuff

  This software is released under a BSD-style license.
  See the file LICENSE for details and copyright.

*/

#include <windows.h>

//
// This include needed to cause VC++ 2005 to use
// C++ runtime library MSVCPRT(D) which contains
// implementation of function wctype
//
#include <cwchar>


//
// DLL entry point
//

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}
