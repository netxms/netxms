/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
** File: nms_script.h
**
**/

#ifndef _nms_script_h_
#define _nms_script_h_


//
// NXSL class representing NetXMS object
//

class NXSL_NetXMSObjectClass : public NXSL_Class
{
public:
   NXSL_NetXMSObjectClass() : NXSL_Class() { strcpy(m_szName, "NetXMS_Object"); }

   virtual NXSL_Value *GetAttr(NXSL_Object *pObject, char *pszAttr);
   virtual BOOL SetAttr(NXSL_Object *pObject, char *pszAttr, NXSL_Value *pValue);
};


//
// "NetXMS node" class
//

class NXSL_NodeClass : public NXSL_Class
{
public:
   NXSL_NodeClass();

   virtual NXSL_Value *GetAttr(NXSL_Object *pObject, char *pszAttr);
};


//
// "NetXMS event" class
//

class NXSL_EventClass : public NXSL_Class
{
public:
   NXSL_EventClass();

   virtual NXSL_Value *GetAttr(NXSL_Object *pObject, char *pszAttr);
};


//
// Functions
//

void LoadScripts(void);
void ReloadScript(DWORD dwScriptId);
BOOL IsValidScriptId(DWORD dwId);


//
// Global variables
//

extern NXSL_Library *g_pScriptLibrary;
extern NXSL_NodeClass g_nxslNodeClass;
extern NXSL_EventClass g_nxslEventClass;


#endif
