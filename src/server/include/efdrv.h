/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: efdrv.h
**
**/

#ifndef _efdrv_h_
#define _efdrv_h_

#include <nms_util.h>

class Event;
class NetObj;

/**
 * Event forwarder driver base class.
 *
 * Unlike notification channel drivers (NCDriver), event forwarder drivers receive the full
 * structured event object together with its source object, so they can forward all event
 * data (severity, parameters, tags, origin) to an external target.
 */
class EventForwarderDriver
{
protected:
   EventForwarderDriver() { }

public:
   virtual ~EventForwarderDriver() { }

   /**
    * Forward single event to the target. Returns true on success, false on failure
    * (failure triggers the forwarder's retry/drop handling).
    */
   virtual bool forward(const Event& event, const shared_ptr<NetObj>& source) = 0;

   /**
    * Optional health check. Returns true if the driver is operational.
    */
   virtual bool checkHealth() { return true; }
};

/**
 * Event forwarder driver factory. Receives forwarder configuration as a parsed JSON object
 * (never null; may be empty) and returns a new driver instance, or nullptr on error.
 */
typedef EventForwarderDriver *(*EventForwarderDriverFactory)(json_t *configuration);

#endif   /* _efdrv_h_ */
