/* 
** NetXMS - Network Management System
** Portable management console
** Copyright (C) 2007, 2008 Victor Kirhenshtein
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
** File: sound.h
**
**/

#ifndef _sound_h_
#define _sound_h_

#include <wx/sound.h>


WX_DEFINE_SORTED_ARRAY_INT(DWORD, DWORD_Array);

//
// Rule of alarm sound policy
//

class AlarmSoundRule
{
protected:
	DWORD_Array m_sourceObjects;
	DWORD_Array m_events;
	TCHAR *m_messagePattern;
	int m_alarmState;
	wxSound *m_sound;
	bool m_isOk;

	bool CheckMatch(NXC_ALARM *alarm);

public:
	AlarmSoundRule();
	AlarmSoundRule(wxXmlNode *root);
	~AlarmSoundRule();

	bool IsOk() { return m_isOk; }
	bool HandleAlarm(NXC_ALARM *alarm);
};


//
// Alarm sound policy
//

class AlarmSoundPolicy
{
protected:
	int m_numRules;
	AlarmSoundRule **m_rules;

public:
	AlarmSoundPolicy();
	AlarmSoundPolicy(wxXmlNode *root);
	~AlarmSoundPolicy();

	void HandleAlarm(NXC_ALARM *alarm);
};

#endif
