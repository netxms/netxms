/**
 *
 */
package org.netxms.base;

import java.io.*;
import java.util.HashMap;
import java.util.Map;

public class NXCPMessage
{
	private int messageCode;
	private long messageId;
	private Map<Long, NXCPVariable> variableMap = new HashMap<Long, NXCPVariable>(0);

	/**
	 * @param msgCode
	 */
	public NXCPMessage(final int msgCode)
	{
		this.messageCode = msgCode;
		messageId = 0L;
	}

	/**
	 * @param msgCode
	 * @param msgId
	 */
	public NXCPMessage(final int msgCode, final long msgId)
	{
		this.messageCode = msgCode;
		this.messageId = msgId;
	}

	/**
	 * Create NXCPMessage from binary NXCP message
	 *
	 * @param nxcpMessage binary NXCP message
	 * @throws java.io.IOException
	 */
	public NXCPMessage(final byte[] nxcpMessage) throws IOException
	{
		final ByteArrayInputStream byteArrayInputStream = new ByteArrayInputStream(nxcpMessage);
		//noinspection IOResourceOpenedButNotSafelyClosed
		final DataInputStream inputStream = new DataInputStream(byteArrayInputStream);

		messageCode = inputStream.readUnsignedShort();
		inputStream.skipBytes(6);	// Message flags and size
		messageId = (long)inputStream.readInt();
		final int numVars = inputStream.readInt();

		for(int i = 0; i < numVars; i++)
		{
			byteArrayInputStream.mark(byteArrayInputStream.available());
			
			// Read first 8 bytes - any DF (data field) is at least 8 bytes long
			byte[] df = new byte[16];
			inputStream.readFully(df, 0, 8);

			switch(df[4])
			{
				case NXCPVariable.TYPE_INT16:
					break;
				case NXCPVariable.TYPE_FLOAT:		// all these types requires additional 8 bytes
				case NXCPVariable.TYPE_INTEGER:
				case NXCPVariable.TYPE_INT64:
					inputStream.readFully(df, 8, 8);
					break;
				case NXCPVariable.TYPE_STRING:		// all these types has 4-byte length field followed by actual content
				case NXCPVariable.TYPE_BINARY:
					int size = inputStream.readInt();
					byteArrayInputStream.reset();
					df = new byte[size + 12];
					inputStream.readFully(df);
					
					// Each df aligned to 8-bytes boundary
					final int rem = (size + 12) % 8;
					if (rem != 0)
					{
						inputStream.skipBytes(8 - rem);
					}
					break;
			}

			final NXCPVariable variable = new NXCPVariable(df);
			variableMap.put(variable.getVariableId(), variable);
		}
	}

	
	/**
	 * @return the msgCode
	 */
	public int getMessageCode()
	{
		return messageCode;
	}

	/**
	 * @param msgCode the msgCode to set
	 */
	public void setMessageCode(final int msgCode)
	{
		this.messageCode = msgCode;
	}

	/**
	 * @return the msgId
	 */
	public long getMessageId()
	{
		return messageId;
	}

	/**
	 * @param msgId the msgId to set
	 */
	public void setMessageId(final long msgId)
	{
		this.messageId = msgId;
	}


	/**
	 * @param varId variable Id to find
	 */
	public NXCPVariable findVariable(final long varId)
	{
		return variableMap.get(varId);
	}

	
	//
	// Getters/Setters for variables
	//
	
	public void setVariable(final NXCPVariable variable)
	{
		variableMap.put(variable.getVariableId(), variable);
	}

	public void setVariable(final long varId, final byte[] value)
	{
		setVariable(new NXCPVariable(varId, value));
	}

	public void setVariable(final long varId, final String value)
	{
		setVariable(new NXCPVariable(varId, value));
	}

	public void setVariable(final long varId, final Double value)
	{
		setVariable(new NXCPVariable(varId, value));
	}

	public void setVariableInt64(final long varId, final long value)
	{
		setVariable(new NXCPVariable(varId, NXCPVariable.TYPE_INT64, value));
	}

	public void setVariableInt32(final long varId, final int value)
	{
		setVariable(new NXCPVariable(varId, NXCPVariable.TYPE_INTEGER, (long)value));
	}

	public void setVariableInt16(final long varId, final int value)
	{
		setVariable(new NXCPVariable(varId, NXCPVariable.TYPE_INT16, (long)value));
	}


	//
	//	Create binary NXCP message
	//
	
	public byte[] createNXCPMessage() throws IOException
	{
		ByteArrayOutputStream byteStream = new ByteArrayOutputStream();
		//noinspection IOResourceOpenedButNotSafelyClosed
		DataOutputStream outputStream = new DataOutputStream(byteStream);

		// Create byte array with all messages
		for (final NXCPVariable nxcpVariable : variableMap.values())
		{
			final byte[] field = nxcpVariable.createNXCPDataField();
			outputStream.write(field);
		}
		final byte[] payload = byteStream.toByteArray();

		// Create message header in new byte stream and add payload
		byteStream = new ByteArrayOutputStream();
		//noinspection IOResourceOpenedButNotSafelyClosed
		outputStream = new DataOutputStream(byteStream);
		outputStream.writeShort(messageCode);
		outputStream.writeShort(0);	// Message flags
		outputStream.writeInt(payload.length + 16);	   // Size
		outputStream.writeInt((int)messageId);
		outputStream.writeInt(variableMap.size());
		outputStream.write(payload);

		return byteStream.toByteArray();
	}
}
