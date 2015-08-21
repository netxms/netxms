/**
 * 
 */
package org.netxms.client.server;

import java.util.Date;
import org.netxms.base.NXCPMessage;

/**
 * Represents information about file in server's file store
 *
 */
public class ServerFile
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
	 * @return the name
	 */
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
	 * @return the size
	 */
	public long getSize()
	{
		return size;
	}

	/**
	 * @return the modifyicationTime
	 */
	public Date getModifyicationTime()
	{
		return modificationTime;
	}
	
   /**
    * @return the type
    */
   public String getExtension()
   {
      return extension;
   }
}
