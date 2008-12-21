/**
 * 
 */
package org.netxms.base;

import java.io.*;


/**
 * @author Victor
 *
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
	
	
	//
	// Get message size from byte array
	//
	
	private int getMessageSize(final byte[] header) throws IOException
	{
		final DataInputStream inputStream = new DataInputStream(new ByteArrayInputStream(header));
		inputStream.skipBytes(4);
		return inputStream.readInt();
	}
	
	
	//
	// Get message from buffer if possible
	//
	
	private NXCPMessage getMessageFromBuffer() throws IOException, NXCPException
	{
		NXCPMessage msg = null;

		if (bufferPos >= NXCPMessage.HEADER_SIZE)
		{
			final int size = getMessageSize(recvBuffer);
			if (size <= bufferPos)
			{
				// Entire message in buffer, create new message object
				msg = new NXCPMessage(recvBuffer);
				System.arraycopy(recvBuffer, size, recvBuffer, 0, bufferPos - size);
				bufferPos -= size;
			}
			else if (size > recvBuffer.length)
			{
				throw new NXCPException(NXCPCodes.ERR_MESSAGE_TOO_LARGE);
			}
		}
		return msg;
	}
	
	
	//
	// Receive NXCP message from input stream
	//
	
	public NXCPMessage receiveMessage(final InputStream in) throws IOException, NXCPException
	{
		NXCPMessage msg = null;
		
		// Receive bytes from network if we don't have full message in buffer 
		while(true)
		{
			msg = getMessageFromBuffer();
			if (msg != null)
				break;
			final int bytes = in.read(recvBuffer, bufferPos, recvBuffer.length - bufferPos);
			bufferPos += bytes;
		}
		
		return msg;
	}
}
