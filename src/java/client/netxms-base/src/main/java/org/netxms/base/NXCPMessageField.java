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
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.Arrays;
import java.util.UUID;

/**
 * NXCP message field (variable)
 */
public class NXCPMessageField
{
	public static final int TYPE_INTEGER = 0;
	public static final int TYPE_STRING = 1;
	public static final int TYPE_INT64 = 2;
	public static final int TYPE_INT16 = 3;
	public static final int TYPE_BINARY = 4;
	public static final int TYPE_FLOAT = 5;
   public static final int TYPE_INETADDR = 6;
   
   private static final int SIGNED = 0x01;
   
   private static final byte[] PADDING = new byte[16];

	private long id;
	private int type;

	private Long integerValue;
	private Double realValue;
	private String stringValue;
	private byte[] binaryValue;
	private InetAddressEx inetAddressValue;

	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString()
	{
		StringBuilder result = new StringBuilder();
		String NEW_LINE = System.getProperty("line.separator");

		result.append(this.getClass().getName() + " Object {" + NEW_LINE);
		result.append(" variableId = " + Long.toString(id) + NEW_LINE);
		result.append(" variableType = " + type + NEW_LINE);
		result.append(" integerValue = " + integerValue + NEW_LINE);
		result.append(" realValue = " + realValue + NEW_LINE);
		result.append(" stringValue = " + stringValue + NEW_LINE);
		result.append(" binaryValue = " + Arrays.toString(binaryValue) + NEW_LINE);
      result.append(" inetAddressValue = " + inetAddressValue.toString() + NEW_LINE);
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
	public NXCPMessageField(final long varId, final int varType, final Long value)
	{
		id = varId;
		type = varType;
		integerValue = value;
		stringValue = integerValue.toString();
		realValue = integerValue.doubleValue();
	}

	/**
	 * @param varId
	 * @param value
	 */
	public NXCPMessageField(final long varId, final String value)
	{
		id = varId;
		type = TYPE_STRING;
		setStringValue(value);
	}

	/**
	 * @param varId
	 * @param value
	 */
	public NXCPMessageField(final long varId, final Double value)
	{
		id = varId;
		type = TYPE_FLOAT;
		realValue = value;
		stringValue = value.toString();
		integerValue = realValue.longValue();
	}

	/**
	 * @param varId
	 * @param value
	 */
	public NXCPMessageField(final long varId, final byte[] value)
	{
		id = varId;
		type = TYPE_BINARY;
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
	public NXCPMessageField(final long varId, final long[] value)
	{
		id = varId;
		type = TYPE_BINARY;

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
	public NXCPMessageField(final long varId, final Long[] value)
	{
		id = varId;
		type = TYPE_BINARY;

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
	public NXCPMessageField(final long varId, final InetAddress value)
	{
		id = varId;
      type = TYPE_INETADDR;
      inetAddressValue = new InetAddressEx(value, (value instanceof Inet4Address) ? 32 : 128);
      stringValue = inetAddressValue.toString();
      integerValue = (long)0;
      realValue = (double)0;
	}

	/**
	 * @param varId
	 * @param value
	 */
	public NXCPMessageField(final long varId, final UUID value)
	{
		id = varId;
		type = TYPE_BINARY;

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
	public NXCPMessageField(final byte[] nxcpDataField) throws IOException
	{
		NXCPDataInputStream in = new NXCPDataInputStream(nxcpDataField);

		id = (long)in.readUnsignedInt();
		type = in.readUnsignedByte();
		int flags = in.readUnsignedByte();
		if (type == TYPE_INT16)
		{
			integerValue = (long)(((flags & SIGNED) != 0) ? in.readShort() : in.readUnsignedShort());
			realValue = integerValue.doubleValue();
			stringValue = integerValue.toString();
		}
		else
		{
			in.skipBytes(2);
			switch(type)
			{
				case TYPE_INTEGER:
					integerValue = ((flags & SIGNED) != 0) ? in.readInt() : in.readUnsignedInt();
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
				case TYPE_INETADDR:
				   binaryValue = new byte[16];
				   in.readFully(binaryValue);
				   int family = in.readUnsignedByte();
				   int bits = in.readUnsignedByte();
				   in.skipBytes(6);
				   inetAddressValue = new InetAddressEx(
				         InetAddress.getByAddress((family == 0) ? Arrays.copyOf(binaryValue, 4) : binaryValue), bits);
				   stringValue = inetAddressValue.toString();
				   break;
			}
		}
		in.close();
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
	 * Get field's value as IP address
	 * 
	 * @return Field's value as IP address
	 */
	public InetAddress getAsInetAddress()
	{
	   if (type == TYPE_INETADDR)
	   {
	      return inetAddressValue.address;
	   }
	   else if (type == TYPE_BINARY)
	   {
         try
         {
            return InetAddress.getByAddress(binaryValue);
         }
         catch(UnknownHostException e)
         {
            return null;
         }
	   }
	   else
	   {
   		final byte[] addr = new byte[4];
   		final long intVal = integerValue.longValue();
   		
   		addr[0] =  (byte)((intVal & 0xFF000000) >> 24);
   		addr[1] =  (byte)((intVal & 0x00FF0000) >> 16);
   		addr[2] =  (byte)((intVal & 0x0000FF00) >> 8);
   		addr[3] =  (byte)(intVal & 0x000000FF);
   		
   		try
   		{
   			return InetAddress.getByAddress(addr);
   		}
   		catch(UnknownHostException e)
   		{
   			return null;
   		}
	   }
	}

   /**
    * Get field's value as IP address/mask pair
    * 
    * @return Field's value as IP address/mask pair
    */
   public InetAddressEx getAsInetAddressEx()
   {
      if (type == TYPE_INETADDR)
      {
         return inetAddressValue;
      }
      else if (type == TYPE_BINARY)
      {
         try
         {
            return new InetAddressEx(InetAddress.getByAddress(binaryValue), binaryValue.length * 8);
         }
         catch(UnknownHostException e)
         {
            return null;
         }
      }
      else
      {
         final byte[] addr = new byte[4];
         final long intVal = integerValue.longValue();
         
         addr[0] =  (byte)((intVal & 0xFF000000) >> 24);
         addr[1] =  (byte)((intVal & 0x00FF0000) >> 16);
         addr[2] =  (byte)((intVal & 0x0000FF00) >> 8);
         addr[3] =  (byte)(intVal & 0x000000FF);
         
         try
         {
            return new InetAddressEx(InetAddress.getByAddress(addr), 32);
         }
         catch(UnknownHostException e)
         {
            return null;
         }
      }
   }
   
	/**
	 * Get variable's value as UUID
	 * 
	 * @return Variable's value as UUID
	 */
	public UUID getAsUUID()
	{
		if ((type != TYPE_BINARY) || (binaryValue == null) || (binaryValue.length != 16))
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
		
		in.close();
		return new UUID(hiBits, loBits);
	}
	
	/**
	 * Get variable's value as array of long integers. Variable should be of
	 * binary type, and integer values should be packet as DWORD's in network byte order.
	 * 
	 * @return Variable's value as array of long integers
	 */
	public long[] getAsUInt32Array()
	{
		if ((type != TYPE_BINARY) || (binaryValue == null))
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
		in.close();
		return value;
	}
	
	/**
	 * Get variable's value as array of long integers. Variable should be of
	 * binary type, and integer values should be packet as DWORD's in network byte order.
	 * 
	 * @return Variable's value as array of long integers
	 */
	public Long[] getAsUInt32ArrayEx()
	{
		if ((type != TYPE_BINARY) || (binaryValue == null))
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
		in.close();
		return value;
	}

	/**
	 * @return the variableId
	 */
	public long getVariableId()
	{
		return id;
	}

	/**
	 * @param variableId the variableId to set
	 */
	public void setVariableId(final long variableId)
	{
		this.id = variableId;
	}

	/**
	 * @return the variableType
	 */
	public int getVariableType()
	{
		return type;
	}

	/**
	 *
	 * @return
	 */
	private int calculateBinarySize()
	{
		final int size;

		switch(type)
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
			case TYPE_INETADDR:
			   size = 32;
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

		out.writeInt(Long.valueOf(id).intValue());
		out.writeByte(Long.valueOf(type).byteValue());
		out.writeByte(0);		// Padding
		if (type == TYPE_INT16)
		{
			out.writeShort(integerValue.shortValue());
		}
		else
		{
			out.writeShort(0);	// Padding
			switch(type)
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
				case TYPE_INETADDR:
				   if (inetAddressValue.address instanceof Inet4Address)
				   {
                  out.write(inetAddressValue.address.getAddress());
                  out.write(PADDING, 0, 12);
	               out.writeByte(0);
				   }
				   else
				   {
	               out.write(inetAddressValue.address.getAddress());
	               out.writeByte(1);
				   }
				   out.writeByte(inetAddressValue.mask);
               out.write(PADDING, 0, 6);
				   break;
			}
		}

		// Align to 8-bytes boundary
		final int rem = byteStream.size() % 8;
		if (rem != 0)
		{
			out.write(PADDING, 0, 8 - rem);
		}

		return byteStream.toByteArray();
	}
}
