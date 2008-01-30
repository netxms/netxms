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
** File: nxcore_situations.h
**
**/

#ifndef _nxcore_situations_h_
#define _nxcore_situations_h_


//
// Situation instance object
//

class SituationInstance
{
private:
	TCHAR *m_name;
	StringMap m_attributes;

public:
	SituationInstance(const TCHAR *name);
	~SituationInstance();

	const TCHAR *GetName() { return m_name; }

	void UpdateAttribute(const TCHAR *attribute, const TCHAR *value);
};


//
// Situation object
//

class Situation
{
private:
	DWORD m_id;
	TCHAR *m_name;
	TCHAR *m_comments;
	int m_numInstances;
	SituationInstance **m_instanceList;

public:
	Situation(const TCHAR *name);
	Situation(DB_RESULT handle, int row);
	~Situation();

	void UpdateSituation(const TCHAR *instance, const TCHAR *attribute, const TCHAR *value);
};

#endif
