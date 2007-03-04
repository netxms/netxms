/* 
** NetXMS - Network Management System
** Copyright (C) 2007 Victor Kirhenshtein
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

#include "nxcore.h"

#ifdef _WITH_ENCRYPTION

//
// Create X509 certificate structure from login message
//

X509 *CertificateFromLoginMessage(CSCPMessage *pMsg)
{
	DWORD dwLen;
	BYTE *pData;
	const BYTE *p;
	X509 *pCert = NULL;

	dwLen = pMsg->GetVariableBinary(VID_CERTIFICATE, NULL, 0);
	if (dwLen > 0)
	{
		pData = (BYTE *)malloc(dwLen);
		pMsg->GetVariableBinary(VID_CERTIFICATE, pData, dwLen);
		p = pData;
		pCert = d2i_X509(NULL, &p, dwLen);
		free(pData);
	}
	return pCert;
}


#endif
