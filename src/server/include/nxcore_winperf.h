/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: nxcore_winperf.h
**
**/

#ifndef _nxcore_winperf_h_
#define _nxcore_winperf_h_

/**
 * Windows performance object
 */
class NXCORE_EXPORTABLE WinPerfObject
{
private:
	TCHAR *m_name;
	StringList *m_counters;
	StringList *m_instances;

public:
	static ObjectArray<WinPerfObject> *getWinPerfObjectsFromNode(Node *node, AgentConnection *conn);

	WinPerfObject(const TCHAR *name);
	~WinPerfObject();

	bool readDataFromAgent(AgentConnection *conn);

	UINT32 fillMessage(CSCPMessage *msg, UINT32 baseId);

	const TCHAR *getName() { return m_name; }
};

#endif
