/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: snmp.cpp
**
**/

#include "nms_core.h"


//
// Get value for SNMP variable
// If szOidStr is not NULL, string representation of OID is used, otherwise -
// binary representation from oidBinary and iOidLen
//

BOOL SnmpGet(char *szNode, char *szCommunity, char *szOidStr, oid *oidBinary,
             size_t iOidLen, void *pValue)
{
   struct snmp_session session, *sptr;
   struct snmp_pdu *pdu, *response;
   oid oidName[MAX_OID_LEN];
   size_t iNameLen = MAX_OID_LEN;
   struct variable_list *pVar;
   int iStatus;
   BOOL bResult = TRUE;

   // Open SNMP session
   snmp_sess_init(&session);
   session.version = SNMP_VERSION_1;
   session.peername = szNode;
   session.community = (unsigned char *)szCommunity;
   session.community_len = strlen((char *)session.community);

   sptr = snmp_open(&session);
   if(sptr == NULL)
   {
      WriteLog(MSG_SNMP_OPEN_FAILED, EVENTLOG_ERROR_TYPE, NULL);
      return FALSE;
   }

   // Create PDU and send request
   pdu = snmp_pdu_create(SNMP_MSG_GET);
   if (szOidStr != NULL)
   {
      if (read_objid(szOidStr, oidName, &iNameLen) == 0)
      {
         WriteLog(MSG_OID_PARSE_ERROR, EVENTLOG_ERROR_TYPE, "s", szOidStr);
         return FALSE;
      }
   }
   else
   {
      memcpy(oidName, oidBinary, iOidLen * sizeof(oid));
      iNameLen = iOidLen;
   }

   snmp_add_null_var(pdu, oidName, iNameLen);
   iStatus = snmp_synch_response(sptr, pdu, &response);

   // Analyze response
   if ((iStatus == STAT_SUCCESS) && (response->errstat == SNMP_ERR_NOERROR))
   {
      for(pVar = response->variables; pVar != NULL; pVar = pVar->next_variable)
      {
         switch(pVar->type)
         {
            case ASN_INTEGER:
            case ASN_UINTEGER:
            case ASN_COUNTER:
            case ASN_GAUGE:
            case ASN_IPADDRESS:
               *((long *)pValue) = *pVar->val.integer;
               break;
            case ASN_OCTET_STR:
               memcpy(pValue, pVar->val.string, pVar->val_len);
               ((char *)pValue)[pVar->val_len]=0;
               break;
            default:
               WriteLog(MSG_SNMP_UNKNOWN_TYPE, EVENTLOG_ERROR_TYPE, "d", pVar->type);
               bResult = FALSE;
               break;
         }
      }
   }
   else
   {
      if (iStatus == STAT_SUCCESS)
      {
         WriteLog(MSG_SNMP_BAD_PACKET, EVENTLOG_ERROR_TYPE, "s", snmp_errstring(response->errstat));
         bResult = FALSE;
      }
      else
      {
         WriteLog(MSG_SNMP_GET_ERROR, EVENTLOG_ERROR_TYPE, "d", iStatus);
         bResult = FALSE;
      }
   }

   // Destroy response PDU if neede
   if (response)
      snmp_free_pdu(response);

   // Close session and cleanup
   snmp_close(sptr);
   return bResult;
}


//
// Enumerate multiple values by walking throgh MIB, starting at given root
//

BOOL SnmpEnumerate(char *szNode, char *szCommunity, char *szRootOid,
                   void (* pHandler)(char *, char *, variable_list *, void *), void *pUserArg)
{
   struct snmp_session session, *sptr;
   struct snmp_pdu *pdu, *response;
   oid oidName[MAX_OID_LEN], oidRoot[MAX_OID_LEN];
   size_t iNameLen, iRootLen = MAX_OID_LEN;
   struct variable_list *pVar;
   int iStatus;
   BOOL bRunning = TRUE, bResult = TRUE;

   // Open SNMP session
   snmp_sess_init(&session);
   session.version = SNMP_VERSION_1;
   session.peername = szNode;
   session.community = (unsigned char *)szCommunity;
   session.community_len = strlen((char *)session.community);

   sptr = snmp_open(&session);
   if(sptr == NULL)
   {
      WriteLog(MSG_SNMP_OPEN_FAILED, EVENTLOG_ERROR_TYPE, NULL);
      return FALSE;
   }

   // Get root
   if (read_objid(szRootOid, oidRoot, &iRootLen) == 0)
   {
      WriteLog(MSG_OID_PARSE_ERROR, EVENTLOG_ERROR_TYPE, "s", szRootOid);
      return FALSE;
   }
   memcpy(oidName, oidRoot, iRootLen * sizeof(oid));
   iNameLen = iRootLen;

   // Walk the MIB
   while(bRunning)
   {
      pdu = snmp_pdu_create(SNMP_MSG_GETNEXT);
      snmp_add_null_var(pdu, oidName, iNameLen);
      iStatus = snmp_synch_response(sptr, pdu, &response);

      if ((iStatus == STAT_SUCCESS) && (response->errstat == SNMP_ERR_NOERROR))
      {
         for(pVar = response->variables; pVar != NULL; pVar = pVar->next_variable)
         {
            // Should we stop walking?
            if ((pVar->name_length < iRootLen) ||
                (memcmp(oidRoot, pVar->name, iRootLen * sizeof(oid))))
            {
               bRunning = 0;
               break;
            }
            memmove(oidName, pVar->name, pVar->name_length * sizeof(oid));
            iNameLen = pVar->name_length;

            // Call user's callback for processing
            pHandler(szNode, szCommunity, pVar, pUserArg);
         }
      }
      else
      {
         if (iStatus == STAT_SUCCESS)
         {
            WriteLog(MSG_SNMP_BAD_PACKET, EVENTLOG_ERROR_TYPE, "s", snmp_errstring(response->errstat));
            bResult = FALSE;
         }
         else
         {
            WriteLog(MSG_SNMP_GET_ERROR, EVENTLOG_ERROR_TYPE, "d", iStatus);
            bResult = FALSE;
         }
         bRunning = FALSE;
      }

      // Destroy responce PDU if needed
      if (response)
      {
         snmp_free_pdu(response);
      }
   }

   // Close session and cleanup
   snmp_close(sptr);
   return bResult;
}