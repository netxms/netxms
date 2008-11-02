package org.netxms.client;

import java.util.HashMap;
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

	public void testGetAlarms() throws Exception
	{
		NXCSession session = new NXCSession(serverAddress, loginName, password);
		session.connect();
		
		session.getAlarms(false);
		
		session.disconnect();
	}

	// NOTE: server must have at least 1 active alarm for this test.
	//       This alarm  will be terminated during test.
	public void testAlarmUpdate() throws Exception
	{
		NXCSession session = new NXCSession(serverAddress, loginName, password);
		session.connect();
		
		HashMap<Long, NXCAlarm> list = session.getAlarms(false);
		if (list.size() > 0)
		{
			final Boolean success = new Boolean(false);
			session.addListener(new NXCListener() {
				public void notificationHandler(NXCNotification n)
				{
//					if (n.getCode() == NXCNotification.ALARM_TERMINATED)
//						success.;
				}
			});
			session.acknowledgeAlarm(list.keySet().iterator().next());
		}
		
		session.disconnect();
	}

	public void testServerVariables() throws Exception
	{
		NXCSession session = new NXCSession(serverAddress, loginName, password);
		session.connect();

		// Create test variable
		session.setServerVariable("TestVariable", "TestValue");
		
		// Get full list
		HashMap<String, NXCServerVariable> varList = session.getServerVariables();
		assertEquals(true, varList.size() > 0);	// Normally server should have at least one variable
		
		// Get variable TestVariable
		NXCServerVariable var = varList.get("TestVariable");
		assertEquals(true, var != null);
		assertEquals("TestValue", var.getValue());

		// Delete test variable
		session.deleteServerVariable("TestVariable");
		
		// Get variable list again and check that test variable was deleted
		varList = session.getServerVariables();
		var = varList.get("TestVariable");
		assertEquals(true, var == null);

		session.disconnect();
	}
}
