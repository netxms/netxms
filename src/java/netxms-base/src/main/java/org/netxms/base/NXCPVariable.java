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

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.Arrays;
import java.util.UUID;

/**
 * NXCP message field (variable)
 */
public class NXCPVariable
{
	public static final int TYPE_INTEGER = 0;
	public static final int TYPE_STRING = 1;
	public static final int TYPE_INT64 = 2;
	public static final int TYPE_INT16 = 3;
	public static final int TYPE_BINARY = 4;
	public static final int TYPE_FLOAT = 5;

	private long variableId;
	private int variableType;

	private Long integerValue;
	private Double realValue;
	private String stringValue;
	private byte[] binaryValue;

	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString()
	{
		StringBuilder result = new StringBuilder();
		String NEW_LINE = System.getProperty("line.separator");

		result.append(this.getClass().getName() + " Object {" + NEW_LINE);
		result.append(" variableId = " + Long.toString(variableId) + NEW_LINE);
		result.append(" variableType = " + variableType + NEW_LINE);
		result.append(" integerValue = " + integerValue + NEW_LINE);
		result.append(" realValue = " + realValue + NEW_LINE);
		result.append(" stringValue = " + stringValue + NEW_LINE);
		result.append(" binaryValue = " + Arrays.toString(binaryValue) + NEW_LINE);
		result.append("}");

		return result.toString();
	}
	
	/**
	 * Set string value and numeric values if possible
	 * 
	 * @param value New string value
	 */
	private void setStringValue(String value)
	{
		stringValue = (value != null) ? value : "";
		try
		{
			integerValue = Long.parseLong(stringValue);
		}
		catch (NumberFormatException e)
		{
			integerValue = 0L;
		}
		try
		{
			realValue = Double.parseDouble(stringValue);
		}
		catch (NumberFormatException e)
		{
			realValue = (double)0;
		}
	}

	/**
	 * @param varId
	 * @param varType
	 * @param value
	 */
	public NXCPVariable(final long varId, final int varType, final Long value)
	{
		variableId = varId;
		variableType = varType;
		integerValue = value;
		stringValue = integerValue.toString();
		realValue = integerValue.doubleValue();
	}

	/**
	 * @param varId
	 * @param value
	 */
	public NXCPVariable(final long varId, final String value)
	{
		variableId = varId;
		variableType = TYPE_STRING;
		setStringValue(value);
	}

	/**
	 * @param varId
	 * @param value
	 */
	public NXCPVariable(final long varId, final Double value)
	{
		variableId = varId;
		variableType = TYPE_FLOAT;
		realValue = value;
		stringValue = value.toString();
		integerValue = realValue.longValue();
	}

	/**
	 * @param varId
	 * @param value
	 */
	public NXCPVariable(final long varId, final byte[] value)
	{
		variableId = varId;
		variableType = TYPE_BINARY;
		binaryValue = value;
		stringValue = "";
		integerValue = (long)0;
		realValue = (double)0;
	}

	/**
	 * Create binary variable from long[]
	 * 
	 * @param varId Variable ID
	 * @param value Value
	 */
	public NXCPVariable(final long varId, final long[] value)
	{
		variableId = varId;
		variableType = TYPE_BINARY;

		final ByteArrayOutputStream byteStream = new ByteArrayOutputStream(value.length * 4);
		final DataOutputStream out = new DataOutputStream(byteStream);
		try
		{
			for(int i = 0; i < value.length; i++)
				out.writeInt((int)value[i]);
		}
		catch(IOException e)
		{
		}
		binaryValue = byteStream.toByteArray();
		stringValue = "";
		integerValue = (long)0;
		realValue = (double)0;
	}

	/**
	 * Create binary variable from Long[]
	 * 
	 * @param varId Variable ID
	 * @param value Value
	 */
	public NXCPVariable(final long varId, final Long[] value)
	{
		variableId = varId;
		variableType = TYPE_BINARY;

		final ByteArrayOutputStream byteStream = new ByteArrayOutputStream(value.length * 4);
		final DataOutputStream out = new DataOutputStream(byteStream);
		try
		{
			for(int i = 0; i < value.length; i++)
				out.writeInt(value[i].intValue());
		}
		catch(IOException e)
		{
		}
		binaryValue = byteStream.toByteArray();
		stringValue = "";
		integerValue = (long)0;
		realValue = (double)0;
	}

