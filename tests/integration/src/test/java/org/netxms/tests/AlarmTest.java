/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.tests;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.RCC;
import org.netxms.client.events.Alarm;
import org.netxms.client.events.BulkAlarmStateChangeData;
import org.netxms.client.events.EventInfo;

/**
 * Alarm management tests
 */
public class AlarmTest extends AbstractSessionTest
{
   @Test
	public void testGetAlarms() throws Exception
	{
      final NXCSession session = connectAndLogin();

		final Map<Long, Alarm> alarms = session.getAlarms();
		for(final Entry<Long, Alarm> e : alarms.entrySet())
		{
			System.out.println(e.getKey() + ": " + e.getValue().getMessage());
		}

		session.disconnect();
	}

	// NOTE: server must have at least 1 active alarm for this test.
   @Test
	public void testGetEvents() throws Exception
	{
		final NXCSession session = connectAndLogin();
		
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
   @Test
	public void testAlarmUpdate() throws Exception
	{
		final NXCSession session = connectAndLogin();

		HashMap<Long, Alarm> list = session.getAlarms();
		if (list.size() > 0)
		{		   
			final Semaphore s = new Semaphore(0);			
			final List<Long> alarmIds = new ArrayList<Long>(2);
			alarmIds.add(list.keySet().iterator().next());
			alarmIds.add(123456789L); // Made up alarm ID to check if it is returned
			final boolean[] success = new boolean[1];
			success[0] = false;
			session.addListener(new SessionListener() {
				public void notificationHandler(SessionNotification n)
				{
               List<Long> termAlarmIds = ((BulkAlarmStateChangeData)n.getObject()).getAlarms();
				   long termAlarmId = termAlarmIds.get(0);
					assertEquals(SessionNotification.MULTIPLE_ALARMS_TERMINATED, n.getCode());
					assertEquals(alarmIds.get(0).longValue(), termAlarmId);
					success[0] = true;
					s.release();
				}
			});
			session.subscribe(NXCSession.CHANNEL_ALARMS);
			Map<Long, Integer> terminationFails = session.bulkTerminateAlarms(alarmIds);
         assertEquals(RCC.INVALID_ALARM_ID, terminationFails.get(alarmIds.get(1)));
			assertTrue(s.tryAcquire(3, TimeUnit.SECONDS));
         assertTrue(success[0]);
		}

		session.disconnect();
	}
}
