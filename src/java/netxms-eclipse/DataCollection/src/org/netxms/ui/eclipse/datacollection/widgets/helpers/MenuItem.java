package org.netxms.ui.eclipse.datacollection.widgets.helpers;

import java.util.List;
import org.simpleframework.xml.Root;


@Root(name="item")
public class MenuItem extends GenericMenuItem
{  

   public MenuItem(String name, String displayName, String command, FolderMenuItem parent)
   {
      this.name = name;
      this.discription = displayName;
      this.command = command;
      this.parent = parent;
      type= GenericMenuItem.ITEM;
   }

   public MenuItem(String name, String displayName, String command)
   {
      this.name = name;
      this.discription = displayName;
      this.command = command;
      this.parent = null;
      type= GenericMenuItem.ITEM;
   }

   public MenuItem()
   {
      this.name = null;
      this.discription = null;
      this.command = null;
      this.parent = null;
      type= GenericMenuItem.ITEM;
   }

   @Override
   public String getCommand()
   {
      return command;
   }

   @Override
   public boolean hasChildren()
   {
      return false;
   }

   @Override
   public List<GenericMenuItem> getChildren()
   {
      return null;
   }

   @Override
   public GenericMenuItem getParent()
   {
      return parent;
   }
   
}