	/**
	 * @param varId
	 * @param value
	 */
	public NXCPVariable(final long varId, final InetAddress value)
	{
		variableId = varId;
		variableType = TYPE_INTEGER;
		
		byte[] addr = value.getAddress();
		integerValue = ((long)(addr[0] & 0xFF) << 24) | ((long)(addr[1] & 0xFF) << 16) | ((long)(addr[2] & 0xFF) << 8) | (long)(addr[3] & 0xFF);
		realValue = integerValue.doubleValue();
		stringValue = integerValue.toString();
	}

	/**
	 * @param varId
	 * @param value
	 */
	public NXCPVariable(final long varId, final UUID value)
	{
		variableId = varId;
		variableType = TYPE_BINARY;

		final ByteArrayOutputStream byteStream = new ByteArrayOutputStream(16);
		final DataOutputStream out = new DataOutputStream(byteStream);
		try
		{
			out.writeLong(value.getMostSignificantBits());
			out.writeLong(value.getLeastSignificantBits());
		}
		catch(IOException e)
		{
		}
		binaryValue = byteStream.toByteArray();
		stringValue = "";
		integerValue = (long)0;
		realValue = (double)0;
	}

	/**
	 * Create NXCPVariable from NXCP message data field
	 *
	 * @param nxcpDataField
	 * @throws java.io.IOException
	 */
	@SuppressWarnings("resource")
	public NXCPVariable(final byte[] nxcpDataField) throws IOException
	{
		NXCPDataInputStream in = new NXCPDataInputStream(nxcpDataField);

		variableId = (long)in.readUnsignedInt();
		variableType = in.readUnsignedByte();
		in.skipBytes(1);	// Skip padding
		if (variableType == TYPE_INT16)
		{
			integerValue = (long)in.readUnsignedShort();
			realValue = integerValue.doubleValue();
			stringValue = integerValue.toString();
		}
		else
		{
			in.skipBytes(2);
			switch(variableType)
			{
				case TYPE_INTEGER:
					integerValue = in.readUnsignedInt();
					realValue = integerValue.doubleValue();
					stringValue = integerValue.toString();
					break;
				case TYPE_INT64:
					integerValue = in.readLong();
					realValue = integerValue.doubleValue();
					stringValue = integerValue.toString();
					break;
				case TYPE_FLOAT:
					realValue = in.readDouble();
					integerValue = realValue.longValue();
					stringValue = realValue.toString();
					break;
				case TYPE_STRING:
					int len = in.readInt() / 2;
					StringBuilder sb = new StringBuilder(len);
					while (len > 0)
					{
						sb.append(in.readChar());
						len--;
					}
					setStringValue(sb.toString());
					break;
				case TYPE_BINARY:
					binaryValue = new byte[in.readInt()];
					in.readFully(binaryValue);
					break;
			}
		}
	}

	/**
	 * Get variable's value as long integer
	 * 
	 * @return Variable's value as long integer
	 */
	public Long getAsInteger()
	{
		return integerValue;
	}

	/**
	 * Get variable's value as floating point number
	 * 
	 * @return Variable's value as floating point number
	 */
	public Double getAsReal()
	{
		return realValue;
	}

	/**
	 * Get variable's value as string
	 * 
	 * @return Variable's value as string
	 */
	public String getAsString()
	{
		return stringValue;
	}

	/**
	 * Get variable's value as byte array
	 * 
	 * @return Variable's value as byte array
	 */
	public byte[] getAsBinary()
	{
		return binaryValue;
	}

	/**
	 * Get variable's value as IP address
	 * 
	 * @return Variable's value as IP address
	 */
	public InetAddress getAsInetAddress()
	{
		final byte[] addr = new byte[4];
		final long intVal = integerValue.longValue();
		InetAddress inetAddr;
		
		addr[0] =  (byte)((intVal & 0xFF000000) >> 24);
		addr[1] =  (byte)((intVal & 0x00FF0000) >> 16);
		addr[2] =  (byte)((intVal & 0x0000FF00) >> 8);
		addr[3] =  (byte)(intVal & 0x000000FF);
		
		try
		{
			inetAddr = InetAddress.getByAddress(addr);
		}
		catch(UnknownHostException e)
		{
			inetAddr = null;
		}
		return inetAddr;
	}
	
