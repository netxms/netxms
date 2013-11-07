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
package org.netxms.client;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 * Represents file received from server
 *
 */
public class NXCReceivedFile
{
	// Status codes
	public static final int OPEN = 0;
	public static final int RECEIVED = 1;
	public static final int FAILED = 2;
	
	private long id;
	private File file;
	private FileOutputStream stream;
	private int status;
	private long timestamp;
	private long size;
	private IOException exception;
	
	/**
	 * Create new received file with given id
	 * @param id
	 */
	protected NXCReceivedFile(final long id)
	{
		this.id = id;
		try
		{
			file = File.createTempFile("nxc", "data");
			file.deleteOnExit();
			stream = new FileOutputStream(file);
			timestamp = System.currentTimeMillis();
			status = OPEN;
			size = 0;
		}
		catch(IOException e)
		{
			exception = e;
			status = FAILED;
		}
	}
	
	/**
	 * Write data to file
	 * @param data
	 */
	protected void writeData(final byte[] data)
	{
		if (status == OPEN)
		{
			try
			{
				stream.write(data);
				size += data.length;
			}
			catch(IOException e)
			{
				try
				{
					stream.close();
				}
				catch(IOException e1)
				{
				}
				status = FAILED;
				exception = e;
			}
			timestamp = System.currentTimeMillis();
		}
	}
	
	/**
	 * Close file
	 */
	protected void close()
	{
		if (status == OPEN)
		{
			try
			{
				stream.close();
				status = RECEIVED;
			}
			catch(IOException e)
			{
				status = FAILED;
				exception = e;
			}
			timestamp = System.currentTimeMillis();
		}
	}
	
	/**
	 * @param cause
	 */
	protected void abortTransfer()
	{
		if (status == OPEN)
		{
			try
			{
				stream.close();
				status = RECEIVED;
			}
			catch(IOException e)
			{
			}
		}
		timestamp = System.currentTimeMillis();
		status = FAILED;
		exception = new IOException();
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @return the file
	 */
	public File getFile()
	{
		return file;
	}

	/**
	 * @return the status
	 */
	public int getStatus()
	{
		return status;
	}

	/**
	 * @return the timestamp
	 */
	public long getTimestamp()
	{
		return timestamp;
	}

	/**
	 * @return the size
	 */
	public long getSize()
	{
		return size;
	}

	/**
	 * @return the exception
	 */
	public IOException getException()
	{
		return (exception != null) ? exception : new IOException();
	}
}
