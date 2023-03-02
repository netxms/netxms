/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.widgets.helpers;

import java.io.File;
import java.util.Date;
import java.util.HashSet;
import java.util.Set;
import java.util.UUID;
import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.ElementList;
import org.simpleframework.xml.Root;

/**
 * Path element for file delivery policy
 */
@Root(name = "element")
public class PathElement
{
   public static final int OWNER_READ     = 0x0001;
   public static final int OWNER_WRITE    = 0x0002;
   public static final int OWNER_EXECUTE  = 0x0004;
   public static final int GROUP_READ     = 0x0008;
   public static final int GROUP_WRITE    = 0x0010;
   public static final int GROUP_EXECUTE  = 0x0020;
   public static final int OTHERS_READ    = 0x0040;
   public static final int OTHERS_WRITE   = 0x0080;
   public static final int OTHERS_EXECUTE = 0x0100;

   @Attribute
   private String name;

   private PathElement parent;

   @ElementList(required = false)
   private Set<PathElement> children;

   @Element(required = false)
   private UUID guid;

   @Element(required = false)
   private Date creationTime;

   @Element(required = false)
   private Integer permissions;

   @Element(required = false)
   private String owner;

   @Element(required = false)
   private String ownerGroup;

   private File localFile = null;

   /**
    * Default constructor
    */
   public PathElement()
   {
      this(null, "", null, null, new Date());
   }

   /**
    * Create new path element
    */
   public PathElement(PathElement parent, String name)
   {
      this(parent, name, null, null, new Date());
   }

   /**
    * Create new path element 
    */
   public PathElement(PathElement parent, String name, File localFile, UUID guid, Date creationDate)
   {
      this.parent = parent;
      this.name = name;
      this.guid = guid;
      this.localFile = localFile;
      this.permissions = (OWNER_WRITE | OWNER_READ | GROUP_READ | OTHERS_READ) | ((guid != null) ? 0 : (OWNER_EXECUTE | GROUP_EXECUTE | OTHERS_EXECUTE));
      this.owner = "";
      this.ownerGroup = "";
      this.creationTime = creationDate;
      children = new HashSet<PathElement>();
      if (parent != null)
      {
         if (parent.children == null)
            parent.children = new HashSet<PathElement>();
         parent.children.add(this);
      }
   }

   /**
    * Update parent reference recursively
    */
   public void updateParentReference(PathElement parent)
   {
      this.parent = parent;
      for(PathElement e : children)
         e.updateParentReference(this);
   }
   
   /**
    * Remove this element
    */
   public void remove()
   {
      if (parent != null)
         parent.children.remove(this);
      parent = null;
   }

   /**
    * Check if path element is a file
    * 
    * @return
    */
   public boolean isFile()
   {
      return guid != null;
   }
   
   /**
    * Get element's parent
    * 
    * @return element's parent
    */
   public PathElement getParent()
   {
      return parent;
   }

   /**
    * Get element's children
    * 
    * @return element's children
    */
   public PathElement[] getChildren()
   {
      return children.toArray(new PathElement[children.size()]);
   }

   /**
    * Get element's children
    * 
    * @return element's children
    */
   public Set<PathElement> getChildrenSet()
   {
      return children;
   }
   
   /**
    * Find child by by name
    */
   public PathElement findChild(String name)
   {      
      for (PathElement element : children)
      {
         if (element.getName().equals(name))
            return element;
      }
      return null;
   }

   /**
    * Check if this element has children
    * 
    * @return true if this element has children
    */
   public boolean hasChildren()
   {
      return !children.isEmpty();
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
   }

   /**
    * @return the guid
    */
   public UUID getGuid()
   {
      return guid;
   }

   /**
    * @return the localFile
    */
   public File getLocalFile()
   {
      return localFile;
   }

   /**
    * @see java.lang.Object#hashCode()
    */
   @Override
   public int hashCode()
   {
      final int prime = 31;
      int result = 1;
      result = prime * result + ((name == null) ? 0 : name.hashCode());
      return result;
   }

   /**
    * @see java.lang.Object#equals(java.lang.Object)
    */
   @Override
   public boolean equals(Object obj)
   {
      if (this == obj)
         return true;
      if (obj == null)
         return false;
      if (getClass() != obj.getClass())
         return false;
      PathElement other = (PathElement)obj;
      if (name == null)
      {
         if (other.name != null)
            return false;
      }
      else if (!name.equals(other.name))
         return false;
      return true;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "PathElement [" +
             "name=" + name +
           ", parent=" + ((parent != null) ? parent.getName() : "(null)") +
           ", childrenCount=" + children.size() +
           ", guid=" + guid +
           ", creationTime=" + creationTime +
           ", localFile=" + localFile +
           ", permissions=" + permissions +
           ", owner=" + owner +
           ", ownerGroup=" + ownerGroup +
           "]";
   }

   /**
    * Update local file
    * 
    * @param f file
    */
   public void setFile(File f)
   {
      localFile = f;
   }

   /**
    * Get file's creation time
    * 
    * @return
    */
   public Date getCreationTime()
   {
      return creationTime;
   }

   /**
    * Update file's creation time
    */
   public void updateCreationTime()
   {
      creationTime = new Date();
   }

   /**
    * Update file's access rights
    */
   public void setPermissions(Integer r)
   {
      permissions = r;
   }

   /**
    * Get file's access rights
    */
   public Integer getPermissions()
   {
      return permissions;
   }
   
   /**
    * Get file's access rights
    */
   public String getPermissionsAsString()
   {
      StringBuilder sb = new StringBuilder();
      sb.append(isFile() ? '-' : 'd');
      sb.append((permissions & (1 << 0)) > 0 ? 'r' : '-');
      sb.append((permissions & (1 << 1)) > 0 ? 'w' : '-');
      sb.append((permissions & (1 << 2)) > 0 ? 'x' : '-');
      sb.append((permissions & (1 << 3)) > 0 ? 'r' : '-');
      sb.append((permissions & (1 << 4)) > 0 ? 'w' : '-');
      sb.append((permissions & (1 << 5)) > 0 ? 'x' : '-');
      sb.append((permissions & (1 << 6)) > 0 ? 'r' : '-');
      sb.append((permissions & (1 << 7)) > 0 ? 'w' : '-');
      sb.append((permissions & (1 << 8)) > 0 ? 'x' : '-');
      return sb.toString();
   }

   /**
    * Set file owner
    */
   public void setOwner(String newOwner)
   {
      owner = newOwner;
   }

   /**
    * Get file owner
    */
   public String getOwner()
   {
      return owner;
   }

   /**
    * Set file owner group
    */
   public void setOwnerGroup(String newOwnerGroup)
   {
      ownerGroup = newOwnerGroup;
   }

   /**
    * Get file owner group
    */
   public String getOwnerGroup()
   {
      return ownerGroup;
   }
}
