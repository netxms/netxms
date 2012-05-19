/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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

import java.io.IOException;
import java.io.InputStream;

/**
 * Message receiver for NXCP protocol
 */
public class NXCPMessageReceiver
{
	private byte[] recvBuffer;
	private int bufferPos = 0;
	
	/**
	 * @param bufferSize Size of receive buffer in bytes
	 */
	public NXCPMessageReceiver(final int bufferSize)
	{
		recvBuffer = new byte[bufferSize];
	}
	
	/**
	 * Get message size from byte array
	 * 
	 * @param header byte array containing message header
	 * @return message size in bytes
	 * @throws IOException
	 */
	private long getMessageSize(final byte[] header) throws IOException
	{
		return (((long)header[4] << 24) & 0xFF000000) | 
		       (((long)header[5] << 16) & 0x00FF0000) | 
		       (((long)header[6] << 8) & 0x0000FF00) |
		       ((long)header[7] & 0x000000FF);
	}
	
	/**
	 * Get message from receiver's buffer
	 * 
	 * @return message object or null if there are not enough data in the buffer
	 * @throws IOException
	 * @throws NXCPException
	 */
	private NXCPMessage getMessageFromBuffer(EncryptionContext ectx) throws IOException, NXCPException
	{
		NXCPMessage msg = null;

		if (bufferPos >= NXCPMessage.HEADER_SIZE)
		{
			final long size = getMessageSize(recvBuffer);
			if (size <= bufferPos)
			{
				// Entire message in buffer, create new message object
				try
				{
					msg = new NXCPMessage(recvBuffer, ectx);
				}
				finally
				{
					System.arraycopy(recvBuffer, (int)size, recvBuffer, 0, bufferPos - (int)size);
					bufferPos -= size;
				}
			}
			else if (size > recvBuffer.length)
			{
				throw new NXCPException(NXCPException.MESSAGE_TOO_LARGE);
			}
		}
		return msg;
	}
	
	/**
	 * Receive NXCP message from input stream
	 * 
	 * @param in input stream
	 * @return message object
	 * @throws IOException
	 * @throws NXCPException
	 */
	public NXCPMessage receiveMessage(final InputStream in, EncryptionContext ectx) throws IOException, NXCPException
	{
		NXCPMessage msg = null;
		
		// Receive bytes from network if we don't have full message in buffer 
		while(true)
		{
			msg = getMessageFromBuffer(ectx);
			if (msg != null)
				break;
			final int bytes = in.read(recvBuffer, bufferPos, recvBuffer.length - bufferPos);
			if (bytes == -1)
				throw new NXCPException(NXCPException.SESSION_CLOSED);
			bufferPos += bytes;
		}
		
		return msg;
	}
}
