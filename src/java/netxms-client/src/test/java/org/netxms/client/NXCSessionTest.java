package org.netxms.client;

import junit.framework.TestCase;


//
// Test NXCSession
//
// Please note that all tests expects that NetXMS server is running
// on local machine, with user admin and no password.
// Change appropriate constants if needed.
//

public class NXCSessionTest extends TestCase
{
	private static final String serverAddress = "127.0.0.1";
	private static final String loginName = "admin";
	private static final String password = "";
	
	public void testConnect() throws Exception
	{
		NXCSession session = new NXCSession(serverAddress, loginName, password);
		session.connect();

		assertEquals(0, session.getUserId());
		
		Thread.sleep(2000);
		session.disconnect();
	}
}
