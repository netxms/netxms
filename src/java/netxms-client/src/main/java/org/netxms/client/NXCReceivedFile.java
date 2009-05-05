/**
 * 
 */
package org.netxms.client;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 * Represents file received from server
 * 
 * @author Victor
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
		}
		catch(IOException e)
		{
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
			}
			timestamp = System.currentTimeMillis();
		}
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
}
