/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.client.server;

import java.util.Date;
import org.netxms.base.NXCPMessage;

/**
 * Represents information about file in server's file store
 */
public class ServerFile implements RemoteFile
{
	private String name;
	private long size;
	private Date modificationTime;
   private String extension;

	/**
	 * Create server file object from NXCP message.
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable ID
	 */
   public ServerFile(NXCPMessage msg, long baseId)
   {
      name = msg.getFieldAsString(baseId);
      size = msg.getFieldAsInt64(baseId + 1);
      modificationTime = msg.getFieldAsDate(baseId + 2);
      setExtension();
	}
   
   /**
    * Set file extension
    */
   private void setExtension()
   {
      if(name.startsWith("."))
      {
         extension = " ";
         return;
      }
      
      String[] parts = name.split("\\."); //$NON-NLS-1$
      if (parts.length > 1)
      {
         extension = parts[parts.length - 1];
      }
      else
      {
         extension = " ";
      }
   }

   /**
    * @see org.netxms.client.server.RemoteFile#getName()
    */
   @Override
	public String getName()
	{
		return name;
	}

	/**
    * @param name the name to set
    */
   public void setName(String name)
   {
      this.name = name;
      setExtension();
   }

   /**
    * @see org.netxms.client.server.RemoteFile#getSize()
    */
   @Override
	public long getSize()
	{
		return size;
	}

   /**
    * @see org.netxms.client.server.RemoteFile#getModificationTime()
    */
   @Override
   public Date getModificationTime()
	{
		return modificationTime;
	}

   /**
    * @see org.netxms.client.server.RemoteFile#getExtension()
    */
   @Override
   public String getExtension()
   {
      return extension;
   }

   /**
    * @see org.netxms.client.server.RemoteFile#isDirectory()
    */
   @Override
   public boolean isDirectory()
   {
      return false;
   }

   /**
    * @see org.netxms.client.server.RemoteFile#isPlaceholder()
    */
   @Override
   public boolean isPlaceholder()
   {
      return false;
   }
}
