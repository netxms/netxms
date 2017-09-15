/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
import java.io.DataInputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * Custom input stream for parsing NXCP messages
 */
public class NXCPDataInputStream extends DataInputStream
{
	/**
    * Create NXCP data input stream on top of existing input stream
	 * 
	 * @param in Input stream
	 */
	public NXCPDataInputStream(InputStream in)
	{
		super(in);
	}

	/**
	 * Create NXCP data input stream for reading from existing byte array
	 * 
	 * @param data Byte array to read data from
	 */
	public NXCPDataInputStream(final byte[] data)
	{
		super(new ByteArrayInputStream(data));
	}

	/**
	 * Read unsigned 32-bit integer from input stream
	 * 
	 * @return unsigned 32-bit integer converted into signed 64 bit integer
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

   /* (non-Javadoc)
    * @see java.io.FilterInputStream#close()
    */
   @Override
   public void close()
   {
      try
      {
         super.close();
      }
      catch(IOException e)
      {
      }
   }
}
