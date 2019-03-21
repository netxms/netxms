package org.netxms.client.objecttools;

import java.util.HashMap;
import org.netxms.base.annotations.Internal;

public class ObjectToolFolder
{
   @Internal
   private ObjectToolFolder parent;
   
   private String name;
   private String displayName;
   private HashMap<String, ObjectToolFolder> subfolders;
   private HashMap<Long, ObjectTool> tools;
   
   public ObjectToolFolder(String name)
   {
      this.name = name;
      displayName = name.replace("&", "");
      this.parent = null;
      this.subfolders = new HashMap<String, ObjectToolFolder>();
      this.tools = new HashMap<Long, ObjectTool>();
   }
   
   /**
    * Add tool to folder
    * 
    * @param t tool to add
    */
   public void addTool(ObjectTool t)
   {
      tools.put(t.getId(), t);
   }
   
   /**
    * Add sub-folder
    * 
    * @param f sub-folder
    */
   public void addFolder(ObjectToolFolder f)
   {
      f.setParent(this);
      subfolders.put(f.getDisplayName(), f);
   }
   
   /**
    * @return folder display name
    */
   public String getDisplayName()
   {
      return displayName;
   }
   
   /**
    * @param f parent to set
    */
   public void setParent(ObjectToolFolder f)
   {
      parent = f;
   }
}
