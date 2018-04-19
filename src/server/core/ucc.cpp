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
 * Method ChannelManager.send
 */
static jmethodID s_channelManagerSend = NULL;

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
   s_channelManagerSend = env->GetStaticMethodID(s_channelManagerClass, "send", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z");
   if ((initialize == NULL) || (s_channelManagerSend == NULL))
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

/**
 * Send message to user communication channel
 */
bool SendMessageToUserCommunicationChannel(const TCHAR *channel, const TCHAR *recipient, const TCHAR *subject, const TCHAR *message)
{
   bool success = false;

   JNIEnv *env = AttachThreadToJavaVM();

   jstring jchannel = JavaStringFromCString(env, channel);
   jstring jrecipient = JavaStringFromCString(env, recipient);
   jstring jsubject = JavaStringFromCString(env, subject);
   jstring jmessage = JavaStringFromCString(env, message);
   if ((jchannel != NULL) && (jrecipient != NULL) && (jmessage != NULL))  // subject can be NULL
   {
      success = env->CallStaticBooleanMethod(s_channelManagerClass, s_channelManagerSend, jchannel, jrecipient, jsubject, jmessage) ? true : false;
      if (success)
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Message to %s sent via channel %s"), recipient, channel);
      else
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot send message to %s via channel %s (channel error)"), recipient, channel);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot call communication channel (at least one required argument missing)"));
   }
   DeleteJavaLocalRef(env, jchannel);
   DeleteJavaLocalRef(env, jrecipient);
   DeleteJavaLocalRef(env, jsubject);
   DeleteJavaLocalRef(env, jmessage);

   DetachThreadFromJavaVM();
   return success;
}
