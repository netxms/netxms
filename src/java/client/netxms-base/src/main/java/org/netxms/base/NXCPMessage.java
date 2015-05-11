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

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.InetAddress;
import java.security.GeneralSecurityException;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.UUID;
import java.util.zip.CRC32;

public class NXCPMessage
{
	public static final int HEADER_SIZE = 16;
	public static final int ENCRYPTION_HEADER_SIZE = 8;

	// Message flags
	public static final int MF_BINARY = 0x0001;
	public static final int MF_END_OF_FILE = 0x0002;
	public static final int MF_DONT_ENCRYPT = 0x0004;
	public static final int MF_END_OF_SEQUENCE = 0x0008;
	public static final int MF_REVERSE_ORDER = 0x0010;
	public static final int MF_CONTROL = 0x0020;
	
	private int messageCode;
	private int messageFlags;
	private long messageId;
	private Map<Long, NXCPMessageField> fields = new HashMap<Long, NXCPMessageField>(0);
	private long timestamp;
	private byte[] binaryData = null;
	private long controlData = 0;

	/**
	 * @param msgCode
	 */
	public NXCPMessage(final int msgCode)
	{
		this.messageCode = msgCode;
		messageId = 0L;
		messageFlags = 0;
	}

	/**
	 * @param msgCode
	 * @param msgId
	 */
	public NXCPMessage(final int msgCode, final long msgId)
	{
		this.messageCode = msgCode;
		this.messageId = msgId;
		messageFlags = 0;
	}

	/**
	 * Create NXCPMessage from binary NXCP message
	 *
	 * @param nxcpMessage binary NXCP message
	 * @throws java.io.IOException
	 * @throws GeneralSecurityException 
	 * @throws NXCPException 
	 */
	public NXCPMessage(final byte[] nxcpMessage, EncryptionContext ectx) throws IOException, NXCPException
	{
		final ByteArrayInputStream byteArrayInputStream = new ByteArrayInputStream(nxcpMessage);
		final NXCPDataInputStream inputStream = new NXCPDataInputStream(byteArrayInputStream);

		messageCode = inputStream.readUnsignedShort();
		if (messageCode == NXCPCodes.CMD_ENCRYPTED_MESSAGE)
		{
			if (ectx == null)
			{
			   inputStream.close();
				throw new NXCPException(NXCPException.DECRYPTION_ERROR);
			}
			
			int padding = inputStream.readByte();
			inputStream.skipBytes(1);
			int msgLen = inputStream.readInt();
			byte[] payload;
			try
			{
				payload = ectx.decryptMessage(inputStream, msgLen - padding - ENCRYPTION_HEADER_SIZE);
			}
			catch(GeneralSecurityException e)
			{		   
				throw new NXCPException(NXCPException.DECRYPTION_ERROR, e);
			}

			final ByteArrayInputStream payloadByteArrayInputStream = new ByteArrayInputStream(payload);
			final NXCPDataInputStream payloadInputStream = new NXCPDataInputStream(payloadByteArrayInputStream);
			
			CRC32 crc32 = new CRC32();
			crc32.update(payload, 8, payload.length - 8);
			if (payloadInputStream.readUnsignedInt() != crc32.getValue())
			{
			   payloadInputStream.close();
				throw new NXCPException(NXCPException.DECRYPTION_ERROR);
			}
			
			payloadInputStream.skip(4);
			messageCode = payloadInputStream.readUnsignedShort();
			createFromStream(payloadInputStream, payloadByteArrayInputStream);
		}
		else
		{
			createFromStream(inputStream, byteArrayInputStream);
		}
	}
	
