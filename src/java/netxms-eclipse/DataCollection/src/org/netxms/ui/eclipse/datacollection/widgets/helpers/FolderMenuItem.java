package org.netxms.ui.eclipse.datacollection.widgets.helpers;

import java.util.List;
import org.eclipse.swt.graphics.Image;
import org.simpleframework.xml.Root;

@Root(name="folder")
public class FolderMenuItem extends GenericMenuItem
{
   public FolderMenuItem(String name)
   {
      this.name = name;
      parent = null;
      type= GenericMenuItem.FOLDER;
   }
   
   public FolderMenuItem()
   {
      name = "Root";
      parent = null;
      type= GenericMenuItem.FOLDER;
   }

   @Override
   public String getCommand()
   {
      return "";
   }

   @Override
   public boolean hasChildren()
   {
      return !children.isEmpty();
   }

   @Override
   public List<GenericMenuItem> getChildren()
   {
      return children;
   }

   @Override
   public GenericMenuItem getParent()
   {
      return parent;
   }

   public void addChild(GenericMenuItem child)
   {
      children.add(child);
   }
   
}
