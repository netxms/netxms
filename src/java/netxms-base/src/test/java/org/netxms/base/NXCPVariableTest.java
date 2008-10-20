package org.netxms.base;

import java.net.InetAddress;
import java.util.Arrays;
import junit.framework.TestCase;

public class NXCPVariableTest extends TestCase
{
	// 
	// String variables
	//
	
	public void testStringConstruction()
	{
		final NXCPVariable variable = new NXCPVariable(1, "Sample String");

		assertEquals(1, variable.getVariableId());
		assertEquals(NXCPVariable.TYPE_STRING, variable.getVariableType());
		assertEquals("Sample String", variable.getAsString());
	}

	public void testStringEncodingAndDecoding() throws Exception
	{
		final NXCPVariable var1 = new NXCPVariable(1, "Sample String");
		
		final byte[] df = var1.createNXCPDataField();
		assertEquals(40, df.length);
		
		final NXCPVariable var2 = new NXCPVariable(df);
		assertEquals(1, var2.getVariableId());
		assertEquals(NXCPVariable.TYPE_STRING, var2.getVariableType());
		assertEquals("Sample String", var2.getAsString());
	}

	
	//
	// Int32 variables
	//
	
	public void testInt32Construction()
	{
		final NXCPVariable variable = new NXCPVariable(1, NXCPVariable.TYPE_INTEGER, 17L);

		assertEquals(1, variable.getVariableId());
		assertEquals(NXCPVariable.TYPE_INTEGER, variable.getVariableType());
		assertEquals(17, variable.getAsInteger().intValue());
	}

	public void testInt32EncodingAndDecoding() throws Exception
	{
		final NXCPVariable var1 = new NXCPVariable(1, NXCPVariable.TYPE_INTEGER, 17L);
		
		final byte[] df = var1.createNXCPDataField();
		final NXCPVariable var2 = new NXCPVariable(df);

		assertEquals(1, var2.getVariableId());
		assertEquals(NXCPVariable.TYPE_INTEGER, var2.getVariableType());
		assertEquals(17, var2.getAsInteger().intValue());
	}

	
	//
	// Int64 variables
	//
	
	public void testInt64Construction()
	{
		final NXCPVariable variable = new NXCPVariable(1, NXCPVariable.TYPE_INT64, 123456789L);

		assertEquals(1, variable.getVariableId());
		assertEquals(NXCPVariable.TYPE_INT64, variable.getVariableType());
		assertEquals(123456789L, variable.getAsInteger().longValue());
	}

	public void testInt64EncodingAndDecoding() throws Exception
	{
		final NXCPVariable var1 = new NXCPVariable(1, NXCPVariable.TYPE_INT64, 123456789L);
		
		final byte[] df = var1.createNXCPDataField();
		final NXCPVariable var2 = new NXCPVariable(df);

		assertEquals(1, var2.getVariableId());
		assertEquals(NXCPVariable.TYPE_INT64, var2.getVariableType());
		assertEquals(123456789L, var2.getAsInteger().longValue());
	}

	
	//
	// Binary variables
	//
	
	public void testBinaryConstruction()
	{
		final byte[] byteArray = { 0x10, 0x20, 0x30 };
		final NXCPVariable variable = new NXCPVariable(1, byteArray);

		assertEquals(1, variable.getVariableId());
		assertEquals(NXCPVariable.TYPE_BINARY, variable.getVariableType());
		assertEquals(byteArray, variable.getAsBinary());
	}

	public void testBinaryEncodingAndDecoding() throws Exception
	{
		final byte[] byteArray = { 0x10, 0x20, 0x30, 0x40, 0x50 };
		final NXCPVariable var1 = new NXCPVariable(1, byteArray);
		
		final byte[] df = var1.createNXCPDataField();
		assertEquals(24, df.length);
		
		final NXCPVariable var2 = new NXCPVariable(df);
		assertEquals(1, var2.getVariableId());
		assertEquals(NXCPVariable.TYPE_BINARY, var2.getVariableType());
		assertEquals(true, Arrays.equals(byteArray, var2.getAsBinary()));
	}

	
	//
	// Test getAsInetAddress()
	//
	
	public void testGetAsInetAddress() throws Exception
	{
		final NXCPVariable variable = new NXCPVariable(1, NXCPVariable.TYPE_INTEGER, 0x0A000102L);

		assertEquals(1, variable.getVariableId());
		assertEquals(NXCPVariable.TYPE_INTEGER, variable.getVariableType());
		assertEquals(InetAddress.getByName("10.0.1.2"), variable.getAsInetAddress());
	}
}
