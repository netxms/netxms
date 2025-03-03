/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.jcraft.jzlib.Inflater;
import com.jcraft.jzlib.JZlib;

/**
 * Represents file received from server
 */
final class NXCReceivedFile
{
	// Status codes
	public static final int OPEN = 0;
	public static final int RECEIVED = 1;
	public static final int FAILED = 2;
   public static final int CANCELLED = 3;
	
   private static Logger logger = LoggerFactory.getLogger(NXCReceivedFile.class);

	private long id;
	private File file;
	private FileOutputStream stream;
	private int status;
	private long timestamp;
	private long size;
	private Exception exception;
	private Inflater decompressor = null;
	
	/**
	 * Create new received file with given id
	 * @param id ID
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
	 * 
	 * @param data data to be written
	 * @param compressedStream true if data is part of compressed data stream
	 * @return number of bytes actually written
	 */
	protected int writeData(final byte[] data, boolean compressedStream)
	{
	   int bytes = 0;
		if (status == OPEN && data.length > 0)
		{
			try
			{
			   if (compressedStream)
			   {
			      if (data[0] != 2)
			         throw new IOException("Unsupported stream compression method " + (int)data[0]);
			      
			      if (decompressor == null)
			      {
			         decompressor = new Inflater();
                  logger.debug("Decompressor created for file " + file.getAbsolutePath());
			      }
			      decompressor.setInput(data, 4, data.length - 4, false);

               int dataLength = (((int)data[2] << 8) & 0xFF00) | ((int)data[3] & 0xFF);
               byte[] uncompressedData = new byte[dataLength];
			      decompressor.setOutput(uncompressedData);
			      
			      int rc = decompressor.inflate(JZlib.Z_SYNC_FLUSH);
			      if ((rc != JZlib.Z_OK) && (rc != JZlib.Z_STREAM_END))
			         throw new IOException("Decompression error " + rc);
               
			      stream.write(uncompressedData);
               bytes = uncompressedData.length;
			   }
			   else
			   {
			      stream.write(data);
	            bytes = data.length;
			   }
			   size += bytes;
			}
			catch(Exception e)
			{
            logger.error("Exception during file processing", e);
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
		return bytes;
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
	 * Abort file transfer
	 */
	protected void abortTransfer(boolean isCancelled)
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
      status = isCancelled ? CANCELLED : FAILED;
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
		return ((exception != null) && (exception instanceof IOException)) ? (IOException)exception : new IOException(exception);
	}
}
