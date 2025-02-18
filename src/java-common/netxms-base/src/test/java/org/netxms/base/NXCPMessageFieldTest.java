package org.netxms.base;

import static org.junit.jupiter.api.Assertions.assertEquals;
import java.net.InetAddress;
import java.util.Arrays;
import java.util.UUID;
import org.junit.jupiter.api.Test;

public class NXCPMessageFieldTest
{
	// String variables
   @Test
	public void testStringConstruction()
	{
		NXCPMessageField variable = new NXCPMessageField(1, "Sample String");
		assertEquals(1, variable.getId());
		assertEquals(NXCPMessageField.TYPE_UTF8_STRING, variable.getType());
		assertEquals("Sample String", variable.getAsString());

      variable = new NXCPMessageField(1, "Sample String", true);
      assertEquals(1, variable.getId());
      assertEquals(NXCPMessageField.TYPE_STRING, variable.getType());
      assertEquals("Sample String", variable.getAsString());
	}

   @Test
	public void testStringUtf8EncodingAndDecoding() throws Exception
	{
		final NXCPMessageField var1 = new NXCPMessageField(1, "Sample String");
		
		final byte[] df = var1.createNXCPDataField();
		assertEquals(32, df.length);
		
		final NXCPMessageField var2 = new NXCPMessageField(df);
		assertEquals(1, var2.getId());
		assertEquals(NXCPMessageField.TYPE_UTF8_STRING, var2.getType());
		assertEquals("Sample String", var2.getAsString());
	}

   @Test
   public void testStringUcs2EncodingAndDecoding() throws Exception
   {
      final NXCPMessageField var1 = new NXCPMessageField(1, "Sample String", true);
      
      final byte[] df = var1.createNXCPDataField();
      assertEquals(40, df.length);
      
      final NXCPMessageField var2 = new NXCPMessageField(df);
      assertEquals(1, var2.getId());
      assertEquals(NXCPMessageField.TYPE_STRING, var2.getType());
      assertEquals("Sample String", var2.getAsString());
   }

	// Int32 variables
   @Test
	public void testInt32Construction()
	{
		final NXCPMessageField variable = new NXCPMessageField(1, NXCPMessageField.TYPE_INTEGER, 17L);

		assertEquals(1, variable.getId());
		assertEquals(NXCPMessageField.TYPE_INTEGER, variable.getType());
		assertEquals(17, variable.getAsInteger().intValue());
	}

   @Test
	public void testInt32EncodingAndDecoding() throws Exception
	{
		final NXCPMessageField var1 = new NXCPMessageField(1, NXCPMessageField.TYPE_INTEGER, 17L);
		
		final byte[] df = var1.createNXCPDataField();
		final NXCPMessageField var2 = new NXCPMessageField(df);

		assertEquals(1, var2.getId());
		assertEquals(NXCPMessageField.TYPE_INTEGER, var2.getType());
		assertEquals(17, var2.getAsInteger().intValue());
	}

   @Test
	public void testInt32SignEncodingAndDecoding() throws Exception
	{
		final NXCPMessageField var1 = new NXCPMessageField(1, NXCPMessageField.TYPE_INTEGER, 0xFFFFFFFFL);
		
		final byte[] df = var1.createNXCPDataField();
		final NXCPMessageField var2 = new NXCPMessageField(df);

		assertEquals(1, var2.getId());
		assertEquals(NXCPMessageField.TYPE_INTEGER, var2.getType());
		assertEquals(-1, var2.getAsInteger().intValue());
	}

	
	//
	// Int64 variables
	//
	
   @Test
	public void testInt64Construction()
	{
		final NXCPMessageField variable = new NXCPMessageField(1, NXCPMessageField.TYPE_INT64, 123456789L);

		assertEquals(1, variable.getId());
		assertEquals(NXCPMessageField.TYPE_INT64, variable.getType());
		assertEquals(123456789L, variable.getAsInteger().longValue());
	}

