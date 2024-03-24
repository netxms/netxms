/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.client.snmp.SnmpObjectIdFormatException;

/**
 * Test SNMP-related functionality of client library
 */
public class SnmpTest
{
   @Test
	public void testSnmpObjectId() throws Exception
	{
		// Test parse and equals
		final SnmpObjectId oid1 = new SnmpObjectId(new long[] { 1, 3, 6, 1, 2, 1, 1, 1, 0 });
		final SnmpObjectId oid2 = SnmpObjectId.parseSnmpObjectId(".1.3.6.1.2.1.1.1.0");
		assertEquals(oid1, oid2);

      final SnmpObjectId oid3 = SnmpObjectId.parseSnmpObjectId("1.3.6.1.2.1.1.1.0");
      assertEquals(oid1, oid3);

		// Test toString
		assertEquals("1.3.6.1.2.1.1.1.0", oid1.toString());

		// Test format exception
      assertThrowsExactly(SnmpObjectIdFormatException.class, () -> SnmpObjectId.parseSnmpObjectId(".1.2.bad.4"));
      assertThrowsExactly(SnmpObjectIdFormatException.class, () -> SnmpObjectId.parseSnmpObjectId("1.2.5000000000.4"));
      assertThrowsExactly(SnmpObjectIdFormatException.class, () -> SnmpObjectId.parseSnmpObjectId("."));
	}
}
