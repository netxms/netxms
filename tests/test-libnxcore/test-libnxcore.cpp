/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: test-libnxcore.cpp
**
** Unit test runner for server core library (libnxcore). Only utility and supplemental
** functions and classes that do not depend on an initialized server (database
** connection pool, loaded objects, user database, configuration cache) belong here.
** Tests that need a running server go to the test-server launcher.
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nms_core.h>
#include <testtools.h>

void TestAICheckLogic();
void TestPhysicalPlacement();

/**
 * main()
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

#ifdef _WIN32
   WSADATA wsaData;
   WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

   TestAICheckLogic();
   TestPhysicalPlacement();

   return 0;
}
