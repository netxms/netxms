/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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

import junit.framework.Test;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyRule;

/**
 * Tests for EPP management
 */
public class EventProcessingPolicyTest extends SessionTest
{
	public void testGetPolicy() throws Exception
	{
		final NXCSession session = connect();

		EventProcessingPolicy p = session.getEventProcessingPolicy();
		for(EventProcessingPolicyRule r : p.getRules())
			System.out.println("  " + r.getGuid() + " " + r.getComments());
		
		session.disconnect();
	}
	
	public void testSendEvent() throws Exception
	{
      final NXCSession session = connect();

      session.sendEvent(TestConstants.EVENT_CODE, new String[] { "test message\nline #2\nline #3" });
      
      session.disconnect();
	}
}