	/**
	 * Get variable's value as UUID
	 * 
	 * @return Variable's value as UUID
	 */
	@SuppressWarnings("resource")
	public UUID getAsUUID()
	{
		if ((variableType != TYPE_BINARY) || (binaryValue == null) || (binaryValue.length != 16))
			return null;
		
		NXCPDataInputStream in = new NXCPDataInputStream(binaryValue);
		long hiBits, loBits;
		try
		{
			hiBits = in.readLong();
			loBits = in.readLong();
		}
		catch(IOException e)
		{
			hiBits = 0;
			loBits = 0;
		}
		
		return new UUID(hiBits, loBits);
	}
	
	/**
	 * Get variable's value as array of long integers. Variable should be of
	 * binary type, and integer values should be packet as DWORD's in network byte order.
	 * 
	 * @return Variable's value as array of long integers
	 */
	@SuppressWarnings("resource")
	public long[] getAsUInt32Array()
	{
		if ((variableType != TYPE_BINARY) || (binaryValue == null))
			return null;
		
		int count = binaryValue.length / 4;
		long[] value = new long[count];
		NXCPDataInputStream in = new NXCPDataInputStream(binaryValue);
		try
		{
			for(int i = 0; i < count; i++)
				value[i] = in.readUnsignedInt();
		}
		catch(IOException e)
		{
		}
		return value;
	}
	
	/**
	 * Get variable's value as array of long integers. Variable should be of
	 * binary type, and integer values should be packet as DWORD's in network byte order.
	 * 
	 * @return Variable's value as array of long integers
	 */
	@SuppressWarnings("resource")
	public Long[] getAsUInt32ArrayEx()
	{
		if ((variableType != TYPE_BINARY) || (binaryValue == null))
			return null;
		
		int count = binaryValue.length / 4;
		Long[] value = new Long[count];
		NXCPDataInputStream in = new NXCPDataInputStream(binaryValue);
		try
		{
			for(int i = 0; i < count; i++)
				value[i] = in.readUnsignedInt();
		}
		catch(IOException e)
		{
		}
		return value;
	}

	/**
	 * @return the variableId
	 */
	public long getVariableId()
	{
		return variableId;
	}

	/**
	 * @param variableId the variableId to set
	 */
	public void setVariableId(final long variableId)
	{
		this.variableId = variableId;
	}

	/**
	 * @return the variableType
	 */
	public int getVariableType()
	{
		return variableType;
	}

	/**
	 *
	 * @return
	 */
	private int calculateBinarySize()
	{
		final int size;

		switch(variableType)
		{
			case TYPE_INTEGER:
				size = 12;
				break;
			case TYPE_INT64:
			case TYPE_FLOAT:
				size = 16;
				break;
			case TYPE_INT16:
				size = 8;
				break;
			case TYPE_STRING:
				size = stringValue.length() * 2 + 12;
				break;
			case TYPE_BINARY:
				size = binaryValue.length + 12;
				break;
			default:
				size = 8;
				break;
		}
		return size;
	}

	/**
	 * Create NXCP DF structure
	 * 
	 * @return
	 * @throws IOException
	 */
	public byte[] createNXCPDataField() throws IOException
	{
		final ByteArrayOutputStream byteStream = new ByteArrayOutputStream(calculateBinarySize());
		final DataOutputStream out = new DataOutputStream(byteStream);

		out.writeInt(Long.valueOf(variableId).intValue());
		out.writeByte(Long.valueOf(variableType).byteValue());
		out.writeByte(0);		// Padding
		if (variableType == TYPE_INT16)
		{
			out.writeShort(integerValue.shortValue());
		}
		else
		{
			out.writeShort(0);	// Padding
			switch(variableType)
			{
				case TYPE_INTEGER:
					out.writeInt(integerValue.intValue());
					break;
				case TYPE_INT64:
					out.writeLong(integerValue);
					break;
				case TYPE_FLOAT:
					out.writeDouble(realValue);
					break;
				case TYPE_STRING:
					out.writeInt(stringValue.length() * 2);
					out.writeChars(stringValue);
					break;
				case TYPE_BINARY:
					out.writeInt(binaryValue.length);
					out.write(binaryValue);
					break;
			}
		}

		// Align to 8-bytes boundary
		final int rem = byteStream.size() % 8;
		if (rem != 0)
		{
			out.write(new byte[8 - rem]);
		}

		return byteStream.toByteArray();
	}
}
