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
package org.netxms.client.snmp;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.zip.InflaterInputStream;
import org.netxms.base.NXCPDataInputStream;
import org.netxms.client.NXCException;
import org.netxms.client.constants.RCC;

/**
 * This class represents MIB tree.
 *
 */
public class MibTree
{
	private static final byte[] MIB_FILE_MAGIC_NUMBER = { 0x4E, 0x58, 0x4D, 0x49, 0x42, 0x20 };
		
	// MIB file flags
	protected static final int SMT_COMPRESS_DATA     = 0x0001;
	protected static final int SMT_SKIP_DESCRIPTIONS = 0x0002;
	
	private MibObject root;
	
	/**
	 * Create empty MIB tree
	 */
	public MibTree()
	{
		root = new MibObject();
	}
	
	/**
	 * Create MIB tree from compiled MIB file.
	 * 
	 * @param file compiled MIB file
	 * @throws IOException if I/O error occurs
	 * @throws NXCException if MIB file format errors detected
	 */
	public MibTree(File file) throws IOException, NXCException
	{
		NXCPDataInputStream in = new NXCPDataInputStream(new BufferedInputStream(new FileInputStream(file)));
		
		try
		{
			// Read header
			byte[] magic = new byte[6];
			int bytes = in.read(magic);
			if ((bytes != 6) || !Arrays.equals(magic, MIB_FILE_MAGIC_NUMBER))
				throw new NXCException(RCC.BAD_MIB_FILE_HEADER);
			int headerSize = in.readUnsignedByte();
			in.skipBytes(1);	// Skip version field
			int flags = in.readUnsignedShort();
			
			if ((flags & SMT_COMPRESS_DATA) != 0)
			{
				in.close();
				in = null;
				BufferedInputStream bufferedInput = new BufferedInputStream(new FileInputStream(file));
				bufferedInput.skip(headerSize);
				in = new NXCPDataInputStream(new InflaterInputStream(bufferedInput));
			}
			else
			{
				in.skipBytes(headerSize - 10);
			}
			
			byte tag = in.readByte();
			if (tag == MibObject.MIB_TAG_OBJECT)
			{
				root = new MibObject(in, null);
			}
			else
			{
				throw new NXCException(RCC.BAD_MIB_FILE_DATA);
			}
		}
		finally
		{
			if (in != null)
				in.close();
		}
	}

	/**
	 * Get root object for MIB tree
	 * 
	 * @return the root object
	 */
	public MibObject getRootObject()
	{
		return root;
	}
}
