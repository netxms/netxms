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
** File: channels.cpp
**
**/

#include "asterisk.h"

/**
 * Get peer name from channel name
 */
char *PeerFromChannelA(const char *channel, char *peer, size_t size)
{
   if (channel == nullptr)
   {
      *peer = 0;
      return nullptr;
   }

   const char *src = channel;
   while((*src != '/') && (*src != 0))
      src++;
   if (*src == 0)
   {
      *peer = 0;
      return nullptr;
   }

   src++;
   char *dst = peer;
   size_t count = 0;
   while((*src != '-') && (*src != 0) && (count < size - 1))
      *dst++ = *src++;
   *dst = 0;
   return peer;
}

#ifdef UNICODE

/**
 * Get peer name from channel name (UNICODE version)
 */
WCHAR *PeerFromChannelW(const char *channel, WCHAR *peer, size_t size)
{
   if (channel == nullptr)
   {
      *peer = 0;
      return nullptr;
   }

   const char *src = channel;
   while((*src != '/') && (*src != 0))
      src++;
   if (*src == 0)
   {
      *peer = 0;
      return nullptr;
   }

   src++;
   WCHAR *dst = peer;
   size_t count = 0;
   while((*src != '-') && (*src != 0) && (count < size - 1))
      *dst++ = *src++;
   *dst = 0;
   return peer;
}

#endif   /* UNICODE */

/**
 * Handler for channels list
 */
LONG H_ChannelList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(0);

   SharedObjectArray<AmiMessage> *messages = sys->readTable("Status");
   if (messages == nullptr)
      return SYSINFO_RC_ERROR;

   for(int i = 0; i < messages->size(); i++)
   {
      const char *ch = messages->get(i)->getTag("Channel");
      if (ch != nullptr)
         value->addMBString(ch);
   }

   delete messages;
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for channels table
 */
LONG H_ChannelTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(0);

   SharedObjectArray<AmiMessage> *messages = sys->readTable("Status");
   if (messages == nullptr)
      return SYSINFO_RC_ERROR;

   value->addColumn(_T("CHANNEL"), DCI_DT_STRING, _T("Channel"), true);
   value->addColumn(_T("STATE"), DCI_DT_INT, _T("State"));
   value->addColumn(_T("STATE_TEXT"), DCI_DT_STRING, _T("State Description"));
   value->addColumn(_T("CALLERID_NUM"), DCI_DT_STRING, _T("Caller ID Number"));
   value->addColumn(_T("CALLERID_NAME"), DCI_DT_STRING, _T("Caller ID Name"));
   value->addColumn(_T("CONNECTED_NUM"), DCI_DT_STRING, _T("Connected Line Number"));
   value->addColumn(_T("CONNECTED_NAME"), DCI_DT_STRING, _T("Connected Line Name"));
   value->addColumn(_T("ACCOUNT_CODE"), DCI_DT_STRING, _T("Account Code"));
   value->addColumn(_T("CONTEXT"), DCI_DT_STRING, _T("Context"));
   value->addColumn(_T("EXTEN"), DCI_DT_STRING, _T("Extension"));
   value->addColumn(_T("PRIORITY"), DCI_DT_INT, _T("Priority"));
   value->addColumn(_T("TYPE"), DCI_DT_STRING, _T("Type"));
   value->addColumn(_T("DNID"), DCI_DT_STRING, _T("DNID"));
   value->addColumn(_T("BRIDGE_ID"), DCI_DT_STRING, _T("Bridge ID"));
   value->addColumn(_T("UNIQUE_ID"), DCI_DT_STRING, _T("Unique ID"));
   value->addColumn(_T("LINKED_ID"), DCI_DT_STRING, _T("Linked ID"));
   value->addColumn(_T("APP"), DCI_DT_STRING, _T("Application"));
   value->addColumn(_T("DATA"), DCI_DT_STRING, _T("Data"));
   value->addColumn(_T("READ_FORMAT"), DCI_DT_STRING, _T("Read Format"));
   value->addColumn(_T("WRITE_FORMAT"), DCI_DT_STRING, _T("Write Format"));
   value->addColumn(_T("CALL_GROUP"), DCI_DT_INT, _T("Call Group"));
   value->addColumn(_T("PICKUP_GROUP"), DCI_DT_INT, _T("Pickup Group"));
   value->addColumn(_T("DURATION"), DCI_DT_INT, _T("Duration"));

   for(int i = 0; i < messages->size(); i++)
   {
      AmiMessage *msg = messages->get(i);
      const char *ch = msg->getTag("Channel");
      if (ch == nullptr)
         continue;

      value->addRow();
      value->set(0, ch);
      value->set(1, msg->getTag("ChannelState"));
      value->set(2, msg->getTag("ChannelStateDesc"));
      value->set(3, msg->getTag("CallerIDNum"));
      value->set(4, msg->getTag("CallerIDName"));
      value->set(5, msg->getTag("ConnectedLineNum"));
      value->set(6, msg->getTag("ConnectedLineName"));
      value->set(7, msg->getTag("Accountcode"));
      value->set(8, msg->getTag("Context"));
      value->set(9, msg->getTag("Exten"));
      value->set(10, msg->getTag("Priority"));
      value->set(11, msg->getTag("Type"));
      value->set(12, msg->getTag("DNID"));
      value->set(13, msg->getTag("BridgeID"));
      value->set(14, msg->getTag("Uniqueid"));
      value->set(15, msg->getTag("Linkedid"));
      value->set(16, msg->getTag("Application"));
      value->set(17, msg->getTag("Data"));
      value->set(18, msg->getTag("Readformat"));
      value->set(19, msg->getTag("Writeformat"));
      value->set(20, msg->getTag("Callgroup"));
      value->set(21, msg->getTag("Pickupgroup"));
      value->set(22, msg->getTag("Seconds"));
   }

   delete messages;
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for channel statistics parameters
 */
LONG H_ChannelStats(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(0);

   SharedObjectArray<AmiMessage> *messages = sys->readTable("Status");
   if (messages == nullptr)
      return SYSINFO_RC_ERROR;

   if (*arg == _T('A'))
   {
      ret_int(value, messages->size());
   }
   else
   {
      int reserved = 0, offhook = 0, dialing = 0, ringing = 0, up = 0, busy = 0;
      for(int i = 0; i < messages->size(); i++)
      {
         AmiMessage *msg = messages->get(i);
         if (msg->getTag("Channel") == nullptr)
            continue;

         int state = atoi(msg->getTag("ChannelState"));
         switch(state)
         {
            case 1:
               reserved++;
               break;
            case 2:
               offhook++;
               break;
            case 3:
               dialing++;
               break;
            case 4:
            case 5:
               ringing++;
               break;
            case 6:
               up++;
               break;
            case 7:
               busy++;
               break;
         }
      }

      switch(*arg)
      {
         case 'A':
            ret_int(value, dialing + ringing + up + busy);
            break;
         case 'B':
            ret_int(value, busy);
            break;
         case 'D':
            ret_int(value, dialing);
            break;
         case 'O':
            ret_int(value, offhook);
            break;
         case 'R':
            ret_int(value, reserved);
            break;
         case 'r':
            ret_int(value, ringing);
            break;
         case 'U':
            ret_int(value, up);
            break;
      }
   }
   delete messages;
   return SYSINFO_RC_SUCCESS;
}