	/**
	 * Create NXCPMessage from prepared input byte stream
	 *
	 * @param nxcpMessage binary NXCP message
	 * @throws java.io.IOException
	 */
	private void createFromStream(NXCPDataInputStream inputStream, ByteArrayInputStream byteArrayInputStream) throws IOException
	{
		messageFlags = inputStream.readUnsignedShort();
		inputStream.skipBytes(4);	// Message size
		messageId = (long)inputStream.readInt();
		
		if ((messageFlags & MF_BINARY) == MF_BINARY)
		{
			final int size = inputStream.readInt();
			binaryData = new byte[size];
			inputStream.readFully(binaryData);
		}
		else if ((messageFlags & MF_CONTROL) == MF_CONTROL)
		{
			controlData = inputStream.readUnsignedInt();
		}
		else
		{
			final int numVars = inputStream.readInt();
	
			for(int i = 0; i < numVars; i++)
			{
				byteArrayInputStream.mark(byteArrayInputStream.available());
				
				// Read first 8 bytes - any DF (data field) is at least 8 bytes long
				byte[] df = new byte[32];
				inputStream.readFully(df, 0, 8);
	
				switch(df[4])
				{
					case NXCPMessageField.TYPE_INT16:
						break;
					case NXCPMessageField.TYPE_FLOAT:		// all these types requires additional 8 bytes
					case NXCPMessageField.TYPE_INTEGER:
					case NXCPMessageField.TYPE_INT64:
						inputStream.readFully(df, 8, 8);
						break;
					case NXCPMessageField.TYPE_STRING:		// all these types has 4-byte length field followed by actual content
					case NXCPMessageField.TYPE_BINARY:
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
					case NXCPMessageField.TYPE_INETADDR:    // address always 32 byte long
					   inputStream.readFully(df, 8, 24);
					   break;
				}
	
				final NXCPMessageField variable = new NXCPMessageField(df);
				fields.put(variable.getVariableId(), variable);
			}
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
	 * @return the timestamp
	 */
	public long getTimestamp()
	{
		return timestamp;
	}

	/**
	 * @param timestamp the timestamp to set
	 */
	public void setTimestamp(final long timestamp)
	{
		this.timestamp = timestamp;
	}

	/**
	 * Find field by ID
	 * 
	 * @param fieldId variable Id to find
	 */
	public NXCPMessageField findField(final long fieldId)
	{
		return fields.get(fieldId);
	}

	/**
	 * Set field as copy of another field
	 * 
	 * @param src source field
	 */
	public void setField(final NXCPMessageField src)
	{
		fields.put(src.getVariableId(), src);
	}

	/**
	 * Set field of BINARY type (array of bytes)
	 * 
	 * @param fieldId
	 * @param value
	 */
	public void setField(final long fieldId, final byte[] value)
	{
		setField(new NXCPMessageField(fieldId, value));
	}

	/**
	 * Set field of BINARY type (array of integers)
	 * 
	 * @param fieldId
	 * @param value
	 */
	public void setField(final long fieldId, final long[] value)
	{
		setField(new NXCPMessageField(fieldId, value));
	}

   /**
    * Set field of BINARY type (array of integers)
    * 
    * @param fieldId
    * @param value
    */
	public void setField(final long fieldId, final Long[] value)
	{
		setField(new NXCPMessageField(fieldId, value));
	}

	/**
	 * Set field of STRING type
	 * 
	 * @param fieldId
	 * @param value
	 */
	public void setField(final long fieldId, final String value)
	{
		setField(new NXCPMessageField(fieldId, value));
	}

	/**
	 * Set field of DOUBLE type
	 * 
	 * @param fieldId
	 * @param value
	 */
	public void setField(final long fieldId, final Double value)
	{
		setField(new NXCPMessageField(fieldId, value));
	}

	/**
	 * Set field of INETADDR type
	 * 
	 * @param fieldId
	 * @param value
	 */
	public void setField(final long fieldId, final InetAddress value)
	{
		setField(new NXCPMessageField(fieldId, value));
	}

   /**
    * Set field of INETADDR type
    * 
    * @param fieldId
    * @param value
    */
   public void setField(long fieldId, InetAddressEx value)
   {
      setField(new NXCPMessageField(fieldId, value));
   }
	
	/**
	 * Set field of BINARY type to GUID value (byte array of length 16).
	 * 
	 * @param fieldId
	 * @param value
	 */
	public void setField(final long fieldId, final UUID value)
	{
		setField(new NXCPMessageField(fieldId, value));
	}

	/**
	 * Set field of type INT64
	 * 
	 * @param fieldId
	 * @param value
	 */
	public void setFieldInt64(final long fieldId, final long value)
	{
		setField(new NXCPMessageField(fieldId, NXCPMessageField.TYPE_INT64, value));
	}

	/**
	 * Set field of type INT32
	 * 
	 * @param fieldId
	 * @param value
	 */
	public void setFieldInt32(final long fieldId, final int value)
	{
		setField(new NXCPMessageField(fieldId, NXCPMessageField.TYPE_INTEGER, (long)value));
	}

	/**
	 * Set field of type INT16
	 * 
	 * @param fieldId
	 * @param value
	 */
	public void setFieldInt16(final long fieldId, final int value)
	{
		setField(new NXCPMessageField(fieldId, NXCPMessageField.TYPE_INT16, (long)value));
	}
	
   /**
    * Set field of type INT16 to 1 if value is true and 0 otherwise.
    * 
    * @param fieldId
    * @param value
    */
   public void setField(final long fieldId, final boolean value)
   {
      setField(new NXCPMessageField(fieldId, NXCPMessageField.TYPE_INT16, value ? 1L : 0L));
   }
   
   /**
    * Set INT64 field from Date object to number of seconds since epoch. If value is null, field will be set to 0.
    * 
    * @param fieldId field ID
    * @param value Date object (can be null)
    */
   public void setField(final long fieldId, final Date value)
   {
      setField(new NXCPMessageField(fieldId, NXCPMessageField.TYPE_INT64, (value != null) ? value.getTime() / 1000L : 0L));
   }
   
	/**
	 * Get field as byte array
	 * 
	 * @param fieldId
	 * @return
	 */
	public byte[] getFieldAsBinary(final long fieldId)
	{
		final NXCPMessageField var = findField(fieldId);
		return (var != null) ? var.getAsBinary() : null;
	}
	
	/**
	 * Get field as string
	 * 
	 * @param fieldId
	 * @return
	 */
	public String getFieldAsString(final long fieldId)
	{
		final NXCPMessageField var = findField(fieldId);
		return (var != null) ? var.getAsString() : "";
	}
	
	/**
	 * Get field as double
	 * 
	 * @param fieldId
	 * @return
	 */
	public Double getFieldAsDouble(final long fieldId)
	{
		final NXCPMessageField var = findField(fieldId);
		return (var != null) ? var.getAsReal() : 0;
	}
	
	/**
	 * Get field as 32 bit integer
	 * 
	 * @param fieldId
	 * @return
	 */
	public int getFieldAsInt32(final long fieldId)
	{
		final NXCPMessageField var = findField(fieldId);
		return (var != null) ? var.getAsInteger().intValue() : 0;
	}
	
	/**
	 * Get field as 64 bit integer
	 * 
	 * @param fieldId
	 * @return
	 */
	public long getFieldAsInt64(final long fieldId)
	{
		final NXCPMessageField var = findField(fieldId);
		return (var != null) ? var.getAsInteger() : 0;
	}
	
	/**
	 * Get field as InetAddress
	 * 
	 * @param fieldId
	 * @return
	 */
	public InetAddress getFieldAsInetAddress(final long fieldId)
	{
		final NXCPMessageField var = findField(fieldId);
		return (var != null) ? var.getAsInetAddress() : null;
	}
	
   /**
    * Get field as InetAddressEx
    * 
    * @param fieldId
    * @return
    */
   public InetAddressEx getFieldAsInetAddressEx(final long fieldId)
   {
      final NXCPMessageField var = findField(fieldId);
      return (var != null) ? var.getAsInetAddressEx() : null;
   }
   
	/**
	 * Get field as UUID (GUID)
	 * 
	 * @param fieldId
	 * @return
	 */
	public UUID getFieldAsUUID(final long fieldId)
	{
		final NXCPMessageField var = findField(fieldId);
		return (var != null) ? var.getAsUUID() : null;
	}
	
	/**
	 * Get field as array of 32 bit integers
	 * 
	 * @param fieldId
	 * @return
	 */
	public long[] getFieldAsUInt32Array(final long fieldId)
	{
		final NXCPMessageField var = findField(fieldId);
		return (var != null) ? var.getAsUInt32Array() : null;
	}
	
	/**
	 * Get field as array of 32 bit integers
	 * 
	 * @param fieldId
	 * @return
	 */
	public Long[] getFieldAsUInt32ArrayEx(final long fieldId)
	{
		final NXCPMessageField var = findField(fieldId);
		return (var != null) ? var.getAsUInt32ArrayEx() : null;
	}
	
	/**
	 * Get field as boolean
	 * 
	 * @param fieldId
	 * @return
	 */
	public boolean getFieldAsBoolean(final long fieldId)
	{
		final NXCPMessageField var = findField(fieldId);
		return (var != null) ? (var.getAsInteger() != 0) : false;
	}

	/**
	 * Get field as date
	 * 
	 * @param fieldId
	 * @return
	 */
	public Date getFieldAsDate(final long fieldId)
	{
		final NXCPMessageField var = findField(fieldId);
		return (var != null) ? new Date(var.getAsInteger() * 1000) : null;
	}

	/**
	 * Create binary NXCP message
	 * 
	 * @return byte stream ready to send
	 */
	public byte[] createNXCPMessage() throws IOException
	{
		ByteArrayOutputStream byteStream = new ByteArrayOutputStream();
		DataOutputStream outputStream = new DataOutputStream(byteStream);

		if ((messageFlags & MF_CONTROL) == MF_CONTROL)
		{
			outputStream.writeShort(messageCode);
			outputStream.writeShort(messageFlags);
			outputStream.writeInt(HEADER_SIZE);	   // Size
			outputStream.writeInt((int)messageId);
			outputStream.writeInt((int)controlData);
		}
		else if ((messageFlags & MF_BINARY) == MF_BINARY) 
		{
			outputStream.writeShort(messageCode); // wCode
			outputStream.writeShort(messageFlags); // wFlags
			final int length = binaryData.length;
			final int padding = (8 - ((length + HEADER_SIZE) % 8)) & 7;
			final int packetSize = length + HEADER_SIZE + padding;
			outputStream.writeInt(packetSize); // dwSize (padded to 8 bytes boundaries)
			outputStream.writeInt((int)messageId); // dwId
			outputStream.writeInt(length); // dwNumVars, here used for real size of the payload (w/o headers and padding)
			outputStream.write(binaryData);
			for (int i = 0; i < padding; i++)
				outputStream.writeByte(0);
		}
		else
		{
			// Create byte array with all variables
			for(final NXCPMessageField nxcpVariable: fields.values())
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
			outputStream.writeShort(messageFlags);
			outputStream.writeInt(payload.length + HEADER_SIZE);	   // Size
			outputStream.writeInt((int)messageId);
			outputStream.writeInt(fields.size());
			outputStream.write(payload);
		}

		return byteStream.toByteArray();
	}

	/**
	 * Get data of raw message. Will return null if message is not a raw message.
	 * 
	 * @return Binary data of raw message
	 */
	public byte[] getBinaryData()
	{
		return binaryData;
	}

	/**
	 * Set data for raw message.
	 * 
	 * @param binaryData
	 */
	public void setBinaryData(final byte[] binaryData)
	{
		this.binaryData = binaryData;
	}
	
	/**
	 * Return true if message is a raw message
	 * @return raw message flag
	 */
	public boolean isBinaryMessage()
	{
		return (messageFlags & MF_BINARY) == MF_BINARY;
	}

	/**
	 * Set or clear raw (binary) message flag
	 * 
	 * @param isControl
	 *           true to set control message flag
	 */
	public void setBinaryMessage(boolean isRaw)
	{
		if (isRaw)
		{
			messageFlags |= MF_BINARY;
		}
		else
		{
			messageFlags &= ~MF_BINARY;
		}
	}
	
	/**
	 * Return true if message is a control message
	 * @return control message flag
	 */
	public boolean isControlMessage()
	{
		return (messageFlags & MF_CONTROL) == MF_CONTROL;
	}
	
	/**
	 * Set or clear control message flag
	 * 
	 * @param isControl true to set control message flag
	 */
	public void setControl(boolean isControl)
	{
		if (isControl)
			messageFlags |= MF_CONTROL;
		else
			messageFlags &= ~MF_CONTROL;
	}
	
	/**
	 * Return true if message has "end of file" flag set
	 * @return "end of file" flag
	 */
	public boolean isEndOfFile()
	{
		return (messageFlags & MF_END_OF_FILE) == MF_END_OF_FILE;
	}
	
	/**
	 * Set end of file message flag
	 * 
	 * @param isEOF true to set end of file message flag
	 */
	public void setEndOfFile(boolean isEOF)
	{
		if (isEOF)
			messageFlags |= MF_END_OF_FILE;
		else
			messageFlags &= ~MF_END_OF_FILE;
	}
	
	/**
	 * Return true if message has "end of sequence" flag set
	 * @return "end of file" flag
	 */
	public boolean isEndOfSequence()
	{
		return (messageFlags & MF_END_OF_SEQUENCE) == MF_END_OF_SEQUENCE;
	}

	/**
	 * Set end of sequence message flag
	 * 
	 * @param isEOS true to set end of sequence message flag
	 */
	public void setEndOfSequence(boolean isEOS)
	{
		if (isEOS)
			messageFlags |= MF_END_OF_SEQUENCE;
		else
			messageFlags &= ~MF_END_OF_SEQUENCE;
	}

	/**
	 * Return true if message has "don't encrypt" flag set
	 * @return "don't encrypr" flag
	 */
	public boolean isEncryptionDisabled()
	{
		return (messageFlags & MF_DONT_ENCRYPT) == MF_DONT_ENCRYPT;
	}

	/**
	 * Set "don't encrypt" message flag
	 * 
	 * @param disabled true to set "don't encrypt" message flag
	 */
	public void setEncryptionDisabled(boolean disabled)
	{
		if (disabled)
			messageFlags |= MF_DONT_ENCRYPT;
		else
			messageFlags &= ~MF_DONT_ENCRYPT;
	}

	/**
	 * @return the controlData
	 */
	public long getControlData()
	{
		return controlData;
	}

	/**
	 * @param controlData the controlData to set
	 */
	public void setControlData(long controlData)
	{
		this.controlData = controlData;
	}

	@Override
	public String toString()
	{
		return "NXCPMessage [messageCode=" + messageCode + ", messageFlags=" + messageFlags + ", messageId=" + messageId
				+ ", variableMap=" + fields + ", timestamp=" + timestamp + "]";
	}
}
