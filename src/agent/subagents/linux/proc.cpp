/* $Id: proc.cpp,v 1.6 2007-01-15 00:16:07 victor Exp $ */

/* 
** NetXMS subagent for GNU/Linux
** Copyright (C) 2004 Alex Kirhenshtein
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
**/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <nms_util.h>

#include "proc.h"

static int ProcFilter(const struct dirent *pEnt)
{
	char *pTmp;

	if (pEnt == NULL)
	{
		return 0; // ignore file
	}

	pTmp = (char *)pEnt->d_name;
	while (*pTmp != 0)
	{
		if (*pTmp < '0' || *pTmp > '9')
		{
			return 0; // skip
		}
		pTmp++;
	}

	return 1;
}

int ProcRead(PROC_ENT **pEnt, char *szProcName, char *szCmdLine)
{
	struct dirent **pNameList;
	int nCount, nFound;
   BOOL bProcFound, bCmdFound;
   BOOL bIgnoreCase = TRUE;

	nFound = -1;

	nCount = scandir("/proc", &pNameList, &ProcFilter, alphasort);
	if (nCount < 0)
	{
		//perror("scandir");
	}
	else
	{
		nFound = 0;

		if (nCount > 0 && pEnt != NULL)
		{
			*pEnt = (PROC_ENT *)malloc(sizeof(PROC_ENT) * (nCount + 1));

			if (*pEnt == NULL)
			{
				nFound = -1;
				// cleanup
				while(nCount--)
				{
					free(pNameList[nCount]);
				}
			}
			else
			{
				memset(*pEnt, 0, sizeof(PROC_ENT) * (nCount + 1));
			}
		}

		while(nCount--)
		{
            bProcFound = bCmdFound = FALSE;
			char szFileName[256];
			FILE *hFile;
		     char *pProcName;
             unsigned long nPid;
          
			  snprintf(szFileName, sizeof(szFileName),
					"/proc/%s/stat", pNameList[nCount]->d_name);
			  hFile = fopen(szFileName, "r");
    		  if (hFile != NULL)
            {
              char szBuff[1024];
              if (fgets(szBuff, sizeof(szBuff), hFile) != NULL)
				  {

                 if (sscanf(szBuff, "%lu ", &nPid) == 1)
                 {
                     pProcName = strchr(szBuff, ')');
                     if (pProcName != NULL)
						   {
							   *pProcName = 0;
                        pProcName =  strchr(szBuff, '(');
                        if (pProcName != NULL)
							   {
							      pProcName++;
                           if (szProcName != NULL)
                           {
                               if (szCmdLine == NULL) // use old style compare
                                   bProcFound = strcasecmp(pProcName, szProcName) == 0;
                               else
                                   bProcFound = RegexpMatch(pProcName, szProcName, bIgnoreCase);
                           }
									else
									{
										bProcFound = TRUE;
									}
                        }
							}
						}
					} // fgets
				} // hFile

				fclose(hFile);

            if (szCmdLine != NULL)
            {
               snprintf(szFileName, sizeof(szFileName),
					            "/proc/%s/cmdline", pNameList[nCount]->d_name);
               hFile = fopen(szFileName, "r");
			      if (hFile != NULL)
               {
                   char szBuff[1024] = {0};
                   bCmdFound = FALSE;
                   if (fgets(szBuff, sizeof(szBuff), hFile) != NULL)
				       {
                        bCmdFound = RegexpMatch(szBuff, szCmdLine, bIgnoreCase);
                   }
               } // hFile != NULL
               fclose(hFile);
            } // szCmdLine
				else
				{
					bCmdFound = TRUE;
				}
            
            if (bProcFound && bCmdFound)
            {
                nFound++;
					 if (pEnt != NULL && pProcName != NULL )
					 {
						  (*pEnt)[nFound].nPid = nPid;
						  nx_strncpy((*pEnt)[nFound].szProcName, pProcName,
									sizeof((*pEnt)[nFound].szProcName));
					 }
            }
            
			free(pNameList[nCount]);
		}
		free(pNameList);
	}

	return nFound;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.5  2005/11/04 23:00:07  victor
Fixed some 64bit portability issues

Revision 1.4  2005/10/27 08:24:06  victor
Minor changes

Revision 1.3  2005/10/17 20:45:46  victor
Fixed incorrect usage of strncpy

Revision 1.2  2005/01/18 17:09:33  alk
process name matching chaged from patern to exact match

Revision 1.1  2004/10/22 22:08:34  alk
source restructured;
implemented:
	Net.IP.Forwarding
	Net.IP6.Forwarding
	Process.Count(*)
	Net.ArpCache
	Net.InterfaceList (if-type not implemented yet)
	System.ProcessList


*/
