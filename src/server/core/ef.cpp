/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
** File: ef.cpp
**/

#include "nxcore.h"

#define DEBUG_TAG _T("isc.ef")

/**
 * Setup event forwarding session
 */
bool EF_SetupSession(ISCSession *, NXCPMessage *request)
{
	return true;
}

/**
 * Close event forwarding session
 */
void EF_CloseSession(ISCSession *)
{
}

/**
 * Process event forwarding session message
 */
bool EF_ProcessMessage(ISCSession *session, NXCPMessage *request, NXCPMessage *response)
{
	if (request->getCode() == CMD_FORWARD_EVENT)
	{
	   TCHAR buffer[64];
		nxlog_debug_tag(DEBUG_TAG, 4, _T("Event forwarding request from %s"), session->getPeerAddress().toString(buffer));

		shared_ptr<NetObj> object;
		uint32_t id = request->getFieldAsUInt32(VID_OBJECT_ID);
		if (id != 0)
			object = FindObjectById(id);  // Object is specified explicitly
		else
			object = FindNodeByIP(0, request->getFieldAsInetAddress(VID_IP_ADDRESS));	// Object is specified by IP address
		
		if (object != nullptr)
		{
         uint32_t code;
			TCHAR name[128] = _T("");
			request->getFieldAsString(VID_EVENT_NAME, name, 128);
			if (name[0] != 0)
			{
				nxlog_debug_tag(DEBUG_TAG, 5, _T("Event specified by name (%s)"), name);
				shared_ptr<EventTemplate> pt = FindEventTemplateByName(name);
				if (pt != nullptr)
				{
					code = pt->getCode();
					nxlog_debug_tag(DEBUG_TAG, 5, _T("Event name %s resolved to event code %u"), name, code);
				}
				else
				{
					code = 0;
					nxlog_debug_tag(DEBUG_TAG, 5, _T("Event name %s cannot be resolved"), name);
				}
			}
			else
			{
				code = request->getFieldAsUInt32(VID_EVENT_CODE);
				nxlog_debug_tag(DEBUG_TAG, 5, _T("Event specified by code (%u)"), code);
			}

			bool success;
         success = EventBuilder(code, object->getId())
            .origin(EventOrigin::REMOTE_SERVER)
            .tags(request->getFieldAsSharedString(VID_TAGS))
            .params(*request, VID_EVENT_ARG_BASE, VID_EVENT_ARG_NAMES_BASE, VID_NUM_ARGS)
            .post();
         response->setField(VID_RCC, success ? ISC_ERR_SUCCESS : ISC_ERR_POST_EVENT_FAILED);
		}
		else
		{
			response->setField(VID_RCC, ISC_ERR_OBJECT_NOT_FOUND);
		}
	}
	else
	{
		response->setField(VID_RCC, ISC_ERR_NOT_IMPLEMENTED);
	}
	return false;	// Don't close session
}
