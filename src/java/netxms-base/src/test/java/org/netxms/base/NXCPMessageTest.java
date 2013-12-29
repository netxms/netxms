/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.base;

import java.util.Arrays;
import junit.framework.TestCase;

/**
 * Tests for NXCPMessage class
 */
public class NXCPMessageTest extends TestCase
{
	public void testMessageSetAndGet()
	{
		final byte[] byteTest = { 0x10, 0x20, 0x30 };
		
		final NXCPMessage msg = new NXCPMessage(1, 2);
		msg.setVariable(1, "string value");
		msg.setVariableInt16(2, 10);
		msg.setVariableInt32(3, 20);
		msg.setVariableInt64(4, 123456789L);
		msg.setVariable(5, byteTest);
		
		assertEquals(1, msg.getMessageCode());
		assertEquals(2L, msg.getMessageId());
		assertEquals("string value", msg.findVariable(1).getAsString());
		assertEquals(10, msg.findVariable(2).getAsInteger().intValue());
		assertEquals(20, msg.findVariable(3).getAsInteger().intValue());
		assertEquals(123456789L, msg.findVariable(4).getAsInteger().longValue());
		assertEquals(true, Arrays.equals(byteTest, msg.findVariable(5).getAsBinary()));
	}

	public void testMessageEncodingAndDecoding() throws Exception
	{
		final byte[] byteTest = { 0x10, 0x20, 0x30, 0x40, 0x50 };
		
		final NXCPMessage msg1 = new NXCPMessage(1, 2);
		msg1.setVariable(1, "string value");
		msg1.setVariableInt16(2, 10);
		msg1.setVariableInt32(3, 20);
		msg1.setVariableInt64(4, 123456789L);
		msg1.setVariable(5, byteTest);
	
		final byte[] bytes = msg1.createNXCPMessage();
		assertEquals(120, bytes.length);
		
		final NXCPMessage msg2 = new NXCPMessage(bytes, null);
		
		assertEquals(1, msg2.getMessageCode());
		assertEquals(2L, msg2.getMessageId());
		assertEquals("string value", msg2.findVariable(1).getAsString());
		assertEquals(10, msg2.findVariable(2).getAsInteger().intValue());
		assertEquals(20, msg2.findVariable(3).getAsInteger().intValue());
		assertEquals(123456789L, msg2.findVariable(4).getAsInteger().longValue());
		assertEquals(true, Arrays.equals(byteTest, msg2.findVariable(5).getAsBinary()));
	}
	
	private void doEncryptionTest(int cipher) throws Exception
	{
	   System.out.println("==== " + EncryptionContext.getCipherName(cipher) + " ====");
	   
      final NXCPMessage msg1 = new NXCPMessage(NXCPCodes.CMD_REQUEST_COMPLETED, 2);
      msg1.setVariableInt32(NXCPCodes.VID_RCC, 0);
      final byte[] bytes = msg1.createNXCPMessage();
      System.out.println("   Message encoded into " + bytes.length + " bytes");
      
	   EncryptionContext ctx = new EncryptionContext(cipher);
	   byte[] encryptedBytes = ctx.encryptMessage(msg1);
      System.out.println("   Message encrypted into " + encryptedBytes.length + " bytes");
      
      final NXCPMessage msg2 = new NXCPMessage(encryptedBytes, ctx);
      assertEquals(NXCPCodes.CMD_REQUEST_COMPLETED, msg2.getMessageCode());
      assertEquals(2L, msg2.getMessageId());
      assertEquals(0, msg2.findVariable(NXCPCodes.VID_RCC).getAsInteger().intValue());
	}
	
	public void testEncryptionAES256() throws Exception
	{
	   doEncryptionTest(0);
	}
   
   public void testEncryptionAES128() throws Exception
   {
      doEncryptionTest(4);
   }

   public void testEncryptionBlowfish256() throws Exception
   {
      doEncryptionTest(1);
   }
   
   public void testEncryptionBlowfish128() throws Exception
   {
      doEncryptionTest(5);
   }
}
