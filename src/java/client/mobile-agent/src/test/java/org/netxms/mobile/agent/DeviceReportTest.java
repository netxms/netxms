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
package org.netxms.mobile.agent;

import org.netxms.base.GeoLocation;

/**
 * Test basic device report functionality
 */
public class DeviceReportTest extends SessionTest
{
	public void testDeviceReports() throws Exception
	{
		final Session session = connect();

		session.reportDeviceSystemInfo("Raden Solutions", "Virtual Device", "JVM", System.getProperty("java.version"), "000000-000000-000000", null);
		session.reportDeviceStatus(null, new GeoLocation(51.5171, 0.1062), 0, 70);
		
		session.disconnect();
	}
}
