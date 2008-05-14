/* $Id$ */
/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004, 2005, 2006, 2007 Victor Kirhenshtein
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
** File: cert.cpp
**
**/

#include "libnxcl.h"


//
// Add trusted CA certificate
//

DWORD LIBNXCL_EXPORTABLE NXCAddCACertificate(NXC_SESSION hSession, DWORD dwCertLen,
                                             BYTE *pCert, TCHAR *pszComments)
{
	CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_ADD_CA_CERTIFICATE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_CERTIFICATE, pCert, dwCertLen);
	msg.SetVariable(VID_COMMENTS, pszComments);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Update certificate's comments
//

DWORD LIBNXCL_EXPORTABLE NXCUpdateCertificateComments(NXC_SESSION hSession, DWORD dwCertId,
                                                      TCHAR *pszComments)
{
	CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_UPDATE_CERT_COMMENTS);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_CERTIFICATE_ID, dwCertId);
	msg.SetVariable(VID_COMMENTS, pszComments);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Delete certificate
//

DWORD LIBNXCL_EXPORTABLE NXCDeleteCertificate(NXC_SESSION hSession, DWORD dwCertId)
{
	CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_DELETE_CERTIFICATE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_CERTIFICATE_ID, dwCertId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Get list of installed certificates
//

DWORD LIBNXCL_EXPORTABLE NXCGetCertificateList(NXC_SESSION hSession, NXC_CERT_LIST **ppList)
{
   CSCPMessage msg, *pResponse;
   DWORD i, dwId, dwRetCode, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();
	*ppList = NULL;

   msg.SetCode(CMD_GET_CERT_LIST);
   msg.SetId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
		{
			*ppList = (NXC_CERT_LIST *)malloc(sizeof(NXC_CERT_LIST));
			(*ppList)->dwNumElements = pResponse->GetVariableLong(VID_NUM_CERTIFICATES);
			if ((*ppList)->dwNumElements > 0)
			{
				(*ppList)->pElements = (NXC_CERT_INFO *)malloc(sizeof(NXC_CERT_INFO) * (*ppList)->dwNumElements);
				for(i = 0, dwId = VID_CERT_LIST_BASE; i < (*ppList)->dwNumElements; i++, dwId += 6)
				{
					(*ppList)->pElements[i].dwId = pResponse->GetVariableLong(dwId++);
					(*ppList)->pElements[i].nType = (int)pResponse->GetVariableShort(dwId++);
					(*ppList)->pElements[i].pszComments = pResponse->GetVariableStr(dwId++);
					(*ppList)->pElements[i].pszSubject = pResponse->GetVariableStr(dwId++);
				}
			}
			else
			{
				(*ppList)->pElements = NULL;
			}
		}
      delete pResponse;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }
   return dwRetCode;
}


//
// Destroy certificate list
//

void LIBNXCL_EXPORTABLE NXCDestroyCertificateList(NXC_CERT_LIST *pList)
{
	DWORD i;

	if (pList == NULL)
		return;

	for(i = 0; i < pList->dwNumElements; i++)
	{
		safe_free(pList->pElements[i].pszComments);
		safe_free(pList->pElements[i].pszSubject);
	}
	safe_free(pList->pElements);
	free(pList);
}
