/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import org.netxms.api.client.SessionNotification;
import org.netxms.client.events.Alarm;
import org.netxms.client.events.EventInfo;

/**
 * Alarm management tests
 */
public class AlarmTest extends SessionTest
{
	public void testGetAlarms() throws Exception
	{
		final NXCSession session = connect(true);
		
		final Map<Long, Alarm> alarms = session.getAlarms();
		for(final Entry<Long, Alarm> e : alarms.entrySet())
		{
			System.out.println(e.getKey() + ": " + e.getValue().getMessage());
		}
		
		session.disconnect();
	}

	// NOTE: server must have at least 1 active alarm for this test.
	public void testGetEvents() throws Exception
	{
		final NXCSession session = connect();
		
		HashMap<Long, Alarm> list = session.getAlarms();
		if (list.size() > 0)
		{
			final Long alarmId = list.keySet().iterator().next();
			List<EventInfo> events = session.getAlarmEvents(alarmId);
			for(EventInfo e : events)
			{
				System.out.println(e.getTimeStamp() + " " + e.getName() + " " + e.getMessage());
			}
		}
		
		session.disconnect();
	}
	
	// NOTE: server must have at least 1 active alarm for this test.
	//       This alarm  will be terminated during test.
	public void testAlarmUpdate() throws Exception
	{
		final NXCSession session = connect();
		
		HashMap<Long, Alarm> list = session.getAlarms();
		if (list.size() > 0)
		{
			final Semaphore s = new Semaphore(0);
			final Long alarmId = list.keySet().iterator().next();
			final boolean[] success = new boolean[1];
			success[0] = false;
			session.addListener(new NXCListener() {
				public void notificationHandler(SessionNotification n)
				{
					assertEquals(NXCNotification.ALARM_TERMINATED, n.getCode());
					assertEquals(alarmId.longValue(), ((Alarm)n.getObject()).getId());
					success[0] = true;
					s.release();
				}
			});
			session.subscribe(NXCSession.CHANNEL_ALARMS);
			session.terminateAlarm(alarmId);
			assertTrue(s.tryAcquire(3, TimeUnit.SECONDS));
			assertEquals(true, success[0]);
		}
		
		session.disconnect();
	}
}