   @Test
	public void testInt64EncodingAndDecoding() throws Exception
	{
		final NXCPMessageField var1 = new NXCPMessageField(1, NXCPMessageField.TYPE_INT64, 123456789L);
		
		final byte[] df = var1.createNXCPDataField();
		final NXCPMessageField var2 = new NXCPMessageField(df);

		assertEquals(1, var2.getId());
		assertEquals(NXCPMessageField.TYPE_INT64, var2.getType());
		assertEquals(123456789L, var2.getAsInteger().longValue());
	}

	
	//
	// Binary variables
	//
	
   @Test
	public void testBinaryConstruction()
	{
		final byte[] byteArray = { 0x10, 0x20, 0x30 };
		final NXCPMessageField variable = new NXCPMessageField(1, byteArray);

		assertEquals(1, variable.getId());
		assertEquals(NXCPMessageField.TYPE_BINARY, variable.getType());
		assertEquals(byteArray, variable.getAsBinary());
	}

   @Test
	public void testBinaryEncodingAndDecoding() throws Exception
	{
		final byte[] byteArray = { 0x10, 0x20, 0x30, 0x40, 0x50 };
		final NXCPMessageField var1 = new NXCPMessageField(1, byteArray);
		
		final byte[] df = var1.createNXCPDataField();
		assertEquals(24, df.length);
		
		final NXCPMessageField var2 = new NXCPMessageField(df);
		assertEquals(1, var2.getId());
		assertEquals(NXCPMessageField.TYPE_BINARY, var2.getType());
		assertEquals(true, Arrays.equals(byteArray, var2.getAsBinary()));
	}

	/**
	 * Test getAsInetAddress()
	 */	
   @Test
	public void testGetAsInetAddress() throws Exception
	{
		final NXCPMessageField v1 = new NXCPMessageField(1, NXCPMessageField.TYPE_INTEGER, 0x0A000102L);

		assertEquals(1, v1.getId());
		assertEquals(NXCPMessageField.TYPE_INTEGER, v1.getType());
		assertEquals(InetAddress.getByName("10.0.1.2"), v1.getAsInetAddress());

      final NXCPMessageField v2 = new NXCPMessageField(2, InetAddress.getByName("10.22.1.2"));

      assertEquals(2, v2.getId());
      assertEquals(NXCPMessageField.TYPE_INETADDR, v2.getType());
      assertEquals(InetAddress.getByName("10.22.1.2"), v2.getAsInetAddress());
	}

	/**
	 * Test set variable from InetAddress
	 */
   @Test
	public void testSetAsInetAddress() throws Exception
	{
		final InetAddress addr = InetAddress.getByName("217.4.172.12");
		final NXCPMessageField variable = new NXCPMessageField(1, addr);

		assertEquals(1, variable.getId());
		assertEquals(NXCPMessageField.TYPE_INETADDR, variable.getType());
		assertEquals(InetAddress.getByName("217.4.172.12"), variable.getAsInetAddress());
	}
	
	/**
	 * Passing UUIDs in variables
	 */
   @Test
	public void testUUIDConstruction() throws Exception
	{
		final String uuidName = "13f2cac0-eadf-11b1-9163-e3f1397b9128";
		final NXCPMessageField variable = new NXCPMessageField(1, UUID.fromString(uuidName));

		assertEquals(1, variable.getId());
		assertEquals(NXCPMessageField.TYPE_BINARY, variable.getType());
		assertEquals(uuidName, variable.getAsUUID().toString());
	}
	
   @Test
	public void testUUIDEncodingAndDecoding() throws Exception
	{
		final String uuidName = "13f2cac0-eadf-11b1-9163-e3f1397b9128";
		final NXCPMessageField var1 = new NXCPMessageField(1, UUID.fromString(uuidName));

		final byte[] df = var1.createNXCPDataField();
		assertEquals(32, df.length);
		
		final NXCPMessageField var2 = new NXCPMessageField(df);
		assertEquals(1, var2.getId());
		assertEquals(NXCPMessageField.TYPE_BINARY, var2.getType());
		assertEquals(uuidName, var2.getAsUUID().toString());
	}
}
