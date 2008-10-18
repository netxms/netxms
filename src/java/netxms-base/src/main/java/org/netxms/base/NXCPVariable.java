package org.netxms.base;

import java.io.*;

/**
 * @author victor
 *
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
		stringValue = value;
		try
		{
			integerValue = Long.parseLong(stringValue);
		}
		catch(NumberFormatException e)
		{
			integerValue = (long)0;
		}
		try
		{
			realValue = Double.parseDouble(stringValue);
		}
		catch(NumberFormatException e)
		{
			realValue = (double)0;
		}
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
	 * Create NXCPVariable from NXCP message data field
	 *
	 * @param nxcpDataField
         * @throws java.io.IOException
	 */
	public NXCPVariable(final byte[] nxcpDataField) throws IOException
	{
		DataInputStream in = new DataInputStream(new ByteArrayInputStream(nxcpDataField));

		variableId = (long)in.readInt();
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
					integerValue = (long)in.readInt();
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
					int len = in.readInt();
					while(len > 0)
					{
						stringValue += in.readChar();
						len--;
					}
					integerValue = Long.parseLong(stringValue);
					realValue = Double.parseDouble(stringValue);
					break;
				case TYPE_BINARY:
					binaryValue = new byte[in.readInt()];
					in.readFully(binaryValue);
					break;
			}
		}
	}


	public Long getAsInteger()
	{
		return integerValue;
	}

	public Double getAsReal()
	{
		return realValue;
	}

	public String getAsString()
	{
		return stringValue;
	}

	public byte[] getAsBinary()
	{
		return binaryValue;
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
	 * Create NXCP DF structure
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
            size = stringValue.length() + 12;
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
