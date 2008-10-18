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
	 */
	public NXCPMessage(final byte[] nxcpMessage) throws IOException
	{
		DataInputStream in = new DataInputStream(new ByteArrayInputStream(nxcpMessage));

		msgCode = in.readUnsignedShort();
		in.skipBytes(6);	// Message flags and size
		msgId = (long)in.readInt();
		final int numVars = in.readInt();

		for(int i = 0; i < numVars; i++)
		{
			// Read first 8 bytes - any DF is at least 8 bytes long
			final byte[] dfTmp = new byte[8];
			in.readFully(dfTmp);
			final NXCPVariable var;
			if (dfTmp[4] == (byte)NXCPVariable.TYPE_INT16)
			{
				// DF of type INT16 is 8 bytes long, no additional data needed
				var = new NXCPVariable(dfTmp);
			}
			else
			{
				byte[] df = null;
				
				switch(dfTmp[4])
				{
					case NXCPVariable.TYPE_FLOAT:		// all these types requires additional 8 bytes
					case NXCPVariable.TYPE_INTEGER:
					case NXCPVariable.TYPE_INT64:
						df = new byte[16];
						System.arraycopy(dfTmp, 0, df, 0, 8);
						in.readFully(dfTmp);
						System.arraycopy(dfTmp, 0, df, 8, 8);
						break;
					case NXCPVariable.TYPE_STRING:		// all these types has 4-byte length field followed by actual content
					case NXCPVariable.TYPE_BINARY:
						break;
				}
				var = new NXCPVariable(df);
			}
			variableMap.put(var.getVariableId(), var);
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
	
	
	/*
	 * @param varId variable Id to find 
	 */
	public NXCPVariable findVariable(final Long varId)
	{
		return variableMap.get(varId);
	}

	
	/***************************************
	 * Variable setters
	 */
	
	/*
	 * @param var variable to add/replace 
	 */
	public void setVariable(final NXCPVariable var)
	{
		variableMap.put(var.getVariableId(), var);
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
	
	
	/*
	 * Create binary NXCP message
	 */
	public byte[] createNXCPMessage() throws IOException
	{
		ByteArrayOutputStream byteStream = new ByteArrayOutputStream();
		DataOutputStream out = new DataOutputStream(byteStream);
		
		// Create byte array with all messages
		for(NXCPVariable nxcpVariable: variableMap.values())
		{
		    out.write(nxcpVariable.createNXCPDataField());
		}
		final byte[] payload = byteStream.toByteArray();
		
		// Create message header in new byte stream and add payload
		byteStream = new ByteArrayOutputStream();
		out = new DataOutputStream(byteStream);
		out.writeShort(msgCode);
		out.writeShort(0);	// Message flags
		out.writeInt(payload.length);	   // Size
		out.writeInt(msgId.intValue());
		out.writeInt(variableMap.size());
		out.write(payload);
		
		return byteStream.toByteArray();
	}
}
