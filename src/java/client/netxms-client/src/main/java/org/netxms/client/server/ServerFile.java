/**
 * 
 */
package org.netxms.client.server;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import org.netxms.base.NXCPMessage;

/**
 * Represents information about file in server's file store
 *
 */
public class ServerFile
{
   public static final int FILE = 1; 
   public static final int DIRECTORY = 2; 
   public static final int SYMBOLYC_LINK = 4; 
   public static final int PLACEHOLDER = 65536; 
   
	private String name;
	private long size;
	private Date modificationTime;
   private String extension;
   private int type = 0;
   private List<ServerFile> children;
   private ServerFile parent;
   private long nodeId;
	
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
    * Create server file object from NXCP message.
    * 
    * @param msg NXCP message
    * @param baseId base variable ID
    */
   public ServerFile(NXCPMessage msg, long baseId, ServerFile parent, long nodeId)
   {
      name = msg.getFieldAsString(baseId);
      size = msg.getFieldAsInt64(baseId + 1);
      modificationTime = msg.getFieldAsDate(baseId + 2);
      type = (int)msg.getFieldAsInt64(baseId + 3);
      this.parent = parent;
      this.nodeId = nodeId;
      setExtension();
   }
   
   /**
    * @param name
    * @param fileType
    * @param parent
    * @param nodeId
    */
   public ServerFile(String name, int fileType, ServerFile parent, long nodeId)
   {
      this.name = name;
      this.type = fileType;
      this.parent = parent;
      this.nodeId = nodeId;
      this.modificationTime = new Date();
      setExtension();
   }
   
   /**
    * 
    */
   private void setExtension()
   {
      if(isDirectory() || name.startsWith("."))
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
   
   public boolean isDirectory()
   {
      return (type & DIRECTORY) > 0 ? true : false;
   }

   public boolean isPlaceholder()
   {
      return (type & PLACEHOLDER) > 0 ? true : false;
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

   /**
    * @return the children
    */
   public ServerFile[] getChildren()
   {
      if(children ==  null)
      {
         return null;
      }
      else
      { 
         return children.toArray(new ServerFile[children.size()]);
      }
   }

   /**
    * @param children the children to set
    */
   public void setChildren(ServerFile[] children)
   {
      this.children = new ArrayList<ServerFile>(Arrays.asList(children));
   }
   
   /**
    * @param chield to be removed
    */
   public void removeChield(ServerFile chield)
   {
      for(int i=0; i<children.size(); i++)
      {
         if(children.get(i).getName().equalsIgnoreCase(chield.getName()))
         {
            children.remove(i);
         }
      }
   }
   
   /**
    * @param chield to be added
    */
   public void addChield(ServerFile chield)
   {
      boolean childReplaced = false;
      for(int i=0; i<children.size(); i++)
      {
         if(children.get(i).getName().equalsIgnoreCase(chield.getName()))
         {
            children.add(i, chield);
            childReplaced = true;
         }
      }
      if(!childReplaced)
      {
         children.add(chield);
      }
   }

   /**
    * @return the fullName
    */
   public String getFullName()
   {
      if (parent == null)
         return name;
      return parent.getFullName() + "/" + name;
   }

   /**
    * @return the parent
    */
   public ServerFile getParent()
   {
      return parent;
   }

   /**
    * @param parent the parent to set
    */
   public void setParent(ServerFile parent)
   {
      this.parent = parent;
   }

   /**
    * @return the nodeId
    */
   public long getNodeId()
   {
      return nodeId;
   }

   /**
    * @return the type
    */
   public int getType()
   {
      return type;
   }
}
