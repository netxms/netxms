/* 
** NetXMS - Network Management System
** Windows Console
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: globals.cpp
** This file contain all global vartiables
**
**/

#include "stdafx.h"
#include "nxcon.h"


//
// Connection parameters
//

char g_szServer[MAX_SERVER_NAME_LEN] = "localhost";
char g_szLogin[MAX_LOGIN_NAME_LEN] = "";
char g_szPassword[MAX_PASSWORD_LEN] = "";


//
// Global string constants
//

char *g_szStatusText[] = { "NORMAL", "MINOR", "WARNING", "MAJOR", "CRITICAL", "UNKNOWN", "UNMANAGED" };
char *g_szStatusTextSmall[] = { "Normal", "Minor", "Warning", "Major", "Critical", "Unknown", "Unmanaged" };
