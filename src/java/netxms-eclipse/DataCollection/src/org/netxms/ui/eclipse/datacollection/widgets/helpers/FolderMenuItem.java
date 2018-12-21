package org.netxms.ui.eclipse.datacollection.widgets.helpers;

import java.util.Arrays;
import org.eclipse.swt.graphics.Image;

public class FolderMenuItem extends GenericMenuItem
{
   public FolderMenuItem(String name)
   {
      this.name = name;
      parent = null;
      type= GenericMenuItem.FOLDER;
   }

   @Override
   public String getName()
   {
      return name;
   }

   @Override
   public String getDisplayName()
   {
      return "";
   }

   @Override
   public String getCommand()
   {
      return "";
   }

   @Override
   public Image getIcon()
   {
      return null;
   }

   @Override
   public boolean hasChildren()
   {
      return true;
   }

   @Override
   public GenericMenuItem[] getChildren()
   {
      return children;
   }

   @Override
   public GenericMenuItem getParent()
   {
      return parent;
   }

   public void addChild(MenuItem child)
   {
      int lenght = children.length;
      children = Arrays.copyOf(children, lenght + 1);
      children[lenght] = child;
   }
   
}
