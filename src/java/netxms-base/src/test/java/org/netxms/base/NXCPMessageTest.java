package org.netxms.base;

import junit.framework.TestCase;

public class NXCPMessageTest extends TestCase
{
	public void testMessageEncodingAndDecoding()
	{
		try
		{
			final NXCPMessage msg1 = new NXCPMessage(1, 2);
			msg1.setVariable(1, "string value");
			msg1.setVariableInt16(2, 10);
			msg1.setVariableInt32(3, 20);
			msg1.setVariableInt64(4, 30L);
		
			final byte[] bytes = msg1.createNXCPMessage();
			
			final NXCPMessage msg2 = new NXCPMessage(bytes);
			
			assertEquals(1, msg2.getMessageCode());
			assertEquals(2L, msg2.getMessageId());
			assertEquals("string value", msg2.findVariable(1).getAsString());
			assertEquals(10, msg2.findVariable(2).getAsInteger().intValue());
			assertEquals(20, msg2.findVariable(3).getAsInteger().intValue());
			assertEquals(30L, msg2.findVariable(4).getAsInteger().longValue());
		}
		catch(Exception e)
		{
			fail(e.toString());
		}
	}
}
