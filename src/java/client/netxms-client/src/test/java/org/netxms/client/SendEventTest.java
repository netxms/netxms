/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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

import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;

/**
 * Test for sendEvent API
 */
public class SendEventTest extends AbstractSessionTest
{
	public void testSendEvent() throws Exception
	{
		final NXCSession session = connect();
		
		session.sendEvent(1, new String[0]);
		session.sendEvent("SYS_NODE_ADDED", new String[0]);
		
		session.syncObjects();
		for(AbstractObject o : session.getAllObjects())
		{
			if (o instanceof Node)
			{
				session.sendEvent(0, "SYS_SCRIPT_ERROR", o.getObjectId(), new String[] { "test1", "test2", "0" }, "TAG");
				break;
			}
		}
		
		session.disconnect();
	}
}
