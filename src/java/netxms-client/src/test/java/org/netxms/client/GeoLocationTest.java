/**
 * 
 */
package org.netxms.client;

import junit.framework.TestCase;
import org.netxms.base.GeoLocation;
import org.netxms.base.GeoLocationFormatException;

/**
 * Tests for GeoLocation class
 */
public class GeoLocationTest extends TestCase
{
	public void testParser() throws Exception
	{
		GeoLocation g = GeoLocation.parseGeoLocation(" 47°20'50.79\"N", " 8°33'56.31\"E");
		assertEquals(47.34744, g.getLatitude(), 0.00005);
		assertEquals(8.56564, g.getLongitude(), 0.00005);

		g = GeoLocation.parseGeoLocation("47.34744", "E8.56564°");
		assertEquals(47.34744, g.getLatitude(), 0.00005);
		assertEquals(8.56564, g.getLongitude(), 0.00005);
		
		try
		{
			g = GeoLocation.parseGeoLocation("W47.34744°", "E8.56564°");
			assertFalse(true);
		}
		catch(GeoLocationFormatException e)
		{
		}
	}
}
