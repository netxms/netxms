/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
** File: tunnel.cpp
**/

#include "nxcore.h"
#include <nxjava.h>

#define DEBUG_TAG _T("ucc.manager")

/**
 * Channel manager class
 */
static jclass s_channelManagerClass = NULL;

/**
 * Initialize user communication channels
 */
bool InitializeUserCommunicationChannels()
{
   JNIEnv *env = AttachThreadToJavaVM();

   s_channelManagerClass = CreateJavaClassGlobalRef(env, "org/netxms/server/ucc/ChannelManager");
   if (s_channelManagerClass == NULL)
   {
      nxlog_write(MSG_UCC_INITIALIZATION_FAILED, NXLOG_ERROR, "s", _T("Cannot find channel manager class"));
      return false;
   }

   jmethodID initialize = env->GetStaticMethodID(s_channelManagerClass, "initialize", "()Z");
   if (initialize == NULL)
   {
      nxlog_write(MSG_UCC_INITIALIZATION_FAILED, NXLOG_ERROR, "s", _T("Cannot find required entry points in channel manager class"));
      return false;
   }

   jboolean success = env->CallStaticBooleanMethod(s_channelManagerClass, initialize);
   if (!success)
   {
      nxlog_write(MSG_UCC_INITIALIZATION_FAILED, NXLOG_ERROR, "s", _T("Java side initialization failure"));
      return false;
   }

   nxlog_debug_tag(DEBUG_TAG, 1, _T("User communication channel manager initialized"));
   return true;
}
