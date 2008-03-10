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
** File: sound.cpp
**
**/

#include "nxmc.h"
#include "sound.h"


//
// Sound policy default constructor
//

AlarmSoundPolicy::AlarmSoundPolicy()
{
	m_numRules = 0;
	m_rules = NULL;
}


//
// Sound policy constructor from XML document
// Root node should be <soundpolicy> node
//

AlarmSoundPolicy::AlarmSoundPolicy(wxXmlNode *root)
{
	AlarmSoundRule *rule;
	wxXmlNode *child;

	m_numRules = 0;
	m_rules = NULL;

	child = root->GetChildren();
	while(child)
	{
		if (child->GetName() == _T("rule"))
		{
			rule = new AlarmSoundRule(child);
			if (rule->IsOk())
			{
				m_rules = (AlarmSoundRule **)realloc(m_rules, sizeof(AlarmSoundRule *) * (m_numRules + 1));
				m_rules[m_numRules] = rule;
				m_numRules++;
			}
			else
			{
				delete rule;
			}
		}
		child = child->GetNext();
	}
}


//
// Sound policy destructor
//

AlarmSoundPolicy::~AlarmSoundPolicy()
{
	int i;

	for(i = 0; i < m_numRules; i++)
		delete m_rules[i];
	safe_free(m_rules);
}


//
// Handle alarm
//

void AlarmSoundPolicy::HandleAlarm(NXC_ALARM *alarm)
{
	int i;

	for(i = 0; i < m_numRules; i++)
		if (m_rules[i]->HandleAlarm(alarm))
			break;
}


//
// Default constructor for sound rule
//

AlarmSoundRule::AlarmSoundRule()
{
	m_sourceObjects = new DWORD_Array(CompareDwords);
	m_events = new DWORD_Array(CompareDwords);
	m_messagePattern = NULL;
	m_alarmState = -1;
	m_sound = NULL;
	m_isOk = false;
}


//
// Constructor for sound rule from XML document
// Root node should be <rule> node
//

AlarmSoundRule::AlarmSoundRule(wxXmlNode *root)
{
	wxXmlNode *child, *data;
	
	m_sourceObjects = new DWORD_Array(CompareDwords);
	m_events = new DWORD_Array(CompareDwords);
	m_messagePattern = NULL;
	m_alarmState = -1;
	m_sound = NULL;
	m_isOk = true;

	child = root->GetChildren();
	while(child)
	{
		if (child->GetName() == _T("objects"))
		{
		}
		else if (child->GetName() == _T("events"))
		{
		}
		else if (child->GetName() == _T("message"))
		{
		}
		else if (child->GetName() == _T("state"))
		{
			data = child->GetChildren();
		}
		else if (child->GetName() == _T("sound"))
		{
		}
		else	// Invalid tag
		{
			m_isOk = false;
			break;
		}
		child = child->GetNext();
	}
}


//
// Sound policy rule destructor
//

AlarmSoundRule::~AlarmSoundRule()
{
	safe_free(m_messagePattern);
	delete m_sound;
	delete m_sourceObjects;
	delete m_events;
}


//
// Handle alarm
//

bool AlarmSoundRule::HandleAlarm(NXC_ALARM *alarm)
{
	if (!m_isOk)
		return false;

	if (CheckMatch(alarm))
	{
		if (m_sound != NULL)
		{
			m_sound->Play();
			return true;
		}
	}
	return false;
}


//
// Check if alarm matched against rule
//

bool AlarmSoundRule::CheckMatch(NXC_ALARM *alarm)
{
	return false;
}
