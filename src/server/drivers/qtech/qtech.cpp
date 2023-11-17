/*
 ** NetXMS - Network Management System
 ** Driver for Qtech switches
 **
 ** Licensed under MIT lisence, as stated by the original
 ** author: https://dev.raden.solutions/issues/779#note-4
 **
 ** Copyright (c) 2015 Procyshin Dmitriy
 ** Copyright (c) 2023 Raden Solutions
 ** Copyleft (l) 2023 Anatoly Rudnev
 **
 ** Permission is hereby granted, free of charge, to any person obtaining a copy of
 ** this software and associated documentation files (the “Software”), to deal in
 ** the Software without restriction, including without limitation the rights to
 ** use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 ** of the Software, and to permit persons to whom the Software is furnished to do
 ** so, subject to the following conditions:
 **
 ** The above copyright notice and this permission notice shall be included in all
 ** copies or substantial portions of the Software.
 **
 ** THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 ** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 ** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 ** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 ** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 ** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 ** SOFTWARE.
 **
 ** File: qtech.cpp
 **/

#include "qtech.h"

/**
 * Driver module entry point
 */
NDD_BEGIN_DRIVER_LIST
   NDD_DRIVER(QtechOLTDriver)
   NDD_DRIVER(QtechSWDriver)
   NDD_END_DRIVER_LIST
DECLARE_NDD_MODULE_ENTRY_POINT

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
