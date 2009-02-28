/**
 * 
 */
package org.netxms.base;

import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * @author Victor
 *
 */
public class NXCPDataInputStream extends DataInputStream
{
	/**
	 * @param in Input stream
	 */
	public NXCPDataInputStream(InputStream in)
	{
		super(in);
	}

	/**
	 * @param data Byte array to read data from
	 */
	public NXCPDataInputStream(final byte[] data)
	{
		super(new ByteArrayInputStream(data));
	}

	/**
	 * Read unsigned 32-bit integer from input stream
	 * @return
	 * @throws IOException 
	 */
	public long readUnsignedInt() throws IOException
	{
		long b1 = readUnsignedByte();
		long b2 = readUnsignedByte();
		long b3 = readUnsignedByte();
		long b4 = readUnsignedByte();
		return (b1 << 24) + (b2 << 16) + (b3 << 8) + b4;
	}
}
