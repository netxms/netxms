/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrowsExactly;
import org.junit.jupiter.api.Test;
import org.netxms.base.GeoLocation;
import org.netxms.base.GeoLocationFormatException;

/**
 * Tests for GeoLocation class
 */
public class GeoLocationTest
{
   @Test
	public void testParser() throws Exception
	{
		GeoLocation g = GeoLocation.parseGeoLocation(" 47°20'50.79\"N", " 8°33'56,31\"E");
		assertEquals(47.34744, g.getLatitude(), 0.00005);
		assertEquals(8.56564, g.getLongitude(), 0.00005);
		System.out.println(g);

		g = GeoLocation.parseGeoLocation("47.34744", "E8.56564°");
		assertEquals(47.34744, g.getLatitude(), 0.00005);
		assertEquals(8.56564, g.getLongitude(), 0.00005);
      System.out.println(g);

      assertThrowsExactly(GeoLocationFormatException.class, () -> GeoLocation.parseGeoLocation("W47.34744°", "E8.56564°"));
	}
}
