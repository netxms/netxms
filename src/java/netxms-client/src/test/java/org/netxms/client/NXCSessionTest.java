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

	public void testObjectSync() throws Exception
	{
		NXCSession session = new NXCSession(serverAddress, loginName, password);
		session.connect();
		
		session.syncObjects();
		final NXCObject obj = session.findObjectById(1);
		assertEquals(true, obj != null);
		assertEquals(1, obj.getObjectId());
		assertEquals("Entire Network", obj.getObjectName());
		
		session.disconnect();
	}
}
