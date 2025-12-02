/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Date;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.UUID;
import java.util.zip.CRC32;
import com.google.gson.Gson;
import com.jcraft.jzlib.Deflater;
import com.jcraft.jzlib.DeflaterOutputStream;
import com.jcraft.jzlib.InflaterInputStream;
import com.jcraft.jzlib.JZlib;

/**
 * NXCP (NetXMS Communication Protocol) message
 */
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
   public static final int MF_COMPRESSED = 0x0040;
   public static final int MF_STREAM = 0x0080;
	
	private int messageCode;
	private int messageFlags;
	private long messageId;
	private Map<Long, NXCPMessageField> fields = new HashMap<Long, NXCPMessageField>(0);
	private long timestamp;
	private byte[] binaryData = null;
	private long controlData = 0;

   /**
     * Create new message with given code and ID 0.
     *
     * @param msgCode message code
     */
	public NXCPMessage(final int msgCode)
	{
		this.messageCode = msgCode;
		messageId = 0L;
		messageFlags = 0;
	}

   /**
     * Create new message with given code and ID.
     *
     * @param msgCode message code
     * @param msgId message ID
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
	 * @param nxcpMessage NXCP message
	 * @param ectx encryption context
	 * @throws IOException if internal byte stream error occurs (normally should not happen)
	 * @throws NXCPException if message cannot be parsed
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
			createFromStream(payloadInputStream);
		}
		else
		{
			createFromStream(inputStream);
		}
	}

	/**
	 * Create NXCPMessage from prepared input byte stream
	 * @param inputStream
	 * @param byteArrayInputStream
	 * @throws IOException
	 */
	private void createFromStream(NXCPDataInputStream inputStream) throws IOException
	{
		messageFlags = inputStream.readUnsignedShort();
		inputStream.skipBytes(4);	// Message size
		messageId = (long)inputStream.readInt();
		
		if ((messageFlags & MF_BINARY) == MF_BINARY)
		{
			final int size = inputStream.readInt();
			binaryData = new byte[size];
         if (((messageFlags & MF_COMPRESSED) == MF_COMPRESSED) && ((messageFlags & MF_STREAM) == 0))
         {
            // Compressed message
            inputStream.skip(4);  // skip original message length
            inputStream = new NXCPDataInputStream(new InflaterInputStream(inputStream));
         }
			inputStream.readFully(binaryData);
		}
		else if ((messageFlags & MF_CONTROL) == MF_CONTROL)
		{
			controlData = inputStream.readUnsignedInt();
		}
		else
		{
         final int numVars = inputStream.readInt();
		   if ((messageFlags & MF_COMPRESSED) == MF_COMPRESSED)
		   {
		      // Compressed message
		      inputStream.skip(4);  // skip original message length
		      inputStream = new NXCPDataInputStream(new InflaterInputStream(inputStream));
		   }
		   
			for(int i = 0; i < numVars; i++)
			{
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
               case NXCPMessageField.TYPE_UTF8_STRING:
					case NXCPMessageField.TYPE_BINARY:
						int size = inputStream.readInt();
						df = Arrays.copyOf(df, size + 12);
						intToBytes(size, df, 8);
						inputStream.readFully(df, 12, size);

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
				fields.put(variable.getId(), variable);
			}
		}
	}

	/**
	 * Encode 32 bit integer into byte array (in network byte order)
	 * 
	 * @param value integer value
	 * @param data byte array
	 * @param offset offset within byte array
	 * @throws ArrayIndexOutOfBoundsException if given offset is invalid or there is not enough space for placing integer
	 */
	private static void intToBytes(int value, byte[] data, int offset) throws ArrayIndexOutOfBoundsException
	{
	   data[offset] = (byte)(value >> 24);
      data[offset + 1] = (byte)((value >> 16) & 0xFF);
      data[offset + 2] = (byte)((value >> 8) & 0xFF);
      data[offset + 3] = (byte)(value & 0xFF);
	}
	
	/**
    * Get this message's code
    * 
    * @return this message's code
    */
	public int getMessageCode()
	{
		return messageCode;
	}

	/**
    * Set new message code
    * 
    * @param msgCode new message code
    */
	public void setMessageCode(final int msgCode)
	{
		this.messageCode = msgCode;
	}

	/**
    * Get this message's ID
    *
    * @return this message's ID
    */
	public long getMessageId()
	{
		return messageId;
	}

	/**
    * Set new message ID
    * 
    * @param msgId new message ID
    */
	public void setMessageId(final long msgId)
	{
		this.messageId = msgId;
	}

	/**
    * Get message timestamp
    *
    * @return the timestamp
    */
	public long getTimestamp()
	{
		return timestamp;
	}

	/**
    * Set message timestamp
    *
    * @param timestamp new timestamp
    */
	public void setTimestamp(final long timestamp)
	{
		this.timestamp = timestamp;
	}

	/**
	 * Find field by ID
	 * 
	 * @param fieldId field ID to find
	 * @return field with given ID or null
	 */
	public NXCPMessageField findField(final long fieldId)
	{
		return fields.get(fieldId);
	}
	
	/**
	 * Check if field with given ID present in the message
	 * 
	 * @param fieldId field ID
	 * @return true if field with given ID present in the message
	 */
	public boolean isFieldPresent(final long fieldId)
	{
	   return fields.containsKey(fieldId);
	}

	/**
	 * Set field as copy of another field
	 * 
	 * @param src source field
	 */
	public void setField(final NXCPMessageField src)
	{
		fields.put(src.getId(), src);
	}

	/**
    * Set binary field from given byte array
    * 
    * @param fieldId field ID
    * @param value binary value
    */
	public void setField(final long fieldId, final byte[] value)
	{
		setField(new NXCPMessageField(fieldId, value));
	}

	/**
    * Set binary field from array of long integers. Each element will be converted to network byte order and then array will be
    * serialized as array of bytes.
    * 
    * @param fieldId field ID
    * @param value value to be encoded
    */
	public void setField(final long fieldId, final long[] value)
	{
		setField(new NXCPMessageField(fieldId, value));
	}

   /**
    * Set binary field from array of long integers. Each element will be converted to network byte order and then array will be
    * serialized as array of bytes.
    * 
    * @param fieldId field ID
    * @param value value to be encoded
    */
   public void setField(final long fieldId, final int[] value)
   {
      setField(new NXCPMessageField(fieldId, value));
   }

   /**
    * Set binary field from array of long integers. Each element will be converted to network byte order and then array will be
    * serialized as array of bytes.
    * 
    * @param fieldId field ID
    * @param value value to be encoded
    */
	public void setField(final long fieldId, final Long[] value)
	{
		setField(new NXCPMessageField(fieldId, value));
	}

   /**
    * Set binary field from array of integers. Each element will be converted to network byte order and then array will be
    * serialized as array of bytes.
    * 
    * @param fieldId field ID
    * @param value value to be encoded
    */
   public void setField(final long fieldId, final Integer[] value)
   {
      setField(new NXCPMessageField(fieldId, value));
   }

   /**
    * Set binary field from collection of integers. Each element will be converted to 32 bit integer in network byte order and then
    * array will be serialized as array of bytes.
    * 
    * @param fieldId field ID
    * @param value value to be encoded
    */
   public <T extends Number> void setField(final long fieldId, final Collection<T> value)
   {
      setField(new NXCPMessageField(fieldId, value));
   }

	/**
	 * Set field of UTF8-STRING type
	 * 
	 * @param fieldId field ID
	 * @param value string value
	 */
	public void setField(final long fieldId, final String value)
	{
		setField(new NXCPMessageField(fieldId, value));
	}

   /**
    * Set field of STRING or UTF8-STRING type
    * 
    * @param fieldId field ID
    * @param value string value
    * @param forceUcsEncoding if true, UCS2 encoding will be used (STRING type)
    */
   public void setField(final long fieldId, final String value, boolean forceUcsEncoding)
   {
      setField(new NXCPMessageField(fieldId, value, forceUcsEncoding));
   }

	/**
    * Set field of DOUBLE type
    * 
    * @param fieldId field ID
    * @param value floating point number value
    */
	public void setField(final long fieldId, final Double value)
	{
		setField(new NXCPMessageField(fieldId, value));
	}

	/**
    * Set field of INETADDR type
    * 
    * @param fieldId field ID
    * @param value value to encode
    */
	public void setField(final long fieldId, final InetAddress value)
	{
		setField(new NXCPMessageField(fieldId, value));
	}

   /**
    * Set field of INETADDR type
    * 
    * @param fieldId field ID
    * @param value value to encode
    */
   public void setField(long fieldId, InetAddressEx value)
   {
      setField(new NXCPMessageField(fieldId, value));
   }

	/**
    * Set field of BINARY type to GUID value (byte array of length 16).
    * 
    * @param fieldId field ID
    * @param value value to encode
    */
	public void setField(final long fieldId, final UUID value)
	{
      setField(new NXCPMessageField(fieldId, (value != null) ? value : NXCommon.EMPTY_GUID));
	}

	/**
    * Set field of type INT64
    * 
    * @param fieldId field ID
    * @param value integer value to encode
    */
	public void setFieldInt64(final long fieldId, final long value)
	{
		setField(new NXCPMessageField(fieldId, NXCPMessageField.TYPE_INT64, value));
	}

	/**
    * Set field of type INT32
    * 
    * @param fieldId field ID
    * @param value integer value to encode
    */
	public void setFieldInt32(final long fieldId, final int value)
	{
		setField(new NXCPMessageField(fieldId, NXCPMessageField.TYPE_INTEGER, (long)value));
	}

   /**
    * Set field of type INT32 (unsigned)
    * 
    * @param fieldId field ID
    * @param value integer value to encode
    */
   public void setFieldUInt32(final long fieldId, final long value)
   {
      setField(new NXCPMessageField(fieldId, NXCPMessageField.TYPE_INTEGER, value));
   }

	/**
    * Set field of type INT16
    * 
    * @param fieldId field ID
    * @param value integer value to encode
    */
	public void setFieldInt16(final long fieldId, final int value)
	{
		setField(new NXCPMessageField(fieldId, NXCPMessageField.TYPE_INT16, (long)value));
	}
	
   /**
    * Set field of type INT16 to 1 if value is true and 0 otherwise.
    * 
    * @param fieldId field ID
    * @param value boolean value to encode
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
    * Set binary field from array of Strings.
    * 
    * @param fieldId field ID
    * @param value value to be encoded
    */
   public void setField(final long fieldId, final String[] value)
   {
      if (value != null)
         setField(new NXCPMessageField(fieldId, value));
   }

   /**
    * Set byte field from MacAddress object value. If value is null, field will not be set.
    * 
    * @param fieldId field ID
    * @param value MacAddress object (can be null)
    */
   public void setField(long fieldId, MacAddress value)
   {
      if (value == null)
         return;
      setField(fieldId, value.getValue());      
   }

   /**
    * Set field of UTF8-STRING type
    * 
    * @param fieldId field ID
    * @param value string value
    */
   public void setFieldJson(final long fieldId, final Object value)
   {
      String json = new Gson().toJson(value);
      setField(new NXCPMessageField(fieldId, json));
   }
   
	/**
	 * Get field as byte array
	 * 
    * @param fieldId field ID
    * @return field value as byte array
	 */
	public byte[] getFieldAsBinary(final long fieldId)
	{
		final NXCPMessageField var = findField(fieldId);
		return (var != null) ? var.getAsBinary() : null;
	}

	/**
	 * Get field as string
	 * 
	 * @param fieldId field ID
    * @return field value as string
	 */
	public String getFieldAsString(final long fieldId)
	{
		final NXCPMessageField var = findField(fieldId);
		return (var != null) ? var.getAsString() : "";
	}

	/**
	 * Get field as double
	 * 
    * @param fieldId field id
    * @return field value as double
	 */
	public Double getFieldAsDouble(final long fieldId)
	{
		final NXCPMessageField var = findField(fieldId);
		return (var != null) ? var.getAsReal() : 0;
	}

   /**
    * Get field as 16 bit integer
    * 
    * @param fieldId field id
    * @return field value as 16 bit integer
    */
   public short getFieldAsInt16(final long fieldId)
   {
      final NXCPMessageField var = findField(fieldId);
      return (var != null) ? var.getAsInteger().shortValue() : 0;
   }

	/**
	 * Get field as 32 bit integer
	 * 
    * @param fieldId field id
    * @return field value as 32 bit integer
	 */
	public int getFieldAsInt32(final long fieldId)
	{
		final NXCPMessageField var = findField(fieldId);
		return (var != null) ? var.getAsInteger().intValue() : 0;
	}

	/**
	 * Get field as 64 bit integer
	 * 
    * @param fieldId field id
    * @return field value as 64 bit integer
	 */
	public long getFieldAsInt64(final long fieldId)
	{
		final NXCPMessageField var = findField(fieldId);
		return (var != null) ? var.getAsInteger() : 0;
	}
	
	/**
	 * Get field as InetAddress
	 * 
    * @param fieldId field id
    * @return field value as IP address
	 */
	public InetAddress getFieldAsInetAddress(final long fieldId)
	{
		final NXCPMessageField var = findField(fieldId);
		return (var != null) ? var.getAsInetAddress() : null;
	}
	
	/**
	 *  Get field as MacAddress
	 *  
	 * @param fieldId field id
	 * @return field value as MAC address
	 */
	public MacAddress getFieldAsMacAddress(final long fieldId)
	{
	   final NXCPMessageField var = findField(fieldId);
	   return (var != null) ? new MacAddress(var.getAsBinary()) : null;
	}
	
   /**
    * Get field as InetAddressEx
    * 
    * @param fieldId field ID
    * @return field value as IP address
    */
   public InetAddressEx getFieldAsInetAddressEx(final long fieldId)
   {
      final NXCPMessageField var = findField(fieldId);
      return (var != null) ? var.getAsInetAddressEx() : null;
   }
   
	/**
	 * Get field as UUID (GUID)
	 * 
    * @param fieldId field ID
    * @return field value as UUID
	 */
	public UUID getFieldAsUUID(final long fieldId)
	{
		final NXCPMessageField var = findField(fieldId);
		return (var != null) ? var.getAsUUID() : null;
	}

	/**
	 * Get field as array of 32 bit integers
	 * 
    * @param fieldId field ID
    * @return field value as array of long integers
	 */
	public long[] getFieldAsUInt32Array(final long fieldId)
	{
		final NXCPMessageField var = findField(fieldId);
		return (var != null) ? var.getAsUInt32Array() : null;
	}
	
	/**
	 * Get field as array of 32 bit integers
	 * 
    * @param fieldId field ID
	 * @return field value as array of Long objects
	 */
	public Long[] getFieldAsUInt32ArrayEx(final long fieldId)
	{
		final NXCPMessageField var = findField(fieldId);
		return (var != null) ? var.getAsUInt32ArrayEx() : null;
	}

   /**
    * Get field as array of 32 bit integers
    * 
    * @param fieldId field ID
    * @return field value as array of long integers
    */
   public int[] getFieldAsInt32Array(final long fieldId)
   {
      final NXCPMessageField var = findField(fieldId);
      return (var != null) ? var.getAsInt32Array() : null;
   }

   /**
    * Get field as array of 32 bit integers
    * 
    * @param fieldId field ID
    * @return field value as array of Long objects
    */
   public Integer[] getFieldAsInt32ArrayEx(final long fieldId)
   {
      final NXCPMessageField var = findField(fieldId);
      return (var != null) ? var.getAsInt32ArrayEx() : null;
   }

	/**
	 * Get field as boolean
	 * 
	 * @param fieldId field ID
	 * @return field value as boolean
	 */
	public boolean getFieldAsBoolean(final long fieldId)
	{
		final NXCPMessageField var = findField(fieldId);
		return (var != null) ? (var.getAsInteger() != 0) : false;
	}

	/**
	 * Get field as date
	 * 
	 * @param fieldId field ID
	 * @return field value as Date object
	 */
	public Date getFieldAsDate(final long fieldId)
	{
		final NXCPMessageField var = findField(fieldId);
		return (var != null) ? new Date(var.getAsInteger() * 1000) : null;
	}

   /**
    * Get field as date, assuming received value is in milliseconds
    * 
    * @param fieldId field ID
    * @return field value as Date object
    */
   public Date getFieldAsTimestamp(final long fieldId)
   {
      final NXCPMessageField var = findField(fieldId);
      return (var != null) ? new Date(var.getAsInteger()) : null;
   }

	/**
    * Create binary NXCP message
    * 
    * @param allowCompression true if message compression is allowed
    * @return byte stream ready to send
    * @throws IOException if write to underlying data output stream fails
    */
	public byte[] createNXCPMessage(boolean allowCompression) throws IOException
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
			byte[] payload = binaryData;
			boolean compressed = false;
			if (allowCompression && ((messageFlags & MF_STREAM) == 0) && (binaryData.length > 128))
			{
			   ByteArrayOutputStream compDataByteStream = new ByteArrayOutputStream();
            byte[] length = new byte[4];
            final int unpackedPadding = (8 - ((binaryData.length + HEADER_SIZE) % 8)) & 7;
            intToBytes(unpackedPadding + HEADER_SIZE, length, 0);
            compDataByteStream.write(length);   // unpacked message size
            
            DeflaterOutputStream deflaterStream = new DeflaterOutputStream(compDataByteStream, new Deflater(JZlib.Z_BEST_COMPRESSION));
            deflaterStream.write(binaryData);
            deflaterStream.close();

            byte[] compPayload = compDataByteStream.toByteArray();
            if (compPayload.length < binaryData.length)
            {
               payload = compPayload;
               compressed = true;
            }
			}

         outputStream.writeShort(messageCode); // wCode
			outputStream.writeShort(compressed ? (messageFlags | MF_COMPRESSED) : messageFlags); // wFlags
			   
			final int length = payload.length;
			final int padding = (8 - ((length + HEADER_SIZE) % 8)) & 7;
			final int packetSize = length + HEADER_SIZE + padding;
			outputStream.writeInt(packetSize); // dwSize (padded to 8 bytes boundaries)
			outputStream.writeInt((int)messageId); // dwId
			outputStream.writeInt(binaryData.length); // dwNumVars, here used for real size of the payload (w/o headers and padding)
			outputStream.write(payload);
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
			byte[] payload = byteStream.toByteArray();
			
			boolean compressed = false;
         if (allowCompression && (payload.length > 128))
         {
            byteStream = new ByteArrayOutputStream();
            byte[] length = new byte[4];
            intToBytes(payload.length + HEADER_SIZE, length, 0);
            byteStream.write(length);
            DeflaterOutputStream deflaterStream = new DeflaterOutputStream(byteStream, new Deflater(JZlib.Z_BEST_COMPRESSION));
            deflaterStream.write(payload);
            deflaterStream.close();

            final int padding = (8 - (byteStream.size() % 8)) & 7;
            for (int i = 0; i < padding; i++)
               byteStream.write(0);
            
            byte[] compPayload = byteStream.toByteArray();
            if (compPayload.length < payload.length - 4)
            {
               payload = compPayload;
               compressed = true;
            }
         }

			// Create message header in new byte stream and add payload
			byteStream = new ByteArrayOutputStream();
			//noinspection IOResourceOpenedButNotSafelyClosed
			outputStream = new DataOutputStream(byteStream);
			outputStream.writeShort(messageCode);
			outputStream.writeShort(messageFlags | (compressed ? MF_COMPRESSED : 0));
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
	 * @param binaryData data to set
	 */
	public void setBinaryData(final byte[] binaryData)
	{
		this.binaryData = binaryData;
	}
	
	/**
	 * Return true if message contains raw (binary) data
	 * @return raw message flag
	 */
	public boolean isBinaryMessage()
	{
		return (messageFlags & MF_BINARY) == MF_BINARY;
	}

	/**
	 * Set or clear raw (binary) message flag
	 * @param isRaw true if message contains raw (binary) data
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

   /**
    * Return true if message has "stream" flags set
    * @return "compressed stream" flag
    */
   public boolean isStream()
   {
      return (messageFlags & MF_STREAM) == MF_STREAM;
   }
   
   /**
    * Return true if message has "compressed" and "stream" flags set
    * @return "compressed stream" flag
    */
   public boolean isCompressedStream()
   {
      return (messageFlags & (MF_COMPRESSED | MF_STREAM)) == (MF_COMPRESSED | MF_STREAM);
   }
   
   /**
    * Set stream related message flags
    * 
    * @param isStream true to set "stream" message flag
    * @param isCompressed true to set "compressed" message flag (ignored if isStream is false)
    */
   public void setStream(boolean isStream, boolean isCompressed)
   {
      if (isStream)
      {
         messageFlags |= MF_STREAM;
         if (isCompressed)
            messageFlags |= MF_COMPRESSED;
         else
            messageFlags &= ~MF_COMPRESSED;
      }
      else
      {
         messageFlags &= ~(MF_COMPRESSED | MF_STREAM);
      }
   }

   /**
    * Set fields in message from string collection
    *  
    * @param strings strings collection
    * @param baseId base (first element) field ID
    * @param countId ID of field containing number of elements
    */
   public void setFieldsFromStringCollection(Collection<String> strings, long baseId, long countId)
   {
      setFieldInt32(countId, strings.size());
      long fieldId = baseId;
      for(String s : strings)
         setField(fieldId++, s);
   }

   /**
    * Set binary field from Strings collection.
    * 
    * @param fieldId field ID
    * @param value value to be encoded
    */
   public void setFieldStringCollection(final long fieldId, Collection<String> value)
   {
      if (value != null)
      {
         final ByteArrayOutputStream byteStream = new ByteArrayOutputStream();
         final DataOutputStream out = new DataOutputStream(byteStream);
         try
         {
            out.writeShort(value.size());
            for(String s: value)
            {
               out.writeUTF(s);
            }
            setField(new NXCPMessageField(fieldId, byteStream.toByteArray()));
         }
         catch(IOException e)
         {
         }
      }
   }

   /**
    * Set fields in message from string map
    * 
    * @param strings string map
    * @param baseId base (first element) field ID
    * @param countId ID of field containing number of elements
    */
   public void setFieldsFromStringMap(Map<String, String> strings, long baseId, long countId)
   {
      setFieldInt32(countId, strings.size());
      long fieldId = baseId;
      for(Entry<String, String> e : strings.entrySet())
      {
         setField(fieldId++, e.getKey());
         setField(fieldId++, e.getValue());
      }
   }

   /**
    * Get string list from fields
    * 
    * @param baseId base (first element) field ID
    * @param countId ID of field containing number of elements
    * @return list of strings
    */
   public List<String> getStringListFromFields(long baseId, long countId)
   {
      int count = getFieldAsInt32(countId);
      List<String> list = new ArrayList<String>(count);
      for(int i = 0; i < count; i++)
         list.add(getFieldAsString(baseId + i));
      return list;
   }

   /**
    * Get string list from field
    * 
    * @param fieldId field ID
    * @return list of strings
    */
   public List<String> getStringListFromField(long fieldId)
   {
      final NXCPMessageField var = findField(fieldId);
      String[] array = (var != null) ? var.getAsStringArrayEx() : null;
      return Arrays.asList(array);      
   }
   
   /**
    * Get field as array of 32 bit integers
    * 
    * @param fieldId field ID
    * @return field value as array of Long objects
    */
   public String[] getFieldAsStringArrayEx(final long fieldId)
   {
      final NXCPMessageField var = findField(fieldId);
      return (var != null) ? var.getAsStringArrayEx() : null;
   }
   
   /**
    * Get string map from fields
    * 
    * @param baseId base (first element) field ID
    * @param countId ID of field containing number of elements
    * @return map of strings
    */
   public Map<String, String> getStringMapFromFields(long baseId, long countId)
   {
      int count = getFieldAsInt32(countId);
      Map<String, String> map = new LinkedHashMap<String, String>(count);
      long fieldId = baseId;
      for(int i = 0; i < count; i++)
      {
         String key = getFieldAsString(fieldId++);
         String value = getFieldAsString(fieldId++);
         map.put(key, value);
      }
      return map;
   }

   /**
    * @see java.lang.Object#toString()
    */
	@Override
	public String toString()
	{
		return "NXCPMessage [code=0x" + Integer.toHexString(messageCode) + ", flags=0x" + Integer.toHexString(messageFlags) + ", id=" + messageId + ", fields=" + fields + "]";
	}
}
