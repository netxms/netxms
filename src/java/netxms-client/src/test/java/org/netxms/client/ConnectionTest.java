/**
 * 
 */
package org.netxms.client;

/**
 * @author Victor
 *
 */
public class ConnectionTest extends SessionTest
{
	public void testConnect() throws Exception
	{
		final NXCSession session = connect();

		assertEquals(0, session.getUserId());
		
		Thread.sleep(2000);
		session.disconnect();
	}
}
