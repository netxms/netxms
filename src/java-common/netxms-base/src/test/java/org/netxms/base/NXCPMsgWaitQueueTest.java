/**
 * 
 */
package org.netxms.base;

import junit.framework.TestCase;

/**
 * @author Victor
 *
 */
public class NXCPMsgWaitQueueTest extends TestCase
{
	public void testWaitForMessage()
	{
		final NXCPMsgWaitQueue mwq = new NXCPMsgWaitQueue(5000, 10000);
		
		mwq.putMessage(new NXCPMessage(10, 1L));
		mwq.putMessage(new NXCPMessage(10, 2L));
		mwq.putMessage(new NXCPMessage(10, 3L));
		
		final NXCPMessage msg = mwq.waitForMessage(10, 2L);
		assertEquals(true, msg != null);
		assertEquals(10, msg.getMessageCode());
		assertEquals(2, msg.getMessageId());
		
		mwq.shutdown();
	}

	public void testHousekeeper()
	{
		final NXCPMsgWaitQueue mwq = new NXCPMsgWaitQueue(5000, 5000);
		
		mwq.putMessage(new NXCPMessage(10, 1L));
		mwq.putMessage(new NXCPMessage(10, 2L));
		mwq.putMessage(new NXCPMessage(10, 3L));
		
		// Sleep for 8 seconds - housekeeper should remove message from queue
		try
		{
			Thread.sleep(8000);
		}
		catch(InterruptedException e)
		{
		}
		
		final NXCPMessage msg = mwq.waitForMessage(10, 2L);
		assertEquals(true, msg == null);
		
		mwq.shutdown();
	}
}
