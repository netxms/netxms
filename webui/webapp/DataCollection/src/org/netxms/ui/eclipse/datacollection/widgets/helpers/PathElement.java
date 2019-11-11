/**
 * 
 */
package org.netxms.ui.eclipse.datacollection.widgets.helpers;

import java.util.HashSet;
import java.util.Set;
import java.util.UUID;

/**
 * Path element for file delivery policy
 */
public class PathElement
{
   private String name;
   private PathElement parent;
   private Set<PathElement> children;
   private UUID guid;
   
   /**
    * Create new path element 
    */
   public PathElement(PathElement parent, String name)
   {
      this(parent, name, null);
   }

   /**
    * Create new path element 
    */
   public PathElement(PathElement parent, String name, UUID guid)
   {
      this.parent = parent;
      this.name = name;
      this.guid = guid;
      children = null;
      if (parent != null)
      {
         if (parent.children == null)
            parent.children = new HashSet<PathElement>();
         children.add(this);
      }
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
   
   public PathElement getParent()
   {
      return parent;
   }

   public PathElement[] getChildren()
   {
      return (children != null) ? children.toArray(new PathElement[children.size()]) : new PathElement[0];
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
}
