/*
** NetXMS Asterisk subagent
** Copyright (C) 2004-2021 Victor Kirhenshtein
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
** File: sip.cpp
**
**/

#include "asterisk.h"

/**
 * Handler for SIP peer list
 */
LONG H_SIPPeerList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(0);

   SharedObjectArray<AmiMessage> *messages = sys->readTable("SIPpeers");
   if (messages == nullptr)
      return SYSINFO_RC_ERROR;

   for(int i = 0; i < messages->size(); i++)
   {
      const char *name = messages->get(i)->getTag("ObjectName");
      if (name != nullptr)
         value->addMBString(name);
   }

   delete messages;
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for SIP peer table
 */
LONG H_SIPPeerTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(0);

   SharedObjectArray<AmiMessage> *messages = sys->readTable("SIPpeers");
   if (messages == nullptr)
      return SYSINFO_RC_ERROR;

   value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
   value->addColumn(_T("TYPE"), DCI_DT_STRING, _T("Type"));
   value->addColumn(_T("IP_ADDRESS"), DCI_DT_STRING, _T("IP Address"));
   value->addColumn(_T("IP_PORT"), DCI_DT_UINT, _T("IP Port"));
   value->addColumn(_T("STATUS"), DCI_DT_STRING, _T("Status"));
   value->addColumn(_T("DYNAMIC"), DCI_DT_STRING, _T("Dynamic"));
   value->addColumn(_T("AUTO_FORCEPORT"), DCI_DT_STRING, _T("Auto Force Port"));
   value->addColumn(_T("FORCE_PORT"), DCI_DT_STRING, _T("Force Port"));
   value->addColumn(_T("AUTO_COMEDIA"), DCI_DT_STRING, _T("Auto CoMedia"));
   value->addColumn(_T("COMEDIA"), DCI_DT_STRING, _T("CoMedia"));
   value->addColumn(_T("VIDEO_SUPPORT"), DCI_DT_STRING, _T("Video Support"));
   value->addColumn(_T("TEXT_SUPPORT"), DCI_DT_STRING, _T("Text Support"));
   value->addColumn(_T("REALTIME"), DCI_DT_STRING, _T("Realtime Device"));
   value->addColumn(_T("DESCRIPTION"), DCI_DT_STRING, _T("Description"));
   value->addColumn(_T("ACCOUNT_CODE"), DCI_DT_STRING, _T("Account Code"));

   for(int i = 0; i < messages->size(); i++)
   {
      AmiMessage *msg = messages->get(i);
      const char *name = msg->getTag("ObjectName");
      if (name == nullptr)
         continue;

      value->addRow();
      value->set(0, name);
      value->set(1, msg->getTag("ChanObjectType"));
      value->set(2, msg->getTag("IPaddress"));
      value->set(3, msg->getTag("IPport"));
      value->set(4, msg->getTag("Status"));
      value->set(5, msg->getTag("Dynamic"));
      value->set(6, msg->getTag("AutoForcerport"));
      value->set(7, msg->getTag("Forcerport"));
      value->set(8, msg->getTag("AutoComedia"));
      value->set(9, msg->getTag("Comedia"));
      value->set(10, msg->getTag("VideoSupport"));
      value->set(11, msg->getTag("TextSupport"));
      value->set(12, msg->getTag("RealtimeDevice"));
      value->set(13, msg->getTag("Description"));
      value->set(14, msg->getTag("Accountcode"));
   }

   delete messages;
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for SIP peer statistics
 */
LONG H_SIPPeerStats(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(0);

   SharedObjectArray<AmiMessage> *messages = sys->readTable("SIPpeers");
   if (messages == nullptr)
      return SYSINFO_RC_ERROR;

   if (*arg == 'T')
   {
      ret_int(value, messages->size());
   }
   else
   {
      int connected = 0, unknown = 0, unmonitored = 0, unreachable = 0;
      for(int i = 0; i < messages->size(); i++)
      {
         AmiMessage *msg = messages->get(i);
         if (msg->getTag("ObjectName") == nullptr)
            continue;
         const char *status = msg->getTag("Status");
         if ((status == nullptr) || !stricmp(status, "Unknown"))
         {
            unknown++;
         }
         else if (!stricmp(status, "Unmonitored"))
         {
            unmonitored++;
         }
         else if (!stricmp(status, "Unreachable"))
         {
            unreachable++;
         }
         else if (!strncmp(status, "OK", 2))
         {
            connected++;
         }
      }
      switch(*arg)
      {
         case 'C':
            ret_int(value, connected);
            break;
         case 'M':
            ret_int(value, unmonitored);
            break;
         case 'R':
            ret_int(value, unreachable);
            break;
         case 'U':
            ret_int(value, unknown);
            break;
      }
   }
   delete messages;
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for SIP peer details
 */
LONG H_SIPPeerDetails(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(1);

   char peerId[128];
   GET_ARGUMENT_A(1, peerId, 128);

   auto request = make_shared<AmiMessage>("SIPshowpeer");
   request->setTag("Peer", peerId);

   shared_ptr<AmiMessage> response = sys->sendRequest(request);
   if (response == nullptr)
      return SYSINFO_RC_ERROR;

   if (!response->isSuccess())
   {
      const char *reason = response->getTag("Message");
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Request \"SIPshowpeer\" to %s failed (%hs)"), sys->getName(), (reason != nullptr) ? reason : "Unknown reason");
      bool unknownPeer = RegexpMatchA(reason, "Peer [^ ]+ not found", false);
      return unknownPeer ? SYSINFO_RC_NO_SUCH_INSTANCE : SYSINFO_RC_ERROR;
   }

   if (arg == nullptr)
   {
      // Direct tag access
      char tagName[128];
      if (!AgentGetParameterArgA(param, 3, tagName, 128))
         return SYSINFO_RC_UNSUPPORTED;

      const char *tagValue = response->getTag(tagName);
      if (tagValue == nullptr)
         return SYSINFO_RC_UNSUPPORTED;

      ret_mbstring(value, tagValue);
   }
   else
   {
      const char *tagName;
      switch(*arg)
      {
         case 'I':   // IP address
            tagName = "Address-IP";
            break;
         case 'S':
            tagName = "Status";
            break;
         case 'T':
            tagName = "ChanObjectType";
            break;
         case 'U':
            tagName = "SIP-Useragent";
            break;
         case 'V':
            tagName = "VoiceMailbox";
            break;
         default:
            tagName = "?";
            break;
      }

      const char *tagValue = response->getTag(tagName);
      if (tagValue == nullptr)
         return SYSINFO_RC_UNSUPPORTED;

      ret_mbstring(value, tagValue);
   }

   return SYSINFO_RC_SUCCESS;
}
