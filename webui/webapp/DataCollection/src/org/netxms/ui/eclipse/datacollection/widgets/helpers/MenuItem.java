package org.netxms.ui.eclipse.datacollection.widgets.helpers;

import org.eclipse.swt.graphics.Image;

public class MenuItem extends GenericMenuItem
{  

   public MenuItem(String name, String displayName, String command, FolderMenuItem parent)
   {
      this.name = name;
      this.displayName = displayName;
      this.command = command;
      this.parent = parent;
      type= GenericMenuItem.ITEM;
   }

   public MenuItem(String name, String displayName, String command)
   {
      this.name = name;
      this.displayName = displayName;
      this.command = command;
      this.parent = null;
      type= GenericMenuItem.ITEM;
   }

   @Override
   public String getName()
   {
      return name;
   }

   @Override
   public String getDisplayName()
   {
      return displayName;
   }

   @Override
   public String getCommand()
   {
      return command;
   }

   @Override
   public Image getIcon()
   {
      return null;
   }

   @Override
   public boolean hasChildren()
   {
      return false;
   }

   @Override
   public GenericMenuItem[] getChildren()
   {
      return null;
   }

   @Override
   public GenericMenuItem getParent()
   {
      return parent;
   }
   
}
