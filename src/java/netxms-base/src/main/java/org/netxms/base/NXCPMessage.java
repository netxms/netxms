/**
 *
 */
package org.netxms.base;

import java.io.*;
import java.util.HashMap;
import java.util.Map;

public class NXCPMessage
{
	private Integer msgCode;
	private Long msgId;
	private Map<Long, NXCPVariable> variableMap = new HashMap<Long, NXCPVariable>(0);

	/**
	 * @param msgCode
	 */
	public NXCPMessage(final Integer msgCode)
	{
		this.msgCode = msgCode;
		msgId = 0L;
	}

	/**
	 * @param msgCode
	 * @param msgId
	 */
	public NXCPMessage(final Integer msgCode, final Long msgId)
	{
		this.msgCode = msgCode;
		this.msgId = msgId;
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

		msgCode = inputStream.readUnsignedShort();
		inputStream.skipBytes(6);	// Message flags and size
		msgId = (long)inputStream.readInt();
		final int numVars = inputStream.readInt();

		for (int i = 0; i < numVars; i++)
		{
			// Read first 8 bytes - any DF is at least 8 bytes long
			final byte[] df = new byte[16];
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
					break;
			}

			final NXCPVariable variable = new NXCPVariable(df);

			variableMap.put(variable.getVariableId(), variable);
		}
	}

	/**
	 * @return the msgCode
	 */
	public Integer getMsgCode()
	{
		return msgCode;
	}

	/**
	 * @param msgCode the msgCode to set
	 */
	public void setMsgCode(final Integer msgCode)
	{
		this.msgCode = msgCode;
	}

	/**
	 * @return the msgId
	 */
	public Long getMsgId()
	{
		return msgId;
	}

	/**
	 * @param msgId the msgId to set
	 */
	public void setMsgId(final Long msgId)
	{
		this.msgId = msgId;
	}


	/**
	 * @param varId variable Id to find
	 */
	public NXCPVariable findVariable(final Long varId)
	{
		return variableMap.get(varId);
	}

	//
	// Getters/Setters
	//
	public void setVariable(final NXCPVariable variable)
	{
		final long id = variable.getVariableId();
		variableMap.put(id, variable);
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

	public void setVariableInt64(final long varId, final Long value)
	{
		setVariable(new NXCPVariable(varId, NXCPVariable.TYPE_INT64, value));
	}

	public void setVariableInt32(final long varId, final Long value)
	{
		setVariable(new NXCPVariable(varId, NXCPVariable.TYPE_INTEGER, value));
	}

	public void setVariableInt16(final long varId, final Long value)
	{
		setVariable(new NXCPVariable(varId, NXCPVariable.TYPE_INT16, value));
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
			// TODO: check for nulls
			outputStream.write(field);
		}
		final byte[] payload = byteStream.toByteArray();

		// Create message header in new byte stream and add payload
		byteStream = new ByteArrayOutputStream();
		//noinspection IOResourceOpenedButNotSafelyClosed
		outputStream = new DataOutputStream(byteStream);
		outputStream.writeShort(msgCode);
		outputStream.writeShort(0);	// Message flags
		outputStream.writeInt(payload.length);	   // Size
		outputStream.writeInt(msgId.intValue());
		outputStream.writeInt(variableMap.size());
		outputStream.write(payload);

		return byteStream.toByteArray();
	}
}
